/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		hf_app.c                                                       **
**  Description:	HF digital mode ownership and shared working RAM               **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#include "semphr.h"

#include "hf_app.h"

// The arena, in the WSPR_RAM region of the linker script (NOLOAD, no flash
// cost). The section keeps its historic name - it is the HF app arena now
__attribute__((section(".wspr_mem"))) __attribute__ ((aligned (32))) \
static uchar				hf_app_ram[HF_APP_RAM_SIZE];

static SemaphoreHandle_t	hf_app_mutex  = NULL;
static volatile uchar		hf_app_sel    = HF_APP_NONE;
static volatile uchar		hf_app_holder = HF_APP_NONE;

static const char *hf_app_name(uchar id)
{
	switch(id)
	{
		case HF_APP_WSPR:	return "wspr";
		case HF_APP_FT8:	return "ft8";
		default:			return "none";
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : hf_app_init
//* Object              :
//* Context    			: CONTEXT_RESET (before the scheduler starts)
//*----------------------------------------------------------------------------
void hf_app_init(void)
{
	hf_app_mutex  = xSemaphoreCreateMutex();
	hf_app_sel    = HF_APP_NONE;
	hf_app_holder = HF_APP_NONE;
}

//*----------------------------------------------------------------------------
//* Function Name       : hf_app_select
//* Object              : set the HF owner (UI mode switch)
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void hf_app_select(uchar id)
{
	if(hf_app_sel == id)
		return;

	hf_app_sel = id;
	printf("hf: owner %s \r\n", hf_app_name(id));
}

uchar hf_app_selected(void)
{
	return hf_app_sel;
}

uchar hf_app_allowed(uchar id)
{
	return (hf_app_sel == HF_APP_NONE) || (hf_app_sel == id);
}

//*----------------------------------------------------------------------------
//* Function Name       : hf_app_ram_acquire
//* Object              : lock the arena for one app
//* Context    			: any task, release from the same task
//*----------------------------------------------------------------------------
void *hf_app_ram_acquire(uchar id, ulong size, TickType_t wait)
{
	if((hf_app_mutex == NULL) || (size > HF_APP_RAM_SIZE))
	{
		printf("hf: %s wants %u bytes, arena %u \r\n", hf_app_name(id), (uint)size, (uint)HF_APP_RAM_SIZE);
		return NULL;
	}

	if(!hf_app_allowed(id))
		return NULL;

	if(xSemaphoreTake(hf_app_mutex, wait) != pdTRUE)
	{
		printf("hf: %s arena busy (%s) \r\n", hf_app_name(id), hf_app_name(hf_app_holder));
		return NULL;
	}

	// The owner may have changed while we were waiting
	if(!hf_app_allowed(id))
	{
		xSemaphoreGive(hf_app_mutex);
		return NULL;
	}

	hf_app_holder = id;
	return hf_app_ram;
}

//*----------------------------------------------------------------------------
//* Function Name       : hf_app_ram_release
//* Object              :
//* Context    			: the task that acquired it
//*----------------------------------------------------------------------------
void hf_app_ram_release(uchar id)
{
	if(hf_app_holder != id)
		return;

	hf_app_holder = HF_APP_NONE;
	xSemaphoreGive(hf_app_mutex);
}
