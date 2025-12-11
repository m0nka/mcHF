
#ifndef __IS42S32160F_H
#define __IS42S32160F_H

#ifdef USE_IS42S32160F

#include "IS42S32160F_conf.h"

typedef struct 
{
  uint32_t TargetBank;           /*!< Target Bank                             */
  uint32_t RefreshMode;          /*!< Refresh Mode                            */
  uint32_t RefreshRate;          /*!< Refresh Rate                            */
  uint32_t BurstLength;          /*!< Burst Length                            */
  uint32_t BurstType;            /*!< Burst Type                              */
  uint32_t CASLatency;           /*!< CAS Latency                             */
  uint32_t OperationMode;        /*!< Operation Mode                          */
  uint32_t WriteBurstMode;       /*!< Write Burst Mode                        */

} IS42S32160F_Context_t;

#define IS42S32160F_OK                (0)
#define IS42S32160F_ERROR             (-1)

/* Register Mode */
#define IS42S32160F_BURST_LENGTH_1              0x00000000U
#define IS42S32160F_BURST_LENGTH_2              0x00000001U
#define IS42S32160F_BURST_LENGTH_4              0x00000002U
#define IS42S32160F_BURST_LENGTH_8              0x00000004U
#define IS42S32160F_BURST_TYPE_SEQUENTIAL       0x00000000U
#define IS42S32160F_BURST_TYPE_INTERLEAVED      0x00000008U
#define IS42S32160F_CAS_LATENCY_2               0x00000020U
#define IS42S32160F_CAS_LATENCY_3               0x00000030U
#define IS42S32160F_OPERATING_MODE_STANDARD     0x00000000U
#define IS42S32160F_WRITEBURST_MODE_PROGRAMMED  0x00000000U
#define IS42S32160F_WRITEBURST_MODE_SINGLE      0x00000200U

/* Command Mode */
#define IS42S32160F_NORMAL_MODE_CMD             0x00000000U
#define IS42S32160F_CLK_ENABLE_CMD              0x00000001U
#define IS42S32160F_PALL_CMD                    0x00000002U
#define IS42S32160F_AUTOREFRESH_MODE_CMD        0x00000003U
#define IS42S32160F_LOAD_MODE_CMD               0x00000004U
#define IS42S32160F_SELFREFRESH_MODE_CMD        0x00000005U
#define IS42S32160F_POWERDOWN_MODE_CMD          0x00000006U

int32_t IS42S32160F_Init(SDRAM_HandleTypeDef *Ctx, IS42S32160F_Context_t *pRegMode);
int32_t IS42S32160F_ClockEnable(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t IS42S32160F_Precharge(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t IS42S32160F_ModeRegConfig(SDRAM_HandleTypeDef *Ctx, IS42S32160F_Context_t *pRegMode);
int32_t IS42S32160F_TimingConfig(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_TimingTypeDef *pTiming);
int32_t IS42S32160F_RefreshMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface, uint32_t RefreshMode);
int32_t IS42S32160F_RefreshRate(SDRAM_HandleTypeDef *Ctx, uint32_t RefreshCount);
int32_t IS42S32160F_EnterPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t IS42S32160F_ExitPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t IS42S32160F_Sendcmd(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_CommandTypeDef *SdramCmd);

#endif
#endif
