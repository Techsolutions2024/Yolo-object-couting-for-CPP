@echo off
echo ===================================
echo Building YOLOv8 Simple Grid Demo
echo ===================================

REM Set Qt6 path (adjust this to your Qt installation)
set QT_DIR=C:\Qt\6.8.1\msvc2022_64

REM Clean previous build
if exist build_demo_temp rmdir /s /q build_demo_temp
mkdir build_demo_temp
cd build_demo_temp

REM Configure CMake with separate CMakeLists
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DBUILD_DEMO_ONLY=ON ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ===================================
echo Build completed successfully!
echo Executable: build_demo_temp\Release\YOLOv8SimpleGridDemo.exe
echo ===================================
echo.

REM Run the demo
echo Running demo...
.\Release\YOLOv8SimpleGridDemo.exe

cd ..
