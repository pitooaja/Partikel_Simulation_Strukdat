@echo off
echo Building Particle Effect Project...

REM Check if MinGW is in PATH
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: g++ compiler not found in PATH
    echo Please install MinGW-w64 and add it to PATH
    pause
    exit /b 1
)

REM Check if SFML paths exist
if not exist "C:\Users\Fito   Dwi Ardiansah\Downloads\libraries\SFML-3.0.0\include\SFML" (
    echo Error: SFML include directory not found at C:\Users\Fito   Dwi Ardiansah\Downloads\libraries\SFML-3.0.0\include\SFML
    echo Please install SFML 3.0 to C:\Users\Fito   Dwi Ardiansah\Downloads\libraries\SFML-3.0.0
    pause
    exit /b 1
)

if not exist "C:\Users\Fito   Dwi Ardiansah\Downloads\libraries\SFML-3.0.0\lib" (
    echo Error: SFML library directory not found at C:\Users\Fito   Dwi Ardiansah\Downloads\libraries\SFML-3.0.0\lib
    echo Please install SFML 3.0 to C:\Users\Fito   Dwi Ardiansah\Downloads\libraries\SFML-3.0.0
    pause
    exit /b 1
)

echo Compiling with g++...
g++ -o main.exe main.cpp -I"C:/Users/Fito   Dwi Ardiansah/Downloads/libraries/SFML-3.0.0/include" -L"C:/Users/Fito   Dwi Ardiansah/Downloads/libraries/SFML-3.0.0/lib" -lsfml-graphics -lsfml-window -lsfml-system

if %errorlevel% equ 0 (
    echo Build successful! Created main.exe
    echo.
    echo To run the program, type: main.exe
    echo.
    echo Note: Make sure SFML DLLs are in the same directory or in PATH
) else (
    echo Build failed with error code %errorlevel%
)

pause
