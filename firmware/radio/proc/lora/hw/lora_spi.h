/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
************************************************************************************/
#ifndef __LORA_SPI_H
#define __LORA_SPI_H

// IRQ handlers from main.c
void lora_spi_rx_callback(void);
void lora_spi_tx_callback(void);
void lora_spi_eot_callback(void);
void lora_spi_err_callback(void);

#endif
