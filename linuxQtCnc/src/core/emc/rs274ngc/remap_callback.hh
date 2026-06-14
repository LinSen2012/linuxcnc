/********************************************************************
* Description: remap_callback.hh
*   C++ Remap Callback Registry for linuxQtCnc
*
*   This file provides a pure C++ replacement for the Python-based
*   remap mechanism in LinuxCNC. Instead of using Boost.Python to
*   call Python functions, this system uses C++ function pointers
*   and std::function callbacks.
*
*   Key concepts:
*   - Remap handlers: C++ functions registered to handle remapped G/M codes
*   - Prolog functions: Called before the main handler to setup parameters
*   - Epilog functions: Called after the main handler to cleanup/teardown
*   - The remap system allows users to override built-in G/M codes with
*     custom implementations via INI file configuration
*
*   Usage:
*   1. Register C++ handlers using RemapCallbackRegistry::instance().registerHandler()
*   2. Configure remaps in INI file: REMAP= M420 modalgroup=6 argspec=pq prolog=setnamedvars ngc=m43.ngc
*   3. Interp will call the registered handlers when the remapped code is executed
*
*   Derived from a work by Michael Haberler, Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef REMAP_CALLBACK_HH
#define REMAP_CALLBACK_HH

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include <variant>

// Forward declarations
struct Interp;
struct block_struct;
struct setup_struct;
struct context_struct;

// ========================================================================
// Remap Callback Types
// ========================================================================

namespace RemapCallbacks {

// Remap handler phase
enum class Phase {
    PROLOG,    // Setup phase - called before handler
    BODY,      // Main handler execution
    EPILOG     // Cleanup phase - called after handler
};

// Parameter container for remap handlers
// Stores G-code word parameters (X, Y, Z, etc.) and special parameters
struct RemapParams {
    // Standard G-code word parameters
    double a = 0.0, b = 0.0, c = 0.0;
    double d = 0.0, e = 0.0, f = 0.0;
    double h = 0.0;
    double i = 0.0, j = 0.0, k = 0.0;
    double l = 0.0;
    double p = 0.0, q = 0.0, r = 0.0;
    double s = 0.0;
    double t = 0.0;  // Tool number
    double u = 0.0, v = 0.0, w = 0.0;
    double x = 0.0, y = 0.0, z = 0.0;

    // Parameter presence flags
    bool a_flag = false, b_flag = false, c_flag = false;
    bool d_flag = false, e_flag = false, f_flag = false;
    bool h_flag = false;
    bool i_flag = false, j_flag = false, k_flag = false;
    bool l_flag = false;
    bool p_flag = false, q_flag = false, r_flag = false;
    bool s_flag = false;
    bool t_flag = false;
    bool u_flag = false, v_flag = false, w_flag = false;
    bool x_flag = false, y_flag = false, z_flag = false;

    // Special parameters
    double feed_rate = 0.0;      // Current feed rate (for '>' check)
    double spindle_speed = 0.0;  // Current spindle speed (for '^' check)
    int line_number = 0;         // Current line number (for 'N' parameter)
    int call_level = 0;          // Subroutine call level
    int param_cnt = 0;           // Number of positional parameters

    // Positional arguments (for '@' argspec)
    std::vector<double> positional_args;

    // Named parameters (key-value pairs)
    std::map<std::string, double> named_params;

    // Clear all parameters
    void clear() {
        a = b = c = d = e = f = h = i = j = k = l = 0.0;
        p = q = r = s = t = u = v = w = x = y = z = 0.0;
        feed_rate = spindle_speed = 0.0;
        line_number = call_level = param_cnt = 0;
        a_flag = b_flag = c_flag = d_flag = e_flag = f_flag = false;
        h_flag = i_flag = j_flag = k_flag = l_flag = false;
        p_flag = q_flag = r_flag = s_flag = t_flag = false;
        u_flag = v_flag = w_flag = x_flag = y_flag = z_flag = false;
        positional_args.clear();
        named_params.clear();
    }
};

// Remap handler function signature
// Parameters:
//   - interp: Pointer to the interpreter
//   - block: Current G-code block
//   - params: Extracted parameters
//   - phase: Current execution phase
// Returns: 0 on success, negative on error
using RemapHandlerFunc = std::function<int(
    Interp* interp,
    block_struct* block,
    RemapParams& params,
    Phase phase
)>;

// Prolog handler - prepares parameters before main handler
// Returns: 0 on success, negative on error
using PrologFunc = std::function<int(
    Interp* interp,
    block_struct* block,
    RemapParams& params
)>;

// Body handler - main remap logic
// Returns: 0 on success, negative on error
using BodyFunc = std::function<int(
    Interp* interp,
    RemapParams& params
)>;

// Epilog handler - cleanup after main handler
// Returns: 0 on success, negative on error
using EpilogFunc = std::function<int(
    Interp* interp,
    RemapParams& params,
    int body_status  // Return value from body handler
)>;

// Complete remap handler container
struct RemapHandler {
    std::string name;              // Remap code name (e.g., "M420")
    std::string argspec;           // Argument specification (e.g., "pq", "xyz")
    int modal_group = -1;          // Modal group for M-codes
    int motion_code = 0;           // Motion code for G-codes
    std::string ngc_file;          // NGC file to execute (optional)
    bool has_cpp_handler = false;  // Whether a C++ handler is registered

    // C++ callback functions
    PrologFunc prolog;
    BodyFunc body;
    EpilogFunc epilog;

    // Combined handler (if all phases are in one function)
    RemapHandlerFunc combined;

    RemapHandler() = default;
    RemapHandler(const std::string& n) : name(n) {}

    bool hasProlog() const { return prolog != nullptr || combined != nullptr; }
    bool hasBody() const { return body != nullptr || combined != nullptr; }
    bool hasEpilog() const { return epilog != nullptr; }
};

// ========================================================================
// Remap Callback Registry (Singleton)
// ========================================================================

class RemapCallbackRegistry {
public:
    // Get singleton instance
    static RemapCallbackRegistry& instance() {
        static RemapCallbackRegistry registry;
        return registry;
    }

    // Disable copy
    RemapCallbackRegistry(const RemapCallbackRegistry&) = delete;
    RemapCallbackRegistry& operator=(const RemapCallbackRegistry&) = delete;

    // ---- Handler Registration ----

    // Register a complete remap handler with prolog/body/epilog
    bool registerHandler(
        const std::string& code,
        const std::string& argspec,
        PrologFunc prolog,
        BodyFunc body,
        EpilogFunc epilog = nullptr
    );

    // Register a combined handler (all phases in one function)
    bool registerHandler(
        const std::string& code,
        const std::string& argspec,
        RemapHandlerFunc handler
    );

    // Register handler by name
    bool registerHandler(const std::string& code, RemapHandler handler);

    // Unregister a handler
    bool unregisterHandler(const std::string& code);

    // ---- Handler Lookup ----

    // Get handler by code name
    RemapHandler* getHandler(const std::string& code);
    const RemapHandler* getHandler(const std::string& code) const;

    // Check if handler exists
    bool hasHandler(const std::string& code) const;

    // Get all registered handlers
    std::vector<std::string> getRegisteredHandlers() const;

    // ---- Handler Execution ----

    // Execute prolog phase
    int executeProlog(
        Interp* interp,
        block_struct* block,
        RemapParams& params,
        const std::string& code
    );

    // Execute body phase
    int executeBody(
        Interp* interp,
        RemapParams& params,
        const std::string& code
    );

    // Execute epilog phase
    int executeEpilog(
        Interp* interp,
        RemapParams& params,
        const std::string& code,
        int body_status
    );

    // Execute complete remap (all phases)
    int executeRemap(
        Interp* interp,
        block_struct* block,
        RemapParams& params,
        const std::string& code
    );

    // ---- Utility ----

    // Clear all handlers
    void clear();

    // Get handler count
    size_t handlerCount() const { return m_handlers.size(); }

    // Set logging level (0=off, 1=error, 2=info, 3=debug)
    void setLogLevel(int level) { m_log_level = level; }
    int logLevel() const { return m_log_level; }

private:
    RemapCallbackRegistry() = default;

    std::map<std::string, RemapHandler> m_handlers;
    int m_log_level = 1;  // Default: error messages only
};

// ========================================================================
// Built-in C++ Remap Handlers
// ========================================================================

namespace BuiltInHandlers {

// M6 - Tool Change (example implementation)
int M6_prolog(Interp* interp, block_struct* block, RemapParams& params);
int M6_body(Interp* interp, RemapParams& params);
int M6_epilog(Interp* interp, RemapParams& params, int body_status);

// M61 - Set Tool Number (example implementation)
int M61_prolog(Interp* interp, block_struct* block, RemapParams& params);
int M61_body(Interp* interp, RemapParams& params);

// Register all built-in handlers
void registerBuiltInHandlers();

} // namespace BuiltInHandlers

// ========================================================================
// INI File Remap Configuration Parser
// ========================================================================

// Parse REMAP= INI line and register handler
// Format: REMAP=<code> modalgroup=<n> argspec=<spec> prolog=<func> body=<func> epilog=<func> ngc=<file>
// Returns: 0 on success, negative on error
int parseRemapIniLine(const char* inistring, int lineno);

// ========================================================================
// Inline helper functions
// ========================================================================

// Check if a parameter is required (uppercase in argspec)
inline bool isRequiredParam(char spec, const std::string& argspec) {
    return argspec.find(toupper(spec)) != std::string::npos;
}

// Check if a parameter is optional (lowercase in argspec)
inline bool isOptionalParam(char spec, const std::string& argspec) {
    return argspec.find(tolower(spec)) != std::string::npos;
}

// Check if feed rate is required ('>' in argspec)
inline bool requiresPositiveFeed(const std::string& argspec) {
    return argspec.find('>') != std::string::npos;
}

// Check if spindle speed is required ('^' in argspec)
inline bool requiresPositiveSpeed(const std::string& argspec) {
    return argspec.find('^') != std::string::npos;
}

// Check if line number should be added ('N' in argspec)
inline bool includesLineNumber(const std::string& argspec) {
    return argspec.find('N') != std::string::npos;
}

// Check if positional arguments should be used ('@' in argspec)
inline bool usesPositionalArgs(const std::string& argspec) {
    return argspec.find('@') != std::string::npos;
}

} // namespace RemapCallbacks

#endif // REMAP_CALLBACK_HH
