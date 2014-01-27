/*
 * BCM Routine, inspiriert vom Thread "24fach 16Bit BCM logarithmisch" auf ledstyles.de, Original von bkwei
 *
 */ 
 
#include "avr/io.h"

#define BITCOUNT  14		; Bittiefe der PWM 10-16
#define MAXLIGHT 255			; Masterhelligkeit 0-255 (kann an Adresse aMaster jederzeit geaendert werden)


// MACROS Zeitschleifen


.macro nops82			; Macro Zeitschleife 82 Takte
	ldi r25,27		; 27 Schleifendurchlaeufe (1 Takt)
L1\@:				; Schleifenbeginn
	dec r25			; 1 Takt
	brne L1\@		; 2 Takte, wenn Sprung, 1 Takt bei der letzten
	nop			; 82
.endm

.macro nops23			; Macro Zeitschleife 23 Takte
	ldi r25,7		; 7 Schleifendurchlaeufe (1 Takt)
L2\@:				; Schleifenbeginn
	dec r25			; 1 Takt
	brne L2\@		; 2 Takte, wenn Sprung, 1 Takt bei der letzten
	nop			; 22
	nop			; 23
.endm

.macro nops17			; Macro Zeitschleife 17 Takte
	ldi r25,5		; 5 Schleifendurchlaeufe (1 Takt)
L3\@:				; Schleifenbeginn
	dec r25			; 1 Takt
	brne L3\@		; 2 Takte, wenn Sprung, 1 Takt bei der letzten
	nop			; 16
	nop			; 17
.endm

#define PWMOut1(a) out _SFR_IO_ADDR(DDRB),a
#define PWMOut2(a) out _SFR_IO_ADDR(DDRC),a
#define PWMOut3(a) out _SFR_IO_ADDR(DDRD),a
#define PWMOut4(a) out _SFR_IO_ADDR(DDRE),a

// MACROS Definition der Ausgabeports - die Ports muessen noch separat auf Ausgang gesetzt werden

.data 

.global LEDval
.global LEDPWM
.global MasterVal
.global PWMISR

.comm LEDVal,24 ;		; Adresse an der die 24 Helligkeitswerte der Led's abgelegt werden muessen
.comm LEDPWM,4*BITCOUNT;	; Adresse der Daten fuer die PWM (Ausgabe an die Ports)
.comm CurrentBit,1; 		; Adresse des Bitzaehlers, hier wird die Wertigkeit des als naechstes abzuarbeitenden Bits gespeichert
.comm MasterVal,1;		; Adresse der Masterhelligkeit, Wert kann an Adresse MasterVal jederzeit geaendert werden

.text

;--------------------------------------------------------------
.global InitBCM
InitBCM:

// Port A,B,C als Ausgang konfigurieren
    push r24
	ldi r24,0
	out _SFR_IO_ADDR(PORTB), r24
	out _SFR_IO_ADDR(PORTC), r24
	in  r24,_SFR_IO_ADDR(PORTD)
	andi r24,0x5E
	out _SFR_IO_ADDR(PORTD), r24
	in  r24,_SFR_IO_ADDR(PORTE)
	andi r24,0xF8
	out _SFR_IO_ADDR(PORTE), r24
	
// CurrentBit im Ram auf 0 setzen
	ldi r24,0
	sts CurrentBit,r24		

// Masterhelligkeit
// Konstante MAXLIGHT an Adresse MasterVal Ram schreiben
// der Wert kann spaeter an Adresse MasterVal jederzeit geaendert werden
// 0 - 255  -  Master Helligkeit
	ldi r24, MAXLIGHT
	sts MasterVal,r24

// Vorteiler Timer1                           
	;0b00000000 TIMER aus                 
	;0b00000001 CLOCK=TAKTFREQUENZ        
	;0b00000010 CLOCK=TAKTFREQUENZ / 8    
	;0b00000011 CLOCK=TAKTFREQUENZ / 64   
	;0b00000100 CLOCK=TAKTFREQUENZ / 256  
	;0b00000101 CLOCK=TAKTFREQUENZ / 1024 
	;0b00000110 Abfallende Flanke Pin T0  
	;0b00000111 Steigende  Flanke Pin T0  
	ldi r24,1           
	sts TCCR1B,r24          ;Timer1 Vorteiler keiner

// Timer1-UEberlauf-Int aktivieren				 
	;OCIE2  => 									 
	;TOIE2  => Interrupt bei UEberlauf (TIMER2)   
	;TICIE1 => Interrupt bei Capture             
	;OCIE1A => Interrupt bei Erreichen Stand in A
	;OCIE1B => Interrupt bei Erreichen Stand in B
	;TOIE1  => Interrupt bei UEberlauf (TIMER1)   
	;TOIE0  => Interrupt bei UEberlauf (TIMER0)   
	ldi r24,(1<<TOIE1)
	sts TIMSK1,r24          

// PWM Werte erzeugen
    rcall FillPWM

// Timer1 Startwert 
	ldi r24,255	;HIGH
	sts TCNT1H,r24         
	ldi r24,0	;LOW   
	sts TCNT1L,r24    
     
    pop r24
    ret

;--------------------------------------------------------------


;--------------------------------------------------------------
 .global TIMER1_OVF_vect                 ; Note [7]
TIMER1_OVF_vect:
// Register sichern
	push r24
	push r25
	push ZL
	push ZH
	push XL
	push XH
	push r2
	push r3
	push r4
	push r5
	push r6
	push r7
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13     ; (36T)
	push r14
	
	in r14,_SFR_IO_ADDR(SREG)
// Ausgleich (bis 11 Takte) - (kostet 20 Takte)
// wenn Timerinterrupt verzoegert wird


	lds ZL,TCNT1L			; Timer LOW in ZL laden
	subi ZL,17					; 17 Subtrahieren (Timer muesste bei 17 stehen)
	cpi ZL,12					; Test, ob weniger als 11 Takte korrigiert werden muessen
	brcs KorOK					; wenn mehr als 11 Takte
	ldi ZL,11					; dann wenigstens 11 Takte korrigieren
KorOK:
	clr ZH
	subi ZL,pm_lo8(-(Korrektur))				; Adresse Korrektur zu Z "addieren"
	sbci ZH,pm_hi8(-(Korrektur))	
	ijmp
Korrektur:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
// BitZaehler aus Ram laden	
	lds r24,CurrentBit		; 56 Takte seit Interrupt
// wenn CurrentBit = 0 dann "Sprung" zu Bit0 sonst zu BitWeiter	
	cpi r24,0			; (1T)
	breq Bit0			; (1T, wenn nicht gesprungen wird)
	
	rjmp BitWeiter			; (2T)

Bit0:

// Adresse LEDPWM in X laden !!! wird immer weiter genutzt	!!!
	ldi XL,lo8(LEDPWM)						
	ldi XH,hi8(LEDPWM)
// hier werden PWM-Werte in die Register vorgeladen	
	ld r2,X+	;1.1 :	 2 Takte
	ld r3,X+	;1.2 :	 2 Takte
	ld r4,X+	;1.3 :	 2 Takte
	ld r5,X+	;1.4 :	 2 Takte
	ld r6,X+	;2.1 :	 4 Takte
	ld r7,X+	;2.2 :	 4 Takte
	ld r8,X+	;2.3 :	 4 Takte
	ld r9,X+	;2.4 :	 4 Takte
	ld r10,X+	;3.1 :	 8 Takte
	ld r11,X+	;3.2 :	 8 Takte
	ld r12,X+	;3.3 :	 8 Takte
	ld r13,X+	;3.4 :	 8 Takte
	ld ZL,X+	;4.1 :	 16 Takte
	ld ZH,X+	;4.2 :	 16 Takte
	ld r24,X+	;4.3 :	 16 Takte
	ld r25,X+	;4.4 :	 16 Takte
// ab hier werden die PWM-Werte auf die Ports ausgegeben
// und immer, wenn Zeit ist die PWM-Werte fuer die naechsten Ausgaben in die Register vorgeladen
	PWMOut1 (ZL)		; 4. Bit
	PWMOut2 (r11)		; 3. Bit
	PWMOut3 (r8)		; 2. Bit
	PWMOut4 (r5)		; 1. Bit
	nop
	PWMOut4 (r25)		; 4. Bit
	PWMOut3 (r4)		; 1. Bit
	nop
	PWMOut3 (r24)		; 4. Bit
	PWMOut2 (r7)		; 2. Bit
	nop
	ld r7,X+
	PWMOut2 (r3)		; 1. Bit
	nop
	PWMOut2 (ZH)		; 4. Bit
	PWMOut1 (r6)		; 2. Bit
	nop
	ld r8,X+
	PWMOut1 (r2)		; 1. Bit
	PWMOut4 (r9)		; 2. Bit
	PWMOut1 (r10)		; 3. Bit
	nop
	PWMOut3 (r12)		; 3. Bit
	PWMOut4 (r13)		; 3. Bit
	ld r4,X+
	ld r5,X+
	PWMOut1 (r7)		; 5. Bit
	PWMOut2 (r8)		; 5. Bit
	PWMOut3 (r4)		; 5. Bit
	PWMOut4 (r5)		; 5. Bit
	ld r4,X+
	ld r5,X+
	ld r6,X+
	ld r7,X+
// 32 - 4 - 8 - 3 = 17 nops
	nops17
// CurrentBit in r24 und im Ram auf 6 setzen	
	ldi r24,6				; fuer spaeter - Bit 6
	sts CurrentBit,r24			
	PWMOut1(r4)		; 6. Bit	(1T)  -> 41 Takte bis zum naechsten PWMOut1 ->23 Takte warten
	PWMOut2(r5)		; 6. Bit	(1T)
	PWMOut3(r6)		; 6. Bit	(1T)
	PWMOut4(r7)		; 6. Bit	(1T)
	nops23
// ab hier werden fuer die hoeherwertigen Bits die Werte aus dem Ram geladen,
// die Werte auf die Ports ausgegeben und der Timer entsprechend des Bits geladen

BitWeiter:					; Hier sind 60 Takte seit Interrupt vergangen, 37 folgen noch bis PWMOut1
						;  ->97 Takte von Int bis PWMOut, gesetzt wurde der Timer  22 vor dem letzten PWMOut
						; -> 119 Takte muessen beim Timer vom 1. Theoriewert abgezogen werden, 97 vom 2.-x.
// Timer laden (hoeherwertige Bits)

	mov ZL,r24				; (1T) Z-Register mit CurrentBit, steht schon in r24, laden (
	mov r25,r24				; (1T)
	clr ZH					; (1T)
	lsl ZL					; (1T) Z Register * 2 wegen Word
	subi ZL,lo8(-(Timer))			; (1T)Anfangsadresse der Tabelle Timer "addieren"
	sbci ZH,hi8(-(Timer))			; (1T)
	lpm r2,Z+				; (3T)Timer(low)  Wert(TimerDaten) aus Flash holen  
	lpm r3,Z				; (3T)Timer(high) Wert(TimerDaten) aus Flash holen  
	sts TCNT1H,r3				; (2T)TimerHigh-Byte setzen
	sts TCNT1L,r2				; (2T)TimerLow- Byte setzen  	22 Takte bis zum naechstem PWMOut1	
// CurrentBit in Ram um 1 erhoehen, wenn hoechstes Bit erreicht dann auf 0 setzen
	inc r25					; (1T) 
	mov ZH,r25				; (1T)

	cpi ZH,BITCOUNT				; (1T)
	brne NotFinished			; (2T inkl clr oder ohne) 	

	clr r25					; ansonsten loesche CurrentBit

NotFinished:	
	sts CurrentBit,r25			; (2T)
// CurrentBit (steht schon in r24) * 4 in ZL laden
	mov ZL,r24				; (1T)
	lsl ZL					; (1T) 
	lsl ZL					; (1T)
// dazu Adresse LEDPWM addieren
	clr ZH					; (1T) 
	subi ZL,lo8(-(LEDPWM))			; (1T) 
	sbci ZH,hi8(-(LEDPWM))			; (1T) 
// PWM-Werte aus Ram holen Byte 1,2,3
	ld r2,Z+				; (2T)
	ld r3,Z+				; (2T)
	ld r4,Z+				; (2T) 
	ld r5,Z+				; (2T)
	PWMOut1(r2)				
	PWMOut2(r3)				
	PWMOut3(r4)
	PWMOut4(r5)				
	
	cpi r25,7		;  da die Zeit fuer einen Interrupt nicht reicht, hier abkuerzen
	brne NotSeventh
				; 128 -6 - 37 -2 Takte = 83 Takte warten

	nops82
	inc r24
	rjmp BitWeiter

NotSeventh:	

	tst r25			;  in r25 ist noch der CurrentBit-Counter, nach dem 14. Bit wieder die PWM fuellen
	brne Finished
	
	rcall FillPWM					

Finished:
// Register wiederherstellen
    out _SFR_IO_ADDR(SREG),r14
    pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop r7
	pop r6
	pop r5
	pop r4
	pop r3
	pop r2
	pop XH
	pop XL
	pop ZH
	pop ZL
	pop r25
	pop r24
	reti  


.global FillPWM
FillPWM:
	push r0
	push r1
	push r2
	push r3
	push r4
	push r5
	push r6
	push r7
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push r16
	push r17
	push r18
	push r19
	push r24
	push r25
	push YL
	push YH
	push ZL
	push ZH
	push r20
	
	in r20,_SFR_IO_ADDR(SREG)

	
// Led Werte mit Masterhelligkeit verrechnen, auf logarithmische Werte umwandeln und
// fuer die PWM Ausgabe die jeweils gleichwertigen Bits (0-15) in das Ram (LEDPWM) schreiben, das Ganze 3 mal
// Schleife fuer 1-3 Byte  -  8,16,24 Werte	
	ldi r18,3						; Lade Register mit Schleifenwert (1-3 Byte = 8,16,24Bit)

Loop1:									
// YL mit Schleifenwert laden (3-1)	
	mov YL, r18
// auf Wert 2,1 oder 0	
	dec YL											
// mit 8 multiplizieren = (16,8,0) 1*Swap + 1*ShiftRight
	swap YL											
	lsr YL
// dazu Anfangsadresse vom 1. LedWert(0-255) "addieren"
	clr YH
	subi YL,lo8(-(LEDVal))				
	sbci YH,hi8(-(LEDVal))	
// Schleife fuer 8 bit, es werden, bei 16Bit Bittiefe,
// 2 * 8 Bit also 16 Bit hintereinander abgearbeitet -> 8*16=128 Bitschiebeoperationen
	ldi r19,8						; Lade Register mit Schleifenwert (8 Bit)

Loop2:									
// hier werden die Helligkeitswert 0-255) in 10-16 Bit logarithmische Werte umgewandelt
// bzw. diese aus der Tabelle geholt
// Offset (entspricht dem Helligkeitswert 0-255)*(MasterHelligkeit+1(1-256)/256 der Adresse der logarithmischen Tabelle in ZL laden 
	lds r24,MasterVal			; Master Helligkeit
	ld  r25,Y+				; Led Helligkeit
	mul r24,r25			; Master * Led
	clr r24
	add r0,r25				; + Led
	adc r1,r24				; Carry "addieren"
	mov ZL,r1				; High Byte in ZL schreiben entspricht r0:r1 /256 -> ZL 				
	clr	ZH									
// Z*2 weil Word
	lsl ZL
	rol ZH
// Offset zur Anfangsadresse der logarithmischen Tabelle "addieren"
	subi ZL,lo8(-(exptab))				
	sbci ZH,hi8(-(exptab))	
// Tabellenwert der logarithmischen Tabelle entsprechend dem Led-Wert und der Tabelle (angepasst, unangepasst) holen	
	lpm r0,Z+						;Tabellenwert lesen low
	lpm r1,Z						;Tabellenwert lesen high
// durch rotieren aus den 8*10-16Bit log-Werten 10-16*8Bit PWM-Werte erstellen
	lsr r0
	ror r2
	lsr r0
	ror r3
	lsr r0
	ror r4
	lsr r0
	ror r5
	lsr r0
	ror r6
	lsr r0
	ror r7
	lsr r0
	ror r8
	lsr r0
	ror r9
	lsr r1
	ror r10
	lsr r1 
	ror r11
	lsr r1
	ror r12
	lsr r1
	ror r13
	lsr r1
	ror r14
	lsr r1
	ror r15
	dec r19				; Zaehlerwert um Eins verringern
	breq Loop2Weiter		; wenn nicht Null dann Schleifenbeginn ueber einen Zwischensprung, weil ueber 63 Schritte
	rjmp Loop2	
// YL mit Schleifenwert laden (3-1)
Loop2Weiter:
	mov YL, r18
// auf Wert 2,1 oder 0	
	dec YL											
// dazu Anfangsadresse vom 1. LedWert(0-255) "addieren"
	clr YH
	subi YL,lo8(-(LEDPWM))				
	sbci YH,hi8(-(LEDPWM))	
// PWM-Werte in Ram speichern (LEDPWM)	
	st Y, r2			; Bit  0
	std Y+4, r3			; Bit  1
	std Y+8, r4			; Bit  2
	std Y+12, r5			; Bit  3
	std Y+16, r6			; Bit  4
	std Y+20, r7			; Bit  5
	std Y+24, r8			; Bit  6
	std Y+28, r9			; Bit  7
	std Y+32, r10			; Bit  8
	std Y+36, r11			; Bit  9
	std Y+40, r12			; Bit 10
	std Y+44, r13			; Bit 11
	std Y+48, r14			; Bit 12
	std Y+52, r15			; Bit 13
	dec r18				; Zaehlerwert um Eins verringern
	breq Loop1Weiter		; wenn nicht Null dann Schleifenbeginn ueber einen Zwischensprung, weil ueber 63 Schritte	
// Bit null Flagregister loeschen, bedeutet Werte sind umgewandelt in 10-16 Bit PWM-Werte
	rjmp Loop1
Loop1Weiter:

	clr YH
	clr YL
	subi YL,lo8(-(LEDPWM))				
	sbci YH,hi8(-(LEDPWM))	
	ldi r18,BITCOUNT						; Lade Register mit Schleifenwert (1-3 Byte = 8,16,24Bit)
//
Loop3:
	ldd r24,Y+1    ; C-Port Pin 1 ist Input vom Bewegungsmelder -> Eingang schalten
	andi r24,253
	ori r24,5       ; C-Port Pin2: LED im Bewegungsmelder,Pin0: PWM-Output -> Ausgang schalten
	std Y+1,r24
	ldd r24,Y+2
	mov r25,r24
	ror r24
	andi r24,7
	in	r19,_SFR_IO_ADDR(DDRE)
	andi r19,0xF8
	or r24,r19
	std Y+3,r24
	andi r25,0xA1
	in	r19,_SFR_IO_ADDR(DDRD)
	andi r19,0x5E
	ori r19,1        	; D-Port Pin0: PWM-Output
	or r25,r19
	std Y+2,r25	
	adiw YL,4
	dec r18
	brne Loop3

	rcall PWMISR

	out _SFR_IO_ADDR(SREG),r20
	pop r20
	pop ZH
	pop ZL
	pop YH
	pop YL
	pop r25
	pop r24
	pop r19
	pop r18
	pop r17
	pop r16
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop r7
	pop r6
	pop r5
	pop r4
	pop r3
	pop r2
	pop r1
	pop r0
	ret

	Timer:		;TimerDaten der hoeheren Bits (7-13), 6. Bit ist ein Dummy-Wert
	.WORD 60000,60000,60000,60000,60000,60000,60000,65399,65143,64631,63607,61559,57463,49271

	// ExpotentialTabelle 
	// 14Bit
exptab:
	.WORD	 	0,1,2,3,4,5,6,7,9,10,11,12,13,15,16,17
	.WORD	 	19,20,22,23,25,26,28,29,31,33,35,36,38,40,42,44
	.WORD	 	46,48,50,52,55,57,59,62,64,67,69,72,74,77,80,83
	.WORD	 	86,89,92,95,98,102,105,109,112,116,119,123,127,131,135,139
	.WORD	 	144,148,153,157,162,167,172,177,182,187,193,198,204,210,216,222
	.WORD	 	228,235,241,248,255,262,269,276,284,292,299,308,316,324,333,342
	.WORD	 	351,360,370,380,390,400,410,421,432,443,455,467,479,491,504,517
	.WORD	 	530,543,557,572,586,601,616,632,648,664,681,698,716,734,752,771
	.WORD	 	790,810,830,851,872,894,916,939,962,986,1010,1035,1061,1087,1114,1142
	.WORD	 	1170,1198,1228,1258,1289,1321,1353,1386,1420,1455,1490,1527,1564,1602,1641,1681
	.WORD	 	1722,1764,1807,1851,1896,1942,1989,2037,2087,2137,2189,2242,2296,2352,2409,2467
	.WORD	 	2526,2588,2650,2714,2779,2846,2915,2985,3057,3131,3206,3284,3363,3443,3526,3611
	.WORD	 	3698,3787,3878,3971,4066,4164,4264,4366,4471,4578,4688,4800,4915,5033,5154,5277
	.WORD	 	5404,5533,5666,5801,5940,6082,6228,6377,6530,6686,6846,7009,7177,7348,7524,7704
	.WORD	 	7888,8076,8269,8467,8669,8876,9088,9305,9527,9755,9988,10226,10470,10720,10976,11237
	.WORD	 	11505,11780,12061,12349,12643,12945,13253,13569,13893,14224,14563,14910,15265,15629,16002,16383

