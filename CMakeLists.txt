cmake_minimum_required(VERSION 3.14)
project(Yolov8CPPInference VERSION 0.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt6 Support
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network QUIET)

if(Qt6_FOUND)
    message(STATUS "✅ Qt6 found: Building Qt GUI application")
    set(BUILD_QT_APP ON)
else()
    message(WARNING "⚠️ Qt6 not found. Only building console application.")
    set(BUILD_QT_APP OFF)
endif()
# --- !Qt6

# --- CUDA (optional)
find_package(CUDA 11)

if(CUDA_FOUND)
    message(STATUS "✅ CUDA found: ${CUDA_VERSION}")
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 11)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    add_definitions(-DUSE_CUDA)
else()
    message(WARNING "⚠️ CUDA not found. Building for CPU only.")
endif()
# --- !CUDA

# --- OpenCV
set(OpenCV_DIR "C:/opencv/build")  # Đường dẫn tới thư mục có OpenCVConfig.cmake
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
# --- !OpenCV

# --- nlohmann/json (header-only library)
# Download json.hpp from: https://github.com/nlohmann/json/releases
# Place it in a folder like "third_party/nlohmann"
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)
# --- !nlohmann/json

# Add ByteTrack subdirectory
add_subdirectory(ByteTrack-cpp)

# ============================================
# Original Console Application
# ============================================
set(CONSOLE_SOURCES
    main.cpp
    inference.h
    inference.cpp
)

add_executable(Yolov8CPPInference ${CONSOLE_SOURCES})

target_link_libraries(Yolov8CPPInference ${OpenCV_LIBS})
target_include_directories(Yolov8CPPInference PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(Yolov8CPPInference bytetrack)
target_include_directories(Yolov8CPPInference PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ByteTrack-cpp/include)

if(CUDA_FOUND)
    target_link_libraries(Yolov8CPPInference ${CUDA_LIBRARIES})
endif()

# ============================================
# Qt GUI Application (if Qt is available)
# ============================================
if(BUILD_QT_APP)
    set(QT_SOURCES
        main_qt.cpp
        inference.h
        inference.cpp
        CameraSource.h
        CameraSource.cpp
        CameraManager.h
        CameraManager.cpp
        CameraWidget.h
        CameraWidget.cpp
        MainWindow.h
        MainWindow.cpp
        GridManager.h
        GridManager.cpp
        CameraGridWidget.h
        CameraGridWidget.cpp
        CropsPanelWidget.h
        CropsPanelWidget.cpp
        TelegramBot.h
        TelegramBot.cpp
        AddCameraDialog.h
        AddCameraDialog.cpp
        Region.h
        Region.cpp
        RegionDrawingWidget.h
        RegionDrawingWidget.cpp
        RegionManagerDialog.h
        RegionManagerDialog.cpp
        DetectionEvent.h
        DetectionEvent.cpp
        EventManager.h
        EventManager.cpp
        EventThumbnailWidget.h
        EventThumbnailWidget.cpp
        EventsViewerWidget.h
        EventsViewerWidget.cpp
        EventsRegionCountWidget.h
        EventsRegionCountWidget.cpp
        DisplaySettingsDialog.h
        DisplaySettingsDialog.cpp
        TelegramSettingsDialog.h
        TelegramSettingsDialog.cpp
        RegionCountManager.h
        RegionCountManager.cpp
        FullScreenCameraView.h
        FullScreenCameraView.cpp
        ClassSelectionDialog.h
        ClassSelectionDialog.cpp
        ClassFilterManager.h
        ClassFilterManager.cpp
    )

    add_executable(Yolov8MultiCameraGUI ${QT_SOURCES})

    # Link Qt libraries
    target_link_libraries(Yolov8MultiCameraGUI
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
    )

    # Link OpenCV
    target_link_libraries(Yolov8MultiCameraGUI ${OpenCV_LIBS})
    target_include_directories(Yolov8MultiCameraGUI PRIVATE ${OpenCV_INCLUDE_DIRS})

    # Link ByteTrack
    target_link_libraries(Yolov8MultiCameraGUI bytetrack)
    target_include_directories(Yolov8MultiCameraGUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ByteTrack-cpp/include)

    # Link CUDA if available
    if(CUDA_FOUND)
        target_link_libraries(Yolov8MultiCameraGUI ${CUDA_LIBRARIES})
    endif()

    message(STATUS "✅ Qt GUI application target created: Yolov8MultiCameraGUI")
endif()
