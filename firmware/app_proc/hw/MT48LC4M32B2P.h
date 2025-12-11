
#ifndef __MT48LC4M32B2P_H
#define __MT48LC4M32B2P_H

#ifdef USE_MT48LC4M32B2P

#include "MT48LC4M32B2P_conf.h"

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
} MT48LC4M32B2P_Context_t;

#define MT48LC4M32B2P_OK                (0)
#define MT48LC4M32B2P_ERROR             (-1)

/* Register Mode */
#define MT48LC4M32B2P_BURST_LENGTH_1              0x00000000U
#define MT48LC4M32B2P_BURST_LENGTH_2              0x00000001U
#define MT48LC4M32B2P_BURST_LENGTH_4              0x00000002U
#define MT48LC4M32B2P_BURST_LENGTH_8              0x00000004U
#define MT48LC4M32B2P_BURST_TYPE_SEQUENTIAL       0x00000000U
#define MT48LC4M32B2P_BURST_TYPE_INTERLEAVED      0x00000008U
#define MT48LC4M32B2P_CAS_LATENCY_2               0x00000020U
#define MT48LC4M32B2P_CAS_LATENCY_3               0x00000030U
#define MT48LC4M32B2P_OPERATING_MODE_STANDARD     0x00000000U
#define MT48LC4M32B2P_WRITEBURST_MODE_PROGRAMMED  0x00000000U
#define MT48LC4M32B2P_WRITEBURST_MODE_SINGLE      0x00000200U

/* Command Mode */
#define MT48LC4M32B2P_NORMAL_MODE_CMD             0x00000000U
#define MT48LC4M32B2P_CLK_ENABLE_CMD              0x00000001U
#define MT48LC4M32B2P_PALL_CMD                    0x00000002U
#define MT48LC4M32B2P_AUTOREFRESH_MODE_CMD        0x00000003U
#define MT48LC4M32B2P_LOAD_MODE_CMD               0x00000004U
#define MT48LC4M32B2P_SELFREFRESH_MODE_CMD        0x00000005U
#define MT48LC4M32B2P_POWERDOWN_MODE_CMD          0x00000006U

int32_t MT48LC4M32B2P_Init(SDRAM_HandleTypeDef *Ctx, MT48LC4M32B2P_Context_t *pRegMode);
int32_t MT48LC4M32B2P_ClockEnable(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t MT48LC4M32B2P_Precharge(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t MT48LC4M32B2P_ModeRegConfig(SDRAM_HandleTypeDef *Ctx, MT48LC4M32B2P_Context_t *pRegMode);
int32_t MT48LC4M32B2P_TimingConfig(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_TimingTypeDef *pTiming);
int32_t MT48LC4M32B2P_RefreshMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface, uint32_t RefreshMode);
int32_t MT48LC4M32B2P_RefreshRate(SDRAM_HandleTypeDef *Ctx, uint32_t RefreshCount);
int32_t MT48LC4M32B2P_EnterPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t MT48LC4M32B2P_ExitPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface);
int32_t MT48LC4M32B2P_Sendcmd(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_CommandTypeDef *SdramCmd);

#endif
#endif
