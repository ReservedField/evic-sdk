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

#ifndef EVICSDK_THREAD_H
#define EVICSDK_THREAD_H

#include <stdint.h>
#include <M451Series.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of system ticks in one millisecond.
 */
#define THREAD_SYSTICK_MS 1

/**
 * Default stack size for the main thread.
 */
#define THREAD_DEFAULT_STACKSIZE 1024

/**
 * Defines the stack size for the main thread.
 * This must be used outside of any function. If this
 * macro isn't used, the stack size will default to
 * THREAD_DEFAULT_STACKSIZE.
 */
#define THREAD_MAIN_STACKSIZE(size) uint16_t Startup_mainThreadStackSize = (size)

/**
 * Type for thread handles.
 */
typedef uint32_t Thread_t;

/**
 * Type for semaphore handles.
 */
typedef uint32_t Thread_Semaphore_t;

/**
 * Type for mutex handles.
 */
typedef uint32_t Thread_Mutex_t;

/**
 * Return values for thread API.
 * Zero means success, a negative value means error.
 */
typedef enum {
	/**< No error. */
	TD_SUCCESS = 0,
	/**< Couldn't allocate memory. */
	TD_NO_MEMORY = -1,
	/**< A try operation failed. */
	TD_TRY_FAIL = -2,
	/**< An invalid value was passed. */
	TD_INVALID_VALUE = -3,
	/**< An invalid thread handle was passed. */
	TD_INVALID_THREAD = -100,
	/**< Attempted to join an already joined thread. */
	TD_ALREADY_JOINED = -101,
	/**< An invalid semaphore handle was passed. */
	TD_INVALID_SEMA = -200,
	/**< An invalid mutex handle was passed. */
	TD_INVALID_MUTEX = -300,
	/**< Bad mutex unlock: already unlocked, or locked by a different thread. */
	TD_MUTEX_BAD_UNLOCK = -301
} Thread_Error_t;

/**
 * Thread entry function pointer type.
 *
 * @param args Arguments parameter from Thread_Create().
 */
typedef void *(*Thread_EntryPtr_t)(void *args);

/* Only *FromISR functions are safe to call from interrupt/callback context. */
#define Thread_SemaphoreTryDownFromISR  Thread_SemaphoreTryDown
#define Thread_SemaphoreUpFromISR       Thread_SemaphoreUp
#define Thread_SemaphoreGetCountFromISR Thread_SemaphoreGetCount
#define Thread_MutexGetStateFromISR     Thread_MutexGetState
#define Thread_GetSysTicksFromISR       Thread_GetSysTicks
#define Thread_IrqDisableFromISR        Thread_IrqDisable
#define Thread_IrqRestoreFromISR        Thread_IrqRestore

/**
 * Initializes the thread manager.
 */
void Thread_Init();

/**
 * Creates a new thread.
 *
 * @param thread    Pointer to receive the thread handle.
 * @param entry     Thread entry function.
 * @param args      Arguments parameter to be passed to entry.
 * @param stackSize Stack size, in bytes.
 *
 * @return TD_SUCCESS or TD_NO_MEMORY. In case of error
 *         the value of thread is undefined.
 */
Thread_Error_t Thread_Create(Thread_t *thread, Thread_EntryPtr_t entry, void *args, uint16_t stackSize);

/**
 * Yields this thread back to the scheduler, letting
 * other threads run.
 */
void Thread_Yield();

/**
 * Waits for another thread to complete.
 * Each thread can only have one other thread waiting on it.
 * Attempting to join a thread that already has another thread
 * waiting on it will fail with TD_ALREADY_JOINED.
 *
 * @param thread Handle of the thread to wait for.
 * @param ret    Pointer to receive the return value of the
 *               joined thread.
 *
 * @return TD_SUCCESS, TD_INVALID_THREAD or TD_ALREADY_JOINED.
 */
Thread_Error_t Thread_Join(Thread_t thread, void **ret);

/**
 * Delays the current thread for the specified amount of time.
 * It is guaranteed that the actual delay time will never be less
 * than the specified time, but it may be longer.
 *
 * @param delay Delay time, in milliseconds.
 */
void Thread_DelayMs(uint32_t delay);

/**
 * Enters a global critical section, disabling scheduling
 * for other threads.
 * Critical sections can be nested.
 */
void Thread_CriticalEnter();

/**
 * Exits a global critical section, re-enabling scheduling
 * for other threads once the last nested critical section
 * is exited.
 */
void Thread_CriticalExit();

/**
 * Creates a semaphore.
 *
 * @param sema  Pointer to receive semaphore handle.
 * @param count Initial count (must not be negative).
 *
 * @return TD_SUCCESS, TD_INVALID_VALUE (negative count) or TD_NO_MEMORY.
 *         In case of error the value of sema is undefined.
 */
Thread_Error_t Thread_SemaphoreCreate(Thread_Semaphore_t *sema, int32_t count);

/**
 * Destroys a semaphore.
 *
 * @param sema Semaphore handle.
 *
 * @return TD_SUCCESS or TD_INVALID_SEMA.
 */
Thread_Error_t Thread_SemaphoreDestroy(Thread_Semaphore_t sema);

/**
 * Decrements the semaphore count.
 * If count is zero, waits until another thread increments it.
 *
 * @param sema Semaphore handle.
 *
 * @return TD_SUCCESS or TD_INVALID_SEMA.
 */
Thread_Error_t Thread_SemaphoreDown(Thread_Semaphore_t sema);

/**
 * Decrements the semaphore count.
 * If count is zero, it fails without waiting.
 *
 * @param sema Semaphore handle.
 *
 * @return TD_SUCCESS, TD_INVALID_SEMA or TD_TRY_FAIL.
 */
Thread_Error_t Thread_SemaphoreTryDown(Thread_Semaphore_t sema);

/**
 * Increments the semaphore count.
 *
 * @param sema Semaphore handle.
 *
 * @return TD_SUCCESS or TD_INVALID_SEMA.
 */
Thread_Error_t Thread_SemaphoreUp(Thread_Semaphore_t sema);

/**
 * Gets the semaphore count.
 *
 * @param sema  Semaphore handle.
 * @param count Pointer to receive the count.
 *
 * @return TD_SUCCESS or TD_INVALID_SEMA. In case of
 *         error the value of count is undefined.
 */
Thread_Error_t Thread_SemaphoreGetCount(Thread_Semaphore_t sema, int32_t *count);

/**
 * Creates a mutex.
 * The mutex is initially unlocked.
 *
 * @param mutex Pointer to receive mutex handle.
 *
 * @return TD_SUCCESS or TD_NO_MEMORY. In case of
 *         error the value of mutex is undefined.
 */
Thread_Error_t Thread_MutexCreate(Thread_Mutex_t *mutex);

/**
 * Destroys a mutex.
 *
 * @param mutex Mutex handle.
 *
 * @return TD_SUCCESS or TD_INVALID_MUTEX.
 */
Thread_Error_t Thread_MutexDestroy(Thread_Mutex_t mutex);

/**
 * Locks a mutex.
 * If the mutex is locked, waits until it's unlocked.
 *
 * @param mutex Mutex handle.
 *
 * @return TD_SUCCESS or TD_INVALID_MUTEX.
 */
Thread_Error_t Thread_MutexLock(Thread_Mutex_t mutex);

/**
 * Locks a mutex.
 * If the mutex is locked, fails without waiting.
 *
 * @param mutex Mutex handle.
 *
 * @return TD_SUCCESS, TD_INVALID_MUTEX or TD_TRY_FAIL.
 */
Thread_Error_t Thread_MutexTryLock(Thread_Mutex_t mutex);

/**
 * Unlocks a mutex.
 * Only the thread that locked a mutex can unlock it.
 *
 * @param mutex Mutex handle.
 *
 * @return TD_SUCCESS or TD_MUTEX_BAD_UNLOCK.
 */
Thread_Error_t Thread_MutexUnlock(Thread_Mutex_t mutex);

/**
 * Checks the state of a mutex.
 *
 * @param mutex    Mutex handle.
 * @param isLocked Pointer to receive state. Will be set to true
 *                 if the mutex is locked, false if unlocked.
 *
 * @return TD_SUCCESS or TD_INVALID_MUTEX. In case of error the value
 *         of isLocked is undefined.
 */
Thread_Error_t Thread_MutexGetState(Thread_Mutex_t mutex, uint8_t *isLocked);

/* Always inline, no extern version. */
#define THREAD_INLINE __attribute__((always_inline)) static inline

/**
 * Gets the current system uptime.
 * Wraps around at 49 days, 17:02:47.296.
 *
 * @return Current system uptime, in ticks.
 */
THREAD_INLINE uint32_t Thread_GetSysTicks() {
	extern volatile uint32_t Thread_sysTick;
	return Thread_sysTick;
}

/**
 * Saves and disables interrupts.
 *
 * @return Interrupt mask for Thread_IrqRestore.
 */
THREAD_INLINE uint32_t Thread_IrqDisable() {
	uint32_t primask = __get_PRIMASK();
	__set_PRIMASK(1);
	return primask;
}

/**
 * Restores interrupts.
 *
 * @param primask Interrupt mask from Thread_IrqDisable.
 */
THREAD_INLINE void Thread_IrqRestore(uint32_t primask) {
	__set_PRIMASK(primask);
}

#undef THREAD_INLINE

#ifdef __cplusplus
}
#endif

#endif
