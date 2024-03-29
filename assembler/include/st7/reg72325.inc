		ifndef	__reg72325inc	; avoid multiple inclusion
__reg72325inc	equ	1

		save
		listing	off		; no listing over this file

;****************************************************************************
;*                                                                          *
;*   AS 1.42 - File REG72325.INC                                            *
;*                                                                          *
;*   contains SFR and Bit Definitions for ST72325                           *
;*                                                                          *
;*   Source: ST72325 Data Sheet, Rev. 4, October 2008                       *
;*                                                                          *
;****************************************************************************

;----------------------------------------------------------------------------
; Memory Addresses

RAMSTART	label	$0080		; start address internal RAM
		switch	substr(MOMCPUNAME,8,1)
		case	"4"
RAMEND		label	$027f		; end     "        "      "
		case	"6"
RAMEND		label	$047f		; end     "        "      "
		case	"7"
RAMEND		label	$067f		; end     "        "      "
		case	"9"
RAMEND		label	$087f		; end     "        "      "
		elsecase
		fatal	"cannot deduce RAM size"
		endcase
		
;----------------------------------------------------------------------------
; Interrupt Vectors

PWM_ART_vect	label	$ffe0		; PWM ART interrupt
I2C_vect	label	$ffe2		; I2C Peripheral interrupts
AVD_vect	label	$ffe4		; Auxiliary Voltage detector interrupt
SCI_vect	label	$ffe6		; SCI Interrupt Vector
TIMB_vect	label	$ffe8		; TIMER B Interrupt Vector
TIMA_vect	label	$ffea		; TIMER A Interrupt Vector
SPI_vect	label	$ffec		; SPI Interrupt Vector
EI3_vect	label	$fff0		; External Interrupt Vector B7..4
EI2_vect	label	$fff2		; External Interrupt Vector B3..0
EI1_vect	label	$fff4		; External Interrupt Vector F2..0
EI0_vect	label	$fff6		; External Interrupt Vector A3..0
MCC_RTC_vect	label	$fff8		; Main clock controller time base interrupt
TLI_vect	label	$fffa		; External top level interrupt
TRAP_vect	label	$fffc		; TRAP (software) Interrupt Vector
RESET_vect	label	$fffe		; RESET Vector

;----------------------------------------------------------------------------
; GPIO

		include	"gpio.inc"
		__defgpio "PA",$0000
		__defgpio "PB",$0003
		__defgpio "PC",$0006
		__defgpio "PD",$0009
		__defgpio "PE",$000c
		__defgpio "PF",$000f

;----------------------------------------------------------------------------
; Miscellaneous

;----------------------------------------------------------------------------
; I2C

		include	"i2c.inc"
		__defi2c $0018

;----------------------------------------------------------------------------
; SPI

		include	"spi2.inc"

;----------------------------------------------------------------------------
; ITC

ISPR0		label	$0024		; Interrupt Software Priority Register 0
I0_0		bit	ISPR0,0		;  TLI
I1_0		bit	ISPR0,1
I0_1		bit	ISPR0,2		;  MCC+SI
I1_1		bit	ISPR0,3
I0_2		bit	ISPR0,4		;  EI0
I1_2		bit	ISPR0,5
I0_3		bit	ISPR0,6		;  EI1
I1_3		bit	ISPR0,7
ISPR1		label	$0025		; Interrupt Software Priority Register 1
I0_4		bit	ISPR1,0		;  EI2
I1_4		bit	ISPR1,1
I0_5		bit	ISPR1,2		;  EI3
I1_5		bit	ISPR1,3
I0_6		bit	ISPR1,4
I1_6		bit	ISPR1,5
I0_7		bit	ISPR1,6		;  SPI
I1_7		bit	ISPR1,7
ISPR2		label	$0026		; Interrupt Software Priority Register 2
I0_8		bit	ISPR2,0		;  Timer A
I1_8		bit	ISPR2,1
I0_9		bit	ISPR2,2		;  Timer B
I1_9		bit	ISPR2,3
I0_10		bit	ISPR2,4		;  SCI
I1_10		bit	ISPR2,5
I0_11		bit	ISPR2,6		;  AVD
I1_11		bit	ISPR2,7
ISPR3		label	$0027		; Interrupt Software Priority Register 3
I0_12		bit	ISPR3,0		;  I2C
I1_12		bit	ISPR3,1
I0_13		bit	ISPR3,2		;  PWMART
I1_13		bit	ISPR3,3
EICR		label	$0028		; External Interrupt Control Register
IS1		bfield	EICR,6,2	;  ei2 and ei3 sensitivity
IPB		bit	EICR,5		;  Interrupt polarity for port B
IS2		bfield	EICR,3,2	;  ei0 and ei1 sensitivity
IPA		bit	EICR,2		;  Interrupt polarity for port A
TLIS		bit	EICR,1		;  TLI sensitivity
TLIE		bit	EICR,0		;  TLI enable

;----------------------------------------------------------------------------
; Flash

FCSR		label	$0029		; Flash Control/Status Register

;----------------------------------------------------------------------------
; Watchdog

WDGCR		label	$002a		; Watchdog Control Register
WDGA		bit	WDGCR,7		; Activation bit

;----------------------------------------------------------------------------

SICSR		label	$002b		; System Integrity Control/Status Register
AVDS		bit	SICSR,7		;  Voltage Detection selection
AVDIE		bit	SICSR,6		;  Voltage Detector interrupt enable
AVDF		bit	SICSR,5		;  Voltage Detector flag
LVDRF		bit	SICSR,4		;  LVD reset flag
CSSIE		bit	SICSR,2		;  Clock security syst. interrupt enable
CSSD		bit	SICSR,1		;  Clock security system detection
WDGRF		bit	SICSR,0		;  Watchdog reset flag

;----------------------------------------------------------------------------
; MCC

MCCSR		label	$002c		; Main Clock Control / Status Register
MCO		bit	MCCSR,7		;  Main clock out selection
CP		bfield	MCCSR,5,2	;  CPU clock prescaler
SMS		bit	MCCSR,4		;  Slow mode select
TB		bfield	MCCSR,2,2	;  Time base control
OIE		bit	MCCSR,1		;  Oscillator interrupt enable
OIF		bit	MCCSR,0		;  Oscillator interrupt flag
MCCBCR		label	$002d		; Main Clock Controller: Beep Control Register
BC		bfield	MCCBCR,0,2	;  Beep control

;----------------------------------------------------------------------------
; Timer A/B

		include	"timer.inc"		
		__deftimer "TA",$0030
		__deftimer "TB",$0040

;----------------------------------------------------------------------------
; Serial Communications Interface

		include	"sci.inc"
SCI_PE		bit	SCISR,0		;  Parity error
SCID		bit	SCICR1,5	;  Disabled for low power consumption
PCE		bit	SCICR1,2	;  Parity control enable
PS		bit	SCICR1,1	;  Parity selection
PIE		bit	SCICR1,0	;  Parity interrupt enable

;----------------------------------------------------------------------------
; Debug Module

DMCR		label	$0058		; DM Control Register
DMSR		label	$0059		; DM Status Register
DMBK1H		label	$005a		; DM Breakpoint Register 1 High
DMBK1L		label	$005b		; DM Breakpoint Register 1 Low
DMBK2H		label	$005c		; DM Breakpoint Register 2 High
DMBK2L		label	$005d		; DM Breakpoint Register 2 Low

;----------------------------------------------------------------------------
; Analog/Digital Converter

ADCCSR		label	$0070		; Control/Status Register
EOC		bit	ADCCSR,7	;  End of Conversion
SPEED		bit	ADCCSR,6	;  ADC clock selection
ADON		bit	ADCCSR,5	;  A/D converter On
CH3		bit	ADCCSR,3	;  Channel Selection
CH2		bit	ADCCSR,2	;  Channel Selection
CH1		bit	ADCCSR,1
CH0		bit	ADCCSR,0
ADCDRH		label	$0071		; Data Register high
ADCDRL		label	$0072		; Data Register low

;----------------------------------------------------------------------------
; PWM ART

PWMDCR3		label	$0073		; PWM AR Timer Duty Cycle Register 3
PWMDCR2		label	$0074		; PWM AR Timer Duty Cycle Register 2
PWMDCR1		label	$0075		; PWM AR Timer Duty Cycle Register 1
PWMDCR0		label	$0076		; PWM AR Timer Duty Cycle Register 0
PWMCR		label	$0077		; PWM AR Timer Control Register
ARTCSR		label	$0078		; Auto-Reload Timer Control/Status Register
EXCL		bit	ARTCSR,7	;  External Clock
CC		bfield	ARTCSR,4,3	;  Counter Clock Control
TCE		bit	ARTCSR,3	;  Timer Counter Enable
FCRL		bit	ARTCSR,2	;  Force Counter Re-Load
PWM_OIE		bit	ARTCSR,1	;  Overflow Interrupt Enable
OVF		bit	ARTCSR,0	;  Overflow Flag
ARTCAR		label	$0079		; Auto-Reload Timer Counter Access Register
ARTARR		label	$007a		; Auto-Reload Timer Auto-Reload Register
ARTICCSR	label	$007b		; AR Timer Input Capture Control/Status Register
ARTICR1		label	$007c		; AR Timer Input Capture Register 1
ARTICR2		label	$007d		; AR Timer Input Capture Register 2

		restore
		endif			; __reg72325inc
