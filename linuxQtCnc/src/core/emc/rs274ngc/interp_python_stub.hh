/********************************************************************
* Description: interp_python_stub.hh
*   Python Interpreter Interface for linuxQtCnc
*
*   This file provides the Python interpreter interface that was
*   originally used for remap handlers and callbacks in LinuxCNC.
*
*   IMPORTANT: In linuxQtCnc, all Python interpreter support is
*   PERMANENTLY DISABLED. The PYUSABLE macro is always false.
*
*   Python-based remap handlers (prolog/epilog) are replaced with
*   pure C++ callbacks via the RemapCallbackRegistry system.
*   See remap_callback.hh for the C++ replacement.
*
*   Python Remap Replacement Strategy:
*   - Instead of: REMAP=M420 python=myhandler.prolog
*   - Use: Register C++ handler via RemapCallbackRegistry::instance().registerHandler()
*
*   Original Python Features and C++ Equivalents:
*   - Python remap prologs  -> C++ RemapHandlerFunc or PrologFunc
*   - Python remap epilogs  -> C++ EpilogFunc
*   - Python remap bodies   -> C++ BodyFunc
*   - Python (py,...)       -> Not supported (use C++ direct calls)
*   - Python plugin module   -> C++ plugin registry (future)
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef INTERP_PYTHON_STUB_HH
#define INTERP_PYTHON_STUB_HH

#include "interp_fwd.hh"
#include "remap_callback.hh"  // C++ remap callback system

// ========================================================================
// Python Interpreter Status
// ========================================================================

// PYUSABLE is always false in linuxQtCnc - Python is permanently disabled
// Any code checking PYUSABLE will use the C++ fallback path
#define PYUSABLE (false)

// Python call types - preserved for API compatibility
// These were used in the original LinuxCNC Python remap system
enum py_calltype {
    PY_OWORDCALL,           // O-word subroutine call
    PY_FINISH_OWORDCALL,    // Finish O-word call
    PY_PROLOG,              // Remap prolog function
    PY_FINISH_PROLOG,       // Finish prolog
    PY_BODY,                // Remap body function
    PY_FINISH_BODY,         // Finish body
    PY_EPILOG,              // Remap epilog function
    PY_FINISH_EPILOG,       // Finish epilog
    PY_INTERNAL,            // Internal Python call
    PY_EXECUTE,             // Execute Python code
    PY_PLUGIN_CALL          // Plugin call
};

// ========================================================================
// Python Context (Stub)
// ========================================================================

// In the original LinuxCNC, pycontext held Python interpreter state.
// In linuxQtCnc, this is a minimal stub since Python is disabled.
struct pycontext_impl {};

struct pycontext {
    pycontext();
    pycontext(const pycontext& other);
    pycontext& operator=(const pycontext& other);
    ~pycontext();

    pycontext_impl* impl;
};

// ========================================================================
// C++ Callback Integration
// ========================================================================

namespace RemapCallbacks {

// Type alias for the C++ remap callback system
// This is the primary replacement for Python callbacks in linuxQtCnc
using CppRemapHandler = RemapHandler;
using CppRemapParams = RemapParams;
using CppRemapPhase = Phase;

// Helper to get the C++ remap registry from within the interpreter
inline RemapCallbackRegistry& getRemapRegistry() {
    return RemapCallbackRegistry::instance();
}

} // namespace RemapCallbacks

// ========================================================================
// Interp Python Interface (Stubs with C++ Fallback)
// ========================================================================

class Interp {
public:
    // Python callback dispatcher
    // In linuxQtCnc: Always returns INTERP_ERROR with a clear message
    // Use C++ remap callbacks via RemapCallbackRegistry instead
    int pycall(setup_pointer settings,
               context_pointer frame,
               const char* module,
               const char* funcname,
               int calltype);

    // Execute Python snippet
    // In linuxQtCnc: Not supported
    int py_execute(const char* cmd, bool as_file);

    // Reload Python modules
    // In linuxQtCnc: No-op (Python not available)
    int py_reload();

    // Check if a name is a Python callable
    // In linuxQtCnc: Always returns false
    static bool is_pycallable(setup_pointer settings, const char* module, const char* name);
};

// Check if a Python handler exists for a module/function
// In linuxQtCnc: Always returns false
bool is_pycallable(setup_pointer settings, const char* module, const char* name);

// ========================================================================
// Python Error Reporting (Stubs)
// ========================================================================

// Python error handling was used for remap failures
// In linuxQtCnc: Use the C++ remap callback error reporting instead
class PythonError : public std::runtime_error {
public:
    explicit PythonError(const std::string& msg) : std::runtime_error(msg) {}
};

// ========================================================================
// Backward Compatibility Macros
// ========================================================================

// These macros were used in the original LinuxCNC for Python remap checks
// In linuxQtCnc: Always use the C++ remap callback system

// Check if Python should be used for a remap
#define USE_PYTHON_REMAP(prolog_func, remap_py, epilog_func) \
    (false)  // Always false in linuxQtCnc

// Check if any Python handler is present
#define HAS_PYTHON_HANDLER(rptr) \
    (false)  // Always false in linuxQtCnc

// Python module name for remaps
#define REMAP_MODULE "remap"

#endif // INTERP_PYTHON_STUB_HH
