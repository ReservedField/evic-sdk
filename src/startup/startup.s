@ This file is part of eVic SDK.
@
@ eVic SDK is free software: you can redistribute it and/or modify
@ it under the terms of the GNU General Public License as published by
@ the Free Software Foundation, either version 3 of the License, or
@ (at your option) any later version.
@
@ eVic SDK is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@ GNU General Public License for more details.
@
@ You should have received a copy of the GNU General Public License
@ along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
@
@ Copyright (C) 2015-2016 ReservedField

	.syntax unified

	@ Verification strings
	.section .verify
	.ascii "Joyetech APROM" @ Vendor
	.ascii "E052"           @ Device (eVic VTC Mini)
	@ Maximum supported hardware version (A.BC)
	.byte 0x01 @ A
	.byte 0x01 @ B
	.byte 0x01 @ C
	@ Vendor string must start at least 26 bytes from end of file
	@ Device string must start at least 9 bytes from end of file
	.space 5

	.section .stack
	.align 3
.ifndef Stack_Size
	.equ    Stack_Size, 0x400
	.global Stack_Size
.endif
Stack_Limit:
	.space  Stack_Size
	.global Stack_Top
Stack_Top:

	@ Define an ISR vector entry (thumb mode)
	.macro ISR_ENTRY name
	.word \name + 1
	.endm

	.section .isr_vector
	.align 2
	.global ISR_Vector_Base
ISR_Vector_Base:
	@ Initial stack pointer
	.word Stack_Top

	@ System interrupts
	ISR_ENTRY Reset_Handler      @ Reset Handler
	ISR_ENTRY NMI_Handler        @ NMI Handler
	ISR_ENTRY HardFault_Handler  @ Hard Fault Handler
	ISR_ENTRY MemManage_Handler  @ MPU Fault Handler
	ISR_ENTRY BusFault_Handler   @ Bus Fault Handler
	ISR_ENTRY UsageFault_Handler @ Usage Fault Handler
	ISR_ENTRY 0                  @ Reserved
	ISR_ENTRY 0                  @ Reserved
	ISR_ENTRY 0                  @ Reserved
	ISR_ENTRY 0                  @ Reserved
	ISR_ENTRY SVC_Handler        @ SVCall Handler
	ISR_ENTRY DebugMon_Handler   @ Debug Monitor Handler
	ISR_ENTRY 0                  @ Reserved
	ISR_ENTRY PendSV_Handler     @ PendSV Handler
	ISR_ENTRY SysTick_Handler    @ SysTick Handler

	@ External interrupts
	ISR_ENTRY BOD_IRQHandler     @  0: Brown Out detection
	ISR_ENTRY IRC_IRQHandler     @  1: Internal RC
	ISR_ENTRY PWRWU_IRQHandler   @  2: Power down wake up 
	ISR_ENTRY RAMPE_IRQHandler   @  3: RAM parity error
	ISR_ENTRY CLKFAIL_IRQHandler @  4: Clock detection fail
	ISR_ENTRY Default_Handler    @  5: Reserved
	ISR_ENTRY RTC_IRQHandler     @  6: Real Time Clock 
	ISR_ENTRY TAMPER_IRQHandler  @  7: Tamper detection
	ISR_ENTRY WDT_IRQHandler     @  8: Watchdog timer
	ISR_ENTRY WWDT_IRQHandler    @  9: Window watchdog timer
	ISR_ENTRY EINT0_IRQHandler   @ 10: External Input 0
	ISR_ENTRY EINT1_IRQHandler   @ 11: External Input 1
	ISR_ENTRY EINT2_IRQHandler   @ 12: External Input 2
	ISR_ENTRY EINT3_IRQHandler   @ 13: External Input 3
	ISR_ENTRY EINT4_IRQHandler   @ 14: External Input 4
	ISR_ENTRY EINT5_IRQHandler   @ 15: External Input 5
	ISR_ENTRY GPA_IRQHandler     @ 16: GPIO Port A
	ISR_ENTRY GPB_IRQHandler     @ 17: GPIO Port B
	ISR_ENTRY GPC_IRQHandler     @ 18: GPIO Port C
	ISR_ENTRY GPD_IRQHandler     @ 19: GPIO Port D
	ISR_ENTRY GPE_IRQHandler     @ 20: GPIO Port E
	ISR_ENTRY GPF_IRQHandler     @ 21: GPIO Port F
	ISR_ENTRY SPI0_IRQHandler    @ 22: SPI0
	ISR_ENTRY SPI1_IRQHandler    @ 23: SPI1
	ISR_ENTRY BRAKE0_IRQHandler  @ 24: PWM0 brake interrupt
	ISR_ENTRY PWM0P0_IRQHandler  @ 25: PWM0 pair 0 interrupt
	ISR_ENTRY PWM0P1_IRQHandler  @ 26: PWM0 pair 1 interrupt
	ISR_ENTRY PWM0P2_IRQHandler  @ 27: PWM0 pair 2 interrupt
	ISR_ENTRY BRAKE1_IRQHandler  @ 28: PWM1 brake interrupt
	ISR_ENTRY PWM1P0_IRQHandler  @ 29: PWM1 pair 0 interrupt
	ISR_ENTRY PWM1P1_IRQHandler  @ 30: PWM1 pair 1 interrupt
	ISR_ENTRY PWM1P2_IRQHandler  @ 31: PWM1 pair 2 interrupt
	ISR_ENTRY TMR0_IRQHandler    @ 32: Timer 0
	ISR_ENTRY TMR1_IRQHandler    @ 33: Timer 1
	ISR_ENTRY TMR2_IRQHandler    @ 34: Timer 2
	ISR_ENTRY TMR3_IRQHandler    @ 35: Timer 3
	ISR_ENTRY UART0_IRQHandler   @ 36: UART0
	ISR_ENTRY UART1_IRQHandler   @ 37: UART1
	ISR_ENTRY I2C0_IRQHandler    @ 38: I2C0
	ISR_ENTRY I2C1_IRQHandler    @ 39: I2C1
	ISR_ENTRY PDMA_IRQHandler    @ 40: Peripheral DMA
	ISR_ENTRY DAC_IRQHandler     @ 41: DAC
	ISR_ENTRY ADC00_IRQHandler   @ 42: ADC0 interrupt source 0
	ISR_ENTRY ADC01_IRQHandler   @ 43: ADC0 interrupt source 1
	ISR_ENTRY ACMP01_IRQHandler  @ 44: ACMP0 and ACMP1
	ISR_ENTRY Default_Handler    @ 45: Reserved
	ISR_ENTRY ADC02_IRQHandler   @ 46: ADC0 interrupt source 2
	ISR_ENTRY ADC03_IRQHandler   @ 47: ADC0 interrupt source 3
	ISR_ENTRY UART2_IRQHandler   @ 48: UART2
	ISR_ENTRY UART3_IRQHandler   @ 49: UART3
	ISR_ENTRY Default_Handler    @ 50: Reserved
	ISR_ENTRY SPI2_IRQHandler    @ 51: SPI2
	ISR_ENTRY Default_Handler    @ 52: Reserved
	ISR_ENTRY USBD_IRQHandler    @ 53: USB device
	ISR_ENTRY USBH_IRQHandler    @ 54: USB host
	ISR_ENTRY USBOTG_IRQHandler  @ 55: USB OTG
	ISR_ENTRY CAN0_IRQHandler    @ 56: CAN0
	ISR_ENTRY Default_Handler    @ 57: Reserved
	ISR_ENTRY SC0_IRQHandler     @ 58: Smart card host 0 interrupt
	ISR_ENTRY Default_Handler    @ 59: Reserved
	ISR_ENTRY Default_Handler    @ 60: Reserved
	ISR_ENTRY Default_Handler    @ 61: Reserved
	ISR_ENTRY Default_Handler    @ 62: Reserved
	ISR_ENTRY TK_IRQHandler      @ 63: Touch key interrupt

	.text
	.align 2
	.thumb
	.global Startup_FpSetup
	.global Reset_Handler
	.weak   Reset_Handler
	.thumb_func
Reset_Handler:
	@ Unlock write-protected registers
	LDR   R0, =0x40000100
	LDR   R1, =0x59
	STR   R1, [R0]
	LDR   R1, =0x16
	STR   R1, [R0]
	LDR   R1, =0x88
	STR   R1, [R0]

	@ Disable POR (SYS_PORCTL = 0x55A5)
	LDR   R0, =0x40000024
	LDR   R1, =0x00005AA5
	STR   R1, [R0]

	@ Select INV type for HXT (CLK_PWRCTL[12] = 0)
	LDR   R0, =0x40000200
	LDR   R1, [R0]
	BIC   R1, #0x1000
	STR   R1, [R0]

	LDR   R0, =0xE000EF34
	LDR   R1, [R0]
#ifdef EVICSDK_FPU_SUPPORT
	@ Enable lazy stacking
	@ (ASPEN = 1, LSPEN = 1, i.e. FPCCR[31:30] = 11)
	ORR   R1, #(0x3 << 30)
#else
	@ Disable lazy stacking, disable FPU state saving
	@ (ASPEN = 0, LSPEN = 0, i.e. FPCCR[31:30] = 00)
	BIC   R1, #(0x3 << 30)
#endif
	STR   R1, [R0]

#ifdef EVICSDK_FPU_SUPPORT
	@ Enable FPU (enable CP10/CP11, i.e. CPACR[23:20] = 1111)
	LDR   R0, =0xE000ED88
	LDR   R1, [R0]
	ORR   R1, #(0xF << 20)
	STR   R1, [R0]

	@ FPU enabled: sync barrier, flush pipeline
	DSB
	ISB
#endif

	@ Copy .data to RAM. Symbols defined by linker script:
	@ Data_Size      : size of .data section
	@ Data_Start_ROM : start of .data section in ROM
	@ Data_Start_RAM : start of .data section in RAM
	LDR   R0, =Data_Size
	CBZ   R0, Data_Copy_End
	LDR   R1, =Data_Start_ROM
	LDR   R2, =Data_Start_RAM
Data_Copy_Loop:
	LDR   R3, [R1], #4
	STR   R3, [R2], #4
	SUBS  R0, #4
	BNE   Data_Copy_Loop
Data_Copy_End:

	@ Zero out BSS. Symbols defined by linker script:
	@ BSS_Size  : size of .bss section
	@ BSS_Start : start of .bss section in RAM
	@ R0 is already zeroed from the .data copy.
	LDR   R1, =BSS_Size
	CBZ   R1, BSS_Zero_End
	LDR   R2, =BSS_Start
BSS_Zero_Loop:
	STR   R0, [R2], #4
	SUBS  R1, #4
	BNE   BSS_Zero_Loop
BSS_Zero_End:

	@ Call Sys_Init
	LDR   R0, =Sys_Init
	BLX   R0

	@ Lock write-protected registers
	LDR   R0, =0x40000100
	MOVS  R1, #0
	STR   R1, [R0]

	@ Create main thread
	LDR   R0, =Startup_CreateMainThread
	BLX   R0

	@ Wait for scheduler
	B     .

	.pool

	@ Starts a weak export
	.macro BEGIN_WEAK_EXPORT name
	.global \name
	.weak   \name
\name :
	.endm

	@ Defines a default handler (weak symbol, infinite loop)
	.macro DEFINE_DEFAULT_HANDLER name
	BEGIN_WEAK_EXPORT \name
	.align 1
	.thumb_func
	@ Infinite loop
	b .
	.endm

	DEFINE_DEFAULT_HANDLER NMI_Handler
	DEFINE_DEFAULT_HANDLER HardFault_Handler
	DEFINE_DEFAULT_HANDLER MemManage_Handler
	DEFINE_DEFAULT_HANDLER BusFault_Handler
	DEFINE_DEFAULT_HANDLER UsageFault_Handler
	DEFINE_DEFAULT_HANDLER SVC_Handler
	DEFINE_DEFAULT_HANDLER DebugMon_Handler
	DEFINE_DEFAULT_HANDLER PendSV_Handler
	DEFINE_DEFAULT_HANDLER SysTick_Handler

	BEGIN_WEAK_EXPORT BOD_IRQHandler
	BEGIN_WEAK_EXPORT IRC_IRQHandler
	BEGIN_WEAK_EXPORT PWRWU_IRQHandler
	BEGIN_WEAK_EXPORT RAMPE_IRQHandler
	BEGIN_WEAK_EXPORT CLKFAIL_IRQHandler
	BEGIN_WEAK_EXPORT RTC_IRQHandler
	BEGIN_WEAK_EXPORT TAMPER_IRQHandler
	BEGIN_WEAK_EXPORT WDT_IRQHandler
	BEGIN_WEAK_EXPORT WWDT_IRQHandler
	BEGIN_WEAK_EXPORT EINT0_IRQHandler
	BEGIN_WEAK_EXPORT EINT1_IRQHandler
	BEGIN_WEAK_EXPORT EINT2_IRQHandler
	BEGIN_WEAK_EXPORT EINT3_IRQHandler
	BEGIN_WEAK_EXPORT EINT4_IRQHandler
	BEGIN_WEAK_EXPORT EINT5_IRQHandler
	BEGIN_WEAK_EXPORT GPA_IRQHandler
	BEGIN_WEAK_EXPORT GPB_IRQHandler
	BEGIN_WEAK_EXPORT GPC_IRQHandler
	BEGIN_WEAK_EXPORT GPD_IRQHandler
	BEGIN_WEAK_EXPORT GPE_IRQHandler
	BEGIN_WEAK_EXPORT GPF_IRQHandler
	BEGIN_WEAK_EXPORT SPI0_IRQHandler
	BEGIN_WEAK_EXPORT SPI1_IRQHandler
	BEGIN_WEAK_EXPORT BRAKE0_IRQHandler
	BEGIN_WEAK_EXPORT PWM0P0_IRQHandler
	BEGIN_WEAK_EXPORT PWM0P1_IRQHandler
	BEGIN_WEAK_EXPORT PWM0P2_IRQHandler
	BEGIN_WEAK_EXPORT BRAKE1_IRQHandler
	BEGIN_WEAK_EXPORT PWM1P0_IRQHandler
	BEGIN_WEAK_EXPORT PWM1P1_IRQHandler
	BEGIN_WEAK_EXPORT PWM1P2_IRQHandler
	BEGIN_WEAK_EXPORT TMR0_IRQHandler
	BEGIN_WEAK_EXPORT TMR1_IRQHandler
	BEGIN_WEAK_EXPORT TMR2_IRQHandler
	BEGIN_WEAK_EXPORT TMR3_IRQHandler
	BEGIN_WEAK_EXPORT UART0_IRQHandler
	BEGIN_WEAK_EXPORT UART1_IRQHandler
	BEGIN_WEAK_EXPORT I2C0_IRQHandler
	BEGIN_WEAK_EXPORT I2C1_IRQHandler
	BEGIN_WEAK_EXPORT PDMA_IRQHandler
	BEGIN_WEAK_EXPORT DAC_IRQHandler
	BEGIN_WEAK_EXPORT ADC00_IRQHandler
	BEGIN_WEAK_EXPORT ADC01_IRQHandler
	BEGIN_WEAK_EXPORT ACMP01_IRQHandler
	BEGIN_WEAK_EXPORT ADC02_IRQHandler
	BEGIN_WEAK_EXPORT ADC03_IRQHandler
	BEGIN_WEAK_EXPORT UART2_IRQHandler
	BEGIN_WEAK_EXPORT UART3_IRQHandler
	BEGIN_WEAK_EXPORT SPI2_IRQHandler
	BEGIN_WEAK_EXPORT USBD_IRQHandler
	BEGIN_WEAK_EXPORT USBH_IRQHandler
	BEGIN_WEAK_EXPORT USBOTG_IRQHandler
	BEGIN_WEAK_EXPORT CAN0_IRQHandler
	BEGIN_WEAK_EXPORT SC0_IRQHandler
	BEGIN_WEAK_EXPORT TK_IRQHandler
	DEFINE_DEFAULT_HANDLER Default_Handler

	.end
