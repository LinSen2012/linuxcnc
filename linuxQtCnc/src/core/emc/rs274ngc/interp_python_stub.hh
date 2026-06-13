#ifndef INTERP_PYTHON_STUB_HH
#define INTERP_PYTHON_STUB_HH

// Stub header to replace interp_python.hh
// Python callbacks are replaced with pure C++ virtual function interfaces
// This is a transitional stub - the actual callback mechanism
// will be implemented as C++ interfaces in Phase 2

#include "interp_fwd.hh"

// Python call type enum (preserved for API compatibility)
enum py_calltype {
    PY_OWORDCALL,
    PY_FINISH_OWORDCALL,
    PY_PROLOG,
    PY_FINISH_PROLOG,
    PY_BODY,
    PY_FINISH_BODY,
    PY_EPILOG,
    PY_FINISH_EPILOG,
    PY_INTERNAL,
    PY_EXECUTE,
    PY_PLUGIN_CALL
};

// Stub: Python plugin is disabled in linuxQtCnc
// All pycall/is_pycallable functions return failure/disabled
#define PYUSABLE (false)

#endif // INTERP_PYTHON_STUB_HH
