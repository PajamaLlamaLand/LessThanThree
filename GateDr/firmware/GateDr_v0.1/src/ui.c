/*
 * source file for UI setup and callback functions
 */ 

#include "ui.h"

#define LONG_PRESS_COUNT_DEFAULT LONG_PRESS_MED
#define ENC_PINA	PIN_PA15
#define ENC_PINB	PIN_PA14

uint16_t longPressTimes[3] = {10, 20, 30};
uint8_t buttonPins[3] = {PIN_PA00, PIN_PA01, PIN_PA28};
uint8_t shortPressActions[3] = {ACTION_CH1, ACTION_CH2, ACTION_ENTER};
uint8_t longPressActions[3] = {ACTION_GLOBAL, ACTION_CV, ACTION_BACK};

/*
 *	set all UI struct values to their defaults, passing 3 tc_module instances from main()
 *	which are then configured and enabled
*/
void ui_init(struct events_resource *resource, struct events_hook *hook, struct rtc_module *instance) {
	for (uint8_t i=0; i<3; i++) {
		ui.pressed[i] = false;
		ui.time_pressed[i] = 0;
		ui.button_ignore[i] = false;
	}
	ui.encA_state = 0xff;
	ui.encB_state = 0xff;
	writeLongPressTimes(LONG_PRESS_COUNT_DEFAULT);

	configure_evsys(resource, hook);
	configure_rtc(instance);
}

/*
 *	configuration for EVSYS module to attach RTC periodic interrupts to the event_counter()
 *	function
*/
void configure_evsys(struct events_resource *resource, struct events_hook *hook) {
	struct events_config events_conf;
	events_get_config_defaults(&events_conf);
	events_conf.generator = EVSYS_ID_GEN_RTC_PER_1;	// RTC period 1- 2kHz
	events_conf.clock_source = GCLK_GENERATOR_2;	// match RTC GCLK generator for synchronous operation
	events_allocate(resource, &events_conf);
	events_attach_user(resource, 0x0);				// no attached user needed
	
	events_create_hook(hook, event_counter);
	events_add_hook(resource, hook);
	events_enable_interrupt_source(resource, EVENTS_INTERRUPT_DETECT);
}

/*
 *	configuration for RTC module, used for UI polling on a periodic event
*/
void configure_rtc(struct rtc_module *instance) {
	struct rtc_count_config rtc_conf;
	rtc_count_get_config_defaults(&rtc_conf);			// GCLK source OSC32K on GCLK2
	rtc_conf.prescaler = RTC_COUNT_PRESCALER_DIV_32;	// 1kHz count frequency
	
	rtc_count_init(instance, RTC, &rtc_conf);
	instance->hw->MODE0.EVCTRL.reg = RTC_MODE0_EVCTRL_PEREO1;	// 2kHz polling freq
	rtc_count_enable(instance);
}

/*
 *	timer interrupt attached to above RTC periodic event set up above
 *	Polls buttons, stores results in the UI struct instance, and increments
 *	counters 
*/
void event_counter(struct events_resource *resource) {
	static uint8_t button_counter = 0;
	
	ui.encA_state = (ui.encA_state << 1) + port_pin_get_input_level(ENC_PINA);
	ui.encB_state = (ui.encB_state << 1) + port_pin_get_input_level(ENC_PINB);
	
	// check pinA falling edge
	if ((ui.encA_state & 0xf) == 0b1100) {
		// check pinB state for decrement
		if (!(ui.encB_state & 0x01)) {
			menu.enc_count -= 1;
		}
	}
	// check pinB falling edge
	else if ((ui.encB_state & 0xf) == 0b1100) {
		// check pinA state for increment
		if (!(ui.encA_state & 0x01)) {
			menu.enc_count += 1;
		}
	}
	
	// set flag for main loop menu function
	if (menu.enc_count != 0) {
		uint8_t action = (menu.enc_count > 0) ? ACTION_INC : ACTION_DEC;
		setMenuFlag(action);
	}
	
	// pushbuttons don't require checking nearly as often, so we only check
	// at around 50Hz 
	button_counter++;
	
	if (button_counter >= 40) {
		for (uint8_t i=0; i<3; i++) {
		ui.pressed[i] = !port_pin_get_input_level(buttonPins[i]);
		if (ui.pressed[i]) {
			ui.time_pressed[i]++;
		}
		
		// check for long press
		if (ui.pressed[i] && (ui.time_pressed[i] >= ui.longPressTime) && !ui.button_ignore[i]) {
			setMenuFlag(longPressActions[i]);
			ui.button_ignore[i] = true;
		}
		// short press & button released logic
		else if (!ui.pressed[i]) {
			if ((ui.time_pressed[i] < ui.longPressTime) && (ui.time_pressed[i] != 0)) {
				setMenuFlag(shortPressActions[i]);
			}
			ui.time_pressed[i] = 0;
			ui.button_ignore[i] = false;
			}
		}
		button_counter = 0;
	}
}

/*
 *	writes the current long press time setting to the ui struct instance
*/
void writeLongPressTimes(uint8_t long_press_enum) {
	ui.longPressTime = longPressTimes[long_press_enum];
}