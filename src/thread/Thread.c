/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

#include <stdlib.h>
#include <malloc.h>
#include <M451Series.h>
#include <Thread.h>
#include <AtomicOps.h>
#include <Queue.h>

/* Load value for Systick. */
#define THREAD_SYSTICK_LOAD (SystemCoreClock / 1000 / THREAD_SYSTICK_MS)

/* Thread quantum, in system ticks. */
#define THREAD_QUANTUM 20

/* Number of remaining ticks before preemption after a delay to justify busy looping it. */
#define THREAD_DELAY_MARGIN (THREAD_QUANTUM / 5)

/* Set when thread is ready for execution, not set when suspended. */
#define THREAD_STATE_MSK_READY (1 << 0)

/* Invalid magic, must be different from all magics. */
#define THREAD_MAGIC_INVALID 0x00000000
/* TCB magic: 'THRD'. */
#define THREAD_MAGIC_TCB     0x44524854
/* Semaphore magic: 'SEMA'. */
#define THREAD_MAGIC_SEMA    0x414D4553

/* True if the size bytes that ptr points to fully reside in RAM. */
#define THREAD_CHECK_RAM(ptr, size) (((uint32_t) (ptr)) >= 0x20000000 && \
	((uint32_t) (ptr)) + (size) <= 0x20008000)
/* True if tcb points to a valid TCB. Needs critical section to protect from exit. */
#define THREAD_CHECK_TCB(tcb) (THREAD_CHECK_RAM((tcb), sizeof(Thread_TCB_t)) && \
	((Thread_TCB_t *) (tcb))->magic == THREAD_MAGIC_TCB)
/* True if sema points to a valid semaphore. Needs critical section to protect from destruction. */
#define THREAD_CHECK_SEMA(sema) (THREAD_CHECK_RAM((sema), sizeof(Thread_SemaphoreInternal_t)) && \
	((Thread_SemaphoreInternal_t *) (sema))->magic == THREAD_MAGIC_SEMA)

/* Number of the current exception, or 0 in thread mode. */
#define THREAD_GET_IRQN() (__get_IPSR() & 0xFF)

/* Marks a thread ready and pushes it to back of ready queue, taking care of IRQ masking. */
#define THREAD_READY(tcb) do { \
	tcb->state |= THREAD_STATE_MSK_READY; \
	uint32_t primask = Thread_IrqDisable(); \
	Queue_PushBack(&Thread_readyQueue, tcb); \
	Thread_IrqRestore(primask); } while(0)

/* Thread_Context_t size, aligned to 8-byte boundary. */
#define THREAD_CTX_SIZE_ALIGN ((sizeof(Thread_Context_t) + 7) & ~7)

/* Default PSR for new threads, only thumb flag set. */
#define THREAD_DEFAULT_PSR 0x01000000

/* Marks the scheduler as pending by flagging PendSV. */
#define THREAD_PEND_SCHED() do { SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; } while(0)

/* Busy waits for the current thread to become ready again. */
#define THREAD_WAIT_READY() do {} while( \
	!(*((volatile uint8_t *) &Thread_curTcb->state) & THREAD_STATE_MSK_READY))

/* Base 2 exponent for stack guard size. Must be 5 <= EXP <= 32. */
#define THREAD_STACKGUARD_SIZE_EXP 5
/* Stack guard size, in bytes. Guaranteed to be 8-byte aligned.
 * The stack guard address must be aligned to its size. */
#define THREAD_STACKGUARD_SIZE     (1 << THREAD_STACKGUARD_SIZE_EXP)
/* Stack guard region number for MPU. */
#define THREAD_STACKGUARD_REGNUM   0

/**
 * Thread context as pushed to stack.
 */
typedef struct {
#ifdef EVICSDK_FPU_SUPPORT
	/**< Software-pushed FPU registers: S16-S31. */
	uint32_t swS[16];
#endif
	/**< Software-pushed registers: R4-R11. */
	uint32_t swR[8];
	/**< Hardware-pushed registers: R0-R3, R12, LR, PC, PSR. */
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
#ifdef EVICSDK_FPU_SUPPORT
	/**< Hardware-pushed FPU registers: S0-S15, FPSCR. */
	uint32_t s[16];
	uint32_t fpscr;
#endif
} Thread_Context_t;

/**
 * Thread control block.
 * Field order is optimized to reduce memory
 * usage, be careful when changing it.
 */
typedef struct Thread_TCB {
	/**< Next TCB in queue. */
	struct Thread_TCB *next;
	/**< TCB magic. */
	uint32_t magic;
	/**< Pointer to the allocated memory block. */
	void *blockPtr;
	/**< Saved stack pointer. */
	void *stackPtr;
	/**< State-specific info field. */
	union {
		/**< Executing: system time for preemption. */
		uint32_t preemptTime;
		/**< In chrono list: system time for wakeup. */
		uint32_t chronoTime;
	} info;
	struct {
		/**< TCB of thread that joined this one, or NULL. */
		struct Thread_TCB *tcb;
		/**< Pointer to store return value (if tcb != NULL). */
		void **retPtr;
	} join;
	/**< Thread state. */
	uint8_t state;
} Thread_TCB_t;

/**
 * Semaphore.
 * Synchronization: global critical section.
 */
typedef struct {
	/**< Semaphore magic. */
	uint32_t magic;
	/**< Counter. Negative values count waiting threads. */
	volatile int32_t count;
	/**< Queue of threads waiting on an up. Synchronization: interrupt masking. */
	Queue_t waitQueue;
} Thread_SemaphoreInternal_t;

/**
 * Mutex.
 */
typedef struct {
	/**< Binary semaphore for the mutex. */
	Thread_SemaphoreInternal_t sema;
	/**< Thread that locked the mutex. NULL when unlocked. */
	Thread_TCB_t *owner;
} Thread_MutexInternal_t;

/**
 * Ready threads queue.
 * Accessed by threads, scheduler and other ISRs.
 * Synchronization: interrupt masking.
 */
static Queue_t Thread_readyQueue;

/**
 * Chronologically ordered queue of suspended threads
 * waiting for a timed event.
 * Accessed by threads and scheduler.
 * Synchronization: global critical section.
 */
static Queue_t Thread_chronoQueue;

/**
 * Current thread TCB.
 * Accessed by threads and scheduler.
 * This must be NULL even before Thread_Init().
 * Synchronization: global critical section.
 */
Thread_TCB_t *Thread_curTcb = NULL;

/**
 * Critical section counter. Zero when not in a critical section.
 */
static volatile uint32_t Thread_criticalCount;

/**
 * System time.
 * Wraps around at 49 days, 17:02:47.296.
 */
volatile uint32_t Thread_sysTick;

/**
 * Inserts a thread into a queue ordered by info.chronoTime.
 * This is an internal function.
 *
 * @param queue Queue.
 * @param tcb   TCB to insert.
 */
static void Thread_ChronoQueueInsert(Queue_t *queue, Thread_TCB_t *tcb) {
	Thread_TCB_t *prev, *next;

	// Look up insertion place
	// If a TCB has the same chronoTime as this one we
	// insert before it to save iterations, since TCBs
	// with the same chronoTime are handled together.
	prev = NULL;
	next = queue->head;
	while(next != NULL && next->info.chronoTime < tcb->info.chronoTime) {
		prev = next;
		next = next->next;
	}

	if(prev == NULL) {
		// Insert as first
		Queue_PushFront(queue, tcb);
	}
	else if(next == NULL) {
		// Insert as last
		Queue_PushBack(queue, tcb);
	}
	else {
		// Middle insertion
		tcb->next = next;
		prev->next = tcb;
	}
}

/**
 * Updates the ready queue, moving suspended threads
 * in the chronological list to it when they become ready.
 * Assumes a critical section has been acquired. Disables
 * and re-enables interrupts for ready queue push only when
 * a new ready thread is found.
 * This is an internal function.
 *
 * @return True if at least one thread was added to the ready
 *         queue, false otherwise.
 */
static uint8_t Thread_UpdateReadyQueueFromChrono() {
	Thread_TCB_t *tcb;
	uint8_t found;

	found = 0;
	while((tcb = Thread_chronoQueue.head) != NULL && tcb->info.chronoTime <= Thread_sysTick) {
		// Wake up this thread (always removing from front)
		Queue_Remove(&Thread_chronoQueue, NULL, tcb);
		THREAD_READY(tcb);
		found = 1;
	}

	return found;
}

/**
 * Sets up memory protection for the stack guard.
 * Must be called from ISR context with IRQs disabled.
 * This is an internal function.
 *
 * @param guardPtr Pointer to beginning of stack guard.
 *                 Must be aligned to stack guard size.
 */
static void Thread_SetupStackGuard(void *guardPtr) {
	uint32_t attrAndSize;

	// No need for pre-update memory barrier because the
	// stack guard is strongly-ordered.

	attrAndSize =
		(1 << MPU_RASR_XN_Pos    ) | // Instruction fetch disabled
		(0 << MPU_RASR_AP_Pos    ) | // No access permissions
		(0 << MPU_RASR_TEX_Pos   ) | // Non-cacheable
		(0 << MPU_RASR_S_Pos     ) | // Shareable (ignored)
		(0 << MPU_RASR_C_Pos     ) | // Non-normal
		(0 << MPU_RASR_B_Pos     ) | // Strongly-ordered
		(0 << MPU_RASR_SRD_Pos   ) | // No subregions
		(1 << MPU_RASR_ENABLE_Pos) | // Enabled
		((THREAD_STACKGUARD_SIZE_EXP - 1) << MPU_RASR_SIZE_Pos); // Size = 2^(SIZE+1)

	// Since our attributes and size are constant, we don't
	// need to disable the region before updating the base
	// address: it'll be invalid only at first setup, and the
	// region isn't yet enabled at that point.

	// Switch to stack guard region
	MPU->RNR = THREAD_STACKGUARD_REGNUM << MPU_RNR_REGION_Pos;
	// Configure base address
	MPU->RBAR = (uint32_t) guardPtr;
	// Configure attributes and size
	MPU->RASR = attrAndSize;

	// No need for post-update memory barrier and pipeline flush
	// because we're always returning from an exception when the
	// thread gets control back (this must be called from ISR).
}

/**
 * Schedules the next thread. Called from PendSV.
 * This is an internal function.
 *
 * @param sp Stack pointer to save for the current thread.
 *
 * @return Stack pointer to restore for the new thread.
 */
void *Thread_Schedule(void *sp) {
	Thread_TCB_t *nextTcb;
	uint32_t primask;

	if(Thread_criticalCount > 0) {
		// Current thread is in a critical section, resume it
		// Scheduler will be invoked again on section exit
		return sp;
	}

	// Since no thread is in a critical section we have free
	// access to all internal data structures that are
	// synchronized by a global critical section. When working
	// with the ready queue we need to disable interrupts.

	// Update ready queue before checking if there's something
	// to do to amortize time complexity.
	Thread_UpdateReadyQueueFromChrono();

	if(Thread_curTcb != NULL && Thread_curTcb->info.preemptTime > Thread_sysTick) {
		// Nothing to do
		return sp;
	}

	// Thread_curTcb will be NULL only if this is the first
	// run (from startup code) or if the current thread has
	// been deleted. In both cases we don't want to save state.
	if(Thread_curTcb != NULL) {
		// Save stack pointer for current thread
		Thread_curTcb->stackPtr = sp;

		if(Thread_curTcb->state & THREAD_STATE_MSK_READY) {
			// Current thread hasn't been suspended
			// Push it to back of ready queue (round-robin)
			THREAD_READY(Thread_curTcb);
		}
	}

	primask = Thread_IrqDisable();
	while((nextTcb = Queue_PopFront(&Thread_readyQueue)) == NULL) {
		// No ready threads to schedule.
		// Instead of having an idle thread, we make use of
		// the fact that we're running in a low priority
		// PendSV interrupt, so we can busy wait for a thread
		// to become ready. We're here because either:
		//  - all threads are delayed, or suspended threads are
		//    waiting on delayed threads or on semaphores that
		//    will be released by delayed threads: we keep
		//    updating the ready queue from the chrono list;
		//  - all threads are waiting on each other or on semas
		//    that will be released by other waiting threads:
		//    this is a deadlock, so scheduler dies here;
		//  - all threads have been terminated: system death.
		// In case of scheduler or system death only ISRs will
		// run from now on.
		// To touch the ready queue we need to disable IRQs,
		// but we can't keep them disabled, otherwise SysTick
		// won't run. We re-enable interrupts and keep calling
		// UpdateReadyQueueFromChrono() to find new ready threads
		// from the chronological list. Since it disables IRQs
		// only when it actually finds a new ready thread, we're
		// good. We then disable IRQs for the Queue_PopFront() call.
		Thread_IrqRestore(primask);
		while(!Thread_UpdateReadyQueueFromChrono());
		primask = Thread_IrqDisable();
	}
	Thread_IrqRestore(primask);

	if(nextTcb != Thread_curTcb) {
		// Switch to next thread
		Thread_curTcb = nextTcb;
		// Configure stack guard: stack is at the beginning
		// of the allocated block.
		primask = Thread_IrqDisable();
		Thread_SetupStackGuard(Thread_curTcb->blockPtr);
		Thread_IrqRestore(primask);
	}

	// Reset quantum
	Thread_curTcb->info.preemptTime = Thread_sysTick + THREAD_QUANTUM;

	return Thread_curTcb->stackPtr;
}

/**
 * System tick handler.
 * This is an internal function.
 */
void SysTick_Handler() {
	Thread_sysTick++;

	// TODO: make this smarter
	THREAD_PEND_SCHED();
}

/**
 * Function to which a dying thread returns.
 * This is an internal function.
 *
 * @param ret Return value of the main thread procedure.
 *            This is taken as an argument because, on ARM,
 *            the R0 register is used both for first argument
 *            and return value.
 */
static void Thread_ExitProc(void *ret) {
	Thread_CriticalEnter();

	// If a thread has joined us, wake him up
	if(Thread_curTcb->join.tcb != NULL) {
		*Thread_curTcb->join.retPtr = ret;
		THREAD_READY(Thread_curTcb->join.tcb);
	}

	// Delete ourselves
	// We won't be using stack or TCB after free
	Thread_curTcb->magic = THREAD_MAGIC_INVALID;
	free(Thread_curTcb->blockPtr);
	Thread_curTcb = NULL;

	// Release all critical sections
	Thread_criticalCount = 0;

	// Pend scheduler and wait for death.
	// Since Thread_curTcb is NULL, the context
	// switcher won't use the stack we just freed.
	// If scheduler runs between critical release
	// and the THREAD_PEND_SCHED() call we won't
	// get back here.
	THREAD_PEND_SCHED();
	while(1);
}

void Thread_Init() {
	Queue_Init(&Thread_readyQueue);
	Queue_Init(&Thread_chronoQueue);
	Thread_curTcb = NULL;
	Thread_criticalCount = 0;
	Thread_sysTick = 0;

	// Configure MPU for stack guard
	MPU->CTRL =
		(1 << MPU_CTRL_PRIVDEFENA_Pos) | // Use default map as background
		(0 << MPU_CTRL_HFNMIENA_Pos  ) | // Disable MPU for fault handlers
		(1 << MPU_CTRL_ENABLE_Pos    ) ; // Enable MPU

	// Set lowest priority for PendSV
	// While NVIC_SetPriority works with system interrupts,
	// it considers priority to always be 4 bits like NVIC
	// interrupts, while system interrupts only have 2 high bits,
	// so we shift to make up for it.
	NVIC_SetPriority(PendSV_IRQn, 3 << 2);

	// Configure systick (highest priority) but leave it disabled
	SysTick->LOAD = THREAD_SYSTICK_LOAD - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
	NVIC_SetPriority(SysTick_IRQn, 0 << 2);
}

Thread_Error_t Thread_Create(Thread_t *thread, Thread_EntryPtr_t entry, void *args, uint16_t stackSize) {
	uint8_t *block;
	Thread_TCB_t *tcb;
	Thread_Context_t *ctx;

	// Align stack size to 8-byte boundary, reserving
	// extra space for context push and stack guard
	stackSize = (stackSize + 7) & ~7;
	stackSize += THREAD_CTX_SIZE_ALIGN;
	stackSize += THREAD_STACKGUARD_SIZE;

	// Allocate space for TCB and stack. TCB is below thread
	// stack (i.e. at higher addresses). Stack must be 8-byte
	// aligned and stack guard must be size-aligned, so align
	// allocation to stack guard size (which is 8-byte aligned).
	block = memalign(THREAD_STACKGUARD_SIZE, stackSize + sizeof(Thread_TCB_t));
	if(block == NULL) {
		return NO_MEMORY;
	}

	// Setup TCB
	tcb = (Thread_TCB_t *) (block + stackSize);
	tcb->magic = THREAD_MAGIC_TCB;
	tcb->blockPtr = block;
	tcb->join.tcb = NULL;
	tcb->state = 0;
	*thread = (Thread_t) tcb;

	// Setup initial thread context and stack
	ctx = (Thread_Context_t *) (block + stackSize - THREAD_CTX_SIZE_ALIGN);
	ctx->r0 = (uint32_t) args;
	ctx->lr = (uint32_t) Thread_ExitProc;
	ctx->pc = (uint32_t) entry;
	ctx->psr = THREAD_DEFAULT_PSR;
	tcb->stackPtr = ctx;

	// Push new thread to back of ready queue
	THREAD_READY(tcb);

	if(!(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)) {
		// SysTick is not enabled, i.e. this is the first
		// run. Enable SysTick and pend scheduler.
		// Once the scheduler runs we will never go back
		// to the startup code.
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
		THREAD_PEND_SCHED();
	}

	return SUCCESS;
}

void Thread_Yield() {
	// Will waste 1 tick if sysTick == 0 when
	// the scheduler runs, but that's *very*
	// unlikely (and would only happen once).
	Thread_curTcb->info.preemptTime = 0;

	// Schedule (even if we might have already
	// been preempted) and wait for suspend/wakeup.
	THREAD_PEND_SCHED();
	THREAD_WAIT_READY();
}

Thread_Error_t Thread_Join(Thread_t thread, void **ret) {
	Thread_TCB_t *tcb;

	Thread_CriticalEnter();

	if(!THREAD_CHECK_TCB(thread)) {
		Thread_CriticalExit();
		return INVALID_THREAD;
	}

	tcb = (Thread_TCB_t *) thread;
	if(tcb->join.tcb != NULL) {
		return ALREADY_JOINED;
	}

	// Setup ourselves as the joined thread
	tcb->join.tcb = Thread_curTcb;
	tcb->join.retPtr = ret;

	// Mark current thread as suspended
	Thread_curTcb->state &= ~THREAD_STATE_MSK_READY;

	Thread_CriticalExit();

	// CriticalExit pended the scheduler, wait for suspend/wakeup
	THREAD_WAIT_READY();

	return SUCCESS;
}

void Thread_DelayMs(uint32_t delay) {
	uint32_t delayEnd;

	Thread_CriticalEnter();

	delayEnd = Thread_sysTick + delay * THREAD_SYSTICK_MS;
	if(delayEnd < Thread_curTcb->info.preemptTime - THREAD_DELAY_MARGIN) {
		// The delay will end before preemption: busy wait.
		Thread_CriticalExit();
		while(Thread_sysTick < delayEnd);
	}
	else {
		// Suspend thread to the chronological queue
		Thread_curTcb->info.chronoTime = delayEnd;
		Thread_curTcb->state &= ~THREAD_STATE_MSK_READY;
		Thread_ChronoQueueInsert(&Thread_chronoQueue, Thread_curTcb);
		Thread_CriticalExit();
		// CriticalExit pended the scheduler, wait for suspend/wakeup
		THREAD_WAIT_READY();
	}
}

void Thread_CriticalEnter() {
	// For internal use only: no-op from ISRs.
	if(THREAD_GET_IRQN() != 0) {
		return;
	}

	// No atomic operations needed here.
	// criticalCount = 0: even if preemption occurs
	// during read-update-store, the atomic store to
	// 1 will still be correct since criticalCount
	// must be zero for this code to be resumed.
	// criticalCount > 0: already in critical section,
	// so read-update-store won't be disturbed.
	Thread_criticalCount++;
}

void Thread_CriticalExit() {
	// For internal use only: no-op from ISRs.
	if(THREAD_GET_IRQN() != 0) {
		return;
	}

	// No atomic operations needed here.
	// criticalCount = 0: even if preemption occurs
	// during read-check, the atomic read of 0 will
	// still be correct since criticalCount must be
	// zero for this code to be resumed.
	// criticalCount > 0: already in critical section,
	// so read-update-store won't be disturbed.
	if(Thread_criticalCount == 0) {
		return;
	}
	Thread_criticalCount--;

	// This check is correct even in case of preemption
	// for the same reasons shown above.
	if(Thread_criticalCount == 0) {
		// Just exited the last critical section, time
		// to reschedule. Even if we've been preempted
		// after exiting, an extra scheduler call won't
		// cause issues.
		THREAD_PEND_SCHED();
	}
}

/**
 * Initializes a semaphore.
 *
 * @param sema  Semaphore.
 * @param count Initial semaphore count.
 */
static void Thread_SemaphoreInit(Thread_SemaphoreInternal_t *sema, int32_t count) {
	sema->magic = THREAD_MAGIC_SEMA;
	sema->count = count;
	Queue_Init(&sema->waitQueue);
}

/**
 * Deletes and deallocates a semaphore.
 *
 * @param sema   Semaphore.
 * @param doFree True to free the memory pointed by sema.
 *
 * @return SUCCESS or INVALID_SEMA.
 */
static Thread_Error_t Thread_SemaphoreDelete(Thread_SemaphoreInternal_t *sema, uint8_t doFree) {
	Thread_CriticalEnter();

	if(!THREAD_CHECK_SEMA(sema)) {
		Thread_CriticalExit();
		return INVALID_SEMA;
	}

	// Delete semaphore
	sema->magic = THREAD_MAGIC_INVALID;
	if(doFree) {
		free(sema);
	}

	Thread_CriticalExit();

	return SUCCESS;
}

Thread_Error_t Thread_SemaphoreCreate(Thread_Semaphore_t *sema, int32_t count) {
	Thread_SemaphoreInternal_t *sm;

	if(count < 0) {
		return INVALID_VALUE;
	}

	// Allocate and initialize semaphore
	sm = malloc(sizeof(Thread_SemaphoreInternal_t));
	if(sm == NULL) {
		return NO_MEMORY;
	}

	// Initialize semaphore
	Thread_SemaphoreInit(sm, count);
	*sema = (Thread_Semaphore_t) sm;

	return SUCCESS;
}

Thread_Error_t Thread_SemaphoreDestroy(Thread_Semaphore_t sema) {
	return Thread_SemaphoreDelete((Thread_SemaphoreInternal_t *) sema, 1);
}

Thread_Error_t Thread_SemaphoreDown(Thread_Semaphore_t sema) {
	Thread_SemaphoreInternal_t *sm;
	int32_t newSema;
	uint32_t primask;
	uint8_t waitWakeup = 0;

	Thread_CriticalEnter();

	if(!THREAD_CHECK_SEMA(sema)) {
		Thread_CriticalExit();
		return INVALID_SEMA;
	}

	// Decrement semaphore
	sm = (Thread_SemaphoreInternal_t *) sema;
	newSema = AtomicOps_Add((volatile uint32_t *) &sm->count, -1);

	if(newSema < 0) {
		// This semaphore can't be downed without waiting.
		// We might have been preempted after determining that
		// newSema < 0, but before disabling interrupts, by a thread
		// or ISR that upped the semaphore.
		// If we don't re-check, we could wrongly suspend ourselves
		// and we wouldn't be woken up until another up occurs.
		// This double check avoids disabling interrupts when
		// the semaphore can be simply downed without suspending and
		// the race described above doesn't happen.
		primask = Thread_IrqDisable();
		if(sm->count < 0) {
			// Mark current thread as suspended
			Thread_curTcb->state &= ~THREAD_STATE_MSK_READY;
			// Push ourselves to back of wait queue
			Queue_PushBack(&sm->waitQueue, Thread_curTcb);
			waitWakeup = 1;
		}
		Thread_IrqRestore(primask);
	}

	Thread_CriticalExit();

	if(waitWakeup) {
		// Thread_CriticalExit pended scheduler, wait for suspend/wakeup
		THREAD_WAIT_READY();
	}

	return SUCCESS;
}

Thread_Error_t Thread_SemaphoreTryDown(Thread_Semaphore_t sema) {
	Thread_SemaphoreInternal_t *sm;
	int32_t oldVal;

	Thread_CriticalEnter();

	if(!THREAD_CHECK_SEMA(sema)) {
		Thread_CriticalExit();
		return INVALID_SEMA;
	}

	// Try to down by compare-and-swap
	sm = (Thread_SemaphoreInternal_t *) sema;
	do {
		oldVal = sm->count;
		if(oldVal <= 0) {
			// Can't be downed without waiting
			Thread_CriticalExit();
			return TRY_FAIL;
		}
	} while(AtomicOps_CmpSwap((volatile uint32_t *) &sm->count, oldVal, oldVal - 1) != oldVal);

	Thread_CriticalExit();

	return SUCCESS;
}

Thread_Error_t Thread_SemaphoreUp(Thread_Semaphore_t sema) {
	Thread_SemaphoreInternal_t *sm;
	Thread_TCB_t *tcb;
	int32_t newSema;
	uint32_t primask;

	Thread_CriticalEnter();

	if(!THREAD_CHECK_SEMA(sema)) {
		Thread_CriticalExit();
		return INVALID_SEMA;
	}

	// Increment semaphore
	sm = (Thread_SemaphoreInternal_t *) sema;
	newSema = AtomicOps_Add((volatile uint32_t *) &sm->count, 1);
	if(newSema <= 0) {
		// The old value was negative: there's at least a thread
		// waiting on this sema. Each up handles a single wakeup
		// only, so another thread or ISR (try)downing between
		// the newSema check and disabling IRQs won't hurt us.
		// Wake up the first thread in queue.
		primask = Thread_IrqDisable();
		if((tcb = Queue_PopFront(&sm->waitQueue)) != NULL) {
			tcb->state |= THREAD_STATE_MSK_READY;
			Queue_PushBack(&Thread_readyQueue, tcb);
		}
		Thread_IrqRestore(primask);
	}

	Thread_CriticalExit();

	return SUCCESS;
}

Thread_Error_t Thread_SemaphoreGetCount(Thread_Semaphore_t sema, int32_t *count) {
	Thread_SemaphoreInternal_t *sm;
	int32_t val;

	Thread_CriticalEnter();

	if(!THREAD_CHECK_SEMA(sema)) {
		Thread_CriticalExit();
		return INVALID_SEMA;
	}

	// We internally use negative values to count
	// waiting threads, mask them as zero
	sm = (Thread_SemaphoreInternal_t *) sema;
	val = sm->count;
	*count = val < 0 ? 0 : val;

	Thread_CriticalExit();

	return SUCCESS;
}

Thread_Error_t Thread_MutexCreate(Thread_Mutex_t *mutex) {
	Thread_MutexInternal_t *mtx;

	// Allocate mutex and binary semaphore
	mtx = malloc(sizeof(Thread_MutexInternal_t));
	if(mtx == NULL) {
		return NO_MEMORY;
	}

	// Initialize mutex/semaphore to 1 (unlocked)
	Thread_SemaphoreInit(&mtx->sema, 1);
	mtx->owner = NULL;
	*mutex = (Thread_Mutex_t) mtx;

	return SUCCESS;
}

Thread_Error_t Thread_MutexDestroy(Thread_Mutex_t mutex) {
	Thread_MutexInternal_t *mtx;

	// Destroy semaphore, without freeing memory
	mtx = (Thread_MutexInternal_t *) mutex;
	if(Thread_SemaphoreDelete(&mtx->sema, 0) == INVALID_SEMA) {
		return INVALID_MUTEX;
	}

	// Destroy mutex
	free(mtx);

	return SUCCESS;
}

Thread_Error_t Thread_MutexLock(Thread_Mutex_t mutex) {
	Thread_MutexInternal_t *mtx;

	// Down semaphore (blocking) and take ownership
	mtx = (Thread_MutexInternal_t *) mutex;
	if(Thread_SemaphoreDown((Thread_Semaphore_t) &mtx->sema) == INVALID_SEMA) {
		return INVALID_MUTEX;
	}
	mtx->owner = Thread_curTcb;

	return SUCCESS;
}

Thread_Error_t Thread_MutexTryLock(Thread_Mutex_t mutex) {
	Thread_MutexInternal_t *mtx;
	Thread_Error_t ret;

	// Try downing the semaphore
	mtx = (Thread_MutexInternal_t *) mutex;
	ret = Thread_SemaphoreTryDown((Thread_Semaphore_t) &mtx->sema);
	switch(ret) {
		case SUCCESS:
			// Lock succeeded, take ownership
			mtx->owner = Thread_curTcb;
			break;
		case INVALID_SEMA:
			ret = INVALID_MUTEX;
			break;
		default:
			ret = TRY_FAIL;
			break;
	}

	return ret;
}

Thread_Error_t Thread_MutexUnlock(Thread_Mutex_t mutex) {
	Thread_MutexInternal_t *mtx;

	mtx = (Thread_MutexInternal_t *) mutex;

	// Assumption: Thread_curTcb != NULL
	if(mtx->owner != Thread_curTcb) {
		// We didn't lock this mutex, it's
		// already unlocked or we preempted
		// a Thread_Mutex(Try)Lock, which can
		// only happen if we don't own the mutex
		return MUTEX_BAD_UNLOCK;
	}

	// Release ownership and up semaphore
	mtx->owner = NULL;
	Thread_SemaphoreUp((Thread_Semaphore_t) &mtx->sema);

	return SUCCESS;
}

Thread_Error_t Thread_MutexGetState(Thread_Mutex_t mutex, uint8_t *isLocked) {
	Thread_MutexInternal_t *mtx;
	int32_t count;

	mtx = (Thread_MutexInternal_t *) mutex;
	if(Thread_SemaphoreGetCount((Thread_Semaphore_t) &mtx->sema, &count) == INVALID_SEMA) {
		return INVALID_MUTEX;
	}
	*isLocked = (count == 0);

	return SUCCESS;
}
