@echo off
setlocal

echo ============================================
echo   OrionEngine Installer Build Script
echo ============================================
echo.

:: --------------------------------------------------
:: Step 1: Build the engine in Release mode
:: --------------------------------------------------
echo [1/3] Building OrionEngine in Release mode...
cd /d "%~dp0.."
cmake --build build --config Release --target Engine Editor Runtime
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed. Fix build errors and try again.
    pause
    exit /b 1
)
echo       Build succeeded.
echo.

:: --------------------------------------------------
:: Step 2: Verify Release output exists
:: --------------------------------------------------
echo [2/3] Verifying build output...

:: Search for Editor.exe + Engine.dll in any Release output folder
set "BIN_DIR="
for /d %%D in (build\bin\Release*) do (
    if exist "%%D\Editor.exe" if exist "%%D\Engine.dll" set "BIN_DIR=%%D"
)

:: Fallback: check flat output directories
if not defined BIN_DIR (
    if exist "build\Release\Editor.exe" if exist "build\Release\Engine.dll" (
        set "BIN_DIR=build\Release"
    )
)

if not defined BIN_DIR (
    echo.
    echo ERROR: Could not find Editor.exe and Engine.dll in Release output.
    echo        Searched: build\bin\Release*\
    echo.
    echo        Make sure cmake --build completes successfully in Release mode.
    pause
    exit /b 1
)

echo       Found binaries in: %BIN_DIR%
echo.

:: --------------------------------------------------
:: Step 2b: Update the .nsi to point to actual output dir
:: --------------------------------------------------
:: The .nsi uses build\bin\Release\ as the path.
:: Create a temporary copy with the correct path if needed.
set "NSI_BIN_PATH=%BIN_DIR:\=\\%"

:: --------------------------------------------------
:: Step 3: Build the installer with NSIS
:: --------------------------------------------------
echo [3/3] Building installer with NSIS...

where makensis >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    :: Try common install locations
    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
    ) else if exist "C:\Program Files\NSIS\makensis.exe" (
        set "MAKENSIS=C:\Program Files\NSIS\makensis.exe"
    ) else (
        echo.
        echo ERROR: NSIS not found. Install it with:
        echo        winget install NSIS.NSIS
        echo.
        echo        Or download from: https://nsis.sourceforge.io/Download
        pause
        exit /b 1
    )
) else (
    set "MAKENSIS=makensis"
)

:: Run NSIS from the installer directory so relative paths work
cd /d "%~dp0"
"%MAKENSIS%" /DBIN_DIR="..\%BIN_DIR%" OrionEngine.nsi
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: NSIS compilation failed. Check the errors above.
    pause
    exit /b 1
)

echo.
echo ============================================
echo   SUCCESS! Installer created:
echo   %~dp0..\OrionEngineSetup.exe
echo ============================================
echo.
pause
