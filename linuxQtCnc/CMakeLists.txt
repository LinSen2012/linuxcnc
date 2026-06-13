cmake_minimum_required(VERSION 3.16)
project(linuxQtCnc VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================================
# linuxQtCnc - LinuxCNC Qt/C++ Refactored CNC Controller
# Phase 1: C/C++ Core Module Inheritance (Compilation-Ready)
# ============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================================
# Install Prefix & Paths
# ============================================================================
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/linuxQtCnc" CACHE PATH "Install prefix" FORCE)
endif()

# ============================================================================
# Output Directories
# ============================================================================
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ============================================================================
# Build Options
# ============================================================================
option(BUILD_RTAPI       "Build RTAPI library"             ON)
option(BUILD_HAL         "Build HAL core + components"     ON)
option(BUILD_LIBNML      "Build NML communication library" ON)
option(BUILD_EMC_CORE    "Build EMC core (nml_intf, ini)"  ON)
option(BUILD_INTERP      "Build G-code interpreter"        ON)
option(BUILD_TASK        "Build task controller"           ON)
option(BUILD_MOTION      "Build motion controller"         ON)
option(BUILD_TP          "Build trajectory planner"        ON)
option(BUILD_KINEMATICS  "Build kinematics modules"        ON)
option(BUILD_TOOLDATA    "Build tool data management"      ON)
option(BUILD_HALCMD      "Build HAL command-line tool"     ON)
option(BUILD_BACKEND     "Build backend tools (halui etc)"  ON)
option(ULAPI_BUILD       "Build user-space API (not kernel)" ON)

# ============================================================================
# Feature Detection (replaces autotools configure checks)
# ============================================================================
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)

check_include_file("sys/io.h"        HAVE_SYS_IO_H)
check_include_file("dlfcn.h"         HAVE_DLFCN_H)
check_include_file("locale.h"         HAVE_LOCALE_H)
check_include_file("libintl.h"        HAVE_LIBINTL_H)
check_function_exists("clock_nanosleep"  HAVE_CLOCK_NANOSLEEP)
check_function_exists("sem_timedwait"    HAVE_SEMTIMEDOP)
check_library_exists("rt" "clock_nanosleep" "" HAVE_RT_CLOCK_NANOSLEEP)

# Check for gettext (i18n support)
find_package(Intl QUIET)
if(Intl_FOUND)
    set(HAVE_LIBINTL 1)
    message(STATUS "gettext/libintl found: ${Intl_LIBRARIES}")
else()
    set(HAVE_LIBINTL 0)
    message(STATUS "gettext/libintl NOT found - using stub gettext")
endif()

# ============================================================================
# Generate config.h (replaces autotools config.h)
# ============================================================================
set(EMC2_NCFILES_DIR "${CMAKE_INSTALL_PREFIX}/share/linuxcnc/ncfiles")
set(EMC2_DEFAULT_TOOLTABLE "${CMAKE_INSTALL_PREFIX}/share/linuxcnc/tool.tbl")
set(EMC2_DEFAULT_NMLFILE "${CMAKE_INSTALL_PREFIX}/etc/linuxcnc/default.nml")

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/generated/config.h
)

# Install the generated config.h so targets can find it
install(FILES ${CMAKE_BINARY_DIR}/generated/config.h DESTINATION include/linuxcnc)

# ============================================================================
# Compiler Flags
# ============================================================================
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(ULAPI_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DULAPI")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DULAPI")
endif()

# ============================================================================
# Find External Dependencies
# ============================================================================
find_package(fmt QUIET)
if(NOT fmt_FOUND)
    message(STATUS "fmt library not found, ini module will use fallback")
endif()

# Threads (needed by several modules)
find_package(Threads QUIET)

# ============================================================================
# Global Include Directories
# These paths support angle-bracket includes used throughout the codebase:
#   #include <rtapi.h>        -> src/core/rtapi
#   #include <hal.h>          -> src/core/hal
#   #include <posemath.h>     -> src/core/libnml/posemath
#   #include <kinematics.h>   -> src/core/emc/kinematics
#   #include <inifile.hh>     -> src/core/emc/ini
#   #include <linuxcnc.h>     -> src/core/emc
#   #include <emcpos.h>       -> src/core/emc/nml_intf
#   #include <config.h>       -> build/generated (CMake output)
# ============================================================================
set(LQT_INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/generated      # for <config.h>
    ${CMAKE_SOURCE_DIR}/src/core        # for <linuxcnc.h>
    ${CMAKE_SOURCE_DIR}/src/core/rtapi   # for <rtapi.h>, <rtapi_*.h>
    ${CMAKE_SOURCE_DIR}/src/core/hal     # for <hal.h>, <hal_list.h>
    ${CMAKE_SOURCE_DIR}/src/core/libnml  # for libnml headers
    ${CMAKE_SOURCE_DIR}/src/core/libnml/posemath  # for <posemath.h>
    ${CMAKE_SOURCE_DIR}/src/core/libnml/nml       # for <nml.hh>, <nml_*.hh>
    ${CMAKE_SOURCE_DIR}/src/core/libnml/cms       # for <cms.hh>
    ${CMAKE_SOURCE_DIR}/src/core/libnml/rcs       # for <rcs.hh>
    ${CMAKE_SOURCE_DIR}/src/core/libnml/os_intf   # for <os_intf.hh>
    ${CMAKE_SOURCE_DIR}/src/core/libnml/buffer    # for <linklist.hh>
    ${CMAKE_SOURCE_DIR}/src/core/libnml/linklist  # for <linklist.hh>
    ${CMAKE_SOURCE_DIR}/src/core/emc              # for <linuxcnc.h>, <emcpos.h>
    ${CMAKE_SOURCE_DIR}/src/core/emc/nml_intf     # for <emcglb.h>, <canon.hh>
    ${CMAKE_SOURCE_DIR}/src/core/emc/ini          # for <inifile.hh>
    ${CMAKE_SOURCE_DIR}/src/core/emc/kinematics   # for <kinematics.h>
    ${CMAKE_SOURCE_DIR}/src/core/emc/rs274ngc      # for rs274ngc headers
    ${CMAKE_SOURCE_DIR}/src/core/emc/tooldata      # for <tooldata.hh>
    ${CMAKE_SOURCE_DIR}/src/core/emc/motion        # for motion headers
    ${CMAKE_SOURCE_DIR}/src/core/emc/tp            # for tp headers
    ${CMAKE_SOURCE_DIR}/src/core/pythonplugin      # for python_plugin_stub.hh
)

# Helper function: add common include dirs to a target
function(lqt_add_common_includes target)
    target_include_directories(${target} PUBLIC ${LQT_INCLUDE_DIRS})
endfunction()

# Helper function: add common link deps
function(lqt_add_common_deps target)
    target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS})
    if(Threads_FOUND)
        target_link_libraries(${target} PRIVATE Threads::Threads)
    endif()
    if(Intl_FOUND)
        target_link_libraries(${target} PRIVATE ${Intl_LIBRARIES})
    endif()
endfunction()

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
    # Exclude Xenomai/RTAI specific files (not needed for ULAPI userspace)
    list(FILTER RTAPI_SOURCES EXCLUDE REGEX ".*xenomai.*")
    list(FILTER RTAPI_SOURCES EXCLUDE REGEX ".*rtai.*")

    add_library(lqt_rtapi STATIC ${RTAPI_SOURCES})
    lqt_add_common_includes(lqt_rtapi)
    target_compile_definitions(lqt_rtapi PUBLIC
        $<$<BOOL:${ULAPI_BUILD}>:ULAPI>
        $<$<BOOL:${ULAPI_BUILD}>:RTAPI_USPACE>
    )
    lqt_add_common_deps(lqt_rtapi)
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
    lqt_add_common_includes(lqt_hal)
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
    lqt_add_common_includes(lqt_nml)
    target_link_libraries(lqt_nml PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        ${CMAKE_DL_LIBS}
    )
    message(STATUS "libnml: ${LIBNML_SOURCES} files")
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
    lqt_add_common_includes(lqt_posemath)
    target_link_libraries(lqt_posemath PRIVATE m)
    message(STATUS "posemath: ${POSEMATH_SOURCES} files")
endif()

# ============================================================================
# Module: EMC Core (NML Interface + INI)
# ============================================================================
if(BUILD_EMC_CORE)
    file(GLOB EMC_NML_SOURCES
        src/core/emc/nml_intf/*.cc
        src/core/emc/nml_intf/*.c
    )

    add_library(lqt_emc_core STATIC ${EMC_NML_SOURCES})
    lqt_add_common_includes(lqt_emc_core)

    # INI library
    file(GLOB EMC_INI_SOURCES
        src/core/emc/ini/*.cc
        src/core/emc/ini/*.c
    )
    list(FILTER EMC_INI_SOURCES EXCLUDE REGEX ".*linuxcnc_check_ini.*")

    add_library(lqt_ini SHARED ${EMC_INI_SOURCES})
    lqt_add_common_includes(lqt_ini)
    if(fmt_FOUND)
        target_link_libraries(lqt_ini PRIVATE fmt::fmt)
    endif()
    target_link_libraries(lqt_ini PRIVATE ${CMAKE_DL_LIBS})

    message(STATUS "EMC core (nml_intf): ${EMC_NML_SOURCES} files")
    message(STATUS "INI: ${EMC_INI_SOURCES} files")
endif()

# ============================================================================
# Module: G-code Interpreter (rs274ngc)
# ============================================================================
if(BUILD_INTERP)
    file(GLOB INTERP_SOURCES
        src/core/emc/rs274ngc/*.cc
    )
    # Exclude Python binding files (pure Boost.Python wrappers)
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*interp_python.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*canonmodule.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*gcodemodule.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*interpmodule.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*pyarrays.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*pyblock.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*pyemctypes.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*pyinterp1.*")
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*pyparamclass.*")

    add_library(lqt_rs274 SHARED ${INTERP_SOURCES})
    lqt_add_common_includes(lqt_rs274)
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
    lqt_add_common_includes(lqt_tooldata)
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
    lqt_add_common_includes(lqt_tp)
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
    lqt_add_common_includes(lqt_motion)
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
    # Exclude Boost.Python-only kinematics modules (disabled in linuxQtCnc)
    list(FILTER KINS_SOURCES EXCLUDE REGEX ".*rotarydeltakins\\.cc$")
    list(FILTER KINS_SOURCES EXCLUDE REGEX ".*lineardeltakins\\.cc$")

    add_library(lqt_kinematics MODULE ${KINS_SOURCES})
    lqt_add_common_includes(lqt_kinematics)
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
    lqt_add_common_includes(lqt_milltask)
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
    lqt_add_common_includes(lqt_halcmd)
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
        lqt_add_common_includes(lqt_halui)
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
# Build Configuration Summary
# ============================================================================
message(STATUS "")
message(STATUS "=============================================================")
message(STATUS " linuxQtCnc Build Configuration Summary")
message(STATUS "=============================================================")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  C Standard:     ${CMAKE_C_STANDARD}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  ULAPI Build:    ${ULAPI_BUILD}")
message(STATUS "  fmt Found:      ${fmt_FOUND}")
message(STATUS "  gettext Found:  ${Intl_FOUND}")
message(STATUS "  Threads Found:  ${Threads_FOUND}")
message(STATUS "=============================================================")
message(STATUS "  Module Build Status:")
message(STATUS "    RTAPI:         ${BUILD_RTAPI}")
message(STATUS "    HAL:           ${BUILD_HAL}")
message(STATUS "    libnml:        ${BUILD_LIBNML}")
message(STATUS "    EMC Core:      ${BUILD_EMC_CORE}")
message(STATUS "    Interpreter:   ${BUILD_INTERP}")
message(STATUS "    Task:          ${BUILD_TASK}")
message(STATUS "    Motion:        ${BUILD_MOTION}")
message(STATUS "    TP:            ${BUILD_TP}")
message(STATUS "    Kinematics:    ${BUILD_KINEMATICS}")
message(STATUS "    Tooldata:      ${BUILD_TOOLDATA}")
message(STATUS "    HALCMD:        ${BUILD_HALCMD}")
message(STATUS "    Backend:       ${BUILD_BACKEND}")
message(STATUS "=============================================================")
