/**
 * \file
 *
 * \brief Simple menu system
 * Copyright (c) 2011-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include "sysfont.h"
#include <conf_menu.h>

#include "gfx_mono_menu.h"

/**
 * \ingroup asfdoc_common2_gfx_mono_menu
 * @{
 */

PROGMEM_DECLARE(gfx_mono_color_t, arrow_right_data[]) = {
	GFX_MONO_MENU_INDICATOR_BITMAP
};

struct gfx_mono_bitmap menu_bitmap_indicator = {
	.height = GFX_MONO_MENU_INDICATOR_HEIGHT,
	.width = GFX_MONO_MENU_INDICATOR_WIDTH,
	.type = GFX_MONO_BITMAP_PROGMEM,
	.data.progmem = arrow_right_data
};

/**
 * \brief Draw menu strings and an icon by the current selection.
 *
 * \param[in] menu     a menu struct with menu settings
 * \param[in] redraw   clear screen before drawing menu
 */
static void menu_draw(struct gfx_mono_menu *menu, bool redraw)
{
	static bool redraw_state;
	uint8_t i;
	uint8_t line = 1;
	uint8_t menu_page = menu->current_selection /
			GFX_MONO_MENU_ELEMENTS_PER_SCREEN;

	if (menu->current_page != menu_page || redraw == true) {
		/* clear screen if we have changed the page or menu and prepare
		 * redraw */
		gfx_mono_draw_filled_rect(0, SYSFONT_LINESPACING,
				GFX_MONO_LCD_WIDTH,
				GFX_MONO_LCD_HEIGHT - SYSFONT_LINESPACING,
				GFX_PIXEL_CLR);
		redraw_state = true;
	}

	menu->current_page = menu_page;

	/* Clear old indicator icon */
	gfx_mono_draw_filled_rect(0, SYSFONT_LINESPACING,
			GFX_MONO_MENU_INDICATOR_WIDTH, GFX_MONO_LCD_HEIGHT -
			SYSFONT_LINESPACING, GFX_PIXEL_CLR);
	gfx_mono_draw_filled_rect(GFX_MONO_LCD_WIDTH - 
			((SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)) +
			GFX_MONO_MENU_INDICATOR_WIDTH), SYSFONT_LINESPACING, 
			GFX_MONO_MENU_INDICATOR_WIDTH, GFX_MONO_LCD_HEIGHT - 
			SYSFONT_LINESPACING, GFX_PIXEL_CLR);

	/* Put indicator icon on current selection */
	if (menu->paramEdit) {
		gfx_mono_put_bitmap(&menu_bitmap_indicator, GFX_MONO_LCD_WIDTH -
				((SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)) + 
				GFX_MONO_MENU_INDICATOR_WIDTH), 
				SYSFONT_LINESPACING * ((menu->current_selection %
				GFX_MONO_MENU_ELEMENTS_PER_SCREEN) + 1));
	}
	else {
		gfx_mono_put_bitmap(&menu_bitmap_indicator, 0,
				SYSFONT_LINESPACING * ((menu->current_selection %
				GFX_MONO_MENU_ELEMENTS_PER_SCREEN) + 1));
	}

	/* Print visible options if page or menu has changed */
	if (redraw_state == true) {
		for (i = menu_page * GFX_MONO_MENU_ELEMENTS_PER_SCREEN;
				i < menu_page *
				GFX_MONO_MENU_ELEMENTS_PER_SCREEN +
				GFX_MONO_MENU_ELEMENTS_PER_SCREEN &&
				i < menu->num_elements; i++) {
			// draw parameter names
			gfx_mono_draw_progmem_string(
					(char PROGMEM_PTR_T)menu->strings[i],
					GFX_MONO_MENU_INDICATOR_WIDTH + 2,
					line * SYSFONT_LINESPACING, &sysfont);
			// invert input/output number if 2nd i/o
			if (menu->strings[i][0] == '2') {
				gfx_mono_draw_filled_rect(GFX_MONO_MENU_INDICATOR_WIDTH + 1,
					line * SYSFONT_LINESPACING, SYSFONT_WIDTH + 1, SYSFONT_LINESPACING,
					GFX_PIXEL_XOR);
			}
			// draw parameter values
			gfx_mono_draw_progmem_string(menu->params[i], GFX_MONO_LCD_WIDTH - 
					(SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)) + 2, 
					line * SYSFONT_LINESPACING, &sysfont);
			// invert param if not set to default value
			if (!(*menu->defaults[i])) {
				gfx_mono_draw_filled_rect(GFX_MONO_LCD_WIDTH -
				(SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)),
				line * SYSFONT_LINESPACING,
				SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1),
				SYSFONT_LINESPACING, GFX_PIXEL_XOR);
			}
			line++;
		}
		redraw_state = false;
	}
}

/**
 * *\brief Initialize the menu handling. Clear screen and draw menu.
 *
 * \param[in] menu  menu struct with menu options
 *
 */
void gfx_mono_menu_init(struct gfx_mono_menu *menu)
{
	/* Clear screen */
	gfx_mono_draw_filled_rect(0, 0,
			GFX_MONO_LCD_WIDTH, GFX_MONO_LCD_HEIGHT, GFX_PIXEL_CLR);

	/* Draw the menu title on the top of the screen */
	gfx_mono_draw_progmem_string((char PROGMEM_PTR_T)menu->title,
			2, 0, &sysfont);
			
	// invert title if menu is in CH2 for clarity, checking this in a cheeky way
	if (menu->title[2] == '2') {
		gfx_mono_draw_filled_rect(0, 0, GFX_MONO_LCD_WIDTH, SYSFONT_LINESPACING, GFX_PIXEL_XOR);
	}

	/* Draw menu options below */
	menu_draw(menu, true);
}

/**
 * \brief Update menu depending on input.
 *
 * \param[in] menu  menu struct with menu options
 * \param[in] keycode  keycode to process
 *
 * \retval selected menu option or status code
 */
uint8_t gfx_mono_menu_process_key(struct gfx_mono_menu *menu, uint8_t keycode)
{
	switch (keycode) {
	case GFX_MONO_MENU_KEYCODE_DOWN:
		if (menu->current_selection == menu->num_elements - 1) {
			menu->current_selection = 0;
		} else {
			menu->current_selection++;
		}

		/* Update menu on display */
		menu_draw(menu, false);
		/* Nothing selected yet */
		return GFX_MONO_MENU_EVENT_IDLE;

	case GFX_MONO_MENU_KEYCODE_UP:
		if (menu->current_selection) {
			menu->current_selection--;
		} else {
			menu->current_selection = menu->num_elements - 1;
		}

		/* Update menu on display */
		menu_draw(menu, false);
		/* Nothing selected yet */
		return GFX_MONO_MENU_EVENT_IDLE;

	case GFX_MONO_MENU_KEYCODE_ENTER:
		/* Got what we want. Return selection. */
		return menu->current_selection;

	case GFX_MONO_MENU_KEYCODE_BACK:
		/* User pressed "back" key, inform user */
		return GFX_MONO_MENU_EVENT_EXIT;

	default:
		/* Unknown key event */
		return GFX_MONO_MENU_EVENT_IDLE;
	}
}

/*
 *	erase the currently selected parameter and redraw the new one
*/
void gfx_mono_menu_update_parameter(struct gfx_mono_menu *menu) {
	uint8_t page_selection = (menu->current_selection % GFX_MONO_MENU_ELEMENTS_PER_SCREEN) + 1;
	
	// erase current parameter string
	gfx_mono_draw_filled_rect(GFX_MONO_LCD_WIDTH - 
			(SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)),
			page_selection * SYSFONT_LINESPACING,
			SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1),
			SYSFONT_LINESPACING, GFX_PIXEL_CLR);
	
	// draw updated parameter
	gfx_mono_draw_progmem_string(menu->params[menu->current_selection],
			GFX_MONO_LCD_WIDTH - 
			(SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)) + 2,
			page_selection * SYSFONT_LINESPACING,
			&sysfont);
	
	// invert param if not set to default value
	if (!(*menu->defaults[menu->current_selection])) {
		gfx_mono_draw_filled_rect(GFX_MONO_LCD_WIDTH -
		(SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1)),
		page_selection * SYSFONT_LINESPACING,
		SYSFONT_WIDTH * (GFX_MONO_MENU_PARAM_MAX_CHAR - 1),
		SYSFONT_LINESPACING, GFX_PIXEL_XOR);
	}
}

/*
 *	toggle the current menu mode (between scrolling and parameter
 *	editing) and redraw screen
*/
void gfx_mono_menu_toggle_mode(struct gfx_mono_menu *menu) {
	menu->paramEdit = !menu->paramEdit;
	menu_draw(menu, false);
}