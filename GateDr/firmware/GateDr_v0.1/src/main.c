/**
 *	
 *	Copyright (c) 2025 Taylor Moralez
 *	Published under standard MIT license
 *	For further license info, see LICENSE.md in /GateDr/documentation/
 */


#include <asf.h>
#include <stdbool.h>
#include <stdlib.h>
#include "gateDr.h"

#define VER "1.02"

// we've got 6 inputs, but AIN[2] and [3] don't have a hardware 
// input on E-variant SAMD21s
#define PIN_SCAN_COUNT 8
#define NVM_EEPROM_EMULATOR_SIZE_DEFAULT NVM_EEPROM_EMULATOR_SIZE_2048

struct adc_module adc_instance;
struct dma_resource dma_instance;
COMPILER_ALIGNED(16)
DmacDescriptor dmac_descriptor;
struct events_resource rtc_event;
struct events_hook rtc_hook;
struct rtc_module rtc_instance;
struct tc_module tc3_instance;

uint16_t adcBuffer[8] = {0};
uint32_t rtcCount = 0;

void configure_adc(void);
void configure_dma(struct dma_resource *resource, DmacDescriptor *descriptor, uint16_t *buffer);
void configure_eeprom(void);
void configure_bod(void);
unsigned int generate_seed(uint16_t *buffer);
int16_t get_adc_mV(uint16_t rawAdc);

int main (void)
{
	system_init();
	system_interrupt_enable_global();
	configure_eeprom();
	configure_bod();
	ssd1306_init();
	
	ui_init(&rtc_event, &rtc_hook, &rtc_instance);		// RTC initialized within function
	initChannel(&chan[0], &rtcCount, 0);
	initChannel(&chan[1], &rtcCount, 1);
	
	configure_adc();
	configure_dma(&dma_instance, &dmac_descriptor, adcBuffer);
	adc_start_conversion(&adc_instance);
	dma_start_transfer_job(&dma_instance);
	
	// initialize menu
	gfx_mono_init();
	menuInit(&rtcCount, VER);
	screenDrawInit(&tc3_instance);						// TC3 initialized within function
	
	int16_t adcResult[PIN_SCAN_COUNT] = {0};			// stores most recent ADC reads in mV
	unsigned int seed = 0;
	
	// initialize the seed from ADC reads
	seed = generate_seed(adcBuffer);
	srand(seed);
	
	while (1) {
		
		// get current RTC count to use in processing blocks
		rtcCount = rtc_count_get_count(&rtc_instance);
		
		// read all ADC inputs
		// due to hardware positions, adc reads in the order:
		// D, C, x, x, B, A, CV2, CV1
		for (uint8_t i = 0; i<PIN_SCAN_COUNT; i++) {
			adcResult[i] = get_adc_mV(adcBuffer[i]);
		}
		cv_instance.value[0] = adcResult[7];
		cv_instance.value[1] = adcResult[6];
		
		processChannel(&chan[0], adcResult[5], adcResult[4], &cv_instance);
		processChannel(&chan[1], adcResult[1], adcResult[0], &cv_instance);
		
		// set output states
		port_pin_set_output_level(PIN_PA11, chan[0].out.output_state[0].out_processed);	// output W
		port_pin_set_output_level(PIN_PA10, chan[0].out.output_state[1].out_processed);	// output X
		port_pin_set_output_level(PIN_PA09, chan[1].out.output_state[0].out_processed);	// output Y
		port_pin_set_output_level(PIN_PA08, chan[1].out.output_state[1].out_processed);	// output Z	
		
		processMenuAction();
		
	}
}

/*
 *	setup for ASF ADC module
*/
void configure_adc(void) {
	struct adc_config adc_conf;
	
	adc_get_config_defaults(&adc_conf);
	adc_conf.clock_prescaler = ADC_CLOCK_PRESCALER_DIV256;
	adc_conf.positive_input = ADC_POSITIVE_INPUT_PIN0;
	adc_conf.pin_scan.inputs_to_scan = PIN_SCAN_COUNT;
	adc_conf.reference = ADC_REFERENCE_INTVCC1;
	adc_conf.gain_factor = ADC_GAIN_FACTOR_DIV2;
	adc_conf.freerunning = true;
	adc_conf.left_adjust = false;
	
	adc_init(&adc_instance, ADC, &adc_conf);	
	adc_enable(&adc_instance);
}

/*
 *	initializes DMA resource & descriptor for ADC conversion
*/
void configure_dma(struct dma_resource *resource, DmacDescriptor *descriptor, uint16_t *buffer) {
	struct dma_resource_config dma_conf;
	
	dma_get_config_defaults(&dma_conf);
	dma_conf.peripheral_trigger = ADC_DMAC_ID_RESRDY;
	dma_conf.trigger_action = DMA_TRIGGER_ACTION_BEAT;
	//dma_conf.event_config.input_action = DMA_EVENT_INPUT_TRIG;
	dma_allocate(resource, &dma_conf);
	
	// DMA descriptor setup
	struct dma_descriptor_config descriptor_conf;
	
	dma_descriptor_get_config_defaults(&descriptor_conf);
	descriptor_conf.beat_size = DMA_BEAT_SIZE_HWORD;
	descriptor_conf.src_increment_enable = false;
	descriptor_conf.block_transfer_count = PIN_SCAN_COUNT;
	descriptor_conf.source_address = (uint32_t)(&adc_instance.hw->RESULT.reg);
	descriptor_conf.destination_address = (uint32_t)(buffer + PIN_SCAN_COUNT);
	descriptor_conf.next_descriptor_address = (uint32_t)descriptor;
	dma_descriptor_create(descriptor, &descriptor_conf);
	dma_add_descriptor(resource, descriptor);
}

/*
 *	setup for emulated EEPROM module, writes fuses & stalls program if NVM fuses have not been
 *	set properly for emulated EEPROM usage, initializes and writes defaults 
 *	if uninitialized or corrupted
*/
void configure_eeprom(void) {
	enum status_code error_code = eeprom_emulator_init();
	struct nvm_fusebits fuses;
	
	// check for NVM fuses, write the EEPROM fuse bits and prompt a restart
	if (error_code == STATUS_ERR_NO_MEMORY) {
		// get current fuses and rewrite EEPROM emulator size
		nvm_get_fuses(&fuses);
		fuses.eeprom_size = NVM_EEPROM_EMULATOR_SIZE_DEFAULT; 
		nvm_set_fuses(&fuses);
		
		/* Code here to prompt user to reset module via screen */
		// for right now we'll just turn output Y & associated LED high
		port_pin_set_output_level(PIN_PA09, true);
		
		// stall program for user reset
		while (true) {
		}
	}
	
	// check for other errors, erase emulated EEPROM & initialize,
	// since it is likely either uninitialized or corrupted
	else if (error_code != STATUS_OK) {
		eeprom_emulator_erase_memory();
		eeprom_emulator_init();
		
		// write channel & global settings defaults to working and NV memory
		setGlobalSettingsDefaults(&globalSettings, &cv_instance);
		setChannelDefaults(&chan[0], 0);
		setChannelDefaults(&chan[1], 1);
	}
	
	// otherwise our memory was good and we're clear to read from the contents
	else {
		readGlobalSettingsNVM(&globalSettings, &cv_instance);
		readChannelNVM(&chan[0], 0);
		readChannelNVM(&chan[1], 1);
	}
}

/*
 *	setup for brown-out-detector 
*/
void configure_bod(void) {
	struct bod_config config_bod33;
	bod_get_config_defaults(&config_bod33);
	config_bod33.action = BOD_ACTION_INTERRUPT;
	config_bod33.level = 48;
	bod_set_config(BOD_BOD33, &config_bod33);
	bod_enable(BOD_BOD33);
	
	SYSCTRL->INTENSET.reg = SYSCTRL_INTENCLR_BOD33DET;
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_SYSCTRL);
}

/*
 *	SYSCTRL interrupt function, called on BOD event
*/
void SYSCTRL_Handler (void) {
	// clear BOD interrupt flag and dump the contents of the EEPROM buffer to NVM
	if (SYSCTRL->INTFLAG.reg & SYSCTRL_INTFLAG_BOD33DET) {
		eeprom_emulator_commit_page_buffer();
		SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33DET;
	}
}

/*
 *	generate rand() seed by bit-shifting ADC reads
*/
unsigned int generate_seed(uint16_t *buffer) {
	unsigned int seed = 0;
	
	for (uint8_t i = 0; i < PIN_SCAN_COUNT; i++) {
		seed += (buffer[i] & 0x3) << (2 * i);
	}
	
	return seed;
}

/*
 *	get an ADC read and convert to +/-8000mV
*/
int16_t get_adc_mV(uint16_t rawAdc) {
	uint32_t temp;
	int16_t result = 0;
	
	rawAdc = 4095 - rawAdc;			// inverting due to input circuitry
	temp = rawAdc * 4000;			// scaling to mV with a multiply + bit shift
	result = temp >> 10;
	result -= 8000;					// offset by -8V
	
	return result;
}