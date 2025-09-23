/**
 * \file
 *
 * \brief User board initialization template
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include <board.h>
#include <conf_board.h>

#if defined(__GNUC__)
void board_init(void) WEAK __attribute__((alias("system_board_init")));
#elif defined(__ICCARM__)
void board_init(void);
#  pragma weak board_init=system_board_init
#endif

void system_board_init(void)
{
	/* This function is meant to contain board-specific initialization code
	 * for, e.g., the I/O pins. The initialization can rely on application-
	 * specific board configuration, found in conf_board.h.
	 */
	
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA11, &pin_conf);	// output W
	port_pin_set_output_level(PIN_PA11, false);
	port_pin_set_config(PIN_PA10, &pin_conf);	// output X
	port_pin_set_output_level(PIN_PA10, false);
	port_pin_set_config(PIN_PA09, &pin_conf);	// output Y
	port_pin_set_output_level(PIN_PA09, false);
	port_pin_set_config(PIN_PA08, &pin_conf);	// output Z
	port_pin_set_output_level(PIN_PA08, false);
	
	port_pin_set_config(PIN_PA18, &pin_conf);	// SPI/SSD1306 MOSI
	port_pin_set_config(PIN_PA19, &pin_conf);	// SPI/SSD1306 SCK
	
	pin_conf.direction = PORT_PIN_DIR_INPUT;

	// the following inputs are set up automatically during the configure_adc() 
	// function in main
// 	pin_conf.input_pull = PORT_PIN_PULL_NONE;
// 	port_pin_set_config(PIN_PA05, &pin_conf);	// input A
// 	port_pin_set_config(PIN_PA04, &pin_conf);	// input B
// 	port_pin_set_config(PIN_PA03, &pin_conf);	// input C
// 	port_pin_set_config(PIN_PA02, &pin_conf);	// input D
// 	port_pin_set_config(PIN_PA07, &pin_conf);	// CV input 1
// 	port_pin_set_config(PIN_PA06, &pin_conf);	// CV input 2

	pin_conf.input_pull = PORT_PIN_PULL_UP;
	
	port_pin_set_config(PIN_PA00, &pin_conf);	// pushbutton 1
	port_pin_set_config(PIN_PA01, &pin_conf);	// pushbutton 2
	port_pin_set_config(PIN_PA15, &pin_conf);	// encoder A
	port_pin_set_config(PIN_PA14, &pin_conf);	// encoder B
	port_pin_set_config(PIN_PA28, &pin_conf);	// encoder pushbutton
}