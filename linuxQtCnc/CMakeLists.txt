cmake_minimum_required(VERSION 3.16)
project(linuxQtCnc VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================================
# linuxQtCnc - LinuxCNC Qt/C++ Refactored CNC Controller
# Phase 1: C/C++ Core Module Inheritance
# ============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ============================================================================
# Build Options
# ============================================================================
option(BUILD_RTAPI      "Build RTAPI library"           ON)
option(BUILD_HAL        "Build HAL core + components"   ON)
option(BUILD_LIBNML     "Build NML communication library" ON)
option(BUILD_EMC_CORE   "Build EMC core (nml_intf, ini)" ON)
option(BUILD_INTERP     "Build G-code interpreter"      ON)
option(BUILD_TASK       "Build task controller"         ON)
option(BUILD_MOTION     "Build motion controller"       ON)
option(BUILD_TP         "Build trajectory planner"      ON)
option(BUILD_KINEMATICS "Build kinematics modules"      ON)
option(BUILD_TOOLDATA   "Build tool data management"     ON)
option(BUILD_HALCMD     "Build HAL command-line tool"    ON)
option(BUILD_BACKEND    "Build backend tools (halui etc)" ON)
option(ULAPI_BUILD       "Build user-space API (not kernel)" ON)

# ============================================================================
# Compiler Flags
# ============================================================================
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(ULAPI_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DULAPI")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DULAPI")
endif()

# Find external dependencies
find_package(fmt QUIET)  # For ini module
if(NOT fmt_FOUND)
    message(STATUS "fmt library not found, ini module will use fallback")
endif()

# ============================================================================
# Include Paths
# ============================================================================
set(LINUXQTINC_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/rtapi
    ${CMAKE_SOURCE_DIR}/src/core/hal
    ${CMAKE_SOURCE_DIR}/src/core/libnml
    ${CMAKE_SOURCE_DIR}/src/core/emc
)

# ============================================================================
# Module: RTAPI (Real-Time Application Programming Interface)
# ============================================================================
if(BUILD_RTAPI)
    file(GLOB_RECURSE RTAPI_SOURCES
        src/core/rtapi/*.c
        src/core/rtapi/*.cc
    )
    # Exclude examples and tests
    list(FILTER RTAPI_SOURCES EXCLUDE REGEX ".*/examples/.*")

    add_library(lqt_rtapi STATIC ${RTAPI_SOURCES})
    target_include_directories(lqt_rtapi PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/rtapi
    )
    target_compile_definitions(lqt_rtapi PUBLIC
        $<$<BOOL:${ULAPI_BUILD}>:ULAPI>
    )
    message(STATUS "RTAPI: ${RTAPI_SOURCES} files")
endif()

# ============================================================================
# Module: HAL (Hardware Abstraction Layer) - Core Library
# ============================================================================
if(BUILD_HAL)
    set(HAL_CORE_SOURCES
        src/core/hal/hal_lib.c
    )

    add_library(lqt_hal SHARED ${HAL_CORE_SOURCES})
    target_include_directories(lqt_hal PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/hal
        $<$<TARGET_EXISTS:lqt_rtapi>:$<TARGET_PROPERTY:lqt_rtapi,INTERFACE_INCLUDE_DIRECTORIES>>
    )
    target_link_libraries(lqt_hal PRIVATE
        $<$<TARGET_EXISTS:lqt_rtapi>:lqt_rtapi>
        ${CMAKE_DL_LIBS}
        rt
    )
    message(STATUS "HAL core: ${HAL_CORE_SOURCES}")
endif()

# ============================================================================
# Module: libnml (NML Communication Library)
# ============================================================================
if(BUILD_LIBNML)
    file(GLOB_RECURSE LIBNML_SOURCES
        src/core/libnml/buffer/*.cc
        src/core/libnml/cms/*.cc
        src/core/libnml/linklist/*.cc
        src/core/libnml/nml/*.cc
        src/core/libnml/os_intf/*.cc
        src/core/libnml/os_intf/_*.c
        src/core/libnml/posemath/*.cc
        src/core/libnml/rcs/*.cc
        src/core/libnml/buffer/*.c
        src/core/libnml/os_intf/*.c
    )

    add_library(lqt_nml SHARED ${LIBNML_SOURCES})
    target_include_directories(lqt_nml PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/libnml
        ${CMAKE_SOURCE_DIR}/src/core/libnml/cms
        ${CMAKE_SOURCE_DIR}/src/core/libnml/nml
        ${CMAKE_SOURCE_DIR}/src/core/libnml/buffer
        ${CMAKE_SOURCE_DIR}/src/core/libnml/os_intf
        ${CMAKE_SOURCE_DIR}/src/core/libnml/posemath
        ${CMAKE_SOURCE_DIR}/src/core/libnml/rcs
        ${CMAKE_SOURCE_DIR}/src/core/libnml/linklist
        ${CMAKE_SOURCE_DIR}/src/core/emc
    )
    target_link_libraries(lqt_nml PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        ${CMAKE_DL_LIBS}
    )
    message(STATUS "libnml: ${LIBNML_SOURCES} files")
endif()

# ============================================================================
# Module: EMC Core (NML Interface + INI)
# ============================================================================
if(BUILD_EMC_CORE)
    file(GLOB EMC_NML_SOURCES
        src/core/emc/nml_intf/*.cc
        src/core/emc/nml_intf/*.c
    )
    list(REMOVE_ITEM EMC_NML_SOURCES
        ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf/canon_position.cc  # if needed
    )

    add_library(lqt_emc_core STATIC ${EMC_NML_SOURCES})
    target_include_directories(lqt_emc_core PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf
        ${CMAKE_SOURCE_DIR}/src/core/emc/motion
    )

    # INI library
    file(GLOB EMC_INI_SOURCES
        src/core/emc/ini/*.cc
        src/core/emc/ini/*.c
    )
    list(FILTER EMC_INI_SOURCES EXCLUDE REGEX ".*linuxcnc_check_ini.*")

    add_library(lqt_ini SHARED ${EMC_INI_SOURCES})
    target_include_directories(lqt_ini PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc/ini
        ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf
        ${CMAKE_SOURCE_DIR}/src/core/emc
    )
    if(fmt_FOUND)
        target_link_libraries(lqt_ini PRIVATE fmt::fmt)
    endif()
    target_link_libraries(lqt_ini PRIVATE ${CMAKE_DL_LIBS})

    message(STATUS "EMC core (nml_intf): ${EMC_NML_SOURCES} files")
    message(STATUS "INI: ${EMC_INI_SOURCES} files")
endif()

# ============================================================================
# Module: Posemath (Geometry Library)
# ============================================================================
if(BUILD_LIBNML)
    file(GLOB POSEMATH_SOURCES
        src/core/libnml/posemath/*.cc
        src/core/libnml/posemath/*.c
    )
    add_library(lqt_posemath SHARED ${POSEMATH_SOURCES})
    target_include_directories(lqt_posemath PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/libnml/posemath
    )
    target_link_libraries(lqt_posemath PRIVATE m)
    message(STATUS "posemath: ${POSEMATH_SOURCES} files")
endif()

# ============================================================================
# Module: G-code Interpreter (rs274ngc)
# ============================================================================
if(BUILD_INTERP)
    file(GLOB INTERP_SOURCES
        src/core/emc/rs274ngc/*.cc
    )
    # Exclude Python binding files
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*interp_python.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*canonmodule.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*gcodemodule.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*interpmodule.*")

    add_library(lqt_rs274 SHARED ${INTERP_SOURCES})
    target_include_directories(lqt_rs274 PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc/rs274ngc
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf
        ${CMAKE_SOURCE_DIR}/src/core/emc/ini
        ${CMAKE_SOURCE_DIR}/src/core/rtapi
    )
    target_link_libraries(lqt_rs274 PRIVATE
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        ${CMAKE_DL_LIBS}
    )
    message(STATUS "rs274ngc interpreter: ${INTERP_SOURCES} files")
endif()

# ============================================================================
# Module: Tool Data Management
# ============================================================================
if(BUILD_TOOLDATA)
    file(GLOB TOOLDATA_SOURCES
        src/core/emc/tooldata/*.cc
    )

    add_library(lqt_tooldata SHARED ${TOOLDATA_SOURCES})
    target_include_directories(lqt_tooldata PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc/tooldata
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf
    )
    target_link_libraries(lqt_tooldata PRIVATE
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        ${CMAKE_DL_LIBS}
    )
    message(STATUS "tooldata: ${TOOLDATA_SOURCES} files")
endif()

# ============================================================================
# Module: Trajectory Planner (TP)
# ============================================================================
if(BUILD_TP)
    file(GLOB TP_SOURCES
        src/core/emc/tp/*.c
    )

    add_library(lqt_tp STATIC ${TP_SOURCES})
    target_include_directories(lqt_tp PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc/tp
        ${CMAKE_SOURCE_DIR}/src/core/emc/motion
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/libnml/posemath
        ${CMAKE_SOURCE_DIR}/src/core/rtapi
    )
    target_link_libraries(lqt_tp PRIVATE m)
    message(STATUS "tp: ${TP_SOURCES} files")
endif()

# ============================================================================
# Module: Motion Controller
# ============================================================================
if(BUILD_MOTION)
    file(GLOB MOTION_SOURCES
        src/core/emc/motion/*.c
        src/core/emc/motion/*.cc
    )

    add_library(lqt_motion STATIC ${MOTION_SOURCES})
    target_include_directories(lqt_motion PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc/motion
        ${CMAKE_SOURCE_DIR}/src/core/emc/tp
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/emc/kinematics
        ${CMAKE_SOURCE_DIR}/src/core/rtapi
        ${CMAKE_SOURCE_DIR}/src/core/hal
    )
    target_link_libraries(lqt_motion PRIVATE
        $<$<TARGET_EXISTS:lqt_tp>:lqt_tp>
        m
    )
    message(STATUS "motion: ${MOTION_SOURCES} files")
endif()

# ============================================================================
# Module: Kinematics
# ============================================================================
if(BUILD_KINEMATICS)
    file(GLOB KINS_SOURCES
        src/core/emc/kinematics/*.c
        src/core/emc/kinematics/*.cc
    )

    add_library(lqt_kinematics MODULE ${KINS_SOURCES})
    target_include_directories(lqt_kinematics PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core/emc/kinematics
        ${CMAKE_SOURCE_DIR}/src/core/emc/motion
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/rtapi
        ${CMAKE_SOURCE_DIR}/src/core/hal
        ${CMAKE_SOURCE_DIR}/src/core/libnml/posemath
    )
    target_link_libraries(lqt_kinematics PRIVATE
        $<$<TARGET_EXISTS:lqt_posemath>:lqt_posemath>
        m
    )
    message(STATUS "kinematics: ${KINS_SOURCES} files")
endif()

# ============================================================================
# Module: Task Controller
# ============================================================================
if(BUILD_TASK)
    file(GLOB TASK_SOURCES
        src/core/emc/task/*.cc
    )

    add_executable(lqt_milltask ${TASK_SOURCES})
    target_include_directories(lqt_milltask PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core/emc/task
        ${CMAKE_SOURCE_DIR}/src/core/emc
        ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf
        ${CMAKE_SOURCE_DIR}/src/core/emc/rs274ngc
        ${CMAKE_SOURCE_DIR}/src/core/emc/motion
        ${CMAKE_SOURCE_DIR}/src/core/emc/ini
        ${CMAKE_SOURCE_DIR}/src/core/emc/tooldata
        ${CMAKE_SOURCE_DIR}/src/core/libnml
        ${CMAKE_SOURCE_DIR}/src/core/rtapi
    )
    target_link_libraries(lqt_milltask PRIVATE
        $<$<TARGET_EXISTS:lqt_rs274>:lqt_rs274>
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        $<$<TARGET_EXISTS:lqt_nml>:lqt_nml>
        $<$<TARGET_EXISTS:lqt_posemath>:lqt_posemath>
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        $<$<TARGET_EXISTS:lqt_tooldata>:lqt_tooldata>
        ${CMAKE_DL_LIBS}
        rt
    )
    message(STATUS "task (milltask): ${TASK_SOURCES} files")
endif()

# ============================================================================
# Module: HALCMD (HAL Command-Line Tool)
# ============================================================================
if(BUILD_HALCMD)
    file(GLOB HALCMD_SOURCES
        src/tools/halcmd/*.c
        src/tools/halcmd/*.cc
    )

    add_executable(lqt_halcmd ${HALCMD_SOURCES})
    target_include_directories(lqt_halcmd PRIVATE
        ${CMAKE_SOURCE_DIR}/src/tools/halcmd
        ${CMAKE_SOURCE_DIR}/src/core/hal
        ${CMAKE_SOURCE_DIR}/src/core/emc/ini
        ${CMAKE_SOURCE_DIR}/src/core/emc
    )
    target_link_libraries(lqt_halcmd PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        ${CMAKE_DL_LIBS}
        rt
    )
    message(STATUS "halcmd: ${HALCMD_SOURCES} files")
endif()

# ============================================================================
# Module: Backend Tools (halui, emcsh, etc.)
# ============================================================================
if(BUILD_BACKEND)
    file(GLOB BACKEND_SOURCES
        src/tools/backend/*.c
        src/tools/backend/*.cc
    )

    if(BACKEND_SOURCES)
        add_executable(lqt_halui ${BACKEND_SOURCES})
        target_include_directories(lqt_halui PRIVATE
            ${CMAKE_SOURCE_DIR}/src/tools/backend
            ${CMAKE_SOURCE_DIR}/src/core/emc
            ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf
            ${CMAKE_SOURCE_DIR}/src/core/emc/motion
            ${CMAKE_SOURCE_DIR}/src/core/emc/ini
            ${CMAKE_SOURCE_DIR}/src/core/emc/tooldata
            ${CMAKE_SOURCE_DIR}/src/core/libnml
            ${CMAKE_SOURCE_DIR}/src/core/rtapi
            ${CMAKE_SOURCE_DIR}/src/core/hal
        )
        target_link_libraries(lqt_halui PRIVATE
            $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
            $<$<TARGET_EXISTS:lqt_nml>:lqt_nml>
            $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
            $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
            $<$<TARGET_EXISTS:lqt_tooldata>:lqt_tooldata>
            ${CMAKE_DL_LIBS}
        )
        message(STATUS "backend tools: ${BACKEND_SOURCES} files")
    endif()
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "=============================================================")
message(STATUS " linuxQtCnc Build Configuration Summary")
message(STATUS "=============================================================")
message(STATUS "  C Standard:     ${CMAKE_C_STANDARD}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  ULAPI Build:     ${ULAPI_BUILD}")
message(STATUS "  fmt Found:       ${fmt_FOUND}")
message(STATUS "=============================================================")
