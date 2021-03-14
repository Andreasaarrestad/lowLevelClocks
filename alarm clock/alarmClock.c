#include "o3.h"
#include "gpio.h"
#include "systick.h"

#define LED_PORT 4
#define LED_PIN 2

#define STATE_SET_SEC 0
#define STATE_SET_MIN 1
#define STATE_SET_HOUR 2
#define STATE_COUNT_DOWN 3
#define STATE_ALARM 4

char timestamp[7];
int state;
port_pin_t PB0;
port_pin_t PB1;
port_pin_t LED0;

typedef struct {
	volatile word CTRL;
	volatile word MODEL;
	volatile word MODEH;
	volatile word DOUT;
	volatile word DOUTSET;
	volatile word DOUTCLR;
	volatile word DOUTTGL;
	volatile word DIN;
	volatile word PINLOCKN;
} gpio_port_map_t;

typedef struct {
	volatile gpio_port_map_t ports[6];
	volatile word unused_space[10];
	volatile word EXTIPSELL;
	volatile word EXTIPSELH;
	volatile word EXTIRISE;
	volatile word EXTIFALL;
	volatile word IEN;
	volatile word IF;
	volatile word IFS;
	volatile word IFC;
	volatile word ROUTE;
	volatile word INSENSE;
	volatile word LOCK;
	volatile word CTRL;
	volatile word CMD;
	volatile word EM4WUEN;
	volatile word EM4WUPOL;
	volatile word EM4WUCAUSE;
} gpio_minne_peker_t;

gpio_minne_peker_t* gpio_minne_peker = (gpio_minne_peker_t*) GPIO_BASE;

volatile struct systick_t {
	word CTRL;
	word LOAD;
	word VAL;
	word CALIB;
}*SYSTICK = (struct systick_t*)SYSTICK_BASE;

struct time_t {
	int h, m, s;
};
struct time_t time = {0, 0, 0};

void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset] = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;
		
	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

void update_led(){
	time_to_string(timestamp, time.h, time.m, time.s);
	lcd_write(timestamp);
}

void decrement_hour() {
	--time.h;
}

void decrement_min() {
	--time.m;
	if (time.m < 0) {
		time.m = 59;
		decrement_hour();
	}
}

void decrement_sec() {
	--time.s;
	if (time.s < 0) {
		time.s = 59;
		decrement_min();
	}
}

void clock_start() {
	SYSTICK->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void clock_stop() {
	SYSTICK->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
}

// PB0
void GPIO_ODD_IRQHandler() {
	switch (state) {
		case STATE_SET_SEC:
			time.s = (time.s + 1) % 60;
			break;
		case STATE_SET_MIN:
			time.m = (time.m + 1) % 60;
			break;
		case STATE_SET_HOUR:
			time.h = (time.h + 1) % 60;
			break;
	}
	update_led();
	btn_IFC(PB0);
}

// PB1
void GPIO_EVEN_IRQHandler() {
	switch (state) {
		case STATE_SET_SEC:
			state = STATE_SET_MIN;
			break;
		case STATE_SET_MIN:
			state = STATE_SET_HOUR;
			break;
		case STATE_SET_HOUR:
			state = STATE_COUNT_DOWN;
			clock_start();
			break;
		case STATE_ALARM:
			state = STATE_SET_SEC;
			gpio_minne_peker -> ports[LED_PORT].DOUTCLR = 1 << LED_PIN; //Turn off LED
			break;
	}
	update_led();
	btn_IFC(PB1);
}


void SysTick_Handler() {
	if(state == STATE_COUNT_DOWN){
		decrement_sec();
		update_led();

		if(time.s == 0 && time.m == 0 && time.h == 0){
			state = STATE_ALARM;
			clock_stop();
			gpio_minne_peker->ports[LED_PORT].DOUTSET = 1 << LED_PIN; //Turn on LED

			return;
		}
	}
	return;
}

void btn_DOUT(port_pin_t btn) {
  gpio_minne_peker->ports[btn.port].DOUT = 0;
  word mask = ~(0b1111 << 4*(btn.pin % 8)) & (*gpio_minne_peker).ports[btn.port].MODEH;
  gpio_minne_peker->ports[btn.port].MODEH = mask | (GPIO_MODE_INPUT << 4*(btn.pin % 8));
}

void btn_EXTIPSELH(port_pin_t btn) {
    word mask = ~(0b1111 << 4*(btn.pin % 8)) & (*gpio_minne_peker).EXTIPSELH;
    gpio_minne_peker->EXTIPSELH = mask | (0b0001 << 4*(btn.pin % 8));
}

void btn_EXTIFALL(port_pin_t btn) {
    word mask = (*gpio_minne_peker).EXTIFALL | (1 << btn.pin);
    gpio_minne_peker->EXTIFALL = mask;
}

void btn_IEN(port_pin_t btn) {
    word mask = (*gpio_minne_peker).IEN | (1 << btn.pin);
    gpio_minne_peker->IEN = mask;
}

void btn_IFC(port_pin_t btn) {
  word mask = (*gpio_minne_peker).IFC | (1 << btn.pin);
  gpio_minne_peker->IFC = mask;
}

void led_MODEL(port_pin_t led) {
	gpio_minne_peker -> ports[led.port].MODEL =((~(0b1111<<led.pin*4)& gpio_minne_peker -> ports[led.port].MODEL))|(GPIO_MODE_OUTPUT<<led.pin*4);
}


int main(void) {
    init();

    //Set default state
    int state = STATE_SET_SEC;

    // Setup systick
    SYSTICK->LOAD = FREQUENCY;
    SYSTICK->VAL = FREQUENCY;
    SYSTICK->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;

    // Setup left button
	PB0.port = GPIO_PORT_B;
	PB0.pin = 9;
	btn_DOUT(PB0);
	btn_EXTIPSELH(PB0);
	btn_EXTIFALL(PB0);
	btn_IEN(PB0);
	btn_IFC(PB0);

	// Setup right button
	PB1.port = GPIO_PORT_B;
	PB1.pin = 10;
	btn_DOUT(PB1);
	btn_EXTIPSELH(PB1);
	btn_EXTIFALL(PB1);
	btn_IEN(PB1);
	btn_IFC(PB1);

	// Setup led
	LED0.port = LED_PORT;
	LED0.pin = LED_PIN;
	led_MODEL(LED0);

    update_led();
}





