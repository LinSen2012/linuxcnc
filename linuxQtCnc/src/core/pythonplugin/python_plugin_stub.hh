#ifndef PYTHON_PLUGIN_STUB_HH
#define PYTHON_PLUGIN_STUB_HH

// Stub header to replace pythonplugin/python_plugin.hh
// Python plugin system is disabled in linuxQtCnc
// All plugin operations are no-ops

#include <string>

class PythonPlugin {
public:
    PythonPlugin() = default;
    ~PythonPlugin() = default;

    bool usable() const { return false; }
    bool init(const char *progname) { return false; }
    void finalize() {}
};

// Global singleton (null in linuxQtCnc)
extern PythonPlugin *python_plugin;

#endif // PYTHON_PLUGIN_STUB_HH
