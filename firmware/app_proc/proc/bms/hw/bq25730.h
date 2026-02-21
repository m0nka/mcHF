// https://github.com/noahcroit/bms-bq25730-ststm32
//
#ifndef __BQ25730_H
#define __BQ25730_H

#define FRAMEWORK_ARDUINO   	0
#define FRAMEWORK_STM32CUBE 	1
#define FRAMEWORK_ZEPHYR    	2
#define SELECTED_FRAMEWORK  	FRAMEWORK_STM32CUBE

#define VCHARGE_TARGET  		21000
#define VSYSMIN_TARGET  		15400
#define ICHRG_TARGET    		900

#define BQ25730_DEFAULT_ADDR 	0x6B
#define ADDR_CHRGOPT0   		0x00
#define ADDR_CHRGOPT1   		0x30
#define ADDR_CHRGCURR   		0x02
#define ADDR_CHRGVOLT   		0x04
#define ADDR_CHRG_STAT   		0x20
#define ADDR_ADCPSYS    		0x26
#define ADDR_ADCVBUS    		0x27
#define ADDR_ADCVBAT    		0x2C
#define ADDR_ADCVSYS    		0x2D
#define ADDR_VSYSMIN    		0x0D
#define ADDR_ADCOPT     		0x3A
#define ADDR_ADCIIN     		0x2B
#define ADDR_ADCICHG    		0x29
#define ADDR_ADCIDCHG   		0x28
#define ADDR_MANUF_ID   		0x2E
#define ADDR_CHIP_ID   			0x2F

#define CHRGOPT0_EN_LWPWR   	7
#define CHRGOPT0_WDTMR_ADJ  	5

// 0x31
#define CHRGOPT1_EN_IBAT    	7
#define CHRGOPT1_RSNS_RAC   	3
#define CHRGOPT1_RSNS_RSR   	2

// 0x30
#define CHRGOPT1_EN_PTM   		2

#define WDTMR_ADJ_DISABLE   	0
#define WDTMR_ADJ_5SEC      	1
#define WDTMR_ADJ_88SEC     	2
#define WDTMR_ADJ_175SEC    	3
#define ADCOPT_ADC_CONV     	7
#define ADCOPT_ADC_START    	6
#define ADC_CONV_ONESHOT    	0
#define ADC_CONV_CONT       	1
#define RSNS_10MOHM         	10
#define RSNS_5MOHM          	5

#define VBUS_LSB 				(float)0.096
#define VSYS_LSB 				(float)0.064
#define VSYS_OFFSET 			(float)2.88
#define VBAT_LSB 				(float)0.064
#define VBAT_OFFSET 			(float)2.88
#define VBAT_OFFSET_5S 			8160
#define VSYSMIN_LSB 			100
#define ICHG_10MOHM_LSB 		(float)0.064
#define ICHG_5MOHM_LSB 			(float)0.128
#define IDCHG_10MOHM_LSB 		(float)0.256
#define IDCHG_5MOHM_LSB 		(float)0.512
#define IIN_10MOHM_LSB 			(float)0.05
#define IIN_5MOHM_LSB 			(float)0.1
#define IIN_10MOHM_LSB 			(float)0.05
#define VCHRG_LSB 				(float)0.008
#define WORD_ICHRG_MAX 			8128
#define WORD_VCHRG_MAX 			23000
#define WORD_VCHRG_MIN 			1024
#define WORD_VSYSMIN_MAX 		23000
#define WORD_VSYSMIN_MIN 		1000

// ---------------------------------------------------------------

typedef struct
{
    uint8_t dev_addr;
    uint8_t adc_mode;
    uint8_t watchdog_adj;
    uint8_t rsr;
    uint8_t rac;

} bq25730_config_t;

// ---------------------------------------------------------------
// Exports
uchar	bq25730_init(bq25730_config_t *cfg);
void 	bq25730_lowpwr_on(bq25730_config_t *cfg);
uchar 	bq25730_lowpwr_off(bq25730_config_t *cfg);
uchar 	bq25730_set_watchdog(bq25730_config_t *cfg);
uchar 	bq25730_adc_enable_all(bq25730_config_t *cfg);
uchar 	bq25730_adc_setmode(bq25730_config_t *cfg);
void 	bq25730_adc_start_conversion(bq25730_config_t *cfg);
ulong 	bq25730_read_vbus(bq25730_config_t *cfg);
ulong 	bq25730_read_vsys(bq25730_config_t *cfg);
ulong 	bq25730_read_vbat(bq25730_config_t *cfg);
float 	bq25730_read_vsysmin(bq25730_config_t *cfg);
bool 	bq25730_set_vsysmin(bq25730_config_t *cfg, ulong vsys_min_mV);
bool 	bq25730_set_rsense(bq25730_config_t *cfg);
uchar 	bq25730_ibat_on(bq25730_config_t *cfg);
uchar 	bq25730_toggle_ptm(bq25730_config_t *cfg, uchar ptm_on);
void 	bq25730_ibat_off(bq25730_config_t *cfg);
void 	bq25730_read_chg_stat(bq25730_config_t *cfg);
void 	bq25730_read_ibat(bq25730_config_t *cfg, float *ibat_charge, float *ibat_discharge);
float 	bq25730_read_iin(bq25730_config_t *cfg);
bool 	bq25730_set_icharge(bq25730_config_t *cfg, ulong ch_cur_mA);
bool 	bq25730_set_vcharge(bq25730_config_t *cfg, ulong ch_v_mV);
uchar 	bq25730_read_chip_id(bq25730_config_t *cfg);

#endif
