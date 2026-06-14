/********************************************************************
* Description: python_plugin_stub.hh
*   Stub header to replace pythonplugin/python_plugin.hh
*   Python plugin system is disabled in linuxQtCnc (pure C/C++ CNC
*   controller). The interpreter is 100% C++ (no Boost.Python, no
*   PyCall). All plugin operations are no-ops.
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef PYTHON_PLUGIN_STUB_HH
#define PYTHON_PLUGIN_STUB_HH

#include <string>

// Forward declaration (not actually used by interpreter in linuxQtCnc;
// kept to make existing type-check macros compile).
class Interp;

// Plugin status codes (redefined from original plugin.hh, which was not migrated)
enum plugin_status_t {
    PLUGIN_OK = 0,
    PLUGIN_EXCEPTION = 1
};

// Module identifiers (needed by CHKS() macros in existing C++ code paths).
// These are intentionally opaque void pointer constants.
#define NAMEDPARAMS_MODULE ((void *)0x10)
#define INTERP_MODULE       ((void *)0x20)

// Dummy placeholder for boost::python object / list / dict types.
// Not actually used at runtime in linuxQtCnc (all Python code paths deleted).
struct dummy_pyobject {
    void *ptr = nullptr;
};
using dummy_bp_object = dummy_pyobject;  // alias for bp::object

// ============================================================================
// PythonPlugin — stub class (replaces full Python plugin)
// ============================================================================
class PythonPlugin {
public:
    PythonPlugin() = default;
    ~PythonPlugin() = default;

    // Always false in linuxQtCnc: Python path not available
    bool usable() const { return false; }

    // Return PLUGIN_OK: stub never raises exceptions
    int plugin_status() const { return PLUGIN_OK; }

    // Returns empty string: stub never has an exception message
    std::string last_exception() const { return std::string(); }

    // No-op stub: call() is only invoked in deleted Python paths
    void call(void * /*module*/, const char * /*name*/,
              const dummy_pyobject & /*tupleargs*/,
              const dummy_pyobject & /*kwargs*/,
              dummy_pyobject & /*retval*/) {}

    // init() / finalize() are called in taskclass.cc initialization
    bool init(const char * /*progname*/) { return false; }
    void finalize() {}
};

// ============================================================================
// The python_plugin global.
//
// The original code uses:   extern PythonPlugin *python_plugin;
//                        #define PYUSABLE (...)
//
// In linuxQtCnc we DEFINE it here (as an inline static — guaranteed to be
// a single global with C++17 semantics), so any .cc file including this
// header sees the same pointer. No linker errors.
// ============================================================================

// C++17 inline variable → single storage across all translation units
inline PythonPlugin *python_plugin = nullptr;

// PYUSABLE evaluates to false because python_plugin is always nullptr:
//   (#define is intentionally kept, as existing code uses it via
//    interp_internal.hh, which in turn references `python_plugin`).
// (interp_internal.hh's own definition of PYUSABLE references `python_plugin`
//  as well — both need to see the same global).
//
// We intentionally do NOT redefine PYUSABLE here. It is redefined in
// interp_internal.hh, and that definition will refer to the inline global
// above.

#endif // PYTHON_PLUGIN_STUB_HH
