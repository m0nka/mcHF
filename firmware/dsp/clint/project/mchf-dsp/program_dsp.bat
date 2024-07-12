:: STM32CubeProgrammer Utility flash script

@ECHO OFF
@setlocal
::COLOR 0B

:: Current Directory
@SET CUR_DIR=%CD%

:: Chip Name
@SET CHIP_NAME=STM32H747I
:: Main Board
@SET MAIN_BOARD=-DISCO
:: Hex filename
@SET BIN_FILE="Debug/mchf-dsp.bin"
@IF NOT EXIST "%BIN_FILE%" @ECHO %BIN_FILE% Does not exist !! && GOTO goError

:: Board ID
@SET BOARD_ID=0

TITLE STM32CubeProgrammer Utility for %CHIP_NAME%%MAIN_BOARD%

@SET STM32_PROGRAMMER_PATH="%ProgramFiles(x86)%\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
@IF NOT EXIST %STM32_PROGRAMMER_PATH% @SET STM32_PROGRAMMER_PATH="%ProgramW6432%\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
@IF NOT EXIST %STM32_PROGRAMMER_PATH% @SET STM32_PROGRAMMER_PATH="%ProgramFiles%\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
@IF NOT EXIST %STM32_PROGRAMMER_PATH% @ECHO STM32CubeProgrammer is not installed !! && GOTO goError
@IF NOT EXIST %STM32_PROGRAMMER_PATH% @ECHO %STM32_PROGRAMMER_PATH% Does not exist !! && GOTO goError

TITLE STM32CubeProgrammer Utility for %CHIP_NAME%%MAIN_BOARD%

:: Add STM32CubeProgrammer to the PATH
@SET PATH=%STM32_PROGRAMMER_PATH%;%PATH%

:StartProg
@ECHO. 
@ECHO ============================================= 
@ECHO Programming %BIN_FILE% on board id %BOARD_ID%
@ECHO =============================================
@ECHO.
STM32_Programmer_CLI.exe -c port=SWD index=%BOARD_ID% reset=HWrst -w %BIN_FILE% 0x081D00000 -HardRst --start 0x08000000
@IF NOT ERRORLEVEL 0 (
  @GOTO goError
)

@GOTO goOut

:goError
@SET RETERROR=%ERRORLEVEL%
@COLOR 0C
@ECHO.
@ECHO Failure Reason Given is %RETERROR%
@PAUSE
@COLOR 07
@EXIT /b %RETERROR%

:goOut
