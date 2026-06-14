/********************************************************************
 * Description: interp_python_stubs.cc
 *   Stub implementations of Python interpreter callback functions
 *   In linuxQtCnc, all Python interpreter support is disabled
 *   (no Boost.Python, no PyCall mechanism). These stubs provide
 *   safe fallback implementations so the linker finds the symbols
 *   referenced by interp_o_word.cc / interp_remap.cc / rs274ngc_pre.cc.
 *
 * Author: linuxQtCnc project
 * License: GPL Version 2 (GPL)
 * Copyright (c) 2025 All rights reserved.
 ********************************************************************/

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "interp_internal.hh"
#include "pythonplugin/python_plugin_stub.hh"

#include <cstdio>
#include <string>

// ========================================================================
// pycontext stub implementations
//   In the original LinuxCNC, pycontext holds Python interpreter state.
//   In linuxQtCnc, Python is disabled, so these are empty stubs.
// ========================================================================

struct pycontext_impl {};

pycontext::pycontext() : impl(new pycontext_impl) {}
pycontext::pycontext(const struct pycontext &other) : impl(new pycontext_impl(*other.impl)) {}
pycontext &pycontext::operator=(const struct pycontext &) { return *this; }
pycontext::~pycontext() { delete impl; }

// ========================================================================
// Interp::pycall() - Python callback dispatcher
//   In linuxQtCnc, Python callbacks are disabled. This function
//   is retained only to satisfy the linker and provide a clear
//   runtime error message.
// ========================================================================
int Interp::pycall(setup_pointer settings,
                    context_pointer frame,
                    const char *module,
                    const char *funcname,
                    int calltype)
{
    (void)settings;
    (void)frame;
    // report Python callback disabled - returning INTERP_ERROR with a clear error.
    Error("Python callback disabled: %s.%s (calltype=%d) - Python plugin not available in linuxQtCnc",
         module, funcname, calltype);
    return INTERP_ERROR;
}

// ========================================================================
// Interp::py_execute() - execute arbitrary Python snippet
// ========================================================================
int Interp::py_execute(const char *cmd, bool as_file)
{
    (void)cmd;
    (void)as_file;
    Error("Python (py,...) comments require Python interpreter, which is disabled in linuxQtCnc");
    return INTERP_ERROR;
}

// ========================================================================
// Interp::py_reload() - reload Python modules
// ========================================================================
int Interp::py_reload()
{
    Error("Python plugin reload() called in linuxQtCnc - disabled at compile time");
    return INTERP_ERROR;
}

// ========================================================================
// handler_returned() - check a Python subroutine result
//   Used in linuxQtCnc to detect whether interpreter's return value.
//   Since Python calls are disabled, this function always returns
//   INTERP_ERROR.
// ========================================================================
int handler_returned(setup_pointer settings,
                     context_pointer frame,
                     const char *subr_name,
                     bool handle_do_run)
{
    (void)settings;
    (void)frame;
    (void)subr_name;
    (void)handle_do_run;
    // no Python callbacks - always indicate error
    return INTERP_ERROR;
}

// ========================================================================
// is_pycallable() - check if a name is a Python callable
// ========================================================================
bool is_pycallable(setup_pointer settings, const char *module, const char *name)
{
    (void)settings;
    (void)module;
    (void)name;
    return false;
}
