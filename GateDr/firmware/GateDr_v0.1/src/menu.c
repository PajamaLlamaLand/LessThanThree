/*
 * source file for menu navigation functions
 */ 

#include "menu.h"

#define DEFAULT_MENU	MENU_CHANNEL_1
#define MENU_COUNT		8

// framebuffers for each menu page 
static uint8_t framebuffers[MENU_COUNT][GFX_MONO_LCD_FRAMEBUFFER_SIZE];

// string lists for menu parameters
const char *globalSettingsStrings[] = {"CH1", "CH2", "CV", "Reset", "Long-press", "Screen off"};
const char *channelMenuStrings[] = {"Inputs", "OP 1", "OP 2", "Outputs"};
const char *inputsMenuStrings[] = {"1-thrsh", "1-hys", "1-inv", "2-copy in1", "2-thrsh", "2-hys", "2-inv"};
const char *outputsMenuStrings[] = {"1-div", "1-div phase", "1-div reset", "1-delay", "1-prob", "1-trig mode", "1-trig len", 
				"2-mode", "2-div", "2-div phase", "2-div reset", "2-delay", "2-prob", "2-trig mode", "2-trig len"};
const char *cvMenuStrings[] = {"CV1 range", "CV1 thresh", "CV2 range", "CV2 thresh"};
	
// screen saver count times
const uint32_t screenSaverTimes[2] = {300000, 900000};

// list of pointers to the menus corresponding to the Menus enum
struct gfx_mono_menu *menuList[] = {&globalMenu, &channel1Menu, &inputs1Menu,
&outputs1Menu, &channel2Menu, &inputs2Menu, &outputs2Menu, &cvMenu};

// declaration for static inline helper functions
//static inline void drawScreen(void);
static inline void setMenu(uint8_t menu_index);
static inline void writeNVM(void);

// this second batch of helper functions is to keep the higher level processAction
// functions cleaner and avoid nested switches 
static void updateGlobalMenuParam(bool inc);
static void globalMenuEnter(void);
static void updateChannelMenuParam(bool inc);
static void channelMenuEnter(void);
static void updateInputsMenuParam(bool inc);
static void inputsMenuEnter(void);
static void updateOutputsMenuParam(bool inc);
static void outputsMenuEnter(void);
static void updateCvMenuParam(bool inc);
static void cvMenuEnter(void);

// generalized pointers to updateParam and menuEnter functions, used by higher level
// processAction functions
typedef void (*updateMenuParam)(bool inc);
static const updateMenuParam updateParamTable[MENU_COUNT] = {updateGlobalMenuParam, updateChannelMenuParam,
					updateInputsMenuParam, updateOutputsMenuParam, updateChannelMenuParam,
					updateInputsMenuParam, updateOutputsMenuParam, updateCvMenuParam};

typedef void (*menuEnter)(void);
static const menuEnter menuEnterTable[MENU_COUNT] = {globalMenuEnter, channelMenuEnter, inputsMenuEnter,
					outputsMenuEnter, channelMenuEnter, inputsMenuEnter,
					outputsMenuEnter, cvMenuEnter};

/*
 *	updates the current menu and channel context to the menu indicated by the 
 *	menu index passed in, and updates the gfx_mono_menu service framebuffer pointer
*/
static inline void setMenu(uint8_t menu_index) {
	menu.currentMenu = menu_index;
	
	// set channel context
	switch (menu_index) {
		case MENU_CHANNEL_1:
		case MENU_INPUTS_1:
		case MENU_OUTPUTS_1:
			menu.currentChannel = 0;
			break;
		case MENU_CHANNEL_2:
		case MENU_INPUTS_2:
		case MENU_OUTPUTS_2:
			menu.currentChannel = 1;
			break;
		default:
			// no change
			break;
	}
	
	gfx_mono_set_framebuffer(menuList[menu.currentMenu]->fbPointer);
}

/*
 *	called when a parameter has been edited, writes the current channel or
 *	global settings (determined on current menu context) to non-volatile memory.
 *	
 *	this shouldn't be too bad to the physical NVM as the buffer implemented in the 
 *	emulated EEPROM service will only write to physical NVM when a different channel
 *	context is edited, or on BOD
*/
static inline void writeNVM(void) {
	switch(menu.currentMenu) {
		// emulated EEPROM page 2
		case MENU_GLOBAL:
		case MENU_CV:
			writeGlobalSettingsNVM(&globalSettings, &cv_instance);
			break;
		// emulated EEPROM pages 0 & 1
		default:
			writeChannelNVM(&chan[menu.currentChannel], menu.currentChannel);
			break;
	}
}

/*
 *	initialize all menus with their default values, draw default menu
*/
void menuInit(uint32_t *currentCount) {
	globalMenu.title = "Global";
	globalMenu.strings = globalSettingsStrings;
	globalMenu.params = globalSettings.globalSettingsParams;
	globalMenu.num_elements = 6;
	globalMenu.current_selection = 0;
	globalMenu.current_page = 0;
	globalMenu.paramEdit = false;
	
	channel1Menu.title = "Channel 1";
	channel1Menu.strings = channelMenuStrings;
	channel1Menu.params = chan[0].chMenuParams;
	channel1Menu.num_elements = 4;
	channel1Menu.current_selection = 0;
	channel1Menu.current_page = 0;
	channel1Menu.paramEdit = false;
	
	inputs1Menu.title = "CH1 Inputs";
	inputs1Menu.strings = inputsMenuStrings;
	inputs1Menu.params = chan[0].inputsMenuParams;
	inputs1Menu.num_elements = 7;
	inputs1Menu.current_selection = 0;
	inputs1Menu.current_page = 0;
	inputs1Menu.paramEdit = false;
	
	outputs1Menu.title = "CH1 Outputs";
	outputs1Menu.strings = outputsMenuStrings;
	outputs1Menu.params = chan[0].outputsMenuParams;
	outputs1Menu.num_elements = 15;
	outputs1Menu.current_selection = 0;
	outputs1Menu.current_page = 0;
	outputs1Menu.paramEdit = false;
	
	channel2Menu.title = "Channel 2";
	channel2Menu.strings = channelMenuStrings;
	channel2Menu.params = chan[1].chMenuParams;
	channel2Menu.num_elements = 4;
	channel2Menu.current_selection = 0;
	channel2Menu.current_page = 0;
	channel2Menu.paramEdit = false;
	
	inputs2Menu.title = "CH2 Inputs";
	inputs2Menu.strings = inputsMenuStrings;
	inputs2Menu.params = chan[1].inputsMenuParams;
	inputs2Menu.num_elements = 7;
	inputs2Menu.current_selection = 0;
	inputs2Menu.current_page = 0;
	inputs2Menu.paramEdit = false;
	
	outputs2Menu.title = "CH2 Outputs";
	outputs2Menu.strings = outputsMenuStrings;
	outputs2Menu.params = chan[1].outputsMenuParams;
	outputs2Menu.num_elements = 15;
	outputs2Menu.current_selection = 0;
	outputs2Menu.current_page = 0;
	outputs2Menu.paramEdit = false;
	
	cvMenu.title = "CV";
	cvMenu.strings = cvMenuStrings;
	cvMenu.params = cv_instance.cvParams;
	cvMenu.num_elements = 4;
	cvMenu.current_selection = 0;
	cvMenu.current_page = 0;
	cvMenu.paramEdit = false;
	
	// assign and initialize framebuffers
	for (uint8_t i=0; i<MENU_COUNT; i++) {
		menuList[i]->fbPointer = framebuffers[i];
		gfx_mono_set_framebuffer(menuList[i]->fbPointer);
		gfx_mono_menu_init(menuList[i]);
	}
	
	menu.currentMenu = DEFAULT_MENU;
	menu.currentChannel = 0;
	menu.actionFlag = ACTION_NONE;
	menu.lastInput = 0;
	menu.enc_count = 0;
	menu.screenSaved = false;
	menu.rtcCurrentCount = currentCount;
	
	gfx_mono_set_framebuffer(menuList[menu.currentMenu]->fbPointer);
}

/*
 *	initialize TC & screen draw callback
*/
void screenDrawInit(struct tc_module *tc_instance) {
	struct tc_config conf;
	
	menu.tc = tc_instance;
	
	// GCLK0 @ ~48MHz (187.5kHz prescaler output)
	// CC0: 18 (~10.4kHz interrupt freq)
	tc_get_config_defaults(&conf);
	conf.counter_size = TC_COUNTER_SIZE_8BIT;
	conf.clock_source = GCLK_GENERATOR_0;
	conf.clock_prescaler = TC_CLOCK_PRESCALER_DIV256;
	conf.wave_generation = TC_WAVE_GENERATION_MATCH_FREQ;
	conf.counter_8_bit.value = 0;
	conf.counter_8_bit.period = 255;
	conf.counter_8_bit.compare_capture_channel[0] = 18;
	tc_init(menu.tc, TC3, &conf);
	tc_enable(menu.tc);
	
	// register & enable our callback
	tc_register_callback(menu.tc, screenDrawCallback, TC_CALLBACK_CC_CHANNEL0);
	tc_enable_callback(menu.tc, TC_CALLBACK_CC_CHANNEL0);
}

/*
 *	interrupt for sending our display framebuffer to SSD1306 display driver
*/
void screenDrawCallback(struct tc_module *const tc_instance) {
	static uint8_t page_address = 0;
	static uint8_t column_address = 0;
	
	if (!menu.screenSaved) {
		// grab current framebuffer data & send to display driver
		uint8_t framebuffer_byte = gfx_mono_framebuffer_get_byte(page_address, column_address);
		ssd1306_write_data(framebuffer_byte);
		
		// update addresses as necessary, update display driver with new addresses
		column_address++;
		if (column_address > 127) {
			column_address = 0;
			page_address = (page_address >= 7) ? 0:(page_address + 1);
			ssd1306_write_command(SSD1306_CMD_SET_PAGE_START_ADDRESS(page_address));
		}
		ssd1306_write_command(SSD1306_CMD_COL_ADD_SET_MSB(column_address >> 4));
		ssd1306_write_command(SSD1306_CMD_COL_ADD_SET_LSB(column_address & 0x0F));
	}
}

/*
 *	set the current flag for a menu action, used by ui functions
 *	note: this will overwrite a previous flag setting, so if multiple actions
 *	      are performed within a single menu loop only the last one will get executed
*/
void setMenuFlag(uint8_t flag) {
	menu.actionFlag = flag;
}

/*
 *	check the current action flag and process action if applicable
*/
void processMenuAction(void) {
	if (!menu.screenSaved) {
		action_table[menu.actionFlag]();	// this is a function call :)
	}
	else {
		if (menu.actionFlag != ACTION_NONE) {
			ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_ON);
			menu.screenSaved = false;
			menu.enc_count = 0;
			menu.lastInput = *menu.rtcCurrentCount;
		}
	}
	
	// reset menu flag for next loop
	menu.actionFlag = ACTION_NONE;
}

/*
 *	increment screen saver counter, send screen sleep 
*/
void processActionNone(void) {
	static uint16_t counter = 0;
	
	if (globalSettings.screenSaverTime != SCREENSAVER_OFF) {
		counter += 1;
		
		// periodically check real time for screen saver
		if (counter >= 10000) {
			uint32_t temp = *menu.rtcCurrentCount;
			temp -= menu.lastInput;
			counter = 0;
			
			if (temp > screenSaverTimes[globalSettings.screenSaverTime]) {
				ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_OFF);
				menu.screenSaved = true;
			}
		}
	}
}

/*
 *	move down in the current menu, otherwise increment the selected parameter
*/
void processActionInc(void) {
	uint8_t i;
	
	// scroll down if we're not editing a parameter
	if (!menuList[menu.currentMenu]->paramEdit) {
		for (i=0; i<menu.enc_count; i++) {
			gfx_mono_menu_process_key(menuList[menu.currentMenu], GFX_MONO_MENU_KEYCODE_DOWN);
		}
	}
	else {	// else if we're in paramEdit mode
		for (i=0; i<menu.enc_count; i++) {
			updateParamTable[menu.currentMenu](true);	// this is a function call :)
		}
		// write the change to the framebuffer
		gfx_mono_menu_update_parameter(menuList[menu.currentMenu]);
	}
	
	menu.enc_count = 0;
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	move up in the current menu, otherwise decrement the selected parameter
*/
void processActionDec(void) {
	uint8_t i;
	uint8_t count = menu.enc_count * -1;
	
	// scroll up if we're not editing a parameter
	if (!menuList[menu.currentMenu]->paramEdit) {
		for (i=0; i<count; i++) {
			gfx_mono_menu_process_key(menuList[menu.currentMenu], GFX_MONO_MENU_KEYCODE_UP);
		}
	}
	else {	// else if we're in paramEdit mode
		for (i=0; i<count; i++) {
			updateParamTable[menu.currentMenu](false);	// this is a function call :)
		}
		// write the change to the framebuffer
		gfx_mono_menu_update_parameter(menuList[menu.currentMenu]);
	}
	
	menu.enc_count = 0;
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	utility function for determining which parameter to update within the 
 *	global settings menu, only called if in paramEdit mode
*/
static void updateGlobalMenuParam(bool inc) {
	switch (menuList[menu.currentMenu]->current_selection) {
		case 3:	// reset
			updateChReset(&globalSettings, inc);
			break;
		case 4:	// long-press time
			updateLongPressTime(&globalSettings, inc);
			break;
		case 5:	// screen saver time
			updateScreenSaverTime(&globalSettings, inc);
	}
}

/*
 *	utility function for determining which parameter to update within the
 *	channel menu, only called if in paramEdit mode
*/
static void updateChannelMenuParam(bool inc) {
	switch (menuList[menu.currentMenu]->current_selection) {
		case 1:	// op 1
			updateOp(&chan[menu.currentChannel], 0, inc);
			break;
		case 2:	// op 2
			updateOp(&chan[menu.currentChannel], 1, inc);
			break;
	}
}

/*
 *	utility function for determining which parameter to update within the
 *	inputs menu, only called if in paramEdit mode
*/
static void updateInputsMenuParam(bool inc) {
	switch (menuList[menu.currentMenu]->current_selection) {
		case 0:	// in1 threshold
			updateThreshold(&chan[menu.currentChannel].input.input_settings[0], inc);
			break;
		case 1:	// in1 hysteresis
			updateHys(&chan[menu.currentChannel].input.input_settings[0], inc);
			break;
		case 2:	// in1 invert
			updateInvert(&chan[menu.currentChannel].input.input_settings[0], inc);
			break;
		case 3: // in2 copy in1
			updateCopyIn1(&chan[menu.currentChannel].input, inc);
			break;
		case 4:	// in2 threshold
			updateThreshold(&chan[menu.currentChannel].input.input_settings[1], inc);
			break;
		case 5:	// in2 hysteresis
			updateHys(&chan[menu.currentChannel].input.input_settings[1], inc);
			break;
		case 6:	// in2 invert
			updateInvert(&chan[menu.currentChannel].input.input_settings[1], inc);
			break;
	}
}

/*
 *	utility function for determining which parameter to update within the
 *	outputs menu, only called if in paramEdit mode
*/
static void updateOutputsMenuParam(bool inc) {
	switch (menuList[menu.currentMenu]->current_selection) {
		case 0:	// out1 clock division
			updateClkDiv(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 1:		// out1 division phase
			updateClkPhase(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 2:		// out1 division reset input
			updateDivRst(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 3:		// out1 delay
			updateDelay(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 4:		// out1 probability
			updateProbability(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 5:		// out1 trig mode
			updateTrig(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 6:		// out1 trig length
			updateTrigLen(&chan[menu.currentChannel].out.output_settings[0], inc);
			break;
		case 7:		// out2 settings mode
			updateOut2Settings(&chan[menu.currentChannel].out, inc);
			break;
		case 8:		// out2 clock division
			updateClkDiv(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
		case 9:		// out2 division phase
			updateClkPhase(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
		case 10:	// out2 division reset input
			updateDivRst(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
		case 11:	// out2 delay
			updateDelay(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
		case 12:	// out2 probability
			updateProbability(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
		case 13:	// out2 trig mode
			updateTrig(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
		case 14:	// out2 trig length
			updateTrigLen(&chan[menu.currentChannel].out.output_settings[1], inc);
			break;
	}
}

/*
 *	utility function for determining which parameter to update within the
 *	CV menu, only called if in paramEdit mode
*/
static void updateCvMenuParam(bool inc) {
	switch (menuList[menu.currentMenu]->current_selection) {
		case 0:	// CV1 range
			updateCvRange(&cv_instance.settings[0], inc);
			break;
		case 1:	// CV1 threshold
			updateCvThreshold(&cv_instance.settings[0], inc);
			break;
		case 2:	// CV2 range
			updateCvRange(&cv_instance.settings[1], inc);
			break;
		case 3:	// CV2 threshold
			updateCvThreshold(&cv_instance.settings[1], inc);
			break;
	}
}

/*
 *	exit paramEdit mode if we're in it, otherwise get current selection
 *	and call the corresponding menuEnter function
*/
void processActionEnter(void) {
	// exit parameter edit mode if we're editing a parameter and write
	// to non-volatile memory
	if (menuList[menu.currentMenu]->paramEdit) {
		// check to see if we need to reset any/all channels
		if (menu.currentMenu == MENU_GLOBAL && menuList[menu.currentMenu]->current_selection == 3) {
			checkReset();
		}
		gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
		writeNVM();
	}
	else {	// if not editing a parameter, call the relevant menuEnter function
		menuEnterTable[menu.currentMenu]();
	}
	
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	function for determining the correct menu enter action
 *	in the global menu, only called if not in paramEdit mode
*/
static void globalMenuEnter(void) {
	switch (menuList[menu.currentMenu]->current_selection) {
		case 0:	// CH1 menu
			setMenu(MENU_CHANNEL_1);
			break;
		case 1:	// CH2 menu
			setMenu(MENU_CHANNEL_2);
			break;
		case 2:	// CV menu
			setMenu(MENU_CV);
			break;
		case 3:	// reset
		case 4:	// long-press time
		case 5:	// screen saver time
		case 6:	// display contrast
			gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
			break;
	}
}

/*
 *	function for determining the correct menu enter action in the
 *	channel menu, only called if not in paramEdit mode
*/
static void channelMenuEnter(void) {
	 switch (menuList[menu.currentMenu]->current_selection) {
		case 0:	// inputs menu
			setMenu((menu.currentChannel == 0) ? MENU_INPUTS_1:MENU_INPUTS_2);
			break;
		case 1:	// op 1
		case 2:	// op 2
			gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
			break;
		case 3:	// outputs menu
			setMenu((menu.currentChannel == 0) ? MENU_OUTPUTS_1:MENU_OUTPUTS_2);
			break;
	 }
}

/*
 *	function for determining the correct menu enter action in the
 *	inputs menu, only called if not in paramEdit mode
*/
static void inputsMenuEnter(void) {
	// all options are parameters (no sub-menus) so all we need to do
	// is switch modes
	gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
}

/*
 *	function for determining the correct menu enter action in the
 *	outputs menu, only called if not in paramEdit mode
*/
static void outputsMenuEnter(void) {
	// all options are parameters (no sub-menus) so all we need to do
	// is switch modes
	gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
}

/*
 *	function for determining the correct menu enter action in the
 *	CV menu, only called if not in paramEdit mode
*/
static void cvMenuEnter(void) {
	// all options are parameters (no sub-menus) so all we need to do
	// is switch modes
	gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
}

/*
 *	if editing a parameter exit param edit mode, otherwise set current menu
 *	to one level above the current menu
*/
void processActionBack(void) {
	// check if we're currently editing a parameter
	if (menuList[menu.currentMenu]->paramEdit) {
		gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
	}
	else {	// otherwise go back one menu
		switch (menu.currentMenu) {
			case MENU_INPUTS_1:
			case MENU_OUTPUTS_1:
				setMenu(MENU_CHANNEL_1);
				break;
			case MENU_INPUTS_2:
			case MENU_OUTPUTS_2:
				setMenu(MENU_CHANNEL_2);
				break;
			default:
				setMenu(MENU_GLOBAL);
				break;
		}
	}
	
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	set menu to Channel 1 menu
*/
void processActionCh1(void) {
	// check if we're currently editing a parameter
	if (menuList[menu.currentMenu]->paramEdit) {
		gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
	}
	
	setMenu(MENU_CHANNEL_1);
	
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	set menu to Channel 2 menu
*/
void processActionCh2(void) {
	// check if we're currently editing a parameter
	if (menuList[menu.currentMenu]->paramEdit) {
		gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
	}
	
	setMenu(MENU_CHANNEL_2);
	
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	set menu to Global Settings menu
*/
void processActionGlobal(void) {
	// check if we're currently editing a parameter
	if (menuList[menu.currentMenu]->paramEdit) {
		gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
	}
	
	setMenu(MENU_GLOBAL);
	
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	set menu to CV menu
*/
void processActionCv(void) {
	// check if we're currently editing a parameter
	if (menuList[menu.currentMenu]->paramEdit) {
		gfx_mono_menu_toggle_mode(menuList[menu.currentMenu]);
	}
	
	setMenu(MENU_CV);
	
	menu.lastInput = *menu.rtcCurrentCount;
}

/*
 *	check if any of the 'reset channel' options have been selected, reset
 *	the relevant channels/CV, and write to NVM
*/
void checkReset(void) {
	switch(globalSettings.chReset) {
		case RESET_CH1:
			setChannelDefaults(&chan[0], 0);
			break;
		case RESET_CH2:
			setChannelDefaults(&chan[1], 1);
			break;
		case RESET_ALL:
			setChannelDefaults(&chan[0], 0);
			setChannelDefaults(&chan[1], 1);
			setCvDefaults(&cv_instance);
			writeGlobalStrings(&globalSettings, &cv_instance);
			break;
	}
	
	if (globalSettings.chReset != RESET_NONE) {
		// stop printing to screen momentarily since we're about to scroll through
		// all of our framebuffers
		tc_disable_callback(menu.tc, TC_CALLBACK_CC_CHANNEL0);
		
		// reprint variables to our framebuffers so they display
		// properly when we next enter the menu
		for (uint8_t i=0; i<MENU_COUNT; i++) {
			gfx_mono_set_framebuffer(menuList[i]->fbPointer);
			menuList[i]->current_page = 0;
			menuList[i]->current_selection = 0;
			gfx_mono_menu_init(menuList[i]);
		}
		
		// set fb pointer back to current menu
		gfx_mono_set_framebuffer(menuList[menu.currentMenu]->fbPointer);
		
		// update reset setting back to default
		globalSettings.chReset = RESET_NONE;
		sprintf(globalSettings.chResetStr, "None");
		menuList[menu.currentMenu]->current_selection = 3;
		gfx_mono_menu_update_parameter(menuList[menu.currentMenu]);
		
		// re-enable screen draw callback
		tc_enable_callback(menu.tc, TC_CALLBACK_CC_CHANNEL0);
	}
}