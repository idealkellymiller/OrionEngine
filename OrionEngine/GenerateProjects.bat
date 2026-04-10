@echo off
if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug

PAUSE

@REM echo CMake is not installed or not found in PATH.
@REM CHOICE /C YN /M "Would you like to install CMake on this device"

@REM IF %ERRORLEVEL%==2 (
@REM     echo You must have CMake installed to configure this project.
@REM     GOTO end
@REM )
@REM IF %ERRORLEVEL%==1 GOTO yes

@REM :yes
@REM echo Downloading CMake...
@REM GOTO end

@REM :: checks if cmake is on PATH
@REM call where /q cmake
@REM IF %ERRORLEVEL% EQU 1 (
@REM     echo CMake is installed and found on PATH.
@REM     call cmake ..
@REM     GOTO end
@REM ) ELSE (
@REM     echo CMake is not installed or not found in PATH.
@REM     CHOICE /C YN /N /M "Would you like to install CMake on this device? [Y/N]"
@REM     echo %ERRORLEVEL%

@REM     IF %ERRORLEVEL%==2 (
@REM         echo You must have CMake installed to configure this project.
@REM         GOTO end
@REM     )

@REM     IF %ERRORLEVEL%==1 (
@REM         echo Downloading CMake...
@REM         call cmake ..
@REM         GOTO end
@REM     )
        
@REM )

@REM :end
@REM     PAUSE