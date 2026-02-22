@echo off
REM Automated setup script for fsquirrel project
REM This script sets up the development environment on a new Windows computer
REM Requirements: Windows 10/11, internet connection
REM Assumes VS Code is installed (if not, install it first)

echo ========================================
echo Setting up fsquirrel development environment
echo ========================================

REM Check if Python is available (try python command first, then py launcher)
set PYTHON_CMD=
python --version >nul 2>&1
if %errorlevel% equ 0 (
    set PYTHON_CMD=python
    echo Python found via 'python' command.
) else (
    py --version >nul 2>&1
    if %errorlevel% equ 0 (
        set PYTHON_CMD=py
        echo Python found via 'py' launcher.
    ) else (
        echo ERROR: Python is not found.
        echo.
        echo Please install Python 3.8+ from https://python.org
        echo IMPORTANT: During installation, make sure to check "Add Python to PATH"
        echo.
        echo If Python is already installed but not in PATH, you can:
        echo 1. Find python.exe location (usually C:\Users\username\AppData\Local\Programs\Python\Python3x\)
        echo 2. Add that directory to your system PATH environment variable
        echo 3. Or use the Python launcher 'py' command
        echo.
        echo After fixing PATH, run this script again.
        pause
        exit /b 1
    )
)

echo Python version:
%PYTHON_CMD% --version

REM Check if git is installed
git --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Git is not installed.
    echo Please install Git from https://git-scm.com
    pause
    exit /b 1
)

echo Git found. Version:
git --version

REM Create virtual environment if it doesn't exist
if not exist venv_OdriveTool (
    echo Creating virtual environment...
    %PYTHON_CMD% -m venv venv_OdriveTool
) else (
    echo Virtual environment already exists.
)

REM Activate virtual environment
echo Activating virtual environment...
call venv_OdriveTool\Scripts\activate.bat

REM Upgrade pip
echo Upgrading pip...
python -m pip install --upgrade pip

REM Install PlatformIO
echo Installing PlatformIO...
pip install platformio

REM Install Python dependencies
echo Installing Python dependencies...
pip install -r requirements.txt

REM Verify installations
echo.
echo Verifying installations...
echo.
pio --version
echo.
python -c "import odrive; print('ODrive package imported successfully')"

REM Install VS Code extensions (requires VS Code CLI in PATH)
echo.
echo Installing VS Code extensions...
echo Note: This requires VS Code CLI (code command) to be in PATH
code --version >nul 2>&1
if %errorlevel% equ 0 (
    echo Installing recommended VS Code extensions...
    code --install-extension platformio.platformio-ide
    code --install-extension ms-vscode.cpptools
    code --install-extension ms-python.python
    code --install-extension ms-vscode.vscode-json
    echo VS Code extensions installed.
) else (
    echo WARNING: VS Code CLI not found in PATH.
    echo Please manually install these extensions in VS Code:
    echo - PlatformIO IDE
    echo - C/C++ (Microsoft)
    echo - Python (Microsoft)
    echo - JSON Language Features
)

echo.
echo ========================================
echo Setup complete!
echo ========================================
echo.
echo Next steps:
echo 1. Open the project folder in VS Code
echo 2. If VS Code extensions weren't installed automatically, install them manually
echo 3. Connect your Teensy 4.1 board
echo 4. Use PlatformIO to build and upload: pio run -e [environment] -t upload
echo    Available environments: teensy41_sine, teensy41_sine_original, teensy41_tester
echo.
echo To activate the environment in future sessions:
echo   venv_OdriveTool\Scripts\activate.bat
echo.
pause