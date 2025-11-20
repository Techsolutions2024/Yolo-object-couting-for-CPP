@echo off
REM Copy configuration files to Release build folder

echo Copying configuration files to build/Release/...

if not exist build\Release\ (
    echo Error: build\Release\ directory not found!
    echo Please build the project first.
    exit /b 1
)

REM Copy Telegram config
if exist telegram_config.json (
    copy /Y telegram_config.json build\Release\
    echo [OK] telegram_config.json copied
) else (
    echo [SKIP] telegram_config.json not found
)

REM Copy cameras config
if exist cameras_config.json (
    copy /Y cameras_config.json build\Release\
    echo [OK] cameras_config.json copied
) else (
    echo [SKIP] cameras_config.json not found
)

REM Copy region count data
if exist region_count.json (
    copy /Y region_count.json build\Release\
    echo [OK] region_count.json copied
) else (
    echo [SKIP] region_count.json not found
)

REM Copy YOLO model
if exist yolo11n.onnx (
    copy /Y yolo11n.onnx build\Release\
    echo [OK] yolo11n.onnx copied
) else if exist yolov8n.onnx (
    copy /Y yolov8n.onnx build\Release\
    echo [OK] yolov8n.onnx copied
) else (
    echo [SKIP] YOLO model not found
)

echo.
echo ========================================
echo All configuration files copied!
echo You can now run build\Release\Yolov8MultiCameraGUI.exe
echo ========================================
pause
