
cmake_minimum_required(VERSION 3.12)
project(linuxQtCncGui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
find_package(Qt5 OPTIONAL_COMPONENTS SerialPort)

# 根据是否由根 CMakeLists 调用，调整路径
if(NOT DEFINED LQTCNC_ROOT_BUILD)
    set(LQTCNC_CORE_DIR "${CMAKE_SOURCE_DIR}/../../src/core")
else()
    set(LQTCNC_CORE_DIR "${CMAKE_SOURCE_DIR}/src/core")
endif()

set(CORE_INCLUDE_DIRS
    ${LQTCNC_CORE_DIR}
    ${LQTCNC_CORE_DIR}/rtapi
    ${LQTCNC_CORE_DIR}/hal
    ${LQTCNC_CORE_DIR}/libnml
    ${LQTCNC_CORE_DIR}/libnml/posemath
    ${LQTCNC_CORE_DIR}/emc
    ${LQTCNC_CORE_DIR}/emc/nml_intf
    ${LQTCNC_CORE_DIR}/emc/ini
    ${LQTCNC_CORE_DIR}/emc/kinematics
    ${LQTCNC_CORE_DIR}/emc/rs274ngc
    ${LQTCNC_CORE_DIR}/emc/tooldata
    ${LQTCNC_CORE_DIR}/emc/motion
    ${LQTCNC_CORE_DIR}/emc/tp
    ${LQTCNC_CORE_DIR}/pythonplugin
)

add_subdirectory(core)
add_subdirectory(widgets)
add_subdirectory(preview)
add_subdirectory(halmon)
add_subdirectory(scope)
add_subdirectory(mainwindow)

add_executable(${PROJECT_NAME}
    main.cpp
    LcncApplication.h
    LcncApplication.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CORE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    lqt_gui_core
    lqt_gui_mainwindow
    lqt_gui_widgets
    lqt_gui_preview
    lqt_gui_halmon
    lqt_gui_scope
)

if(Qt5SerialPort_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::SerialPort)
endif()

