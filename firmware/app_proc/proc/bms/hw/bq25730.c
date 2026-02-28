// https://github.com/noahcroit/bms-bq25730-ststm32
//
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_BMS

#include "shared_i2c.h"

#include "bq25730.h"

static uchar bq25730_i2c_write_registers(uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_write_reg(0xD7, word_addr, data, len);
	if(err != 0)
	{
		printf("write block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

static uchar bq25730_i2c_read_registers(uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_read_reg(0xD6, word_addr, data, len );
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
	if(shared_i2c_is_ready((BQ25730_DEFAULT_ADDR<<1), 10) != 0)
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
    if(!bq25730_set_vsysmin(cfg, VSYSMIN_TARGET))
    	return 8;

    // Set charger voltage
    if(!bq25730_set_vcharge(cfg, VCHARGE_TARGET))
    	return 9;

    // Set charging current
    if(!bq25730_set_icharge(cfg, ICHRG_TARGET))
    	return 10;

    //if(!bq25730_set_rsense(cfg))
    //	return 11;

    // Enable pass through mode
    //if(bq25730_toggle_ptm(cfg, 1))
    //	return 12;

    // Read charger status on start
    //bq25730_read_chg_stat(cfg);

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
    bq25730_i2c_read_registers(ADDR_CHRGOPT0, databuf, 2);

    // Enable low-power mode to ChargeOption0
    databuf[1] |= (1 << CHRGOPT0_EN_LWPWR);  

    bq25730_i2c_write_registers(ADDR_CHRGOPT0, databuf, 2);

}

uchar bq25730_lowpwr_off(bq25730_config_t *cfg)
{
    uint8_t databuf[2];

    // Read current value of ChargeOption0
    if(bq25730_i2c_read_registers(ADDR_CHRGOPT0, databuf, 2))
    	return 1;

    // Disable low-power mode to ChargeOption0
    databuf[1] &= ~(1 << CHRGOPT0_EN_LWPWR);  

    if(bq25730_i2c_write_registers(ADDR_CHRGOPT0, databuf, 2))
    	return 2;

    return 0;
}

uchar bq25730_set_watchdog(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current value of ChargeOption0 (2nd byte)
    if(bq25730_i2c_read_registers(ADDR_CHRGOPT0+1, &databuf, 1))
    	return 1;

    //printf("wd: %x \r\n", databuf);

    databuf &= ~(3 << CHRGOPT0_WDTMR_ADJ);
    databuf |= (cfg->watchdog_adj << CHRGOPT0_WDTMR_ADJ);

    //printf("wd: %x \r\n", databuf);

    if(bq25730_i2c_write_registers(ADDR_CHRGOPT0+1, &databuf, 1))
    	return 2;

    // Read current value of ChargeOption0 (2nd byte)
    if(bq25730_i2c_read_registers(ADDR_CHRGOPT0+1, &databuf, 1))
    	return 1;

    //printf("wd: %x \r\n", databuf);

    return 0;
}

uchar bq25730_adc_enable_all(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Enable ADC for all inputs
    databuf = databuf | 0xFF; 

    if(bq25730_i2c_write_registers(ADDR_ADCOPT, &databuf, 1))
    	return 1;

    return 0;
}

uchar bq25730_adc_setmode(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCOPT (2nd byte)
    if(bq25730_i2c_read_registers(ADDR_ADCOPT+1, &databuf, 1))
    	return 1;

    if (cfg->adc_mode == ADC_CONV_ONESHOT) {
        databuf &= ~(1 << ADCOPT_ADC_CONV);
    }
    else if (cfg->adc_mode == ADC_CONV_CONT) {
        databuf |= (1 << ADCOPT_ADC_CONV);
    }

    // Set ADC conversion mode
    if(bq25730_i2c_write_registers(ADDR_ADCOPT+1, &databuf, 1))
    	return 2;

    return 0;
}

void bq25730_adc_start_conversion(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCOPT (2nd byte)
    bq25730_i2c_read_registers(ADDR_ADCOPT+1, &databuf, 1);

    // Set start ADC flag
    databuf |= (1 << ADCOPT_ADC_START);

    bq25730_i2c_write_registers(ADDR_ADCOPT+1, &databuf, 1);
}

ulong bq25730_read_vbus(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(ADDR_ADCVBUS, &databuf, 1))
    	return 0;

    //return (float)(VBUS_LSB * databuf);
    return (databuf*96);
}

ulong bq25730_read_vsys(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(ADDR_ADCVSYS, &databuf, 1))
    	return 0;

    if(databuf)
    	return ((databuf*64) + VBAT_OFFSET_5S);
    else
    	return 0;
}

ulong bq25730_read_vbat(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(ADDR_ADCVBAT, &databuf, 1))
    	return 0;

    if(databuf)
    	return ((databuf*64) + VBAT_OFFSET_5S);
    else
    	return 0;
}

float bq25730_read_vsysmin(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    bq25730_i2c_read_registers(ADDR_VSYSMIN, &databuf, 1);

    return (float)(VSYSMIN_LSB * databuf);
}

bool bq25730_set_vsysmin(bq25730_config_t *cfg, ulong vsys_min_mV)
{
    uint8_t databuf;

	#if 0
    if(bq25730_i2c_read_registers(ADDR_VSYSMIN, &databuf, 1))
    	return false;

    printf("vsysmin read: %02x \r\n", databuf);
	#endif

    // Check range
    if((vsys_min_mV > WORD_VSYSMIN_MAX)||(vsys_min_mV < WORD_VSYSMIN_MIN))
       return false;

    databuf = vsys_min_mV/VSYSMIN_LSB;

    //printf("vsysmin set: 0x%x(%dmV) \r\n", databuf, (int)vsys_min_mV);

    if(bq25730_i2c_write_registers(ADDR_VSYSMIN, &databuf, 1))
    	return false;

    return true;
}

bool bq25730_set_rsense(bq25730_config_t *cfg)
{
    uint8_t databuf;

    //if ((cfg->rsr != RSNS_10MOHM) || (cfg->rsr != RSNS_5MOHM)) {
    //    return false;
    //}
    //if ((cfg->rac != RSNS_10MOHM) || (cfg->rac != RSNS_5MOHM)) {
    //    return false;
    //}

    // Read current value of ChargeOption1 (2nd byte)
    bq25730_i2c_read_registers(ADDR_CHRGOPT1+1, &databuf, 1);

    printf("chg opt read: %02x \r\n", databuf);

    // Set RNS_RSR and RSNS_RAC to RSNS_5MOHM or RSNS_10MOHM
    databuf &= ~((1 << CHRGOPT1_RSNS_RSR) | (1 << CHRGOPT1_RSNS_RAC));
    databuf |= (cfg->rsr << CHRGOPT1_RSNS_RSR);
    databuf |= (cfg->rac << CHRGOPT1_RSNS_RAC);

    printf("chg opt modf: %02x \r\n", databuf);

    bq25730_i2c_write_registers(ADDR_CHRGOPT1+1, &databuf, 1);

    bq25730_i2c_read_registers(ADDR_CHRGOPT1+1, &databuf, 1);

    printf("chg opt read: %02x \r\n", databuf);

    return true;
}

uchar bq25730_ibat_on(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current value of ChargeOption1 (2nd byte)
    if(bq25730_i2c_read_registers(ADDR_CHRGOPT1+1, &databuf, 1))
    	return 1;

    //printf("chg opt read: %02x \r\n", databuf);

    // Enable IBAT at ChargeOption1
    databuf |= (1 << CHRGOPT1_EN_IBAT);

    //printf("chg opt modf: %02x \r\n", databuf);

    if(bq25730_i2c_write_registers(ADDR_CHRGOPT1+1, &databuf, 1))
    	return 2;

    if(bq25730_i2c_read_registers(ADDR_CHRGOPT1+1, &databuf, 1))
    	return 1;

    //printf("chg opt read: %02x \r\n", databuf);

    return 0;
}

uchar bq25730_toggle_ptm(bq25730_config_t *cfg, uchar ptm_on)
{
    uint8_t databuf[2];

    // Read current value of ChargeOption1 (1st byte)
    if(bq25730_i2c_read_registers(ADDR_CHRGOPT1, databuf, 2))
    	return 1;

    printf("chg opt read: %02x%02x \r\n", databuf[0], databuf[1]);

    // Toggle pass through mode on ChargeOption1
    if(ptm_on)
    	databuf[0] |= (1 << CHRGOPT1_EN_PTM);
    else
    	databuf[0] &= ~(1 << CHRGOPT1_EN_PTM);

    printf("chg opt modf: %02x%02x \r\n", databuf[0], databuf[1]);

    if(bq25730_i2c_write_registers(ADDR_CHRGOPT1, databuf, 2))
    	return 2;

    if(bq25730_i2c_read_registers(ADDR_CHRGOPT1, databuf, 2))
    	return 1;

    printf("chg opt read: %02x%02x \r\n", databuf[0], databuf[1]);

    return 0;
}

void bq25730_ibat_off(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current value of ChargeOption1 (2nd byte)
    bq25730_i2c_read_registers(ADDR_CHRGOPT1+1, &databuf, 1);

    // Enable IBAT at ChargeOption1
    databuf &= ~(1 << CHRGOPT1_EN_IBAT);  

    bq25730_i2c_write_registers(ADDR_CHRGOPT1+1, &databuf, 1);
}

ushort bq25730_read_chg_stat(bq25730_config_t *cfg)
{
    uint8_t databuf[2];

    // Read status bits
    if(bq25730_i2c_read_registers(ADDR_CHRG_STAT, databuf, 2))
    	return 0xFFFF;

    //printf("stat: %02x %02x \r\n", databuf[0], databuf[1]);

    return ((databuf[0] << 8) | databuf[1]);
}

void bq25730_read_ibat(bq25730_config_t *cfg, ushort *ibat_charge, ushort *ibat_discharge)
{
    uint8_t databuf[2];
    //float lsb_charge;
    //float lsb_discharge;

    if((ibat_charge == NULL)||(ibat_discharge == NULL))
    	return;

    // Read charging & discharging current of ADCIBAT 
    if(bq25730_i2c_read_registers(ADDR_ADCIDCHG, databuf, 2))
    	return;

    //if(cfg->rsr == RSNS_10MOHM){
     //   lsb_charge = ICHG_10MOHM_LSB;
     //   lsb_discharge = IDCHG_10MOHM_LSB;
   // }
    //else if(cfg->rsr == RSNS_5MOHM){
    //    lsb_charge = ICHG_5MOHM_LSB;
    //    lsb_discharge = IDCHG_5MOHM_LSB;
    //}

    //printf("dis: %dmA ch: %dmA\r\n", databuf[0]*512, databuf[1]*128);

    if(cfg->rsr == RSNS_10MOHM)
    {
    	*ibat_charge 	= (ushort)(databuf[1] * 64);
    	*ibat_discharge = (ushort)(databuf[0] * 256);
    }
    else
    {
    	*ibat_charge 	= (ushort)(databuf[1] * 128);
    	*ibat_discharge = (ushort)(databuf[0] * 512);
    }
}

ushort bq25730_read_iin(bq25730_config_t *cfg)
{
    uint8_t databuf;

    // Read current ADCVBUS
    if(bq25730_i2c_read_registers(ADDR_ADCIIN, &databuf, 1))
    	return 0;

    //printf("in %dmA\r\n", databuf*100);

    if(cfg->rac == RSNS_10MOHM){
        return (float)(50 * databuf);
    }
    else{
        return (float)(100 * databuf);
    }
}

//*----------------------------------------------------------------------------
//* Function Name       : bq25730_set_icharge
//* Object              :
//* Notes    			: set charging current
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
bool bq25730_set_icharge(bq25730_config_t *cfg, ulong ch_cur_mA)
{
    uint8_t databuf[2];

    // Check range
    if((cfg->rsr == RSNS_10MOHM)&&(ch_cur_mA > WORD_ICHRG_MAX))
    	return false;
    else if((cfg->rsr == RSNS_5MOHM)&&(ch_cur_mA > WORD_ICHRG_MAX*2))
    	return false;

    databuf[0] = (uint8_t)ch_cur_mA;
    databuf[1] = (uint8_t)(ch_cur_mA >> 8);

    //printf("icharge set: 0x%02x%02x(%dmA) \r\n", databuf[1], databuf[0], (int)ch_cur_mA);

    if(bq25730_i2c_write_registers(ADDR_CHRGCURR, databuf, 2))
    	return false;

    //bq25730_i2c_read_registers(ADDR_CHRGCURR, databuf, 2);
    //printf("icharge read: 0x%02x%02x \r\n", databuf[1], databuf[0]);

    return true;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq25730_set_vcharge
//* Object              :
//* Notes    			: set charging voltage
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
bool bq25730_set_vcharge(bq25730_config_t *cfg, ulong ch_v_mV)
{
    uint8_t databuf[2];

    // Check range
    if((ch_v_mV > WORD_VCHRG_MAX) || (ch_v_mV < WORD_VCHRG_MIN))
        return false;

    databuf[0] = (uint8_t)ch_v_mV;
    databuf[1] = (uint8_t)(ch_v_mV >> 8);

    //printf("vcharge set: 0x%02x%02x(%dmV) \r\n", databuf[0], databuf[1], (int)ch_v_mV);

    if(bq25730_i2c_write_registers(ADDR_CHRGVOLT, databuf, 2))
    	return false;

	#if 0
    if(bq25730_i2c_read_registers(ADDR_CHRGVOLT, databuf, 2))
    	return false;

    printf("vcharge read: %02x %02x \r\n", databuf[0], databuf[1]);
	#endif

    return true;
}

uchar bq25730_read_chip_id(bq25730_config_t *cfg)
{
    uint8_t databuf[2];

    // Read manuf id
    if(bq25730_i2c_read_registers(ADDR_MANUF_ID, databuf, 1))
    	return 1;

    // Read chip id
    if(bq25730_i2c_read_registers(ADDR_CHIP_ID, databuf + 1, 1))
    	return 2;

    printf("chid: 0x%x%x\r\n", databuf[0], databuf[1]);

    // Correct identity ?
    if((databuf[0] != 0x40)||(databuf[1] != 0xD5))
    	return 3;

    return 0;
}

#endif
