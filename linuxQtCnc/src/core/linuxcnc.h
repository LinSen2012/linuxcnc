/********************************************************************
* Description: linuxcnc.h
*   Common defines used in many linuxQtCnc source files.
*   Pure C/C++ — no Python, no Boost.Python.
*
* Author: linuxQtCnc project (originally Petter Reinholdtsen, LinuxCNC)
* License: LGPL Version 2
* System: Cross-platform (Linux / Windows)
*
********************************************************************/

#ifndef __LINUXCNC_LINUXCNC_H
#define __LINUXCNC_LINUXCNC_H

/* =====================================================================
 * 平台识别
 * ===================================================================== */

/* 在 Windows 上：自动引入 posix_compat 层，提供 mmap/dlopen/shm 等
 * 在 Linux 上：什么都不做，使用系统原生 POSIX API
 *
 * 注意：posix_compat.h 通过宏重命名来做 API，
 *       必须在所有其他系统头之前（或在包含任何使用 POSIX API 的代码文件首行）。
 * ===================================================================== */

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64)
#include "posix_compat/posix_compat.h"
#endif

/* =====================================================================
 * 跨平台基础宏
 * ===================================================================== */

/* LINELEN 用于各种缓冲区大小、文件名长度等，统一一个常量 */
#define LINELEN 255

/* sprintf 等通用缓冲区长度 */
#define BUFFERLEN 80

/* 英制 / 公制单位换算 */
#define MM_PER_INCH 25.4
#define INCH_PER_MM (1.0 / MM_PER_INCH)

/* =====================================================================
 * Python/Boost 禁用声明
 *
 * 在 linuxQtCnc 中完全去除了所有 Python/Boost.Python 绑定。
 * 所有 Python plugin 均由 C++ stub：
 *   - #include "pythonplugin/python_plugin_stub.hh"
 *     （在需要 Python plugin 时包含）
 *
 * 若源文件出现以下任一内容，说明它来自未完成的迁移：
 *   #include <Python.h>             → 应该删除
 *   #include <boost/python.hpp>       → 应该删除
 *   #include "interp_python.hh"        → 已替换为 stub
 * ===================================================================== */

#endif /* __LINUXCNC_LINUXCNC_H */
