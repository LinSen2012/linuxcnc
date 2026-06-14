cmake_minimum_required(VERSION 3.14)
project(linuxQtCnc VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================================
# linuxQtCnc - LinuxCNC Qt/C++ Refactored CNC Controller
# Phase 1: C/C++ Core Module Inheritance (Compilation-Ready)
# ============================================================================

# ============================================================================
# Platform Detection & Workarounds
# ============================================================================
set(LQTCNC_IS_WINDOWS OFF)
set(LQTCNC_IS_LINUX OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32)
    set(LQTCNC_IS_WINDOWS ON)
    message(STATUS "Detected platform: Windows - building MINIMAL core subset (no Linux RT/HAL drivers)")
    # Windows 专属编译定义
    add_compile_definitions(LQTCNC_WINDOWS=1)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Win7+
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
    # Windows 下默认关闭 Linux 专属模块
    set(BUILD_RTAPI OFF CACHE BOOL "" FORCE)
    set(BUILD_MOTION OFF CACHE BOOL "" FORCE)
    set(BUILD_TP OFF CACHE BOOL "" FORCE)
    set(BUILD_HALCMD OFF CACHE BOOL "" FORCE)
    set(BUILD_BACKEND OFF CACHE BOOL "" FORCE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LQTCNC_IS_LINUX ON)
    add_compile_definitions(_GNU_SOURCE=1)
    message(STATUS "Detected platform: Linux - building FULL core + realtime modules")
endif()

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
option(BUILD_TOOLDATA    "Build tool data management"       ON)
option(BUILD_HALCMD      "Build HAL command-line tool"     ON)
option(BUILD_BACKEND     "Build backend tools (halui etc)"  ON)
option(BUILD_GUI         "Build Qt5 GUI application"       ON)
option(ULAPI_BUILD       "Build user-space API (not kernel)" ON)

# ============================================================================
# Qt5 / Qt6 detection (Windows priority path)
# On Windows, CMake's find_package may not find Qt if installed under
# C:\Qt\5.12.3\mingw73_64.  Set CMAKE_PREFIX_PATH in the Qt Creator
# Kits panel, or use the fallback list below.
# ============================================================================
if(LQTCNC_IS_WINDOWS AND BUILD_GUI)
    # Common Qt5 install roots on Windows (try in order, first hit wins)
    set(_QT5_PREFIXES
        "D:/Qt/Qt5.12.3/5.12.3/msvc2017"    # user's confirmed install
        "D:/Qt/Qt5.12.3/5.12.3/mingw73_64"
        "D:/Qt/Qt5.12.3"
    )
    foreach(_prefix ${_QT5_PREFIXES})
        if(EXISTS "${_prefix}/lib/cmake/Qt5/Qt5Config.cmake")
            list(INSERT CMAKE_PREFIX_PATH 0 "${_prefix}")
            message(STATUS "Qt5 found at: ${_prefix}")
            break()
        endif()
    endforeach()
    unset(_QT5_PREFIXES)
    unset(_prefix)
endif()

# ============================================================================
# Feature Detection (replaces autotools configure checks)
# ============================================================================
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)

if(NOT LQTCNC_IS_WINDOWS)
    # Linux 专属头仅在 Linux 下检测
    check_include_file("sys/io.h"           HAVE_SYS_IO_H)
    check_include_file("dlfcn.h"            HAVE_DLFCN_H)
    check_function_exists("clock_nanosleep" HAVE_CLOCK_NANOSLEEP)
    check_function_exists("sem_timedwait"   HAVE_SEMTIMEDOP)
    check_library_exists("rt" "clock_nanosleep" "" HAVE_RT_CLOCK_NANOSLEEP)
endif()

check_include_file("locale.h"         HAVE_LOCALE_H)
check_include_file("libintl.h"        HAVE_LIBINTL_H)

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
# Compiler Flags (MSVC vs GCC/Clang)
# ============================================================================
if(MSVC)
    # MSVC uses /W-style warnings
    add_compile_options(/W3 /EHsc /permissive-)
    # MSVC-specific definitions for CRT
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
else()
    # GCC / Clang use -W-style warnings
    add_compile_options(-Wall -Wextra -fPIC)
endif()

if(ULAPI_BUILD)
    add_compile_definitions(ULAPI)
endif()

# ============================================================================
# Find External Dependencies
# ============================================================================
# fmt: used by inifile.cc for formatted error messages.
# On Windows, gracefully fall back to a header-only stub if the library
# cannot be found, so developers don't need to install vcpkg to try.
find_package(fmt QUIET)
if(fmt_FOUND)
    message(STATUS "fmt library found: ${fmt_VERSION}")
    set(HAVE_FMT 1)
    add_compile_definitions(HAVE_FMT=1)
else()
    set(HAVE_FMT 0)
    message(STATUS "fmt library NOT found - inifile.cc will use printf-style fallback")
endif()

# Boost: required for real-time uspace_rtapi_main.cc (boost::lockfree::queue).
# On Windows this path is excluded (BUILD_RTAPI=OFF), so Boost is optional.
# Only the headers are needed; SYSTEM suppresses Boost warnings.
if(NOT LQTCNC_IS_WINDOWS)
    find_package(Boost REQUIRED)
    if(Boost_FOUND)
        message(STATUS "Boost headers found (version ${Boost_VERSION}); target Boost::boost available")
    endif()
else()
    message(STATUS "Boost: not required on Windows (BUILD_RTAPI disabled)")
endif()

# Threads (needed by several modules)
find_package(Threads REQUIRED)

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
if(BUILD_RTAPI AND NOT LQTCNC_IS_WINDOWS)
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
    # Boost (lockfree queue in uspace_rtapi_main.cc) - header only
    target_link_libraries(lqt_rtapi PRIVATE Boost::boost)
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
        $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:rt>
    )
    message(STATUS "HAL core: ${HAL_CORE_SOURCES}")

    # ------------------------------------------------------------------
    # HAL components: each .c is compiled into a standalone loadable module
    # ------------------------------------------------------------------
    set(HAL_COMPONENT_SOURCES
        src/core/hal/components/pid.c
        src/core/hal/components/stepgen.c
        src/core/hal/components/encoder.c
        src/core/hal/components/encoder_ratio.c
        src/core/hal/components/siggen.c
        src/core/hal/components/pwmgen.c
        src/core/hal/components/counter.c
        src/core/hal/components/debounce.c
        src/core/hal/components/threads.c
        src/core/hal/components/watchdog.c
        src/core/hal/components/supply.c
        src/core/hal/components/modmath.c
        src/core/hal/components/mux_generic.c
        src/core/hal/components/demux_generic.c
        src/core/hal/components/enum.c
        src/core/hal/components/lcd.c
        src/core/hal/components/matrix_kb.c
        src/core/hal/components/sampler.c
        src/core/hal/components/sampler_usr.c
        src/core/hal/components/streamer.c
        src/core/hal/components/streamer_usr.c
        src/core/hal/components/sim_encoder.c
    )
    # panelui.c depends on Python.h (<Python.h>) and is a GUI helper - skip it.
    # src/core/hal/components/panelui.c intentionally excluded.

    foreach(comp_src ${HAL_COMPONENT_SOURCES})
        get_filename_component(comp_name ${comp_src} NAME_WE)
        add_library(hal_${comp_name} MODULE ${comp_src})
        lqt_add_common_includes(hal_${comp_name})
        target_link_libraries(hal_${comp_name} PRIVATE
            $<$<TARGET_EXISTS:lqt_rtapi>:lqt_rtapi>
            ${CMAKE_DL_LIBS}
        )
        set_target_properties(hal_${comp_name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${comp_name}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/hal
        )
    endforeach()

    # ---------- HAL hardware drivers (Linux only; optional) ----------
    # parport 并口驱动 (PC parallel port)
    if(NOT LQTCNC_IS_WINDOWS)
        set(HAL_DRIVER_SOURCES
            src/core/hal/drivers/hal_parport.c
            src/core/hal/drivers/hal_ppmc.c
            src/core/hal/drivers/hal_motenc.c
            src/core/hal/drivers/hal_ax5214h.c
            src/core/hal/drivers/hal_skeleton.c
        )
        foreach(drv_src ${HAL_DRIVER_SOURCES})
            if(EXISTS ${CMAKE_SOURCE_DIR}/${drv_src})
                get_filename_component(drv_name ${drv_src} NAME_WE)
                add_library(${drv_name} MODULE ${drv_src})
                lqt_add_common_includes(${drv_name})
                target_link_libraries(${drv_name} PRIVATE
                    $<$<TARGET_EXISTS:lqt_rtapi>:lqt_rtapi>
                    ${CMAKE_DL_LIBS}
                )
                set_target_properties(${drv_name} PROPERTIES
                    PREFIX ""
                    OUTPUT_NAME ${drv_name}
                    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/hal
                )
            endif()
        endforeach()

        # ---------- Mesa Hostmot2 驱动 (多文件) ----------
        file(GLOB MESA_SRC
            src/core/hal/drivers/mesa-hostmot2/hm2_*.c
            src/core/hal/drivers/mesa-hostmot2/abs_encoder.c
            src/core/hal/drivers/mesa-hostmot2/bspi.c
            src/core/hal/drivers/mesa-hostmot2/dpll.c
            src/core/hal/drivers/mesa-hostmot2/encoder.c
            src/core/hal/drivers/mesa-hostmot2/hostmot2.c
            src/core/hal/drivers/mesa-hostmot2/inm.c
            src/core/hal/drivers/mesa-hostmot2/inmux.c
            src/core/hal/drivers/mesa-hostmot2/ioport.c
            src/core/hal/drivers/mesa-hostmot2/led.c
            src/core/hal/drivers/mesa-hostmot2/oneshot.c
            src/core/hal/drivers/mesa-hostmot2/outm.c
            src/core/hal/drivers/mesa-hostmot2/periodm.c
            src/core/hal/drivers/mesa-hostmot2/pins.c
            src/core/hal/drivers/mesa-hostmot2/pktuart.c
            src/core/hal/drivers/mesa-hostmot2/pwmgen.c
            src/core/hal/drivers/mesa-hostmot2/raw.c
            src/core/hal/drivers/mesa-hostmot2/rcpwmgen.c
            src/core/hal/drivers/mesa-hostmot2/resolver.c
            src/core/hal/drivers/mesa-hostmot2/setsserial.c
            src/core/hal/drivers/mesa-hostmot2/spi_common_rpspi.c
            src/core/hal/drivers/mesa-hostmot2/sserial.c
            src/core/hal/drivers/mesa-hostmot2/ssr.c
            src/core/hal/drivers/mesa-hostmot2/stepgen.c
            src/core/hal/drivers/mesa-hostmot2/tp_pwmgen.c
            src/core/hal/drivers/mesa-hostmot2/tram.c
            src/core/hal/drivers/mesa-hostmot2/uart.c
            src/core/hal/drivers/mesa-hostmot2/watchdog.c
            src/core/hal/drivers/mesa-hostmot2/xy2mod.c
        )

        # Each bus (eth / pci / spi / 7i43 / 7i90) becomes a separate .so.
        # For each bus .c file, we also link the common hostmot2 sources.
        set(MESA_BUSSES
            hm2_eth
            hm2_pci
            hm2_7i43
            hm2_7i90
            hm2_spi
        )

        # Common (non-bus) sources are shared across all bus modules.
        set(MESA_COMMON_SOURCES ${MESA_SRC})
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_eth\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_pci\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_7i43\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_7i90\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_spi\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_test\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*kmod_check\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*hm2_modbus\\.c$")
        list(FILTER MESA_COMMON_SOURCES EXCLUDE REGEX ".*spix_.*\\.c$")

        # Build each bus module
        foreach(bus ${MESA_BUSSES})
            if(EXISTS ${CMAKE_SOURCE_DIR}/src/core/hal/drivers/mesa-hostmot2/${bus}.c)
                add_library(${bus} MODULE
                    src/core/hal/drivers/mesa-hostmot2/${bus}.c
                    ${MESA_COMMON_SOURCES}
                )
                lqt_add_common_includes(${bus})
                target_link_libraries(${bus} PRIVATE
                    $<$<TARGET_EXISTS:lqt_rtapi>:lqt_rtapi>
                    ${CMAKE_DL_LIBS}
                )
                set_target_properties(${bus} PROPERTIES
                    PREFIX ""
                    OUTPUT_NAME ${bus}
                    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/hal
                )
            endif()
        endforeach()
    endif()

    message(STATUS "HAL components: ${HAL_COMPONENT_SOURCES}")
endif()

# ============================================================================
# Module: libnml (NML Communication Library)
# ============================================================================
if(BUILD_LIBNML)
    # 公共源（不受平台影响）
    set(LIBNML_COMMON_SOURCES
        src/core/libnml/buffer/locmem.cc
        src/core/libnml/buffer/memsem.cc
        src/core/libnml/buffer/phantom.cc
        src/core/libnml/buffer/physmem.cc
        src/core/libnml/buffer/recvn.c
        src/core/libnml/buffer/shmem.cc
        src/core/libnml/buffer/tcpmem.cc
        src/core/libnml/buffer/sendn.c
        src/core/libnml/cms/cms.cc
        src/core/libnml/cms/cms_aup.cc
        src/core/libnml/cms/cms_cfg.cc
        src/core/libnml/cms/cms_dup.cc
        src/core/libnml/cms/cms_in.cc
        src/core/libnml/cms/cms_pm.cc
        src/core/libnml/cms/cms_srv.cc
        src/core/libnml/cms/cms_up.cc
        src/core/libnml/cms/tcp_opts.cc
        src/core/libnml/cms/tcp_srv.cc
        src/core/libnml/linklist/linklist.cc
        src/core/libnml/nml/cmd_msg.cc
        src/core/libnml/nml/nml.cc
        src/core/libnml/nml/nml_oi.cc
        src/core/libnml/nml/nml_srv.cc
        src/core/libnml/rcs/rcs_exit.cc
        src/core/libnml/rcs/rcs_print.cc
        src/core/libnml/os_intf/inetnull.cc
        src/core/libnml/os_intf/timer.cc
    )

    # os_intf 平台差异部分
    if(NOT LQTCNC_IS_WINDOWS)
        set(LIBNML_OS_SOURCES
            src/core/libnml/os_intf/_sem.c
            src/core/libnml/os_intf/_shm.c
            src/core/libnml/os_intf/_timer.c
            src/core/libnml/os_intf/shm.cc
            src/core/libnml/os_intf/sem.cc
        )
    else()
        set(LIBNML_OS_SOURCES
            src/core/posix_compat/posix_compat.c
        )
    endif()

    set(LIBNML_SOURCES ${LIBNML_COMMON_SOURCES} ${LIBNML_OS_SOURCES})

    add_library(lqt_nml SHARED ${LIBNML_SOURCES})
    lqt_add_common_includes(lqt_nml)
    target_link_libraries(lqt_nml PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:${CMAKE_DL_LIBS}>
    )
    if(LQTCNC_IS_WINDOWS)
        target_link_libraries(lqt_nml PRIVATE ws2_32)
    endif()
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
    target_link_libraries(lqt_posemath PRIVATE
        $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:m>
    )
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
        target_link_libraries(lqt_ini PRIVATE fmt::fmt ${CMAKE_DL_LIBS})
    else()
        target_link_libraries(lqt_ini PRIVATE ${CMAKE_DL_LIBS})
    endif()

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
    list(FILTER INTERP_SOURCES EXCLUDE REGEX ".*interp_python\\.cc$")
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
if(BUILD_TP AND NOT LQTCNC_IS_WINDOWS)
    file(GLOB TP_SOURCES
        src/core/emc/tp/*.c
        src/core/emc/tp/cruckig/*.c
    )

    add_library(lqt_tp STATIC ${TP_SOURCES})
    lqt_add_common_includes(lqt_tp)
    target_link_libraries(lqt_tp PRIVATE m)
    message(STATUS "tp: ${TP_SOURCES} files")
endif()

# ============================================================================
# Module: Motion Controller
# ============================================================================
if(BUILD_MOTION AND NOT LQTCNC_IS_WINDOWS)
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
#
# Each kinematics module exposes a standalone HAL module, compiled as a single shared MODULE.
# Helper source files (cubic.c / kins_util.c / genserfuncs.c / trtfuncs.c /
# userkfuncs.c) are compiled into a common static library and linked into every
# genserkins-family modules.
# ============================================================================
if(BUILD_KINEMATICS)
    # Common helper sources (not standalone modules; shared by multiple kinematics)
    set(KINS_COMMON_SOURCES
        src/core/emc/kinematics/cubic.c
        src/core/emc/kinematics/kins_util.c
        src/core/emc/kinematics/genserfuncs.c
        src/core/emc/kinematics/trtfuncs.c
        src/core/emc/kinematics/userkfuncs.c
    )
    add_library(lqt_kins_common STATIC ${KINS_COMMON_SOURCES})
    lqt_add_common_includes(lqt_kins_common)
    target_link_libraries(lqt_kins_common PRIVATE
        $<$<TARGET_EXISTS:lqt_posemath>:lqt_posemath>
        $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:m>
    )
    set_target_properties(lqt_kins_common PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # Standalone kinematics modules (each file = one .so)
    set(KINS_MODULE_SOURCES
        trivkins
        genserkins
        genhexkins
        pentakins
        pumakins
        maxkins
        5axiskins
        ugenserkins
        rosekins
        rotatekins
        scarakins
        scorbot-kins
        switchkins
        tripodkins
        corexykins
        rotarydeltakins
        lineardeltakins
        xyzac-trt-kins
        xyzbc-trt-kins
    )

    # Modules that depend on one of the helper sources above
    # (genserkins family uses cubic.c/genserfuncs.c/kins_util.c;
    #  xyz*ac/xyz*bc-trt-kins and scorbot-kins use trtfuncs.c;
    #  scorbot-kins additionally uses userkfuncs.c)
    set(KINS_WITH_COMMON
        genserkins genhexkins pentakins pumakins maxkins 5axiskins ugenserkins
        rosekins scorbot-kins tripodkins xyzac-trt-kins xyzbc-trt-kins
    )

    foreach(kins ${KINS_MODULE_SOURCES})
        # Strip trailing comments and validate file exists
        set(_src ${CMAKE_SOURCE_DIR}/src/core/emc/kinematics/${kins}.c)
        if(NOT EXISTS ${_src})
            message(WARNING "Kinematics source ${kins}.c not found, skipping")
            continue()
        endif()
        add_library(${kins} MODULE ${_src})
        lqt_add_common_includes(${kins})
        # link common helpers for modules that depend on them
        list(FIND KINS_WITH_COMMON ${kins} _idx)
        if(NOT _idx EQUAL -1)
            target_link_libraries(${kins} PRIVATE
                lqt_kins_common
                $<$<TARGET_EXISTS:lqt_posemath>:lqt_posemath>
                $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:m>
            )
        else()
            target_link_libraries(${kins} PRIVATE
                $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:m>
            )
        endif()
        set_target_properties(${kins} PROPERTIES
            PREFIX ""
            OUTPUT_NAME ${kins}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/kins
        )
    endforeach()
    message(STATUS "Kinematics modules: ${KINS_MODULE_SOURCES}")
endif()

# ============================================================================
# Module: Task Controller (library) + milltask (executable)
#
# The task controller's CANON/C++ core is built as a shared library so the
# Qt GUI / backend tools can link against it directly. The traditional
# milltask standalone executable links to lqt_task.
# ============================================================================
if(BUILD_TASK)
    # ---------- lqt_task (shared library - liblqt_task.so) ----------
    file(GLOB TASK_LIB_SOURCES
        src/core/emc/task/taskclass.cc
        src/core/emc/task/taskintf.cc
        src/core/emc/task/emctask.cc
        src/core/emc/task/emccanon.cc
        src/core/emc/task/backtrace.cc
        src/core/emc/task/signalhandler.cc
    )

    add_library(lqt_task SHARED ${TASK_LIB_SOURCES})
    lqt_add_common_includes(lqt_task)
    target_link_libraries(lqt_task PRIVATE
        $<$<TARGET_EXISTS:lqt_rs274>:lqt_rs274>
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        $<$<TARGET_EXISTS:lqt_nml>:lqt_nml>
        $<$<TARGET_EXISTS:lqt_posemath>:lqt_posemath>
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        $<$<TARGET_EXISTS:lqt_tooldata>:lqt_tooldata>
        ${CMAKE_DL_LIBS}
        $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:rt>
    )

    # ---------- lqt_milltask (executable - the main task process) ----------
    add_executable(lqt_milltask
        src/core/emc/task/emctaskmain.cc
        src/core/emc/task/emcsvr.cc
    )
    lqt_add_common_includes(lqt_milltask)
    target_link_libraries(lqt_milltask PRIVATE
        lqt_task
        $<$<TARGET_EXISTS:lqt_rs274>:lqt_rs274>
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        $<$<TARGET_EXISTS:lqt_nml>:lqt_nml>
        $<$<TARGET_EXISTS:lqt_posemath>:lqt_posemath>
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        $<$<TARGET_EXISTS:lqt_tooldata>:lqt_tooldata>
        ${CMAKE_DL_LIBS}
        $<$<NOT:$<BOOL:${LQTCNC_IS_WINDOWS}>>:rt>
    )

    message(STATUS "task: lqt_task.so (${TASK_LIB_SOURCES}) + lqt_milltask")
endif()

# ============================================================================
# Module: HALCMD (HAL Command-Line Tool)
#
# src/core/hal/utils/ contains several loosely-coupled tools:
#   halcmd      : the main command-line HAL shell (the classic "halcmd")
#   halsh       : a read-only shell for inspecting an already-running HAL
#   halrmt      : remote-HAL client tool
#   meter       : simple real-time signal meter
#   scope_*     : oscilloscope GUI (GTK-based; optional)
#   upci / pci_*: userspace PCI helpers (for mesa-hostmot2 / parport)
#   bitfile     : shared helper for FPGA bitfile handling
#   miscgtk     : shared GTK widget helpers for scope/meter
#
# P1: keep only halcmd / halsh / halrmt compiling. scope/meter/pci/miscgtk
# depend on GTK and are disabled at this stage (add a GTK cmake find_package
# to enable them later, together with the Qt GUI).
# ============================================================================
if(BUILD_HALCMD AND NOT LQTCNC_IS_WINDOWS)
    set(HALCMD_ROOT src/core/hal/utils)

    # ---------- lqt_halcmd (primary command-line tool) ----------
    set(HALCMD_SOURCES
        ${HALCMD_ROOT}/halcmd_main.c
        ${HALCMD_ROOT}/halcmd.c
        ${HALCMD_ROOT}/halcmd_commands.cc
        ${HALCMD_ROOT}/halcmd_completion.c
        ${HALCMD_ROOT}/bitfile.c
    )
    add_executable(lqt_halcmd ${HALCMD_SOURCES})
    lqt_add_common_includes(lqt_halcmd)
    target_link_libraries(lqt_halcmd PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        ${CMAKE_DL_LIBS}
        rt
    )

    # ---------- lqt_halsh (read-only shell) ----------
    add_executable(lqt_halsh
        ${HALCMD_ROOT}/halsh.c
    )
    lqt_add_common_includes(lqt_halsh)
    target_link_libraries(lqt_halsh PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        ${CMAKE_DL_LIBS}
        rt
    )

    # ---------- lqt_halrmt (remote HAL client) ----------
    add_executable(lqt_halrmt
        ${HALCMD_ROOT}/halrmt.c
    )
    lqt_add_common_includes(lqt_halrmt)
    target_link_libraries(lqt_halrmt PRIVATE
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        ${CMAKE_DL_LIBS}
        rt
    )

    message(STATUS "halcmd: ${HALCMD_SOURCES}")
    message(STATUS "halsh + halrmt (P1 enabled)")
    message(STATUS "  (scope/meter/pci/miscgtk disabled - require GTK; enable when Qt GUI is ready)")
endif()

# ============================================================================
# Module: Backend Tools (halui, emcsh, etc.)
#
# These programs communicate with the running task controller via NML
# and the HAL. GTK-based frontends (halui etc.) are currently skipped in
# favour of the future Qt GUI (see src/gui/). Only emcsh/emcrsh are kept
# because they are pure-C/text socket interfaces useful for scripting.
# ============================================================================
if(BUILD_BACKEND AND NOT LQTCNC_IS_WINDOWS)
    set(BACKEND_ROOT src/tools/backend)

    # emcsh: scriptable NML shell
    add_executable(lqt_emcsh
        ${BACKEND_ROOT}/emcsh.cc
        ${BACKEND_ROOT}/shcom.cc
        ${BACKEND_ROOT}/sockets.c
    )
    lqt_add_common_includes(lqt_emcsh)
    target_link_libraries(lqt_emcsh PRIVATE
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        $<$<TARGET_EXISTS:lqt_nml>:lqt_nml>
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        $<$<TARGET_EXISTS:lqt_tooldata>:lqt_tooldata>
        ${CMAKE_DL_LIBS}
    )

    # emcrsh: remote NML server (similar to emcsh but listens on a TCP socket)
    add_executable(lqt_emcrsh
        ${BACKEND_ROOT}/emcrsh.cc
        ${BACKEND_ROOT}/shcom.cc
        ${BACKEND_ROOT}/sockets.c
        ${BACKEND_ROOT}/schedrmt.cc
        ${BACKEND_ROOT}/emcsched.cc
    )
    lqt_add_common_includes(lqt_emcrsh)
    target_link_libraries(lqt_emcrsh PRIVATE
        $<$<TARGET_EXISTS:lqt_emc_core>:lqt_emc_core>
        $<$<TARGET_EXISTS:lqt_nml>:lqt_nml>
        $<$<TARGET_EXISTS:lqt_hal>:lqt_hal>
        $<$<TARGET_EXISTS:lqt_ini>:lqt_ini>
        $<$<TARGET_EXISTS:lqt_tooldata>:lqt_tooldata>
        ${CMAKE_DL_LIBS}
    )

    message(STATUS "backend tools: emcsh + emcrsh")
    message(STATUS "  (halui/emclcd/mapini/emcsched: GTK based; disabled until Qt GUI ready)")
endif()

# ============================================================================
# Module: Qt5 GUI Application
#
# The GUI is a thin C++ wrapper around the linuxCNC core libraries (NML/HAL/
# motion/task).  On Windows it is the PRIMARY deliverable; on Linux it
# supplements the headless CLI tools.
# ============================================================================
if(BUILD_GUI)
    # Tell gui/CMakeLists.txt that we are being included from the root,
    # so it uses the correct relative path to src/core.
    set(LQTCNC_ROOT_BUILD ON)
    add_subdirectory(src/gui)

    # On Windows, also try to pick up Qt5 from the standard install location.
    # (CMAKE_PREFIX_PATH propagates automatically into subdirectories;
    #  we additionally append the Windows-specific fallback path.)
    if(LQTCNC_IS_WINDOWS)
        list(APPEND CMAKE_PREFIX_PATH
             "D:/Qt/Qt5.12.3/5.12.3/msvc2017"
             "D:/Qt/Qt5.12.3/5.12.3/mingw73_64")
    endif()
endif()
# ============================================================================
# Platform string (cmake 3.12-compatible: use if-else, not $<IF:>)
# ============================================================================
if(LQTCNC_IS_WINDOWS)
    set(LQTCNC_PLATFORM_STR "Windows (minimal)")
else()
    set(LQTCNC_PLATFORM_STR "Linux (full)")
endif()

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
message(STATUS "  BUILD_GUI:      ${BUILD_GUI}")
message(STATUS "  Platform:       ${LQTCNC_PLATFORM_STR}")
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
