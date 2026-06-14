
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

# -------------------------------------------------------------------
# Link to linuxQtCnc core C++ libraries (if built by the root CMake):
# These targets are exported by the root project; they enable the
# Qt GUI to call into the G-code interpreter, INI parser, HAL, etc.
# On Windows, only a minimal subset (lqt_rs274 / lqt_ini / lqt_emc_core
# / lqt_nml / lqt_posemath / lqt_tooldata) is available; on Linux the
# full RT/HAL/Motion stack is linked as well.
# -------------------------------------------------------------------
set(_GUI_CORE_DEPS
    lqt_rs274 lqt_ini lqt_emc_core lqt_nml lqt_posemath lqt_tooldata
)
set(_GUI_LINUX_ONLY_DEPS
    lqt_hal lqt_task lqt_motion lqt_tp
)
foreach(_dep ${_GUI_CORE_DEPS})
    if(TARGET ${_dep})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${_dep})
    endif()
endforeach()
if(NOT WIN32)
    foreach(_dep ${_GUI_LINUX_ONLY_DEPS})
        if(TARGET ${_dep})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${_dep})
        endif()
    endforeach()
endif()

if(Qt5SerialPort_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::SerialPort)
endif()

# Linker flags (Linux only)
if(NOT WIN32)
    find_library(LIBDL_LIBRARY dl)
    if(LIBDL_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDL_LIBRARY})
    endif()
endif()

