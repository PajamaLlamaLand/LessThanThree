/*
 * data structures and methods for processing UI inputs
 */ 


#ifndef UI_H_
#define UI_H_

#include <stdio.h>
#include <ssd1306.h>
#include "conf_menu.h"	// for parameter string max char limit
#include "extint.h"
#include "events_hooks.h"
#include "rtc_count.h"
#include "tc_interrupt.h"
#include "menu.h"

enum UiElement {
	CH1_BUTTON,
	CH2_BUTTON,
	ENC_BUTTON
	};

enum LongPressCount {
	LONG_PRESS_SHORT,
	LONG_PRESS_MED,
	LONG_PRESS_LONG
	};

struct UI {
	uint8_t encA_state;			// holds most recent encoder pinA polls
	uint8_t encB_state;			// holds most recent encoder pinB polls
	uint16_t longPressTime;		// current global long press time in increments of the
								// polling period
	bool pressed[3];			// holds current states for long press checks
	uint16_t time_pressed[3];	// counter for button presses
	bool button_ignore[3];		// used for long-press logic in event_counter()
	
	};

struct UI ui;

void ui_init(struct events_resource *resource, struct events_hook *hook, struct rtc_module *instance);
void configure_evsys(struct events_resource *resource, struct events_hook *hook);
void configure_rtc(struct rtc_module *instance);

void event_counter(struct events_resource *resource);	// timer interrupt function

void writeLongPressTimes(uint8_t long_press_enum);

#endif /* UI_H_ */