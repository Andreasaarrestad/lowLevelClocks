.thumb
.syntax unified

.include "gpio_constants.s"
.include "sys-tick_constants.s"

.text
	.global Start

.global SysTick_Handler
.thumb_func
SysTick_Handler:

	// Inkrementer hvert tidels sekund
	ldr r5, [r7]
	ldr r2, = 1
	add r2, r5, r2

	cmp r2, 10
	beq Seconds

	str r2, [r7]

	BX LR

Seconds:

	ldr r2, = 0
	str r2, [r7]

	ldr r1, [r8]
	ldr r3, = 1
	add r1, r3, r1

	//Toggle LED
	ldr r2, = 1 << LED_PIN
	tst r1, #1 // TST bruker bare bakerste bit, typ modulo sammen med BNE
	BNE Odd

	//Skru av LED
	ldr r3, = GPIO_BASE + (PORT_SIZE * LED_PORT) + GPIO_PORT_DOUTCLR
	str r2, [r3] // Lampens out
	B Even

Odd:
	//Skru på LED
	ldr r3, = GPIO_BASE + (PORT_SIZE * LED_PORT) + GPIO_PORT_DOUTSET
	str r2, [r3]

Even:

	cmp r1, 60
	beq Minutes

	str r1, [r8]

	BX LR

Minutes:
	ldr r1, = 0
	str r1, [r8]

	ldr r3, [r9]
	ldr r2, = 1
	add r3, r2, r3

	str r3, [r9]

	BX LR

Start:

	//Konfigurer systick
	LDR R0, =SYSTICK_BASE+SYSTICK_CTRL
	LDR R1, = #0b110 //Siste bit 0 så klokka starter off
	STR R1, [R0]

	//Hvor lenge klokka går før interrupt
	LDR R0, =SYSTICK_BASE+SYSTICK_LOAD
	LDR R1, =FREQUENCY/10 // Klokka går på tidels sekunder
    STR R1, [R0]

	//Startsverdi for klokka
	LDR R5, =SYSTICK_BASE+SYSTICK_VAL
	STR R1, [R5]

	LDR R0, =GPIO_BASE
	ADD R0, R0, GPIO_EXTIPSELH //bRUKER EXTIPSELH siden knappen er på pin 9
	LDR R1, =#0b1111
	LSL R1, R1, #4 //Left shifter fra pin 8 til 9
	MVN R1, R1 //Intverterer alle bits
	LDR R2, [R0]
	AND R3, R2, R1
	LDR R1, =#PORT_B
	LSL R1, R1, #4 //Left shifter fra pin 9 til 8
	ORR R2, R3, R1
	STR R2, [R0]
	LDR R5, =GPIO_BASE
	ADD R5, R5, GPIO_EXTIFALL
	LDR R1, [R5]
	LDR R2, =#1 //Left shifter 1
	LSL R2, R2, #BUTTON_PIN
	ORR R3, R2, R1
	STR R3, [R5]
	LDR R5, =GPIO_BASE
	ADD R5, R5, GPIO_IEN
	LDR R1, [R5]
	LDR R2, =#1
	LSL R2, #BUTTON_PIN //Left shifter for å komme til buttons pin
	ORR R3, R2, R1
	STR R3, [R5]

	PUSH {LR}
	BL Clear
	POP {LR}

	ldr r7, = tenths
	ldr r8, = seconds
	ldr r9, = minutes



LOOP: // Evig løkke mens man venter på interrupt
B LOOP

// Eventhandler for knappen
.global GPIO_ODD_IRQHandler
.thumb_func
GPIO_ODD_IRQHandler:
	PUSH {LR}
	BL Clock
	POP {LR}

	PUSH {LR}
	BL Clear
	POP {LR}
	BX LR

Clock:
	LDR R0, =SYSTICK_BASE
	ADD R0, R0, SYSTICK_CTRL
	LDR R1, [R0]
	AND R1, R1, #1
	CMP R1, #1
	BNE ResumeClock //Branch if not equal

	PauseClock:
		LDR R2, =#0b110
		STR R2, [R0]
		B EndToggle

	ResumeClock:
		LDR R2, =#0b111
		STR R2, [R0]
	EndToggle:
	MOV PC, LR

Clear:
	LDR R0, =GPIO_BASE
	ADD R0, R0, GPIO_IFC
	LDR R1, [R0]
	LDR R2, =#1
	LSL R2, R2, #BUTTON_PIN //Left shift til button pin
	ORR R4, R4, R1
	STR R4, [R0]
	MOV PC, LR

NOP

