#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#define PWM_CHANNELS 22

uint16_t Actual[PWM_CHANNELS] ;
int16_t Delta[PWM_CHANNELS] ;
uint8_t Counter[PWM_CHANNELS] ;
uint8_t Step[PWM_CHANNELS] ;

/* Programm-Aufbau:
   Command,[Value],Command,[Value],...
   Command: 0: End
            1..200: DimTo Value: Dimme in Command Tics (Wenn >4 Sekunden, muss aufgeteilt werden) auf Wert
            201..220: JumpTo: Springe nach Command-201
	    221: SetTo Value: Setze Kanal auf Wert
	    222: Delay Value: Warte Wert Tics
*/

uint8_t PROG[PWM_CHANNELS][20] = {{ 221,127,20,10,20,220,201},{221,255,20,127,20,50,201}} ;

uint8_t GetProgram(uint8_t Channel, uint8_t PStep)
{
  if (PStep>=20) return (0) ;
  return (PROG[Channel][PStep]) ;
}

void pwm_update(void)
{
  int i ;
  for (i=0;i<PWM_CHANNELS;i++) printf ("%2X ",(uint8_t)(Actual[i]>>8)) ;
  printf ("\n") ;
}

void StepLight (void)
{
  uint8_t Channel ;
  uint8_t Command ;

  /* Alle Kanäle abarbeiten */
  for (Channel=0;Channel<PWM_CHANNELS;Channel++) {
    if (Counter[Channel]>0) {
      Actual[Channel] += Delta[Channel] ;
      Counter[Channel]-- ;
    } else {
      if (Step[Channel]>=20) continue ;
      Command = GetProgram (Channel,Step[Channel]) ;
      Step[Channel]++ ;
      if (Command==0) { /* Programm Ende */
	Step[Channel] = 20 ;
      } else if	(Command<201) { /* DimTo */
	Counter[Channel] = Command ;
	Command = GetProgram(Channel,Step[Channel]) ;
	Step[Channel]++ ;
	Delta[Channel] = (int16_t)(((((int32_t)Command)<<8)-(int32_t)Actual[Channel])/(int16_t)Counter[Channel]) ;
	if (Delta[Channel]>0) Delta[Channel]++ ; // Rundungsfehler ausgleichen
	Actual[Channel] += Delta[Channel] ; // und ersten Schritt ausführen
	Counter[Channel]-- ;
      } else if (Command<221) { /* JumpTo */
	Step[Channel] = Command-201 ;
      } else if (Command==221) { /* SetTo */
	Actual[Channel] = ((uint16_t)GetProgram(Channel,Step[Channel]))<<8 ;
	Step[Channel]++ ;
      } else if (Command==222) { /* Delay */
	Delta[Channel] = 0 ;
	Counter[Channel] = GetProgram(Channel,Step[Channel]) ;
	Step[Channel]++ ;
      } else { /* Unknown command */
	Step[Channel] = 20 ; /* Auf Ende Setzen */
      } ;
    } ;
  } ;
  pwm_update() ;
}


int main (int argc, char *argv[])
{
  int i ;
  for (i=0;i<PWM_CHANNELS;i++) {
    Step[i]= 0 ;
    Delta[i] = 0 ;
    Counter[i] = 0 ;
  }
  for (i=0;i<200;i++) StepLight() ;
  printf ("Test\n") ;
}
