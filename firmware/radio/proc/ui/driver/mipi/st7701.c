/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include "main.h"
#include "st7701.h"

#include <stddef.h>

#if 1

// Vendor specific/not documented
static const uint8_t cmd2_bk1_e0[] = {0x00, 0x00, 0x02};
static const uint8_t cmd2_bk1_e1[] = {0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t cmd2_bk1_e3[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e4[] = {0x44, 0x44};
static const uint8_t cmd2_bk1_e5[] = {0x0E, 0x60, 0xA0, 0xA0, 0x10, 0x60, 0xA0, 0xA0, 0x0A, 0x60, 0xA0, 0xA0, 0x0C, 0x60, 0xA0, 0xA0};
static const uint8_t cmd2_bk1_e6[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e7[] = {0x44, 0x44};
static const uint8_t cmd2_bk1_e8[] = {0x0D, 0x60, 0xA0, 0xA0, 0x0F, 0x60, 0xA0, 0xA0, 0x09, 0x60, 0xA0, 0xA0, 0x0B, 0x60, 0xA0, 0xA0};
static const uint8_t cmd2_bk1_eb[] = {0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40};
static const uint8_t cmd2_bk1_ec[] = {0x02, 0x01};
static const uint8_t cmd2_bk1_ed[] = {0xAB, 0x89, 0x76, 0x54, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x45, 0x67, 0x98, 0xBA};

const uint8_t sleep_out[] = {OTM8009A_CMD_SLPOUT, 0x00};

extern DSI_HandleTypeDef   hdsi;

//#define LOC_DEBUG

void mipi_change_page(unsigned char page)
{
	static const uint8_t lcd_reg_data00[] =   {0x77, 0x01, 0x00, 0x00, 0x00};
	uint8_t lcd_reg_data[10];

	memcpy(lcd_reg_data, lcd_reg_data00, 4);
	lcd_reg_data[4] = page;

	HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 6, 0xFF, lcd_reg_data);

	#ifdef LOC_DEBUG
	lcd_reg_data[0] = 0xFF;
	memcpy(lcd_reg_data + 1, lcd_reg_data00, 4);
	lcd_reg_data[5] = page;
	print_hex_array(lcd_reg_data,6);
	#endif
}

// Write cmd + single byte data
void mipi_write_short(uint8_t reg, uint8_t data)
{
	HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, reg, data);

	#ifdef LOC_DEBUG
	uint8_t lcd_reg_data[2];
	lcd_reg_data[0] = reg;
	lcd_reg_data[1] = data;
	print_hex_array(lcd_reg_data,2);
	#endif
}

void mipi_write_long(uchar cmd, const uchar * data, ushort size)
{
	HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, size, cmd, (unsigned char *)data);

	#ifdef LOC_DEBUG
	uint8_t lcd_reg_data[100];
	lcd_reg_data[0] = cmd;
	memcpy(lcd_reg_data + 1, data, size);
	print_hex_array(lcd_reg_data,size + 1);
	#endif
}

int ST7701S_Init(unsigned long ColorCoding)
{
	static const uint8_t lcd_reg_data03[] = {0x02, 0x00, 0x00};
	static const uint8_t lcd_reg_data04[] = {0x00, 0x0D, 0x14, 0x0D, 0x10, 0x05, 0x02, 0x08, 0x08, 0x1E, 0x05, 0x13, 0x11, 0xA3, 0x29, 0x18};
	static const uint8_t lcd_reg_data05[] = {0x00, 0x0C, 0x14, 0x0C, 0x10, 0x05, 0x03, 0x08, 0x07, 0x20, 0x05, 0x13, 0x11, 0xA4, 0x29, 0x18};

	unsigned char buff[40];

	//printf("ST7701_Init...\r\n");

	// After reset delay	-	 ToDo: check if kernel running, if yes, use OS delay!
	//HAL_Delay(200);

	//mipi_write_long(  MIPI_DCS_EXIT_SLEEP_MODE, buff, 0);
	//DSI_IO_WriteCmd(0, (uint8_t *)sleep_out);
	mipi_write_short(MIPI_DCS_EXIT_SLEEP_MODE, 0);

	// After sleep delay	-	 ToDo: check if kernel running, if yes, use OS delay!
	HAL_Delay(120);

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Page Command2_BK0 (0x10)
	mipi_change_page(DSI_CMD2BK0_SEL);

	// -------------------------------------------------------------------------------
	// Display Line setting (0xC0)
	//
	// EX:(C0:0x6b,0x00) ((0x6b+1) x 8) = 864;
	// EX:(C0:0xe9,0x03) ((0x69+1) x 8) + ( 3x2 ) = 854
	//
	buff[0] = DSI_CMD2_BK0_LNESET_B0;
	buff[1] = DSI_CMD2_BK0_LNESET_B1;
	mipi_write_long(DSI_CMD2_BK0_LNESET, buff, 2);

	// -------------------------------------------------------------------------------
	// Porch control (0xC1)
	//
	buff[0] = DSI_CMD2_BK0_PORCTRL_B0;
	buff[1] = DSI_CMD2_BK0_PORCTRL_B1;
	mipi_write_long(DSI_CMD2_BK0_PORCTRL, buff, 2);

	// -------------------------------------------------------------------------------
	// Inversion selection and frame rate control
	//
	// Densitron: 	0x37 0x08
	// Linux:		0x37 0x06
	//
	buff[0] = DSI_CMD2_BK0_INVSEL_B0;
	buff[1] = DSI_CMD2_BK0_INVSEL_B1;
	mipi_write_long(0xC2, buff, 2);

	mipi_write_short(0xCC, 0x10);
	mipi_write_long(0xC3, &lcd_reg_data03[0], sizeof(lcd_reg_data03));
	mipi_write_long(0xB0, &lcd_reg_data04[0], sizeof(lcd_reg_data04));
	mipi_write_long(0xB1, &lcd_reg_data05[0], sizeof(lcd_reg_data05));

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Command2_BK1 (0x11)
	mipi_change_page(DSI_CMD2BK1_SEL);

	mipi_write_short(0xB0, 0x6C);
	mipi_write_short(0xB1, 0x3F);
	mipi_write_short(0xB2, 0x07);
	mipi_write_short(0xCC, 0x10);
	mipi_write_short(0xB3, 0x80);
	mipi_write_short(0xB5, 0x47);
//	mipi_write_short(0xB7, DSI_CMD2_BK1_PWCTLR1_SET);	// 0x85
	mipi_write_short(0xB8, 0x20);
	mipi_write_short(0xB9, 0x10);
	mipi_write_short(0xC1, 0x78);
	mipi_write_short(0xC2, 0x78);
	mipi_write_short(0xD0, 0x88);

	// -------------------------------------------------------------------------------
	// Vendor specific/not documented
	mipi_write_long(0xE0, &cmd2_bk1_e0[0], sizeof(cmd2_bk1_e0));
	mipi_write_long(0xE1, &cmd2_bk1_e1[0], sizeof(cmd2_bk1_e1));
	mipi_write_long(0xE2, &cmd2_bk1_e2[0], sizeof(cmd2_bk1_e2));
	mipi_write_long(0xE3, &cmd2_bk1_e3[0], sizeof(cmd2_bk1_e3));
	mipi_write_long(0xE4, &cmd2_bk1_e4[0], sizeof(cmd2_bk1_e4));
	mipi_write_long(0xE5, &cmd2_bk1_e5[0], sizeof(cmd2_bk1_e5));
	mipi_write_long(0xE6, &cmd2_bk1_e6[0], sizeof(cmd2_bk1_e6));
	mipi_write_long(0xE7, &cmd2_bk1_e7[0], sizeof(cmd2_bk1_e7));
	mipi_write_long(0xE8, &cmd2_bk1_e8[0], sizeof(cmd2_bk1_e8));
	mipi_write_long(0xEB, &cmd2_bk1_eb[0], sizeof(cmd2_bk1_eb));
	mipi_write_long(0xEC, &cmd2_bk1_ec[0], sizeof(cmd2_bk1_ec));
	mipi_write_long(0xED, &cmd2_bk1_ed[0], sizeof(cmd2_bk1_ed));

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Normal command (0x00)
	mipi_change_page(DSI_CMD2BKX_SEL_NONE);

	// 24BIT
	if(ColorCoding == DSI_RGB565)
		mipi_write_short(0x3A, 0x50);
	else
		mipi_write_short(0x3A, 0x70);

    mipi_write_short(OTM8009A_CMD_DISPON, 0);

	//printf("ST7701_Init done.\r\n");

	return 0;
}
#endif

#if 0

#include "main.h"

#include "otm8009a.h"
#include "st7701.h"

#include <stddef.h>
//#include "stm32h747i_discovery_lcd.h"

#if 1

typedef int32_t (*OTM8009A_GetTick_Func) (void);
typedef int32_t (*OTM8009A_Delay_Func)   (uint32_t);
typedef int32_t (*OTM8009A_WriteReg_Func)(uint16_t, uint16_t, uint8_t*, uint16_t);
typedef int32_t (*OTM8009A_ReadReg_Func) (uint16_t, uint16_t, uint8_t*, uint16_t);

typedef int32_t (*OTM8009A_Write_Func)(void *, uint16_t, uint8_t*, uint16_t);
typedef int32_t (*OTM8009A_Read_Func) (void *, uint16_t, uint8_t*, uint16_t);

typedef struct
{
  OTM8009A_Write_Func   WriteReg;
  OTM8009A_Read_Func    ReadReg;
  void                  *handle;
} otm8009a_ctx_t;

typedef struct
{
  uint16_t                  Address;
  OTM8009A_WriteReg_Func    WriteReg;
  OTM8009A_ReadReg_Func     ReadReg;
  OTM8009A_GetTick_Func     GetTick;
} OTM8009A_IO_t;

typedef struct
{
  OTM8009A_IO_t         IO;
  otm8009a_ctx_t        Ctx;
  uint8_t               IsInitialized;
} OTM8009A_Object_t;

typedef struct
{
  uint32_t  Orientation;
  uint32_t  ColorCode;
  uint32_t  Brightness;
} OTM8009A_LCD_Ctx_t;


#define OTM8009A_ERROR 	1
#define OTM8009A_OK		0

static OTM8009A_LCD_Ctx_t OTM8009ACtx;

// Vendor specific/not documented
static const uint8_t cmd2_bk1_e0[] = {0x00, 0x00, 0x02};
static const uint8_t cmd2_bk1_e1[] = {0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t cmd2_bk1_e3[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e4[] = {0x44, 0x44};
static const uint8_t cmd2_bk1_e5[] = {0x0E, 0x60, 0xA0, 0xA0, 0x10, 0x60, 0xA0, 0xA0, 0x0A, 0x60, 0xA0, 0xA0, 0x0C, 0x60, 0xA0, 0xA0};
static const uint8_t cmd2_bk1_e6[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e7[] = {0x44, 0x44};
static const uint8_t cmd2_bk1_e8[] = {0x0D, 0x60, 0xA0, 0xA0, 0x0F, 0x60, 0xA0, 0xA0, 0x09, 0x60, 0xA0, 0xA0, 0x0B, 0x60, 0xA0, 0xA0};
static const uint8_t cmd2_bk1_eb[] = {0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40};
static const uint8_t cmd2_bk1_ec[] = {0x02, 0x01};
static const uint8_t cmd2_bk1_ed[] = {0xAB, 0x89, 0x76, 0x54, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x45, 0x67, 0x98, 0xBA};

/*
  * CASET value (Column Address Set) : X direction LCD GRAM boundaries
  * depending on LCD orientation mode and PASET value (Page Address Set) : Y direction
  * LCD GRAM boundaries depending on LCD orientation mode
  * XS[15:0] = 0x000 = 0, XE[15:0] = 0x31F = 799 for landscape mode : apply to CASET
  * YS[15:0] = 0x000 = 0, YE[15:0] = 0x31F = 799 for portrait mode : apply to PASET
  */
//static const uint8_t LcdRegData27[] = {0x00, 0x00, 0x03, 0x1F};
static const uint8_t LcdRegData27[] = {0x00, 0x00, 0x03, 0xff};
/*
  * XS[15:0] = 0x000 = 0, XE[15:0] = 0x1DF = 479 for portrait mode : apply to CASET
  * YS[15:0] = 0x000 = 0, YE[15:0] = 0x1DF = 479 for landscape mode : apply to PASET
 */
static const uint8_t LcdRegData28[] = {0x00, 0x00, 0x01, 0xDF};

static int32_t ST7701_ReadRegWrap(void *Handle, uint16_t Reg, uint8_t* Data, uint16_t Length);
static int32_t ST7701_WriteRegWrap(void *Handle, uint16_t Reg, uint8_t *pData, uint16_t Length);
static int32_t ST7701_IO_Delay(OTM8009A_Object_t *pObj, uint32_t Delay);
int32_t ST7701_DisplayOnA(OTM8009A_Object_t *pObj);

void print_hex_array(unsigned char *pArray, unsigned short aSize)
{
	long i = 0,j = 0, c = 0;
	char buf[200];

	//buf = (char *)malloc(aSize * 10);
	memset(buf, 0, (aSize * 10));

	for (i = 0; i < aSize; i++)
	{
		j += sprintf( buf+j ,"%02x ", *pArray );
		pArray++;
		//if (c++==15)
		//{
		//	j += sprintf(buf+j ,"\r\n");
		//	c = 0;
		//}
	}
	j += sprintf(buf+j ,"\r\n");
	printf(buf);
	//free(buf);
}

extern DSI_HandleTypeDef   hdsi;

int otm8009a_write_reg(otm8009a_ctx_t *ctx, uint16_t reg, const uint8_t *pdata, uint16_t length)
{
	if(length <= 1)
		HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, pdata[0], pdata[1]);
	else
		HAL_DSI_LongWrite(&hdsi,  0, DSI_DCS_LONG_PKT_WRITE, length, reg, pdata);

	return 0;
}

int32_t otm8009a_write_rega(otm8009a_ctx_t *ctx, uint16_t reg, const uint8_t *pdata, uint16_t length)
{
	char temp[100];

	temp[0] = reg;
	memcpy(temp + 1, pdata, length);
	print_hex_array(temp, length + 1);

	return otm8009a_write_reg(ctx, reg, pdata, length);
}

// Write cmd + single byte data
void mipi_write(otm8009a_ctx_t *ctx, uint16_t reg, uint8_t data)
{
	uint8_t lcd_reg_data[2];

	lcd_reg_data[0] = data;

	otm8009a_write_rega(ctx, reg, lcd_reg_data, 1);

	//lcd_reg_data[0] = reg;
	//lcd_reg_data[1] = data;
	//print_hex_array(lcd_reg_data,2);
}

#if 0
void mipi_reset(otm8009a_ctx_t *ctx)
{
	uint8_t lcd_reg_data[1];

	otm8009a_write_reg(ctx, MIPI_DCS_SOFT_RESET, &lcd_reg_data[0], 0);
	HAL_Delay(5);

	otm8009a_write_reg(ctx,  MIPI_DCS_EXIT_SLEEP_MODE, &lcd_reg_data[0], 0);
	HAL_Delay(120);
}
#endif

void mipi_change_page(otm8009a_ctx_t *ctx, unsigned char page)
{
	//static const uint8_t lcd_reg_data00[] = {0xff, 0x98, 0x06, 0x04, 0x00};
	static const uint8_t lcd_reg_data00[] =   {0x77, 0x01, 0x00, 0x00, 0x00};
	uint8_t lcd_reg_data[10];

	memcpy(lcd_reg_data,lcd_reg_data00,sizeof(lcd_reg_data00));
	lcd_reg_data[4] = page;

	//print_hex_array(lcd_reg_data,sizeof(lcd_reg_data00));
	otm8009a_write_rega(ctx, 0xFF, lcd_reg_data, sizeof(lcd_reg_data00));
}

#if 0
int32_t ST7701_RegisterBusIO (OTM8009A_Object_t *pObj, OTM8009A_IO_t *pIO)
{
  int32_t ret = OTM8009A_OK;

  if(pObj == NULL)
  {
    ret = OTM8009A_ERROR;
  }
  else
  {
    pObj->IO.WriteReg  = pIO->WriteReg;
    pObj->IO.ReadReg   = pIO->ReadReg;
    pObj->IO.GetTick   = pIO->GetTick;

    pObj->Ctx.ReadReg  = ST7701_ReadRegWrap;
    pObj->Ctx.WriteReg = ST7701_WriteRegWrap;
    pObj->Ctx.handle   = pObj;
  }

  return ret;
}
#endif

//
// Densitron Parallel init - work more or less
//
int32_t ST7701_Init(OTM8009A_Object_t *pObj, uint32_t ColorCoding, uint32_t Orientation)
{
	static const uint8_t lcd_reg_data03[] = {0x02, 0x00, 0x00};
	static const uint8_t lcd_reg_data04[] = {0x00, 0x0D, 0x14, 0x0D, 0x10, 0x05, 0x02, 0x08, 0x08, 0x1E, 0x05, 0x13, 0x11, 0xA3, 0x29, 0x18};
	static const uint8_t lcd_reg_data05[] = {0x00, 0x0C, 0x14, 0x0C, 0x10, 0x05, 0x03, 0x08, 0x07, 0x20, 0x05, 0x13, 0x11, 0xA4, 0x29, 0x18};

	unsigned char buff[40];

	// After reset delay	-	 ToDo: check if kernel running, if yes, use OS delay!
	HAL_Delay(200);

	otm8009a_write_rega(&pObj->Ctx,  MIPI_DCS_EXIT_SLEEP_MODE, buff, 0);

	// After sleep delay	-	 ToDo: check if kernel running, if yes, use OS delay!
	HAL_Delay(120);

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Page Command2_BK0 (0x10)
	mipi_change_page(&pObj->Ctx, DSI_CMD2BK0_SEL);

	// -------------------------------------------------------------------------------
	// Display Line setting (0xC0)
	//
	// EX:(C0:0x6b,0x00) ((0x6b+1) x 8) = 864;
	// EX:(C0:0xe9,0x03) ((0x69+1) x 8) + ( 3x2 ) = 854
	//
	buff[0] = DSI_CMD2_BK0_LNESET_B0;
	buff[1] = DSI_CMD2_BK0_LNESET_B1;
	//print_hex_array(buff, 2);
	otm8009a_write_rega(&pObj->Ctx, DSI_CMD2_BK0_LNESET, buff, 2);

	// -------------------------------------------------------------------------------
	// Porch control (0xC1)
	//
	buff[0] = DSI_CMD2_BK0_PORCTRL_B0;
	buff[1] = DSI_CMD2_BK0_PORCTRL_B1;
	//print_hex_array(buff, 2);
	otm8009a_write_rega(&pObj->Ctx, DSI_CMD2_BK0_PORCTRL, buff, 2);

	// -------------------------------------------------------------------------------
	// Inversion selection and frame rate control
	//
	// Densitron: 	0x37 0x08
	// Linux:		0x37 0x06
	//
	buff[0] = DSI_CMD2_BK0_INVSEL_B0;
	buff[1] = DSI_CMD2_BK0_INVSEL_B1;
	//print_hex_array(buff, 2);
	otm8009a_write_rega(&pObj->Ctx, 0xC2, buff, 2);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xCC, 0x10);

	// -------------------------------------------------------------------------------
	//
	//
	otm8009a_write_rega(&pObj->Ctx, 	0xC3, &lcd_reg_data03[0], sizeof(lcd_reg_data03));

	// -------------------------------------------------------------------------------
	//
	//
	otm8009a_write_rega(&pObj->Ctx, 	0xB0, &lcd_reg_data04[0], sizeof(lcd_reg_data04));

	// -------------------------------------------------------------------------------
	//
	//
	otm8009a_write_rega(&pObj->Ctx, 	0xB1, &lcd_reg_data05[0], sizeof(lcd_reg_data05));

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Command2_BK1 (0x11)
	mipi_change_page(&pObj->Ctx, DSI_CMD2BK1_SEL);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB0, 0x6C);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB1, 0x3F);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB2, 0x07);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xCC, 0x10);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB3, 0x80);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB5, 0x47);

	// -------------------------------------------------------------------------------
	// Kills picture!
	//printf("val: %02x\r\n", DSI_CMD2_BK1_PWCTLR1_SET);
	//mipi_write(&pObj->Ctx, 	0xB7, DSI_CMD2_BK1_PWCTLR1_SET);	//0x85);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB8, 0x20);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xB9, 0x10);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xC1, 0x78);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xC2, 0x78);

	// -------------------------------------------------------------------------------
	//
	//
	mipi_write(&pObj->Ctx, 	0xD0, 0x88);

	// -------------------------------------------------------------------------------
	// Vendor specific/not documented
	otm8009a_write_rega(&pObj->Ctx, 	0xE0, &cmd2_bk1_e0[0], sizeof(cmd2_bk1_e0));
	otm8009a_write_rega(&pObj->Ctx, 	0xE1, &cmd2_bk1_e1[0], sizeof(cmd2_bk1_e1));
	otm8009a_write_rega(&pObj->Ctx, 	0xE2, &cmd2_bk1_e2[0], sizeof(cmd2_bk1_e2));
	otm8009a_write_rega(&pObj->Ctx, 	0xE3, &cmd2_bk1_e3[0], sizeof(cmd2_bk1_e3));
	otm8009a_write_rega(&pObj->Ctx, 	0xE4, &cmd2_bk1_e4[0], sizeof(cmd2_bk1_e4));
	otm8009a_write_rega(&pObj->Ctx, 	0xE5, &cmd2_bk1_e5[0], sizeof(cmd2_bk1_e5));
	otm8009a_write_rega(&pObj->Ctx, 	0xE6, &cmd2_bk1_e6[0], sizeof(cmd2_bk1_e6));
	otm8009a_write_rega(&pObj->Ctx, 	0xE7, &cmd2_bk1_e7[0], sizeof(cmd2_bk1_e7));
	otm8009a_write_rega(&pObj->Ctx, 	0xE8, &cmd2_bk1_e8[0], sizeof(cmd2_bk1_e8));
	otm8009a_write_rega(&pObj->Ctx, 	0xEB, &cmd2_bk1_eb[0], sizeof(cmd2_bk1_eb));
	otm8009a_write_rega(&pObj->Ctx, 	0xEC, &cmd2_bk1_ec[0], sizeof(cmd2_bk1_ec));
	otm8009a_write_rega(&pObj->Ctx, 	0xED, &cmd2_bk1_ed[0], sizeof(cmd2_bk1_ed));

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Normal command (0x00)
	mipi_change_page(&pObj->Ctx, DSI_CMD2BKX_SEL_NONE);

	// 24BIT
	mipi_write(&pObj->Ctx, 	0x3A, 0x70);

	//if(Orientation == LCD_ORIENTATION_LANDSCAPE)
	//	ST7701_SetOrientation(pObj, OTM8009A_ORIENTATION_LANDSCAPE);
	//else
	//	ST7701_SetOrientation(pObj, OTM8009A_ORIENTATION_PORTRAIT);

	ST7701_DisplayOnA(pObj);

	return OTM8009A_OK;
}

int32_t ST7701_DeInit(OTM8009A_Object_t *pObj)
{
  return OTM8009A_ERROR;
}

int32_t ST7701_ReadID(OTM8009A_Object_t *pObj, uint32_t *Id)
{
	int32_t ret;
	if(otm8009a_read_reg(&pObj->Ctx, 0xDA, (uint8_t *)Id, 1)!= OTM8009A_OK)
	//if(otm8009a_read_reg(&pObj->Ctx, 0x04, (uint8_t *)Id, 3)!= OTM8009A_OK)
	{
		ret = OTM8009A_ERROR;
	}
	else
	{
		ret = OTM8009A_OK;
	}

	//otm8009a_read_reg(&pObj->Ctx, 0xDA, (uint8_t *)Id, 1);
	printf("chip id0: %02x\r\n",*Id);
	//otm8009a_read_reg(&pObj->Ctx, 0xDB, (uint8_t *)Id, 1);
	//printf("chip id1: %02x\r\n",*Id);
	//otm8009a_read_reg(&pObj->Ctx, 0xDC, (uint8_t *)Id, 1);
	//printf("chip id2: %02x\r\n",*Id);

	return OTM8009A_OK;//ret;
}

//
// Doesnt' affect brightness!
//
int32_t ST7701_SetBrightness(OTM8009A_Object_t *pObj, uint32_t Brightness)
{
#if 0
  int32_t ret;
  uint8_t brightness = (uint8_t)((Brightness * 255U)/100U);

  /* Send Display on DCS command to display */
  if(otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_WRDISBV, &brightness, 0) != OTM8009A_OK)
  {
    ret = OTM8009A_ERROR;
  }
  else
  {
    OTM8009ACtx.Brightness = Brightness;
    ret = OTM8009A_OK;
  }

  return ret;
#endif
}

/**
  * @brief  Get the display brightness.
  * @param  pObj Component object
  * @param  Brightness   display brightness to be returned
  * @retval Component status
  */
int32_t ST7701_GetBrightness(OTM8009A_Object_t *pObj, uint32_t *Brightness)
{
 // *Brightness = OTM8009ACtx.Brightness;
  return OTM8009A_OK;
}

#if 1
// == Works ==
//
int32_t ST7701_DisplayOnA(OTM8009A_Object_t *pObj)
{
  int32_t ret;
  uint8_t display = 0;

  /* Send Display on DCS command to display */
  if(otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_DISPON, &display, 0) != OTM8009A_OK)
  {
    ret = OTM8009A_ERROR;
  }
  else
  {
    ret = OTM8009A_OK;
  }

  return ret;
}
#endif

// Works
//
int32_t ST7701_DisplayOff(OTM8009A_Object_t *pObj)
{
  int32_t ret;
  uint8_t display = 0;

  /* Send Display on DCS command to display */
  if(otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_DISPOFF, &display, 0) != OTM8009A_OK)
  {
    ret = OTM8009A_ERROR;
  }
  else
  {
    ret = OTM8009A_OK;
  }

  return ret;
}

int32_t ST7701_SetOrientation(OTM8009A_Object_t *pObj, uint32_t Orientation)
{
  int32_t ret;
  uint8_t tmp = OTM8009A_MADCTR_MODE_LANDSCAPE;
  uint8_t tmp1 = OTM8009A_MADCTR_MODE_PORTRAIT;

  if((Orientation != OTM8009A_ORIENTATION_LANDSCAPE) && (Orientation != OTM8009A_ORIENTATION_PORTRAIT))
  {
    ret = OTM8009A_ERROR;
  }/* Send command to configure display orientation mode  */
  else if(Orientation == OTM8009A_ORIENTATION_LANDSCAPE)
  {
    ret = otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_MADCTR, &tmp, 0);
    ret += otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_CASET, LcdRegData27, 4);
    ret += otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_PASET, LcdRegData28, 4);

    OTM8009ACtx.Orientation = OTM8009A_ORIENTATION_LANDSCAPE;
  }
  else
  {
    ret = otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_MADCTR, &tmp1, 0);
    ret += otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_CASET, LcdRegData27, 4);
    ret += otm8009a_write_rega(&pObj->Ctx, OTM8009A_CMD_PASET, LcdRegData28, 4);

    OTM8009ACtx.Orientation = OTM8009A_ORIENTATION_PORTRAIT;
  }

  if(ret != OTM8009A_OK)
  {
    ret = OTM8009A_ERROR;
  }

  return ret;
}

int32_t ST7701_GetOrientation(OTM8009A_Object_t *pObj, uint32_t *Orientation)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  *Orientation = OTM8009ACtx.Orientation;

  return OTM8009A_OK;
}

int32_t ST7701_GetXSize(OTM8009A_Object_t *pObj, uint32_t *Xsize)
{
#if 0
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  switch(OTM8009ACtx.Orientation)
  {
  case OTM8009A_ORIENTATION_PORTRAIT:
    *Xsize = 480;
    break;
  case OTM8009A_ORIENTATION_LANDSCAPE:
    *Xsize = 854;
    break;
  default:
    *Xsize = 854;
    break;
  }
#endif
  return OTM8009A_OK;
}

int32_t ST7701_GetYSize(OTM8009A_Object_t *pObj, uint32_t *Ysize)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);
#if 0
  switch(OTM8009ACtx.Orientation)
  {
  case OTM8009A_ORIENTATION_PORTRAIT:
    *Ysize = 854;
    break;
  case OTM8009A_ORIENTATION_LANDSCAPE:
    *Ysize = 480;
    break;
  default:
    *Ysize = 480;
    break;
  }
#endif
  return OTM8009A_OK;
}

int32_t ST7701_SetCursor(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_DrawBitmap(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_FillRGBRect(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_DrawHLine(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_DrawVLine(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_FillRect(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_GetPixel(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

int32_t ST7701_SetPixel(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  return OTM8009A_ERROR;
}

static int32_t ST7701_ReadRegWrap(void *Handle, uint16_t Reg, uint8_t* pData, uint16_t Length)
{
  OTM8009A_Object_t *pObj = (OTM8009A_Object_t *)Handle;

  return pObj->IO.ReadReg(pObj->IO.Address, Reg, pData, Length);
}

static int32_t ST7701_WriteRegWrap(void *Handle, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  OTM8009A_Object_t *pObj = (OTM8009A_Object_t *)Handle;

  return pObj->IO.WriteReg(pObj->IO.Address, Reg, pData, Length);
}

static int32_t ST7701_IO_Delay(OTM8009A_Object_t *pObj, uint32_t Delay)
{
  uint32_t tickstart;
  tickstart = pObj->IO.GetTick();
  while((pObj->IO.GetTick() - tickstart) < Delay)
  {
  }
  return OTM8009A_OK;
}
#endif
#endif
