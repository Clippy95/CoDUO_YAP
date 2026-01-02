@echo off
cd /d "%~dp0"

:: Check for admin privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo This script requires administrator privileges.
    echo Attempting to relaunch as administrator...
    powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo Patching Call of Duty: United Offensive
echo.

:: Check for xdelta3
if not exist "patches\xdelta3.exe" (
    echo ERROR: xdelta3.exe not found in patches folder.
    pause
    exit /b
)

:: Check for patch files
if not exist "patches\uosp_steamtocd.vcdiff" (
    echo ERROR: uosp_steamtocd.vcdiff not found.
    pause
    exit /b
)

if not exist "patches\uomp_steamtocd.vcdiff" (
    echo ERROR: uomp_steamtocd.vcdiff not found.
    pause
    exit /b
)

set "PATCHED=0"

:: Try patching Single Player
if exist "CoDUOSP.exe" (
    echo Found CoDUOSP.exe - attempting to patch...
    
    :: Backup if not already backed up
    if not exist "CoDUOSP_unpatched.exe" (
        echo Backing up original CoDUOSP.exe...
        attrib -r -s -h "CoDUOSP.exe" >nul 2>&1
        copy "CoDUOSP.exe" "CoDUOSP_unpatched.exe" >nul 2>&1
    )
    
    :: Kill running process
    taskkill /f /im CoDUOSP.exe >nul 2>&1
    
    :: Remove existing patched file
    attrib -r -s -h "CoDUOSP.exe" >nul 2>&1
    del /f /q "CoDUOSP.exe" >nul 2>&1
    
    :: Apply patch
    echo Applying Single Player patch...
    "patches\xdelta3.exe" -d -s "CoDUOSP_unpatched.exe" "patches\uosp_steamtocd.vcdiff" "CoDUOSP.exe" >nul 2>&1
    
    if errorlevel 1 (
        echo Single Player patch failed - file may already be patched or is not the Steam version.
        :: Restore original
        if exist "CoDUOSP_unpatched.exe" (
            copy "CoDUOSP_unpatched.exe" "CoDUOSP.exe" >nul 2>&1
        )
    ) else (
        echo Single Player patched successfully!
        set "PATCHED=1"
    )
    echo.
)

:: Try patching Multiplayer
if exist "CoDUOMP.exe" (
    echo Found CoDUOMP.exe - attempting to patch...
    
    :: Backup if not already backed up
    if not exist "CoDUOMP_unpatched.exe" (
        echo Backing up original CoDUOMP.exe...
        attrib -r -s -h "CoDUOMP.exe" >nul 2>&1
        copy "CoDUOMP.exe" "CoDUOMP_unpatched.exe" >nul 2>&1
    )
    
    :: Kill running process
    taskkill /f /im CoDUOMP.exe >nul 2>&1
    
    :: Remove existing patched file
    attrib -r -s -h "CoDUOMP.exe" >nul 2>&1
    del /f /q "CoDUOMP.exe" >nul 2>&1
    
    :: Apply patch
    echo Applying Multiplayer patch...
    "patches\xdelta3.exe" -d -s "CoDUOMP_unpatched.exe" "patches\uomp_steamtocd.vcdiff" "CoDUOMP.exe" >nul 2>&1
    
    if errorlevel 1 (
        echo Multiplayer patch failed - file may already be patched or is not the Steam version.
        :: Restore original
        if exist "CoDUOMP_unpatched.exe" (
            copy "CoDUOMP_unpatched.exe" "CoDUOMP.exe" >nul 2>&1
        )
    ) else (
        echo Multiplayer patched successfully!
        set "PATCHED=1"
    )
    echo.
)

if "%PATCHED%"=="0" (
    echo No files were patched. Your executables may already be patched or are not the Steam versions.
    pause
    exit /b
)

echo Patching complete!
echo Please run the game again
pause