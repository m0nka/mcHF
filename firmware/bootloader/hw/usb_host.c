/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       usb_host.c                                                   **
**  Description:     USB Host MSC driver for bootloader (usbh_conf + init/process) **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"

// -----------------------------------------------------------------------
// Handles
// -----------------------------------------------------------------------
USBH_HandleTypeDef  hUSBHost;
static HCD_HandleTypeDef hhcd;

static volatile uchar usb_state = USB_STATE_IDLE;

// -----------------------------------------------------------------------
// User callback — called by USBH core on state changes
// -----------------------------------------------------------------------
static void usbh_user_callback(USBH_HandleTypeDef *phost, uint8_t id)
{
    switch(id)
    {
        case HOST_USER_SELECT_CONFIGURATION:
            break;

        case HOST_USER_CLASS_ACTIVE:
            printf("usb: device ready\r\n");
            usb_state = USB_STATE_READY;
            break;

        case HOST_USER_CLASS_SELECTED:
            break;

        case HOST_USER_CONNECTION:
            printf("usb: connected\r\n");
            break;

        case HOST_USER_DISCONNECTION:
            printf("usb: disconnected\r\n");
            usb_state = USB_STATE_DISCONNECTED;
            break;

        case HOST_USER_UNRECOVERED_ERROR:
            printf("usb: unrecovered error\r\n");
            usb_state = USB_STATE_ERROR;
            break;

        default:
            break;
    }
}

// -----------------------------------------------------------------------
// OTG_HS IRQ handler — bare-metal, no RTOS
// -----------------------------------------------------------------------
void OTG_HS_IRQHandler(void)
{
    HAL_HCD_IRQHandler(&hhcd);
}

// =====================================================================
//  HAL HCD BSP callbacks — hardware init/deinit
// =====================================================================

// -----------------------------------------------------------------------
// HAL_HCD_MspInit — GPIO + clock + IRQ for OTG_HS on PB14/PB15
//
// mcHF V9 uses the OTG_HS peripheral's internal FS transceiver:
//   PB14 = OTG_HS_DM  (AF12)
//   PB15 = OTG_HS_DP  (AF12)
//
// This is full-speed (12 Mbps) — no external ULPI PHY needed.
// -----------------------------------------------------------------------
void HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd_p)
{
    GPIO_InitTypeDef gpio = {0};

    if(hhcd_p->Instance == USB1_OTG_HS)
    {
        // GPIO clocks already enabled in main.c (A-I)

        // PB14 = OTG_HS_DM, PB15 = OTG_HS_DP
        gpio.Pin       = GPIO_PIN_14 | GPIO_PIN_15;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = GPIO_AF12_OTG1_FS;    // AF12 = 0x0C, embedded FS PHY
        HAL_GPIO_Init(GPIOB, &gpio);

        // Enable USB OTG HS clock
        __HAL_RCC_USB1_OTG_HS_CLK_ENABLE();

        // Disable ULPI clock — we use the internal FS transceiver
        __HAL_RCC_USB1_OTG_HS_ULPI_CLK_DISABLE();

        // Set interrupt priority and enable
        HAL_NVIC_SetPriority(OTG_HS_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    }
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hhcd_p)
{
    if(hhcd_p->Instance == USB1_OTG_HS)
    {
        HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
        __HAL_RCC_USB1_OTG_HS_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14 | GPIO_PIN_15);
    }
}

// =====================================================================
//  HAL HCD callbacks → USB Host Library notifications
// =====================================================================
void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd_p)
{
    USBH_LL_IncTimer(hhcd_p->pData);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd_p)
{
    USBH_LL_Connect(hhcd_p->pData);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd_p)
{
    USBH_LL_Disconnect(hhcd_p->pData);
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd_p)
{
    USBH_LL_PortEnabled(hhcd_p->pData);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd_p)
{
    USBH_LL_PortDisabled(hhcd_p->pData);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd_p,
                                         uint8_t chnum,
                                         HCD_URBStateTypeDef urb_state)
{
    // Used with OS to sync URB state — not needed bare-metal
}

// =====================================================================
//  USBH_LL_* — Low-Level interface (USB Host Library → HCD)
// =====================================================================
USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
    // OTG_HS with internal FS transceiver on PB14/PB15
    hhcd.Instance              = USB1_OTG_HS;
    hhcd.Init.Host_channels    = 12;
    hhcd.Init.dma_enable       = 0;
    hhcd.Init.low_power_enable = 0;
    hhcd.Init.phy_itface       = HCD_PHY_EMBEDDED;
    hhcd.Init.Sof_enable       = 0;
    hhcd.Init.speed            = HCD_SPEED_FULL;
    hhcd.Init.vbus_sensing_enable = 0;
    hhcd.Init.lpm_enable       = 0;

    // Cross-link handles
    hhcd.pData  = phost;
    phost->pData = &hhcd;

    HAL_HCD_Init(&hhcd);

    USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd));

    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
    HAL_HCD_DeInit(phost->pData);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
    HAL_HCD_Start(phost->pData);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
    HAL_HCD_Stop(phost->pData);
    return USBH_OK;
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
    USBH_SpeedTypeDef speed = USBH_SPEED_FULL;

    switch(HAL_HCD_GetCurrentSpeed(phost->pData))
    {
        case 0:  speed = USBH_SPEED_HIGH; break;
        case 1:  speed = USBH_SPEED_FULL; break;
        case 2:  speed = USBH_SPEED_LOW;  break;
        default: speed = USBH_SPEED_FULL; break;
    }
    return speed;
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
    HAL_HCD_ResetPort(phost->pData);
    return USBH_OK;
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost,
                                    uint8_t pipe, uint8_t epnum,
                                    uint8_t dev_address, uint8_t speed,
                                    uint8_t ep_type, uint16_t mps)
{
    HAL_HCD_HC_Init(phost->pData, pipe, epnum, dev_address,
                    speed, ep_type, mps);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    HAL_HCD_HC_Halt(phost->pData, pipe);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost,
                                     uint8_t pipe, uint8_t direction,
                                     uint8_t ep_type, uint8_t token,
                                     uint8_t *pbuff, uint16_t length,
                                     uint8_t do_ping)
{
    HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction,
                             ep_type, token, pbuff, length, do_ping);
    return USBH_OK;
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost,
                                         uint8_t pipe)
{
    return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
    // VBUS is always on from the USB connector — no switch to drive
    HAL_Delay(200);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost,
                                     uint8_t pipe, uint8_t toggle)
{
    if(hhcd.hc[pipe].ep_is_in)
        hhcd.hc[pipe].toggle_in = toggle;
    else
        hhcd.hc[pipe].toggle_out = toggle;

    return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    if(hhcd.hc[pipe].ep_is_in)
        return hhcd.hc[pipe].toggle_in;
    else
        return hhcd.hc[pipe].toggle_out;
}

void USBH_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

// =====================================================================
//  Public API
// =====================================================================

uchar usb_host_init(void)
{
    usb_state = USB_STATE_IDLE;

    printf("usb: init host\r\n");

    if(USBH_Init(&hUSBHost, usbh_user_callback, 0) != USBH_OK)
    {
        printf("usb: init fail\r\n");
        return 1;
    }

    if(USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS) != USBH_OK)
    {
        printf("usb: register MSC fail\r\n");
        return 2;
    }

    if(USBH_Start(&hUSBHost) != USBH_OK)
    {
        printf("usb: start fail\r\n");
        return 3;
    }

    printf("usb: waiting for device\r\n");
    return 0;
}

void usb_host_process(void)
{
    USBH_Process(&hUSBHost);
}

uchar usb_host_get_state(void)
{
    return usb_state;
}

uchar usb_host_wait_ready(ulong timeout_ms)
{
    ulong start = HAL_GetTick();

    while((HAL_GetTick() - start) < timeout_ms)
    {
        USBH_Process(&hUSBHost);

        if(usb_state == USB_STATE_READY)
            return 0;

        if(usb_state == USB_STATE_ERROR)
            return 2;
    }

    printf("usb: timeout\r\n");
    return 1;
}

void usb_host_deinit(void)
{
    USBH_Stop(&hUSBHost);
    USBH_DeInit(&hUSBHost);
    usb_state = USB_STATE_IDLE;

    printf("usb: deinit\r\n");
}
