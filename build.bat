@echo off
REM Build script for YOLOv8 Multi-Camera Tracking System
REM Windows batch file

echo ========================================
echo YOLOv8 Multi-Camera Build Script
echo ========================================
echo.

REM Set Qt path (modify if your Qt installation is different)
set QT_PATH=D:\Qt\6.9.3\msvc2022_64

REM Check if build directory exists
if not exist build (
    echo Creating build directory...
    mkdir build
) else (
    echo Build directory already exists
)

cd build

echo.
echo Configuring CMake...
echo.

REM Configure with CMake
cmake .. -DCMAKE_PREFIX_PATH=%QT_PATH% -G "Visual Studio 17 2022"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ CMake configuration failed!
    echo.
    echo Possible issues:
    echo - Qt6 not found. Update QT_PATH in this script
    echo - OpenCV path incorrect in CMakeLists.txt
    echo - Visual Studio not installed
    pause
    exit /b 1
)

echo.
echo Building project (Release)...
echo.

REM Build the project
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Build failed!
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo ✅ Build completed successfully!
echo ========================================
echo.
echo Copying configuration files to Release folder...
echo.

REM Copy configuration files
call copy_configs.bat

echo.
echo ========================================
echo 🚀 Ready to run!
echo ========================================
echo.
echo Executables can be found in:
echo - build\Release\Yolov8CPPInference.exe (Console version)
echo - build\Release\Yolov8MultiCameraGUI.exe (Qt GUI version)
echo.

pause
