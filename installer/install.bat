@echo off
rem SpreMachineScope installer for Windows
rem   1. writes the firmware of Maincore and Subcore1 to Spresense
rem   2. copies the setting files to the SD card
rem (messages are in English: cmd.exe can't show UTF-8 Japanese on every Windows)
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo === 1. Write the firmware ===
echo Serial ports:
powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames()"
set PORT=
set /p PORT=Port of Spresense, e.g. COM3 (empty: skip): 
if "%PORT%"=="" goto sdcard
echo.
echo Writing the firmware (Maincore, Subcore1): about 1-2 minutes.
echo Do not disconnect the USB cable or press the reset button until it finishes.
echo The progress may not be shown. Please wait for "Done.".
echo.
tools\flash_writer.exe -s -c %PORT% -d -n firmware\Maincore.spk firmware\Subcore1.spk
if errorlevel 1 (
  echo Failed. Check the bootloader, the USB driver and the cable ^(README.txt^).
  goto finish
)
echo Done.

:sdcard
echo.
echo === 2. Prepare the SD card ===
set SD=
set /p SD=Drive letter of the SD card, e.g. E (empty: skip): 
if "%SD%"=="" goto finish
set SD=%SD::=%
if not exist %SD%:\ (
  echo Drive %SD%: is not found.
  goto finish
)
if exist %SD%:\config (
  for /f %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMddHHmmss"') do set TS=%%t
  ren %SD%:\config config_backup_!TS!
  echo The old settings are kept in %SD%:\config_backup_!TS!
)
xcopy /e /i /q config %SD%:\config >nul
if errorlevel 1 (
  echo Failed to copy config.
  goto finish
)
echo Copied config.
rem the DSP for 16 kHz and 192 kHz: keep the one on the SD card
if exist %SD%:\BIN\SRC (
  echo BIN\SRC is already on the SD card.
  goto finish
)
if not exist %SD%:\BIN mkdir %SD%:\BIN
copy /y dsp\SRC %SD%:\BIN\SRC >nul
if errorlevel 1 (
  echo Failed to copy BIN\SRC.
  goto finish
)
echo Copied BIN\SRC.

:finish
echo.
echo Finished.
pause
