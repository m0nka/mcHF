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
**  Licence:               GNU GPLv3                                               **
************************************************************************************/

#ifndef DSP_CORE
#include "mchf_pro_board.h"
#include "main.h"
#else
#include "mchf_board.h"
#endif

#ifdef DSP_CORE
#include "mchf_icc_def.h"

#include "stm32h7xx_hal.h"
#include "stm32h747i_discovery.h"
#endif

#include <stdio.h>
#include <stdarg.h>

// ITM or UART
//--#define USE_ITM_SWO

#ifdef RADIO
// Use SW semaphore for task sharing
#define USE_TASK_SHARING
//
#endif

// Use HW semaphore for core sharing
#define USE_CORE_SHARING

// Maximum string size allowed (in bytes)
#define MAX_STRING_SIZE         300

// Required for proper compilation
struct _reent r = {0, (FILE *) 0, (FILE *) 1, (FILE *) 0};
//struct _reent *_impure_ptr = &r;	// some conflict with lwIP!!!

#ifndef USE_ITM_SWO
UART_HandleTypeDef 	DEBUG_UART_Handle;
#endif
uchar				share_port  = 0;

#ifndef DSP_CORE
#ifdef BOOTLOADER
const char 			cpu_id_str[] = "b:";
#else
const char 			cpu_id_str[] = "r:";
#endif
#else
const char 			cpu_id_str[] = "d:";
#endif

#ifdef RADIO
SemaphoreHandle_t 	dPrintSemaphore = NULL;
#endif

#ifdef USE_ITM_SWO
static void swoInit (uint32_t portMask, uint32_t cpuCoreFreqHz, uint32_t baudrate)
{
	uint32_t SWOPrescaler = (cpuCoreFreqHz / baudrate) - 1u ; // baudrate in Hz, note that cpuCoreFreqHz is expected to match the CPU core clock

	CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; 		// Debug Exception and Monitor Control Register (DEMCR): enable trace in core debug
	DBGMCU->CR	= 0x00000027u ;							// DBGMCU_CR : TRACE_IOEN DBG_STANDBY DBG_STOP 	DBG_SLEEP
	TPI->SPPR	= 0x00000002u ;							// Selected PIN Protocol Register: Select which protocol to use for trace output (2: SWO)
	TPI->ACPR	= SWOPrescaler ;						// Async Clock Prescaler Register: Scale the baud rate of the asynchronous output
	ITM->LAR	= 0xC5ACCE55u ;							// ITM Lock Access Register: C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
	ITM->TCR	= 0x0001000Du ;							// ITM Trace Control Register
	ITM->TPR	= ITM_TPR_PRIVMASK_Msk ;				// ITM Trace Privilege Register: All stimulus ports
	ITM->TER	= portMask ;							// ITM Trace Enable Register: Enabled tracing on stimulus ports. One bit per stimulus port.
	DWT->CTRL	= 0x400003FEu ;							// Data Watchpoint and Trace Register
	TPI->FFCR	= 0x00000100u ;							// Formatter and Flush Control Register

	// ITM/SWO works only if enabled from debugger.
	// If ITM stimulus 0 is not free, don't try to send data to SWO
	if (ITM->PORT [0].u8 == 1)
	{
		//bItmAvailable = 1 ;
	}
}
void SWD_Init(void)
{
	// H7 dual core part
	#if 0
	*(__IO uint32_t*)(0x5C001004) |= 0x00700000; // DBGMCU_CR D3DBGCKEN D1DBGCKEN TRACECLKEN

	// UNLOCK FUNNEL
	*(__IO uint32_t*)(0x5C004FB0) = 0xC5ACCE55; // SWTF_LAR
	*(__IO uint32_t*)(0x5C003FB0) = 0xC5ACCE55; // SWO_LAR

	// SWO current output divisor register
	// This divisor value (0x000000C7) corresponds to 400Mhz
	// To change it, you can use the following rule
	// value = (CPU Freq/sw speed )-1
	*(__IO uint32_t*)(0x5C003010) = ((SystemCoreClock / 2000000) - 1); // SWO_CODR

	// SWO selected pin protocol register
	*(__IO uint32_t*)(0x5C0030F0) = 0x00000002; // SWO_SPPR

	// Enable ITM input of SWO trace funnel
	*(__IO uint32_t*)(0x5C004000) |= 0x00000001; // SWFT_CTRL

	// RCC_AHB4ENR enable GPIOB clock
	*(__IO uint32_t*)(0x580244E0) |= 0x00000002;

    // Configure GPIOB pin 3 as AF
	*(__IO uint32_t*)(0x58020400) = (*(__IO uint32_t*)(0x58020400) & 0xffffff3f) | 0x00000080;

	// Configure GPIOB pin 3 Speed
	*(__IO uint32_t*)(0x58020408) |= 0x00000080;

	// Force AF0 for GPIOB pin 3
	*(__IO uint32_t*)(0x58020420) &= 0xFFFF0FFF;
	#else
	// F4 and L series parts init
	swoInit (1, SystemCoreClock , 2000000);
	#endif
}
#endif

void print_itm_header(void)
{
	#ifndef USE_ITM_SWO
	HAL_UART_Transmit(&DEBUG_UART_Handle, (uint8_t *)cpu_id_str,  sizeof(cpu_id_str) - 1, 0xFFFF);
	#else
	ITM_SendChar('[');
	ITM_SendChar(0x30 + FIRMWARE_VERSION_HIGH);
	ITM_SendChar('.');
	ITM_SendChar(0x30 + FIRMWARE_VERSION_MID_HIGH);
	ITM_SendChar('.');
	ITM_SendChar(0x30 + FIRMWARE_VERSION_MID_LOW);
	ITM_SendChar('.');
	ITM_SendChar(0x30 + FIRMWARE_VERSION_LOW);
	ITM_SendChar(']');
	ITM_SendChar(' ');
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : claim_debug_port
//*						:
//* Object              : dynamic open and close of debug port
//* Notes    			:
//* Notes    			:
//* Notes    			: HAL_UART_MspInit callback in stm32h7xx_hal_msp.c
//* Notes    			:
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
static int claim_debug_port(void)
{
	// Success anyway
	if(share_port == 0)
		return 0;

	#ifdef USE_TASK_SHARING
	// Mutex invalid
	if(dPrintSemaphore == NULL)
		return 1;

	// Make all printf calls thread safe
	if(xSemaphoreTake(dPrintSemaphore, (TickType_t)50) != pdTRUE)
		return 2;
	#endif

	#ifdef USE_CORE_SHARING
	ulong timeout = 0xFFFFFF;
	// Wait to be free
	while(HAL_HSEM_IsSemTaken(HSEM_ID_28) == 1)
	{
		// Not possible to claim it
		if(timeout == 0)
			return 3;

		__asm("nop");
		timeout--;
	}

	// Take shared resource
	HAL_HSEM_Take(HSEM_ID_28,0);
	#endif

	#ifndef USE_ITM_SWO
	HAL_UART_Init(&DEBUG_UART_Handle);
	#endif

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : release_debug_port
//*						:
//* Object              : dynamic open and close of debug port
//* Notes    			:
//* Notes    			:
//* Notes    			: HAL_UART_MspDeInit callback in stm32h7xx_hal_msp.c
//* Notes    			:
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
static void release_debug_port(void)
{
	// Success anyway
	if(share_port == 0)
		return;

	#ifndef USE_ITM_SWO
	// Release HW
	HAL_UART_DeInit(&DEBUG_UART_Handle);
	#endif

	#ifdef USE_CORE_SHARING
	// Release shared resource
	HAL_HSEM_Release(HSEM_ID_28, 0);
	#endif

	#ifdef USE_TASK_SHARING
	// Release OS mutex
	xSemaphoreGive(dPrintSemaphore);
	#endif
}

/**
 * @brief  Transmit a char, if you want to use printf(), 
 *         you need implement this function
 *
 * @param  pStr	Storage string.
 * @param  c    Character to write.
 */
void PrintChar(char c)
{
	#ifndef USE_ITM_SWO
	HAL_UART_Transmit(&DEBUG_UART_Handle, (uint8_t *)&c, 1, 0xFFFF);
	#else
	ITM_SendChar((uint32_t)c);
	#endif
}

/**
 * @brief  Writes a character inside the given string. Returns 1.
 *
 * @param  pStr	Storage string.
 * @param  c    Character to write.
 */
signed int PutChar(char *pStr, char c)
{
    *pStr = c;
    return 1;
}


/**
 * @brief  Writes a string inside the given string.
 *
 * @param  pStr     Storage string.
 * @param  pSource  Source string.
 * @return  The size of the written
 */
signed int PutString(char *pStr, const char *pSource)
{
    signed int num = 0;

    while (*pSource != 0) {

        *pStr++ = *pSource++;
        num++;
    }

    return num;
}


/**
 * @brief  Writes an unsigned int inside the given string, using the provided fill &
 *         width parameters.
 *
 * @param  pStr  Storage string.
 * @param  fill  Fill character.
 * @param  width  Minimum integer width.
 * @param  value  Integer value.   
 */
signed int PutUnsignedInt(
    char *pStr,
    char fill,
    signed int width,
    unsigned int value)
{
    signed int num = 0;

    /* Take current digit into account when calculating width */
    width--;

    /* Recursively write upper digits */
    if ((value / 10) > 0) {

        num = PutUnsignedInt(pStr, fill, width, value / 10);
        pStr += num;
    }
    
    /* Write filler characters */
    else {

        while (width > 0) {

            PutChar(pStr, fill);
            pStr++;
            num++;
            width--;
        }
    }

    /* Write lower digit */
    num += PutChar(pStr, (value % 10) + '0');

    return num;
}


/**
 * @brief  Writes a signed int inside the given string, using the provided fill & width
 *         parameters.
 *
 * @param pStr   Storage string.
 * @param fill   Fill character.
 * @param width  Minimum integer width.
 * @param value  Signed integer value.
 */
signed int PutSignedInt(
    char *pStr,
    char fill,
    signed int width,
    signed int value)
{
    signed int num = 0;
    unsigned int absolute;

    /* Compute absolute value */
    if (value < 0) {

        absolute = -value;
    }
    else {

        absolute = value;
    }

    /* Take current digit into account when calculating width */
    width--;

    /* Recursively write upper digits */
    if ((absolute / 10) > 0) {

        if (value < 0) {
        
            num = PutSignedInt(pStr, fill, width, -(absolute / 10));
        }
        else {

            num = PutSignedInt(pStr, fill, width, absolute / 10);
        }
        pStr += num;
    }
    else {

        /* Reserve space for sign */
        if (value < 0) {

            width--;
        }

        /* Write filler characters */
        while (width > 0) {

            PutChar(pStr, fill);
            pStr++;
            num++;
            width--;
        }

        /* Write sign */
        if (value < 0) {

            num += PutChar(pStr, '-');
            pStr++;
        }
    }

    /* Write lower digit */
    num += PutChar(pStr, (absolute % 10) + '0');

    return num;
}


/**
 * @brief  Writes an hexadecimal value into a string, using the given fill, width &
 *         capital parameters.
 *
 * @param pStr   Storage string.
 * @param fill   Fill character.
 * @param width  Minimum integer width.
 * @param maj    Indicates if the letters must be printed in lower- or upper-case.
 * @param value  Hexadecimal value.
 *
 * @return  The number of char written
 */
signed int PutHexa(
    char *pStr,
    char fill,
    signed int width,
    unsigned char maj,
    unsigned int value)
{
    signed int num = 0;

    /* Decrement width */
    width--;

    /* Recursively output upper digits */
    if ((value >> 4) > 0) {

        num += PutHexa(pStr, fill, width, maj, value >> 4);
        pStr += num;
    }
    /* Write filler chars */
    else {

        while (width > 0) {

            PutChar(pStr, fill);
            pStr++;
            num++;
            width--;
        }
    }

    /* Write current digit */
    if ((value & 0xF) < 10) {

        PutChar(pStr, (value & 0xF) + '0');
    }
    else if (maj) {

        PutChar(pStr, (value & 0xF) - 10 + 'A');
    }
    else {

        PutChar(pStr, (value & 0xF) - 10 + 'a');
    }
    num++;

    return num;
}



/* Global Functions ----------------------------------------------------------- */


/**
 * @brief  Stores the result of a formatted string into another string. Format
 *         arguments are given in a va_list instance.
 *
 * @param pStr    Destination string.
 * @param length  Length of Destination string.
 * @param pFormat Format string.
 * @param ap      Argument list.
 *
 * @return  The number of characters written.
 */
signed int vsnprintf(char *pStr, size_t length, const char *pFormat, va_list ap)
{
    char          fill;
    unsigned char width;
    signed int    num = 0;
    signed int    size = 0;

    /* Clear the string */
    if (pStr) {

        *pStr = 0;
    }

    /* Phase string */
    while (*pFormat != 0 && size < length) {

        /* Normal character */
        if (*pFormat != '%') {

            *pStr++ = *pFormat++;
            size++;
        }
        /* Escaped '%' */
        else if (*(pFormat+1) == '%') {

            *pStr++ = '%';
            pFormat += 2;
            size++;
        }
        /* Token delimiter */
        else {

            fill = ' ';
            width = 0;
            pFormat++;

            /* Parse filler */
            if (*pFormat == '0') {

                fill = '0';
                pFormat++;
            }

            /* Parse width */
            while ((*pFormat >= '0') && (*pFormat <= '9')) {
        
                width = (width*10) + *pFormat-'0';
                pFormat++;
            }

            /* Check if there is enough space */
            if (size + width > length) {

                width = length - size;
            }
        
            /* Parse type */
            switch (*pFormat) {
            case 'd': 
            case 'i': num = PutSignedInt(pStr, fill, width, va_arg(ap, signed int)); break;
            case 'u': num = PutUnsignedInt(pStr, fill, width, va_arg(ap, unsigned int)); break;
            case 'x': num = PutHexa(pStr, fill, width, 0, va_arg(ap, unsigned int)); break;
            case 'X': num = PutHexa(pStr, fill, width, 1, va_arg(ap, unsigned int)); break;
            case 's': num = PutString(pStr, va_arg(ap, char *)); break;
            case 'c': num = PutChar(pStr, va_arg(ap, unsigned int)); break;
            default:
                return EOF;
            }

            pFormat++;
            pStr += num;
            size += num;
        }
    }

    /* NULL-terminated (final \0 is not counted) */
    if (size < length) {

        *pStr = 0;
    }
    else {

        *(--pStr) = 0;
        size--;
    }

    return size;
}


/**
 * @brief  Stores the result of a formatted string into another string. Format
 *         arguments are given in a va_list instance.
 *
 * @param pStr    Destination string.
 * @param length  Length of Destination string.
 * @param pFormat Format string.
 * @param ...     Other arguments
 *
 * @return  The number of characters written.
 */
signed int snprintf(char *pString, size_t length, const char *pFormat, ...)
{
    va_list    ap;
    signed int rc;

    va_start(ap, pFormat);
    rc = vsnprintf(pString, length, pFormat, ap);
    va_end(ap);

    return rc;
}


/**
 * @brief  Stores the result of a formatted string into another string. Format
 *         arguments are given in a va_list instance.
 *
 * @param pString  Destination string.
 * @param length   Length of Destination string.
 * @param pFormat  Format string.
 * @param ap       Argument list.
 *
 * @return  The number of characters written.
 */
signed int vsprintf(char *pString, const char *pFormat, va_list ap)
{
   return vsnprintf(pString, MAX_STRING_SIZE, pFormat, ap);
}

// ToDo: Cube port uses a linker lib that declares this function, check if affects print!!!
//
#if 1
/**
 * @brief  Outputs a formatted string on the given stream. Format arguments are given
 *         in a va_list instance.
 *
 * @param pStream  Output stream.
 * @param pFormat  Format string
 * @param ap       Argument list. 
 */
signed int vfprintf__(FILE *pStream, const char *pFormat, va_list ap)
{
    char pStr[MAX_STRING_SIZE];
    char pError[] = "stdio.c: increase MAX_STRING_SIZE\n\r";

    /* Write formatted string in buffer */
    if (vsprintf(pStr, pFormat, ap) >= MAX_STRING_SIZE) {

        fputs(pError, stderr);
        while (1); /* Increase MAX_STRING_SIZE */
    }

    /* Display string */
    return fputs(pStr, pStream);
}
#endif


/**
 * @brief  Outputs a formatted string on the DBGU stream. Format arguments are given
 *         in a va_list instance.
 *
 * @param pFormat  Format string.
 * @param ap  Argument list.
 */
signed int vprintf(const char *pFormat, va_list ap)
{
    return vfprintf__(stdout, pFormat, ap);
}


#if 1
/**
 * @brief  Outputs a formatted string on the given stream, using a variable 
 *         number of arguments.
 *
 * @param pStream  Output stream.
 * @param pFormat  Format string.
 */
signed int fprintf__(FILE *pStream, const char *pFormat, ...)
{
    va_list ap;
    signed int result;

    /* Forward call to vfprintf */
    va_start(ap, pFormat);
    result = vfprintf__(pStream, pFormat, ap);
    va_end(ap);

    return result;
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : printf
//* Object              :
//* Notes    			: printf here
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
signed int printf(const char *pFormat, ...)
{
    va_list ap;
    signed int result;

    if(claim_debug_port())
    	return 0;

    print_itm_header();

    // Forward call to vprintf
    va_start(ap, pFormat);
    result = vprintf(pFormat, ap);
    va_end(ap);

    release_debug_port();

    return result;
}


/**
 * @brief  Writes a formatted string inside another string.
 *
 * @param pStr     torage string.
 * @param pFormat  Format string.
 */
signed int sprintf(char *pStr, const char *pFormat, ...)
{
    va_list ap;
    signed int result;

    // Forward call to vsprintf
    va_start(ap, pFormat);
    result = vsprintf(pStr, pFormat, ap);
    va_end(ap);

    return result;
}

//*----------------------------------------------------------------------------
//* Function Name       : puts
//* Object              :
//* Notes    			: printf with single str arg lands here!
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
signed int puts(const char *pStr)
{
	signed int result;

    if(claim_debug_port())
    	return 0;

    print_itm_header();

	result = fputs(pStr, stdout);

    release_debug_port();

    return result;
}

/**
 * @brief  Implementation of fputc using the DBGU as the standard output. Required
 *         for printf().
 *
 * @param c        Character to write.
 * @param pStream  Output stream.
 * @param The character written if successful, or -1 if the output stream is
 *        not stdout or stderr.
 */
signed int fputc(signed int c, FILE *pStream)
{
    if ((pStream == stdout) || (pStream == stderr)) {

    	PrintChar(c);

        return c;
    }
    else {

        return EOF;
    }
}


/**
 * @brief  Implementation of fputs using the DBGU as the standard output. Required
 *         for printf().
 *
 * @param pStr     String to write.
 * @param pStream  Output stream.
 *
 * @return  Number of characters written if successful, or -1 if the output
 *          stream is not stdout or stderr.
 */
signed int fputs(const char *pStr, FILE *pStream)
{
    signed int num = 0;

    while (*pStr != 0) {

        if (fputc(*pStr, pStream) == -1) {

            return -1;
        }
        num++;
        pStr++;
    }

    return num;
}

//*----------------------------------------------------------------------------
//* Function Name       : print_hex_array
//* Object              :
//* Notes    			: nicely formatted hex array print
//* Notes    			:
//* Context    			: any task
//*----------------------------------------------------------------------------
void print_hex_array(uchar *pArray, ushort aSize)
{
	ulong i, j, k, num;
	char buf[100];

	num = aSize/16;
	for(k = 0; k < num; k++)
	{
		for(i = 0,j = 0; i < 16; i++)
		{
			j += sprintf( buf+j ,"%02x ", *pArray );
			pArray++;
		}
		printf("%s\r\n",buf);
		aSize -= 16;
	}

	if(aSize%16)
	{
		for(i = 0,j = 0; i < aSize; i++)
		{
			j += sprintf( buf+j ,"%02x ", *pArray );
			pArray++;
		}
		printf("%s\r\n",buf);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : printf_init
//* Object              :
//* Notes    			: init core to core shared resource and debug port params
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void printf_init(uchar is_shared)
{
	#ifndef USE_ITM_SWO
	// Port settings
	#ifdef BOARD_EVAL_747
	DEBUG_UART_Handle.Instance            = USART1;
	#endif
	#ifdef BOARD_MCHF_PRO
	DEBUG_UART_Handle.Instance            = USART2;
	#endif
	DEBUG_UART_Handle.Init.BaudRate       = 115200;
	DEBUG_UART_Handle.Init.WordLength     = UART_WORDLENGTH_8B;
	DEBUG_UART_Handle.Init.StopBits       = UART_STOPBITS_1;
	DEBUG_UART_Handle.Init.Parity         = UART_PARITY_NONE;
	DEBUG_UART_Handle.Init.Mode           = UART_MODE_TX;
	DEBUG_UART_Handle.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
	DEBUG_UART_Handle.Init.OverSampling   = UART_OVERSAMPLING_16;
	#endif

	if(share_port == 0)
	{
		// Open and keep it that way
		#ifndef USE_ITM_SWO
		HAL_UART_Init(&DEBUG_UART_Handle);
		#else
		SWD_Init();
		#endif
	}
	else
	{
		#ifdef USE_TASK_SHARING
		// Local thread safety
		dPrintSemaphore = xSemaphoreCreateMutex();
		#endif

		share_port = 1;
	}
}
