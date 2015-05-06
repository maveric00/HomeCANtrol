/*****************************************************/

#ifndef WS2812_H
#define WS2812_H

#define	MAXWSNUM 500

#define WS_TIM_FREQ 72000000
#define WS_OUT_FREQ   800000

// timer values to generate a "one" or a "zero" 
#define WS_ONE			54
#define WS_ZERO			27

// number of timer cycles (~1.25µs) for the reset pulse
#define WS_RESET_LEN		90

// three colors per led, eight bits per color
#define WS_DMA_LEN		(MAXWSNUM * 3 * 8 + WS_RESET_LEN)

typedef struct {
	uint8_t		R;
	uint8_t		G;
	uint8_t		B;
} rgb_t;

// ----------------------------- variables -----------------------------

extern rgb_t WSRGB[MAXWSNUM];
extern volatile uint8_t	ledBusy;					// = 1 while dma is sending data to leds
extern int WSDimmer ;
extern int CurrentWSNum ;

// ----------------------------- functions -----------------------------
void WSinit(void);
void WSupdate(void);
#endif
