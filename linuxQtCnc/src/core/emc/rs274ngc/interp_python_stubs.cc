/********************************************************************
 * Description: interp_python_stubs.cc
 *   Python Interpreter Stubs Implementation for linuxQtCnc
 *
 *   IMPORTANT: In linuxQtCnc, all Python interpreter support is
 *   PERMANENTLY DISABLED. This file provides stub implementations
 *   that either:
 *   1. Return error/false to indicate Python is unavailable
 *   2. Delegate to the C++ RemapCallbackRegistry for remap handlers
 *
 *   For remap handlers, use the C++ callback system instead:
 *   - RemapCallbackRegistry::instance().registerHandler()
 *   - See remap_callback.hh for the C++ API
 *
 *   Derived from a work by Fred Proctor & Will Shackleford
 *
 * Author: linuxQtCnc project
 * License: GPL Version 2
 * System: Linux / Windows
 *
 * Copyright (c) 2026 All rights reserved.
 ********************************************************************/

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "interp_internal.hh"
#include "interp_python_stub.hh"
#include "remap_callback.hh"

#include <cstdio>
#include <cstring>
#include <string>

// ========================================================================
// pycontext Implementation
// ========================================================================

// Minimal stub - no Python interpreter state needed
pycontext::pycontext() : impl(new pycontext_impl) {}
pycontext::pycontext(const struct pycontext &other) : impl(new pycontext_impl(*other.impl)) {}
pycontext& pycontext::operator=(const struct pycontext &) { return *this; }
pycontext::~pycontext() { delete impl; }

// ========================================================================
// Interp::pycall() - Python Callback Dispatcher
// ========================================================================

int Interp::pycall(setup_pointer /*settings*/,
                    context_pointer /*frame*/,
                    const char *module,
                    const char *funcname,
                    int calltype)
{
    // Python is permanently disabled in linuxQtCnc
    // This function is kept for API compatibility
    // For C++ remap handlers, use RemapCallbackRegistry::instance()

    Error("Python callback called but Python is disabled in linuxQtCnc: "
          "%s.%s (calltype=%d). "
          "Use C++ RemapCallbackRegistry for remap handlers instead.",
          module, funcname, calltype);

    return INTERP_ERROR;
}

// ========================================================================
// Interp::py_execute() - Execute Python Snippet
// ========================================================================

int Interp::py_execute(const char *cmd, bool as_file)
{
    (void)cmd;
    (void)as_file;

    // Python (py,...) comments are not supported in linuxQtCnc
    Error("Python (py,...) comments require Python interpreter, "
          "which is permanently disabled in linuxQtCnc. "
          "Use C++ direct calls or NGC subroutines instead.");

    return INTERP_ERROR;
}

// ========================================================================
// Interp::py_reload() - Reload Python Modules
// ========================================================================

int Interp::py_reload()
{
    // No-op in linuxQtCnc - Python modules don't exist
    Info("Python plugin reload() called but Python is disabled in linuxQtCnc");
    return 0;  // Return success (nothing to do)
}

// ========================================================================
// Interp::is_pycallable() - Check if Callable
// ========================================================================

bool Interp::is_pycallable(setup_pointer /*settings*/,
                            const char *module,
                            const char *name)
{
    (void)module;
    (void)name;

    // Python is never available in linuxQtCnc
    return false;
}

// ========================================================================
// is_pycallable() - Global Function
// ========================================================================

bool is_pycallable(setup_pointer settings, const char *module, const char *name)
{
    return Interp::is_pycallable(settings, module, name);
}

// ========================================================================
// C++ Remap Callback Integration
// ========================================================================

// This function bridges the C++ remap callback system with the interpreter
// It is called from interp_remap.cc when executing remapped codes
namespace {
    int execute_cpp_remap(
        Interp* interp,
        block_struct* block,
        setup_pointer settings,
        RemapCallbacks::RemapParams& params,
        const char* code
    ) {
        using namespace RemapCallbacks;

        auto& registry = RemapCallbackRegistry::instance();
        RemapHandler* handler = registry.getHandler(code);

        if (!handler) {
            // No C++ handler registered - might be NGC file based
            return 0;  // Let interp handle NGC file
        }

        // Execute C++ remap handler
        return registry.executeRemap(interp, block, params, code);
    }
}

// ========================================================================
// Python Remap Parameter Extraction
// ========================================================================

// Extract parameters from G-code block for remap handlers
// This replaces the Python kwargs dict that was used in original LinuxCNC
namespace RemapCallbacks {

void extractBlockParams(block_struct* block, RemapParams& params) {
    // Clear existing params
    params.clear();

    // Extract all G-code words present in the block
    if (block->a_flag) { params.a = block->a_number; params.a_flag = true; }
    if (block->b_flag) { params.b = block->b_number; params.b_flag = true; }
    if (block->c_flag) { params.c = block->c_number; params.c_flag = true; }
    if (block->d_flag) { params.d = block->d_number_float; params.d_flag = true; }
    if (block->e_flag) { params.e = block->e_number; params.e_flag = true; }
    if (block->f_flag) { params.f = block->f_number; params.f_flag = true; }
    if (block->h_flag) { params.h = block->h_number; params.h_flag = true; }
    if (block->i_flag) { params.i = block->i_number; params.i_flag = true; }
    if (block->j_flag) { params.j = block->j_number; params.j_flag = true; }
    if (block->k_flag) { params.k = block->k_number; params.k_flag = true; }
    if (block->l_flag) { params.l = block->l_number; params.l_flag = true; }
    if (block->p_flag) { params.p = block->p_number; params.p_flag = true; }
    if (block->q_flag) { params.q = block->q_number; params.q_flag = true; }
    if (block->r_flag) { params.r = block->r_number; params.r_flag = true; }
    if (block->s_flag) { params.s = block->s_number; params.s_flag = true; }
    if (block->t_flag) { params.t = block->t_number; params.t_flag = true; }
    if (block->u_flag) { params.u = block->u_number; params.u_flag = true; }
    if (block->v_flag) { params.v = block->v_number; params.v_flag = true; }
    if (block->w_flag) { params.w = block->w_number; params.w_flag = true; }
    if (block->x_flag) { params.x = block->x_number; params.x_flag = true; }
    if (block->y_flag) { params.y = block->y_number; params.y_flag = true; }
    if (block->z_flag) { params.z = block->z_number; params.z_flag = true; }
}

} // namespace RemapCallbacks

// ========================================================================
// Handler Status Check
// ========================================================================

// Check Python subroutine result
// In linuxQtCnc: C++ remap handlers return status directly
// This function always returns INTERP_ERROR since Python is disabled
int handler_returned(setup_pointer /*settings*/,
                     context_pointer /*frame*/,
                     const char *subr_name,
                     bool handle_do_run)
{
    (void)subr_name;
    (void)handle_do_run;

    // Python is disabled - no handler to check
    // For C++ handlers, status is returned directly from executeRemap()
    return INTERP_ERROR;
}

// ========================================================================
// Initialization
// ========================================================================

// Register built-in C++ remap handlers at startup
// This is called from the interpreter initialization
namespace {
    struct BuiltInRemapRegistrar {
        BuiltInRemapRegistrar() {
            // Register default C++ remap handlers
            // These can be overridden by user code if needed
            RemapCallbacks::BuiltInHandlers::registerBuiltInHandlers();
        }
    };

    static BuiltInRemapRegistrar g_builtin_registrar;
}

