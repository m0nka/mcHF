
#include "MT48LC4M32B2P.h"

#ifdef USE_MT48LC4M32B2P

static FMC_SDRAM_CommandTypeDef Command;

static int32_t MT48LC4M32B2P_Delay(uint32_t Delay);

int32_t MT48LC4M32B2P_Init(SDRAM_HandleTypeDef *Ctx, MT48LC4M32B2P_Context_t *pRegMode)
{
  int32_t ret = MT48LC4M32B2P_ERROR;
  
  /* Step 1: Configure a clock configuration enable command */
  if(MT48LC4M32B2P_ClockEnable(Ctx, pRegMode->TargetBank) == MT48LC4M32B2P_OK)
  {
    /* Step 2: Insert 100 us minimum delay */ 
    /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
    (void)MT48LC4M32B2P_Delay(1);
    
    /* Step 3: Configure a PALL (precharge all) command */ 
    if(MT48LC4M32B2P_Precharge(Ctx, pRegMode->TargetBank) == MT48LC4M32B2P_OK)
    {
      /* Step 4: Configure a Refresh command */ 
      if(MT48LC4M32B2P_RefreshMode(Ctx, pRegMode->TargetBank, pRegMode->RefreshMode) == MT48LC4M32B2P_OK)
      {
        /* Step 5: Program the external memory mode register */
        if(MT48LC4M32B2P_ModeRegConfig(Ctx, pRegMode) == MT48LC4M32B2P_OK)
        {
          /* Step 6: Set the refresh rate counter */
          if(MT48LC4M32B2P_RefreshRate(Ctx, pRegMode->RefreshRate) == MT48LC4M32B2P_OK)
          {
            ret = MT48LC4M32B2P_OK;
          }
        }
      }
    }
  } 
  return ret;
}

int32_t MT48LC4M32B2P_ClockEnable(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = MT48LC4M32B2P_CLK_ENABLE_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_Precharge(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = MT48LC4M32B2P_PALL_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_ModeRegConfig(SDRAM_HandleTypeDef *Ctx, MT48LC4M32B2P_Context_t *pRegMode)
{
  uint32_t tmpmrd;

  /* Program the external memory mode register */
  tmpmrd = (uint32_t)pRegMode->BurstLength   |\
                     pRegMode->BurstType     |\
                     pRegMode->CASLatency    |\
                     pRegMode->OperationMode |\
                     pRegMode->WriteBurstMode;
  
  Command.CommandMode            = MT48LC4M32B2P_LOAD_MODE_CMD;
  Command.CommandTarget          = pRegMode->TargetBank;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = tmpmrd;
  
  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_TimingConfig(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_TimingTypeDef *pTiming)
{
  /* Program the SDRAM timing */
  if(HAL_SDRAM_Init(Ctx, pTiming) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_RefreshMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface, uint32_t RefreshMode)
{
  Command.CommandMode            = RefreshMode;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 8;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_RefreshRate(SDRAM_HandleTypeDef *Ctx, uint32_t RefreshCount)
{
  /* Set the device refresh rate */
  if(HAL_SDRAM_ProgramRefreshRate(Ctx, RefreshCount) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_EnterPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = MT48LC4M32B2P_POWERDOWN_MODE_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_ExitPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = MT48LC4M32B2P_NORMAL_MODE_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

int32_t MT48LC4M32B2P_Sendcmd(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_CommandTypeDef *SdramCmd)
{
  if(HAL_SDRAM_SendCommand(Ctx, SdramCmd, MT48LC4M32B2P_TIMEOUT) != HAL_OK)
  {
    return MT48LC4M32B2P_ERROR;
  }
  else
  {
    return MT48LC4M32B2P_OK;
  }
}

static int32_t MT48LC4M32B2P_Delay(uint32_t Delay)
{  
  uint32_t tickstart;
  tickstart = HAL_GetTick();
  while((HAL_GetTick() - tickstart) < Delay)
  {
  }
  return MT48LC4M32B2P_OK;
}
#endif

