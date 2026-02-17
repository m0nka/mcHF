// https://github.com/noahcroit/bms-bq25730-ststm32
//
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_BMS

#include "shared_i2c.h"

#include "bq25730.h"

static uchar bq25730_i2c_write_registers(uint8_t dev_addr, uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_write_reg(dev_addr, word_addr, data, len);
	if(err != 0)
	{
		printf("write block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

static uchar bq25730_i2c_read_registers(uint8_t dev_addr, uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_read_reg(dev_addr, word_addr, data, len );
	if(err != 0)
	{
		printf("read block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

uchar bq25730_init(bq25730_config_t *cfg)
{
	// Do we have device on the bus ?
	if(shared_i2c_is_ready(cfg->dev_addr, 10) != 0)
		return 1;

	// Read manuf and chip id
	if(bq25730_read_chip_id(cfg))
		return 2;

    // Disable Low-power & Disable watchdog on ChargeOption0
    if(bq25730_set_watchdog(cfg))
    	return 3;

    // Exit low power mode
    if(bq25730_lowpwr_off(cfg))
    	return 4;

    // Enable ADC
    if(bq25730_adc_enable_all(cfg))
    	return 5;

    // Set ADC conversion mode as continuous-mode
    if(bq25730_adc_setmode(cfg))
    	return 6;

    // Enable IBAT buffer to measure battery current
    if(bq25730_ibat_on(cfg))
    	return 7;

    // Set a new VSYSMIN value
    if(!bq25730_set_vsysmin(cfg))
    	return 8;

    // Enable pass through mode
    if(bq25730_toggle_ptm(cfg, 1))
    	return 9;

    printf("vbat: %dmV \r\n", (int)bq25730_read_vbat(cfg));
    printf("vbus: %dmV \r\n", (int)bq25730_read_vbus(cfg));
    printf("vsys: %dmV \r\n", (int)bq25730_read_vsys(cfg));

    bq25730_read_ibat(cfg, NULL, NULL);

    //printf("charger in PTM mode \r\n");
    return 0;
}

void bq25730_lowpwr_on(bq25730_config_t *cfg)
{
    uint8_t databuf[2];

    // Read current value of ChargeOption0
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT0, databuf, 2);

    // Enable low-power mode to ChargeOption0
    databuf[1] |= (1 << CHRGOPT0_EN_LWPWR);  

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT0, databuf, 2);

}

uchar bq25730_lowpwr_off(bq25730_config_t *cfg)
{
    uint8_t databuf[2];

    // Read current value of ChargeOption0
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT0, databuf, 2))
    	return 1;

    // Disable low-power mode to ChargeOption0
    databuf[1] &= ~(1 << CHRGOPT0_EN_LWPWR);  

    if(bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT0, databuf, 2))
    	return 2;

    return 0;
}

uchar bq25730_set_watchdog(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current value of ChargeOption0 (2nd byte)
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT0+1, &databuf, 1))
    	return 1;

    //printf("wd: %x \r\n", databuf);
    databuf &= ~(3 << CHRGOPT0_WDTMR_ADJ);
    databuf |= (cfg->watchdog_adj << CHRGOPT0_WDTMR_ADJ);

    if(bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT0+1, &databuf, 1))
    	return 2;

    return 0;
}

uchar bq25730_adc_enable_all(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Enable ADC for all inputs
    databuf = databuf | 0xFF; 

    if(bq25730_i2c_write_registers(cfg->dev_addr, ADDR_ADCOPT, &databuf, 1))
    	return 1;

    return 0;
}

uchar bq25730_adc_setmode(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCOPT (2nd byte)
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCOPT+1, &databuf, 1))
    	return 1;

    if (cfg->adc_mode == ADC_CONV_ONESHOT) {
        databuf &= ~(1 << ADCOPT_ADC_CONV);
    }
    else if (cfg->adc_mode == ADC_CONV_CONT) {
        databuf |= (1 << ADCOPT_ADC_CONV);
    }

    // Set ADC conversion mode
    if(bq25730_i2c_write_registers(cfg->dev_addr, ADDR_ADCOPT+1, &databuf, 1))
    	return 2;

    return 0;
}

void bq25730_adc_start_conversion(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCOPT (2nd byte)
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCOPT+1, &databuf, 1);

    // Set start ADC flag
    databuf |= (1 << ADCOPT_ADC_START);

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_ADCOPT+1, &databuf, 1);
}

ulong bq25730_read_vbus(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCVBUS, &databuf, 1))
    	return 0;

    //return (float)(VBUS_LSB * databuf);
    return (databuf*96);
}

ulong bq25730_read_vsys(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCVSYS, &databuf, 1))
    	return 0;

    //return (float)(VSYS_LSB * databuf) + VSYS_OFFSET;
    return ((databuf*64) + VBAT_OFFSET_5S);
}

ulong bq25730_read_vbat(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCVBAT, &databuf, 1))
    	return 0;

    //return (float)(VBAT_LSB * databuf) + VBAT_OFFSET;
    return ((databuf*64) + VBAT_OFFSET_5S);
}

float bq25730_read_vsysmin(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_VSYSMIN, &databuf, 1);

    return (float)(VSYSMIN_LSB * databuf);
}

bool bq25730_set_vsysmin(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    databuf = (uint8_t)(cfg->vsysmin / VSYSMIN_LSB);
    if ((databuf > WORD_VSYSMIN_MAX) || (databuf < WORD_VSYSMIN_MIN)) {
        return false;
    }

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_VSYSMIN, &databuf, 1);

    return true;
}

bool bq25730_set_rsense(bq25730_config_t *cfg)
{
    uint8_t databuf;
    if ((cfg->rsr != RSNS_10MOHM) || (cfg->rsr != RSNS_5MOHM)) {
        return false;
    }
    if ((cfg->rac != RSNS_10MOHM) || (cfg->rac != RSNS_5MOHM)) {
        return false;
    }

    // Read current value of ChargeOption1 (2nd byte)
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT1+1, &databuf, 1);

    // Set RNS_RSR and RSNS_RAC to RSNS_5MOHM or RSNS_10MOHM
    databuf &= ~((1 << CHRGOPT1_RSNS_RSR) | (1 << CHRGOPT1_RSNS_RAC));
    databuf |= (cfg->rsr << CHRGOPT1_RSNS_RSR);
    databuf |= (cfg->rac << CHRGOPT1_RSNS_RAC);

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT1+1, &databuf, 1);

    return true;
}

uchar bq25730_ibat_on(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current value of ChargeOption1 (2nd byte)
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT1+1, &databuf, 1))
    	return 1;

    // Enable IBAT at ChargeOption1
    databuf |= (1 << CHRGOPT1_EN_IBAT);

    if(bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT1+1, &databuf, 1))
    	return 2;

    return 0;
}

uchar bq25730_toggle_ptm(bq25730_config_t *cfg, uchar ptm_on)
{
    uint8_t databuf;

    // Read current value of ChargeOption1 (1st byte)
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT1, &databuf, 1))
    	return 1;

    // Toggle pass through mode on ChargeOption1
    if(ptm_on)
    	databuf |= (1 << CHRGOPT1_EN_PTM);
    else
    	databuf &= ~(1 << CHRGOPT1_EN_PTM);

    if(bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT1, &databuf, 1))
    	return 2;

    return 0;
}

void bq25730_ibat_off(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current value of ChargeOption1 (2nd byte)
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHRGOPT1+1, &databuf, 1);

    // Enable IBAT at ChargeOption1
    databuf &= ~(1 << CHRGOPT1_EN_IBAT);  

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGOPT1+1, &databuf, 1);
}

void bq25730_read_ibat(bq25730_config_t *cfg, float *ibat_charge, float *ibat_discharge)
{
    uint8_t databuf[2];
    //float lsb_charge;
    //float lsb_discharge;

    // Read charging & discharging current of ADCIBAT 
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCIDCHG, databuf, 2);

    //if(cfg->rsr == RSNS_10MOHM){
     //   lsb_charge = ICHG_10MOHM_LSB;
     //   lsb_discharge = IDCHG_10MOHM_LSB;
   // }
    //else if(cfg->rsr == RSNS_5MOHM){
    //    lsb_charge = ICHG_5MOHM_LSB;
    //    lsb_discharge = IDCHG_5MOHM_LSB;
    //}

    printf("b_ch: %dmA \r\n", databuf[1] * 128);
    printf("b_dc: %dmA \r\n", databuf[0] * 512);

    //*ibat_charge = (float)(databuf[1] * lsb_charge);
    //*ibat_discharge = (float)(databuf[0] * lsb_discharge);
}

float bq25730_read_iin(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    bq25730_i2c_read_registers(cfg->dev_addr, ADDR_ADCIIN, &databuf, 1);

    if(cfg->rac == RSNS_10MOHM){
        return (float)(IIN_10MOHM_LSB * databuf);
    }
    else{
        return (float)(IIN_5MOHM_LSB * databuf);
    }
}

bool bq25730_set_icharge(bq25730_config_t *cfg)
{
    uint8_t databuf[2];
    uint16_t word;
    float icharge_lsb;

    if (cfg->rsr == RSNS_10MOHM) {
        icharge_lsb = ICHG_10MOHM_LSB; 
    }
    else if(cfg->rsr == RSNS_5MOHM) {
        icharge_lsb = ICHG_5MOHM_LSB; 
    }
    else{
        return false;
    }
    word = (uint8_t)(cfg->icharge / icharge_lsb) << 6;
    if(word > WORD_ICHRG_MAX){
        return false;
    }

    databuf[0] = (uint8_t)word;
    databuf[1] = (uint8_t)(word >> 8);

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGCURR, databuf, 2);

    return true;
}

bool bq25730_set_vcharge(bq25730_config_t *cfg)
{
    uint8_t databuf[2];
    uint16_t word;
    word = (uint16_t)(cfg->vcharge / VCHRG_LSB) << 3;
    if((word > WORD_VCHRG_MAX) || (word < WORD_VCHRG_MIN))
    {
        return false; 
    }

    databuf[0] = (uint8_t)word;
    databuf[1] = (uint8_t)(word >> 8);

    bq25730_i2c_write_registers(cfg->dev_addr, ADDR_CHRGVOLT, databuf, 2);

    return true;
}

uchar bq25730_read_chip_id(bq25730_config_t *cfg)
{
    uint8_t databuf[2];

    // Read manuf id
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_MANUF_ID, databuf, 1))
    	return 1;

    // Read chip id
    if(bq25730_i2c_read_registers(cfg->dev_addr, ADDR_CHIP_ID, databuf + 1, 1))
    	return 2;

    //printf("cid: 0x%x%x\r\n", databuf[0], databuf[1]);

    // Correct identity ?
    if((databuf[0] != 0x40)||(databuf[1] != 0xD5))
    	return 3;

    return 0;
}

#endif
