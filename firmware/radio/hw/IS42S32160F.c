
#include "IS42S32160F.h"

#ifdef USE_IS42S32160F

static FMC_SDRAM_CommandTypeDef Command;

static int32_t IS42S32160F_Delay(uint32_t Delay);

int32_t IS42S32160F_Init(SDRAM_HandleTypeDef *Ctx, IS42S32160F_Context_t *pRegMode)
{
  int32_t ret = IS42S32160F_ERROR;
  
  /* Step 1: Configure a clock configuration enable command */
  if(IS42S32160F_ClockEnable(Ctx, pRegMode->TargetBank) == IS42S32160F_OK)
  {
    /* Step 2: Insert 100 us minimum delay */ 
    /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
    (void)IS42S32160F_Delay(1);
    
    /* Step 3: Configure a PALL (precharge all) command */ 
    if(IS42S32160F_Precharge(Ctx, pRegMode->TargetBank) == IS42S32160F_OK)
    {
      /* Step 4: Configure a Refresh command */ 
      if(IS42S32160F_RefreshMode(Ctx, pRegMode->TargetBank, pRegMode->RefreshMode) == IS42S32160F_OK)
      {
        /* Step 5: Program the external memory mode register */
        if(IS42S32160F_ModeRegConfig(Ctx, pRegMode) == IS42S32160F_OK)
        {
          /* Step 6: Set the refresh rate counter */
          if(IS42S32160F_RefreshRate(Ctx, pRegMode->RefreshRate) == IS42S32160F_OK)
          {
            ret = IS42S32160F_OK;
          }
        }
      }
    }
  } 
  return ret;
}

int32_t IS42S32160F_ClockEnable(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = IS42S32160F_CLK_ENABLE_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_Precharge(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = IS42S32160F_PALL_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_ModeRegConfig(SDRAM_HandleTypeDef *Ctx, IS42S32160F_Context_t *pRegMode)
{
  uint32_t tmpmrd;

  /* Program the external memory mode register */
  tmpmrd = (uint32_t)pRegMode->BurstLength   |\
                     pRegMode->BurstType     |\
                     pRegMode->CASLatency    |\
                     pRegMode->OperationMode |\
                     pRegMode->WriteBurstMode;
  
  Command.CommandMode            = IS42S32160F_LOAD_MODE_CMD;
  Command.CommandTarget          = pRegMode->TargetBank;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = tmpmrd;
  
  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_TimingConfig(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_TimingTypeDef *pTiming)
{
  /* Program the SDRAM timing */
  if(HAL_SDRAM_Init(Ctx, pTiming) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_RefreshMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface, uint32_t RefreshMode)
{
  Command.CommandMode            = RefreshMode;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 8;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_RefreshRate(SDRAM_HandleTypeDef *Ctx, uint32_t RefreshCount)
{
  /* Set the device refresh rate */
  if(HAL_SDRAM_ProgramRefreshRate(Ctx, RefreshCount) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_EnterPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = IS42S32160F_POWERDOWN_MODE_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_ExitPowerMode(SDRAM_HandleTypeDef *Ctx, uint32_t Interface)
{
  Command.CommandMode            = IS42S32160F_NORMAL_MODE_CMD;
  Command.CommandTarget          = Interface;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  if(HAL_SDRAM_SendCommand(Ctx, &Command, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

int32_t IS42S32160F_Sendcmd(SDRAM_HandleTypeDef *Ctx, FMC_SDRAM_CommandTypeDef *SdramCmd)
{
  if(HAL_SDRAM_SendCommand(Ctx, SdramCmd, IS42S32160F_TIMEOUT) != HAL_OK)
  {
    return IS42S32160F_ERROR;
  }
  else
  {
    return IS42S32160F_OK;
  }
}

static int32_t IS42S32160F_Delay(uint32_t Delay)
{  
  uint32_t tickstart;
  tickstart = HAL_GetTick();
  while((HAL_GetTick() - tickstart) < Delay)
  {
  }
  return IS42S32160F_OK;
}
#endif

