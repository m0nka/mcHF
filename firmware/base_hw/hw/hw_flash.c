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
#include "mchf_pro_board.h"
#include "main.h"

#include "ff_gen_drv.h"
#include "hw_flash.h"

#define MAX_FLASH_WRITE_FAILURE         10

#ifndef EXT_FLASH_SIZE
#define EXT_FLASH_SIZE                  MT25TL01G_FLASH_SIZE
#endif /* EXT_FLASH_SIZE */
#ifndef EXT_FLASH_SECTOR_SIZE
#define EXT_FLASH_SECTOR_SIZE           MT25TL01G_SECTOR_SIZE
#endif /* EXT_FLASH_SECTOR_SIZE */
#ifndef EXT_FLASH_SUBSECTOR_SIZE
#define EXT_FLASH_SUBSECTOR_SIZE        (2 * MT25TL01G_SUBSECTOR_SIZE)
#endif /* EXT_FLASH_SUBSECTOR_SIZE */
#ifndef EXT_FLASH_PAGE_SIZE
#define EXT_FLASH_PAGE_SIZE             MT25TL01G_PAGE_SIZE
#endif /* EXT_FLASH_PAGE_SIZE */

#define FLASH_BURST_WIDTH               256 /* in bits */

#define FLASH_TIMEOUT_VALUE             50000U /* 50 s */

#define FLASH_CHUNK						(1024*16)

//#define __RAM_CODE_SECTION __attribute__ ((section ("flasher_code_section")))
#define __RAM_CODE_SECTION __RAM_FUNC

__RAM_CODE_SECTION static inline uint32_t hw_flash_get_page(uint32_t Addr)
{
  if IS_FLASH_PROGRAM_ADDRESS_BANK1(Addr)
  {
    /* Bank 1 */
    return (Addr - FLASH_BANK1_BASE) / FLASH_SECTOR_SIZE;
  }
  else
  {
    /* Bank 2 */
    return (Addr - FLASH_BANK2_BASE) / FLASH_SECTOR_SIZE;
  }
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
__RAM_CODE_SECTION static inline uint32_t hw_flash_get_bank(uint32_t Addr)
{
  uint32_t bank = 0;

  if (READ_BIT(SYSCFG->UR0, SYSCFG_UR0_BKS) == 0)
  {
  	/* No Bank swap */
    if IS_FLASH_PROGRAM_ADDRESS_BANK1(Addr)
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
  	/* Bank swap */
    if IS_FLASH_PROGRAM_ADDRESS_BANK1(Addr)
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }

  return bank;
}

__RAM_CODE_SECTION static int hw_flash_verify_data(const uint64_t *pData, const uint64_t *pFlash, uint32_t DataSize)
{
  uint32_t i;
  for(i = 0; i < DataSize; i+=8)
  {
    if (pData[i] != pFlash[i] )
      return 1;
  }

  return 0;
}

__RAM_CODE_SECTION static void hw_flash_mass_erase(uint32_t VoltageRange, uint32_t Banks)
{
  /* Check the parameters */
#if defined (FLASH_CR_PSIZE)
  assert_param(IS_VOLTAGERANGE(VoltageRange));
#else
  UNUSED(VoltageRange);
#endif /* FLASH_CR_PSIZE */
  assert_param(IS_FLASH_BANK(Banks));

  /* Flash Mass Erase */
  if((Banks & FLASH_BANK_BOTH) == FLASH_BANK_BOTH)
  {
#if defined (FLASH_CR_PSIZE)
    /* Reset Program/erase VoltageRange for Bank1 and Bank2 */
    FLASH->CR1 &= (~FLASH_CR_PSIZE);
    FLASH->CR2 &= (~FLASH_CR_PSIZE);

    /* Set voltage range */
    FLASH->CR1 |= VoltageRange;
    FLASH->CR2 |= VoltageRange;
#endif /* FLASH_CR_PSIZE */

    /* Set Mass Erase Bit */
    FLASH->OPTCR |= FLASH_OPTCR_MER;
  }
  else
  {
    /* Proceed to erase Flash Bank  */
    if((Banks & FLASH_BANK_1) == FLASH_BANK_1)
    {
#if defined (FLASH_CR_PSIZE)
      /* Set Program/erase VoltageRange for Bank1 */
      FLASH->CR1 &= (~FLASH_CR_PSIZE);
      FLASH->CR1 |=  VoltageRange;
#endif /* FLASH_CR_PSIZE */

      /* Erase Bank1 */
      FLASH->CR1 |= (FLASH_CR_BER | FLASH_CR_START);
    }
    if((Banks & FLASH_BANK_2) == FLASH_BANK_2)
    {
#if defined (FLASH_CR_PSIZE)
      /* Set Program/erase VoltageRange for Bank2 */
      FLASH->CR2 &= (~FLASH_CR_PSIZE);
      FLASH->CR2 |= VoltageRange;
#endif /* FLASH_CR_PSIZE */

      /* Erase Bank2 */
      FLASH->CR2 |= (FLASH_CR_BER | FLASH_CR_START);
    }
  }
}

__RAM_CODE_SECTION static HAL_StatusTypeDef hw_flash_wait_for_last_operation(uint32_t Timeout, uint32_t Bank)
{
  /* Wait for the FLASH operation to complete by polling on QW flag to be reset.
     Even if the FLASH operation fails, the QW flag will be reset and an error
     flag will be set */

  uint32_t bsyflag, errorflag;
  uint32_t tickstart = HAL_GetTick();

  assert_param(IS_FLASH_BANK_EXCLUSIVE(Bank));

  /* Select bsyflag depending on Bank */
  if(Bank == FLASH_BANK_1)
  {
    bsyflag = FLASH_FLAG_QW_BANK1;
  }
  else
  {
    bsyflag = FLASH_FLAG_QW_BANK2;
  }

  while(__HAL_FLASH_GET_FLAG(bsyflag))
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U))
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Get Error Flags */
  if (Bank == FLASH_BANK_1)
  {
    errorflag = FLASH->SR1 & FLASH_FLAG_ALL_ERRORS_BANK1;
  }
  else
  {
    errorflag = (FLASH->SR2 & FLASH_FLAG_ALL_ERRORS_BANK2) | 0x80000000U;
  }

  /* In case of error reported in Flash SR1 or SR2 registers (ECCC not managed as an error) */
  if((errorflag & 0x7DFFFFFFU) != 0U)
  {
    /*Save the error code*/
    pFlash.ErrorCode |= errorflag;

    /* Clear error programming flags */
    __HAL_FLASH_CLEAR_FLAG(errorflag);

    return HAL_ERROR;
  }

  /* Check FLASH End of Operation flag  */
  if(Bank == FLASH_BANK_1)
  {
    if (__HAL_FLASH_GET_FLAG_BANK1(FLASH_FLAG_EOP_BANK1))
    {
      /* Clear FLASH End of Operation pending bit */
      __HAL_FLASH_CLEAR_FLAG_BANK1(FLASH_FLAG_EOP_BANK1);
    }
  }
  else
  {
    if (__HAL_FLASH_GET_FLAG_BANK2(FLASH_FLAG_EOP_BANK2))
    {
      /* Clear FLASH End of Operation pending bit */
      __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_EOP_BANK2);
    }
  }

  return HAL_OK;
}

__RAM_CODE_SECTION static void hw_flash_erase_sector(uint32_t Sector, uint32_t Banks, uint32_t VoltageRange)
{
  assert_param(IS_FLASH_SECTOR(Sector));
  assert_param(IS_FLASH_BANK_EXCLUSIVE(Banks));
#if defined (FLASH_CR_PSIZE)
  assert_param(IS_VOLTAGERANGE(VoltageRange));
#else
  UNUSED(VoltageRange);
#endif /* FLASH_CR_PSIZE */

  if((Banks & FLASH_BANK_1) == FLASH_BANK_1)
  {
#if defined (FLASH_CR_PSIZE)
    /* Reset Program/erase VoltageRange and Sector Number for Bank1 */
    FLASH->CR1 &= ~(FLASH_CR_PSIZE | FLASH_CR_SNB);

    FLASH->CR1 |= (FLASH_CR_SER | VoltageRange | (Sector << FLASH_CR_SNB_Pos) | FLASH_CR_START);
#else
    /* Reset Sector Number for Bank1 */
    FLASH->CR1 &= ~(FLASH_CR_SNB);

    FLASH->CR1 |= (FLASH_CR_SER | (Sector << FLASH_CR_SNB_Pos) | FLASH_CR_START);
#endif /* FLASH_CR_PSIZE */
  }

  if((Banks & FLASH_BANK_2) == FLASH_BANK_2)
  {
#if defined (FLASH_CR_PSIZE)
    /* Reset Program/erase VoltageRange and Sector Number for Bank2 */
    FLASH->CR2 &= ~(FLASH_CR_PSIZE | FLASH_CR_SNB);

    FLASH->CR2 |= (FLASH_CR_SER | VoltageRange  | (Sector << FLASH_CR_SNB_Pos) | FLASH_CR_START);
#else
    /* Reset Sector Number for Bank2 */
    FLASH->CR2 &= ~(FLASH_CR_SNB);

    FLASH->CR2 |= (FLASH_CR_SER | (Sector << FLASH_CR_SNB_Pos) | FLASH_CR_START);
#endif /* FLASH_CR_PSIZE */
  }
}

__RAM_CODE_SECTION static HAL_StatusTypeDef hw_flash_erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t sector_index;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));
  assert_param(IS_FLASH_BANK(pEraseInit->Banks));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Wait for last operation to be completed on Bank1 */
  if((pEraseInit->Banks & FLASH_BANK_1) == FLASH_BANK_1)
  {
    if(hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, FLASH_BANK_1) != HAL_OK)
    {
      status = HAL_ERROR;
    }
  }

  /* Wait for last operation to be completed on Bank2 */
  if((pEraseInit->Banks & FLASH_BANK_2) == FLASH_BANK_2)
  {
    if(hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, FLASH_BANK_2) != HAL_OK)
    {
      status = HAL_ERROR;
    }
  }

  if(status == HAL_OK)
  {
    if(pEraseInit->TypeErase == FLASH_TYPEERASE_MASSERASE)
    {
      /* Mass erase to be done */
      hw_flash_mass_erase(pEraseInit->VoltageRange, pEraseInit->Banks);

      /* Wait for last operation to be completed */
      if((pEraseInit->Banks & FLASH_BANK_1) == FLASH_BANK_1)
      {
        if(hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, FLASH_BANK_1) != HAL_OK)
        {
          status = HAL_ERROR;
        }
        /* if the erase operation is completed, disable the Bank1 BER Bit */
        FLASH->CR1 &= (~FLASH_CR_BER);
      }
      if((pEraseInit->Banks & FLASH_BANK_2) == FLASH_BANK_2)
      {
        if(hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, FLASH_BANK_2) != HAL_OK)
        {
          status = HAL_ERROR;
        }
        /* if the erase operation is completed, disable the Bank2 BER Bit */
        FLASH->CR2 &= (~FLASH_CR_BER);
      }
    }
    else
    {
      /*Initialization of SectorError variable*/
      *SectorError = 0xFFFFFFFFU;

      /* Erase by sector by sector to be done*/
      for(sector_index = pEraseInit->Sector; sector_index < (pEraseInit->NbSectors + pEraseInit->Sector); sector_index++)
      {
    	  hw_flash_erase_sector(sector_index, pEraseInit->Banks, pEraseInit->VoltageRange);

        if((pEraseInit->Banks & FLASH_BANK_1) == FLASH_BANK_1)
        {
          /* Wait for last operation to be completed */
          status = hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, FLASH_BANK_1);

          /* If the erase operation is completed, disable the SER Bit */
          FLASH->CR1 &= (~(FLASH_CR_SER | FLASH_CR_SNB));
        }
        if((pEraseInit->Banks & FLASH_BANK_2) == FLASH_BANK_2)
        {
          /* Wait for last operation to be completed */
          status = hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, FLASH_BANK_2);

          /* If the erase operation is completed, disable the SER Bit */
          FLASH->CR2 &= (~(FLASH_CR_SER | FLASH_CR_SNB));
        }

        if(status != HAL_OK)
        {
          /* In case of error, stop erase procedure and return the faulty sector */
          *SectorError = sector_index;
          break;
        }
      }
    }
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

__RAM_CODE_SECTION static HAL_StatusTypeDef hw_flash_program(uint32_t TypeProgram, uint32_t FlashAddress, uint32_t DataAddress)
{
  HAL_StatusTypeDef status;
  __IO uint32_t *dest_addr = (__IO uint32_t *)FlashAddress;
  __IO uint32_t *src_addr = (__IO uint32_t*)DataAddress;
  uint32_t bank;
  uint8_t row_index = FLASH_NB_32BITWORD_IN_FLASHWORD;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
  assert_param(IS_FLASH_PROGRAM_ADDRESS(FlashAddress));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

#if defined (FLASH_OPTCR_PG_OTP)
  if((IS_FLASH_PROGRAM_ADDRESS_BANK1(FlashAddress)) || (IS_FLASH_PROGRAM_ADDRESS_OTP(FlashAddress)))
#else
  if(IS_FLASH_PROGRAM_ADDRESS_BANK1(FlashAddress))
#endif /* FLASH_OPTCR_PG_OTP */
  {
    bank = FLASH_BANK_1;
  }
  else
  {
    bank = FLASH_BANK_2;
  }

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Wait for last operation to be completed */
  status = hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, bank);

  if(status == HAL_OK)
  {
    if(bank == FLASH_BANK_1)
    {
#if defined (FLASH_OPTCR_PG_OTP)
      if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
      {
        /* Set OTP_PG bit */
        SET_BIT(FLASH->OPTCR, FLASH_OPTCR_PG_OTP);
      }
      else
#endif /* FLASH_OPTCR_PG_OTP */
      {
        /* Set PG bit */
        SET_BIT(FLASH->CR1, FLASH_CR_PG);
      }
    }
    else
    {
      /* Set PG bit */
      SET_BIT(FLASH->CR2, FLASH_CR_PG);
    }

    __ISB();
    __DSB();

#if defined (FLASH_OPTCR_PG_OTP)
    if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
    {
      /* Program an OTP word (16 bits) */
      *(__IO uint16_t *)FlashAddress = *(__IO uint16_t*)DataAddress;
    }
    else
#endif /* FLASH_OPTCR_PG_OTP */
    {
      /* Program the flash word */
      do
      {
        *dest_addr = *src_addr;
        dest_addr++;
        src_addr++;
        row_index--;
     } while (row_index != 0U);
    }

    __ISB();
    __DSB();

    /* Wait for last operation to be completed */
    status = hw_flash_wait_for_last_operation((uint32_t)FLASH_TIMEOUT_VALUE, bank);

#if defined (FLASH_OPTCR_PG_OTP)
    if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
    {
      /* If the program operation is completed, disable the OTP_PG */
      CLEAR_BIT(FLASH->OPTCR, FLASH_OPTCR_PG_OTP);
    }
    else
#endif /* FLASH_OPTCR_PG_OTP */
    {
      if(bank == FLASH_BANK_1)
      {
        /* If the program operation is completed, disable the PG */
        CLEAR_BIT(FLASH->CR1, FLASH_CR_PG);
      }
      else
      {
        /* If the program operation is completed, disable the PG */
        CLEAR_BIT(FLASH->CR2, FLASH_CR_PG);
      }
    }
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

__RAM_CODE_SECTION static HAL_StatusTypeDef hw_flash_unlock(void)
{
  if(READ_BIT(FLASH->CR1, FLASH_CR_LOCK) != 0U)
  {
    /* Authorize the FLASH Bank1 Registers access */
    WRITE_REG(FLASH->KEYR1, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR1, FLASH_KEY2);

    /* Verify Flash Bank1 is unlocked */
    if (READ_BIT(FLASH->CR1, FLASH_CR_LOCK) != 0U)
    {
      return HAL_ERROR;
    }
  }

  if(READ_BIT(FLASH->CR2, FLASH_CR_LOCK) != 0U)
  {
    /* Authorize the FLASH Bank2 Registers access */
    WRITE_REG(FLASH->KEYR2, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR2, FLASH_KEY2);

    /* Verify Flash Bank2 is unlocked */
    if (READ_BIT(FLASH->CR2, FLASH_CR_LOCK) != 0U)
    {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

/**
  * @brief  Copy program code file into the Internal Flash memory
  * @param  hItem    : Progress bar used to indicate the transfer progression
  * @param  pResFile : Program code file to be copied in the Internal Flash memory
  * @param  Address  : Address where the code will be copied
  * @retval ErrorCode : 0 if success -1 otherwise
  */
__RAM_CODE_SECTION int hw_flash_program_file(FIL * pResFile, uint32_t Address)
{
  FLASH_EraseInitTypeDef EraseInitStruct;

  int Ret = 0;
  uint8_t dual_core = 0;
  uint32_t EraseError = 0;
  uint32_t numOfReadBytes = 0, nbTotalBytes = 0;
  uint32_t offset = 0;
  uint8_t *pSdData;
  uint64_t *pData256, *pFlash256;
  int sector = 0;

  //printf("hw_flash_program_file\r\n");

  pSdData = (uint8_t *)ff_malloc(FLASH_CHUNK);
  if (pSdData == NULL)
  {
    return -1;
  }

  if (f_lseek(pResFile, 0) != FR_OK)
  {
    ff_free(pSdData);
    return -2;
  }

  /* Get the 1st page to erase */
  const uint32_t StartSector = hw_flash_get_page(Address);
  /* Get the number of pages to erase from 1st page */
  const uint32_t NbOfSectors = hw_flash_get_page(Address + f_size(pResFile) -1) - StartSector + 1;
  /* Get the bank */
  const uint32_t BankNumber = hw_flash_get_bank(Address);

  /* Clear pending flags (if any) */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK1);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK2);

  /* Unlock the Flash to enable the flash control register access *************/
  hw_flash_unlock();

  /* Clear all error flags */
  if(BankNumber == FLASH_BANK_1)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
  else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

  //printf("erase\r\n");

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Banks         = BankNumber;
  EraseInitStruct.Sector        = StartSector;
  EraseInitStruct.NbSectors     = NbOfSectors;

  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
  if (hw_flash_erase(&EraseInitStruct, &EraseError) != HAL_OK)
  {
    Ret = HAL_FLASH_GetError();
    goto unlock_and_exit;
  }

  //PROGBAR_SetMinMax(hItem, 0, NbOfSectors);
  pFlash256 = (uint64_t *)Address;

  //printf("flash\r\n");

  do
  {
    memset(pSdData, 0xFF, FLASH_CHUNK);

    Ret = f_read(pResFile, pSdData, FLASH_CHUNK, (void *)&numOfReadBytes);
    if(Ret == FR_OK)
    {
      uint8_t failure = 0;
      offset          = 0;
      pData256       = (uint64_t *)pSdData;

      /* Disable, Clean and Invalidate D-Cache */
      SCB_DisableDCache();
      SCB_CleanInvalidateDCache();

      /* Program the user Flash area word by word */
      while (offset < numOfReadBytes)
      {
        if (hw_flash_program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t )pFlash256, (uint32_t )pData256) == HAL_OK)
        {
        	//printf("verify\r\n");

          if( hw_flash_verify_data(pData256, pFlash256, (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)))) )
          {
            Ret = -1;

            // Enable D-Cache
            SCB_EnableDCache();
            goto unlock_and_exit;
          }

          pData256  += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
          pFlash256 += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
          offset    += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint8_t)));
          failure    = 0;
        }
       else
        {
          if (failure++ > MAX_FLASH_WRITE_FAILURE)
          {
            Ret = HAL_FLASH_GetError();

            // Enable D-Cache *
            SCB_EnableDCache();
            goto unlock_and_exit;
          }
        }
      }

      /* Enable D-Cache */
      SCB_EnableDCache();

      nbTotalBytes += numOfReadBytes;
    }
    else
    {
      goto unlock_and_exit;
    }

    printf("curr %d\r\n", ++sector);
    //PROGBAR_SetValue(hItem, ++sector);

  } while(numOfReadBytes == FLASH_CHUNK);

unlock_and_exit:

  if(BankNumber == FLASH_BANK_1)
    HAL_FLASHEx_Unlock_Bank1();
  else
    HAL_FLASHEx_Unlock_Bank2();

  ff_free(pSdData);

  return Ret;
}

#if 0
__RAM_CODE_SECTION static int __update_flash(FLASH_EraseInitTypeDef *EraseInitStruct, const uint64_t *pFlash256, const uint64_t *pData256, uint32_t Size)
{
  int       Ret = 0;
  uint32_t  EraseError = 0;
  uint32_t  offset = 0;
  uint8_t   failure = 0;

  /* Clear pending flags (if any) */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK1);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK2);

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Clear all error flags */
  if(EraseInitStruct->Banks == FLASH_BANK_1)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
  else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

  __disable_irq();

  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
  if (HAL_FLASHEx_Erase(EraseInitStruct, &EraseError) != HAL_OK)
  {
    Ret = HAL_FLASH_GetError();
    goto unlock_and_exit;
  }

  /* Program the user Flash area word by word */
  offset    = 0;
  while (offset < FLASH_SECTOR_SIZE)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t )pFlash256, (uint32_t )pData256) == HAL_OK)
    {
      if( hw_flash_verify_data(pData256, pFlash256, (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)))) )
      {
        Ret = -1;
        goto unlock_and_exit;
      }
      pData256  += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
      pFlash256 += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
      offset    += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint8_t)));
      failure    = 0;
    }
    else
    {
      if (failure++ > MAX_FLASH_WRITE_FAILURE)
      {
        Ret = HAL_FLASH_GetError();
        goto unlock_and_exit;
      }
    }
  }

unlock_and_exit:
  if(EraseInitStruct->Banks == FLASH_BANK_1)
    HAL_FLASHEx_Unlock_Bank1();
  else
    HAL_FLASHEx_Unlock_Bank2();

  __enable_irq();
  return Ret;
}

/**
  * @brief  Copy program code file into the Internal Flash memory
  * @param  Address   : Address where the code will be copied
  * @param  pData     : Program code file to be copied in the Internal Flash memory
  * @param  Size      : Size of the data to be programmed
  * @retval ErrorCode : 0 if success -1 otherwise
  */
int BSP_FlashUpdate(uint32_t Address, uint8_t *pData, uint32_t Size)
{
  int             Ret = 0;
  uint8_t         dual_core = 0;
  const uint64_t *pFlash256;
  uint64_t *pData256;
  uint64_t *pDst256;
  uint64_t *SectorData = NULL;
  uint32_t  offset = 0;
  FLASH_EraseInitTypeDef EraseInitStruct;

  if (pData == NULL)
  {
    return -1;
  }
#if 0
  SectorData = (uint64_t *)ff_malloc(FLASH_SECTOR_SIZE);
  if (SectorData == NULL)
  {
    return -2;
  }
  memset(SectorData, 0xFF, FLASH_SECTOR_SIZE);
#endif

  /* Get the 1st page to erase */
  const uint32_t StartSector = hw_flash_get_page(Address);
  /* Get the number of pages to erase from 1st page */
  const uint32_t NbOfSectors = hw_flash_get_page(Address + Size - 1) - StartSector + 1;
  /* Get the bank */
  const uint32_t BankNumber = hw_flash_get_bank(Address);

  pFlash256 = (uint64_t *)(FLASH_BASE + (StartSector * FLASH_SECTOR_SIZE));
  /* Read current data */
  for(offset = 0; offset < (FLASH_SECTOR_SIZE / 8); offset++ )
  {
    SectorData[offset] = pFlash256[offset];
  }

  pData256  = (uint64_t *)pData;
  pDst256   = (uint64_t *)((uint32_t )SectorData + (uint32_t )(Address & (FLASH_SECTOR_SIZE -1)));
  /* Update DC */
  for(offset = 0; offset < (Size/8); offset++ )
  {
    *pDst256++ = *pData256++;
  }

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Banks         = BankNumber;
  EraseInitStruct.Sector        = StartSector;
  EraseInitStruct.NbSectors     = NbOfSectors;

  /* Disable, Clean and Invalidate D-Cache */
  SCB_DisableDCache();
  SCB_CleanInvalidateDCache();

  Ret = __update_flash(&EraseInitStruct, pFlash256, SectorData, Size );

  /* Enable D-Cache */
  SCB_EnableDCache();

  if(dual_core)
  {
    BSP_ResumeCPU2();
  }

//  ff_free(SectorData);
  return Ret;
}
#endif

#if 0
/**
  * @brief  Copy resource file into the OctoSPI memory
  * @param  hItem    : Progress bar used to indicate the transfer progression
  * @param  pResFile : Resource file to be copied in the OctoSPI memory
  * @param  Address  : Address where the resource will be copied
  * @retval ErrorCode : 0 if success -1 otherwise
  */
int BSP_ResourcesCopy(WM_HWIN hItem, FIL * pResFile, uint32_t Address)
{
  int RetErr = 0;
  BSP_QSPI_Info_t QSPIInfo;
  uint32_t file_size = 0;
  uint32_t numOfReadBytes = 0, FlashAddr = 0, nbTotalBytes = 0;
  int32_t ospiStatus = BSP_ERROR_NONE;
  uint8_t *pSdData = 0;
  int sector = 0, nb_sectors = 0;

  /* Sop Memory Mapping mode */
  BSP_QSPI_DeInit(0);

  /* Re-initialize NOR OctoSPI flash to exit memory-mapped mode */
  BSP_QSPI_Init_t init ;
  init.InterfaceMode=MT25TL01G_QPI_MODE;
  init.TransferRate= MT25TL01G_DTR_TRANSFER ;
  init.DualFlashMode= MT25TL01G_DUALFLASH_ENABLE;
  if (BSP_QSPI_Init(0,&init) != BSP_ERROR_NONE)
  {
    /* Initialization Error */
    return -1;
  }

  /* Initialize the structure */
  QSPIInfo.FlashSize          = (uint32_t)0x00;
  QSPIInfo.EraseSectorSize    = (uint32_t)0x00;
  QSPIInfo.EraseSectorsNumber = (uint32_t)0x00;
  QSPIInfo.ProgPageSize       = (uint32_t)0x00;
  QSPIInfo.ProgPagesNumber    = (uint32_t)0x00;

  ospiStatus = BSP_QSPI_GetInfo(0,&QSPIInfo);
  if (ospiStatus != BSP_ERROR_NONE)
  {
    RetErr = -1;
    goto exit;
  }

  /* Test the correctness */
  if((QSPIInfo.FlashSize          != EXT_FLASH_SIZE) ||
     (QSPIInfo.EraseSectorSize    != EXT_FLASH_SUBSECTOR_SIZE) ||
     (QSPIInfo.EraseSectorsNumber != (EXT_FLASH_SIZE/EXT_FLASH_SUBSECTOR_SIZE)) ||
     (QSPIInfo.ProgPageSize       != EXT_FLASH_PAGE_SIZE)  ||
     (QSPIInfo.ProgPagesNumber    != (EXT_FLASH_SIZE/EXT_FLASH_PAGE_SIZE)))
  {
    RetErr = -1;
    goto exit;
  }

   pSdData = ff_malloc(QSPIInfo.EraseSectorSize);
   if (pSdData == 0)
   {
     RetErr = -2;
     goto exit;
   }

  if (f_lseek(pResFile, 0) != FR_OK)
  {
    RetErr = -3;
    goto exit;
  }

  file_size = f_size(pResFile);

  nb_sectors = (file_size / QSPIInfo.EraseSectorSize);
  if(file_size % QSPIInfo.EraseSectorSize)
    nb_sectors++;

  PROGBAR_SetMinMax(hItem, 0, nb_sectors);

  FlashAddr = (Address - QSPI_BASE_ADDRESS);
  do
  {
    memset(pSdData, 0xFF, QSPIInfo.EraseSectorSize);

    /* Read and Program data */
    if(f_read(pResFile, pSdData, QSPIInfo.EraseSectorSize, (void *)&numOfReadBytes) != FR_OK)
    {
      RetErr = -4;
      goto exit;
    }

    ospiStatus = BSP_QSPI_EraseBlock(0,FlashAddr,MT25TL01G_ERASE_4K);
    if (ospiStatus != BSP_ERROR_NONE)
    {
      RetErr = -1;
      goto exit;
    }

    ospiStatus = BSP_QSPI_Write(0,(uint8_t *)pSdData, FlashAddr, numOfReadBytes);
    if (ospiStatus != BSP_ERROR_NONE)
    {
      RetErr = -1;
      goto exit;
    }

    /* Wait the end of write operation */
    do
    {
      ospiStatus = BSP_QSPI_GetStatus(0);
    } while (ospiStatus == BSP_ERROR_BUSY);

    /* Check the write operation correctness */
    if (ospiStatus != BSP_ERROR_NONE)
    {
      RetErr = -1;
      goto exit;
    }

    FlashAddr     += numOfReadBytes;
    nbTotalBytes  += numOfReadBytes;

    PROGBAR_SetValue(hItem, ++sector);
    GUI_Exec();

  } while((numOfReadBytes == QSPIInfo.EraseSectorSize) && (nbTotalBytes < file_size));

exit:
  if(pSdData)
  {
    ff_free(pSdData);
  }

  /* Reconfigure memory mapped mode */
  if(BSP_QSPI_EnableMemoryMappedMode(0) != BSP_ERROR_NONE)
  {
    RetErr = -1;
  }

  if(RetErr)
    printf("\nFailed : Error=%d, ospiStatus=%d !!\n", RetErr, (int)ospiStatus);

  GUI_Exec();

  return RetErr;
}
#endif
