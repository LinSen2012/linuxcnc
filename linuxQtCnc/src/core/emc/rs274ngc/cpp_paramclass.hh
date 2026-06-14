/********************************************************************
* Description: cpp_paramclass.hh
*   C++ Parameter Access Class for linuxQtCnc
*
*   This file provides a pure C++ replacement for the Python-based
*   ParamClass that was used in LinuxCNC for accessing interpreter
*   parameters (named and numbered G-code parameters).
*
*   Original Python ParamClass features:
*   - params["paramname"] - access named parameters
*   - params[5400]       - access numbered parameters
*   - params.locals()    - get local parameters
*   - params.globals()   - get global parameters
*
*   C++ Replacement:
*   - params.get("paramname")  - get named parameter
*   - params.set("paramname", value) - set named parameter
*   - params.get(5400)         - get numbered parameter
*   - params.set(5400, value)  - set numbered parameter
*   - params.locals()          - get local parameter names
*   - params.globals()         - get global parameter names
*
*   Derived from a work by Michael Haberler
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef CPP_PARAMCLASS_HH
#define CPP_PARAMCLASS_HH

#include "interp_fwd.hh"
#include <string>
#include <vector>
#include <map>
#include <functional>

// Forward declarations
struct Interp;
struct context_struct;

// ========================================================================
// Parameter Access Class
// ========================================================================

namespace InterpParams {

// Parameter class for accessing G-code named and numbered parameters
// This replaces the Python ParamClass with pure C++ implementation
class ParamAccess {
public:
    explicit ParamAccess(Interp& interp);

    // ---- Named Parameter Access ----

    // Get parameter by name (e.g., "x", "y", "feed_rate")
    double get(const std::string& name) const;

    // Set parameter by name
    void set(const std::string& name, double value);

    // Check if named parameter exists
    bool has(const std::string& name) const;

    // Get all named parameters in current context
    std::map<std::string, double> getAllNamed() const;

    // ---- Numbered Parameter Access ----

    // Get parameter by number (e.g., 5400 = G54 X offset)
    double get(int index) const;

    // Set parameter by number
    void set(int index, double value);

    // Check if numbered parameter is valid
    bool isValidIndex(int index) const;

    // Get range of valid parameter indices
    int minIndex() const;
    int maxIndex() const;

    // ---- Context Management ----

    // Get local parameter names in current call context
    std::vector<std::string> locals() const;

    // Get global parameter names
    std::vector<std::string> globals() const;

    // Get all parameter names (locals + globals)
    std::vector<std::string> allNames() const;

    // Get parameter count
    int count() const;

    // ---- Special Parameters ----

    // Get current feed rate
    double feedRate() const;

    // Get current spindle speed (spindle 0)
    double spindleSpeed() const;

    // Get current tool number
    int toolNumber() const;

    // Get current work coordinate system (G54-G59.3)
    int coordinateSystem() const;

    // ---- Utility ----

    // Clear all local parameters
    void clearLocals();

    // Push/pop call context (for subroutine calls)
    void pushContext();
    void popContext();

private:
    Interp& m_interp;

    // Helper to find parameter by name
    int findParamIndex(const std::string& name) const;

    // Helper to validate and get context
    context_struct* getCurrentContext() const;
};

// ========================================================================
// Parameter Name Registry
// ========================================================================

// Registry of known parameter names and their indices
class ParamNameRegistry {
public:
    static ParamNameRegistry& instance() {
        static ParamNameRegistry registry;
        return registry;
    }

    // Register a named parameter
    void registerParam(const std::string& name, int index);

    // Look up index by name
    int lookup(const std::string& name) const;

    // Look up name by index
    std::string lookupName(int index) const;

    // Get all registered names
    std::vector<std::string> allRegisteredNames() const;

    // Check if name is registered
    bool isRegistered(const std::string& name) const;

private:
    ParamNameRegistry();

    std::map<std::string, int> m_name_to_index;
    std::map<int, std::string> m_index_to_name;
};

// ========================================================================
// Inline Helper Functions
// ========================================================================

// Standard parameter indices (from LinuxCNC)
// These are the numbered parameters accessible via #<param>

// Coordinate system offsets (G54-G59.3)
constexpr int PARAM_G54_X = 5221;
constexpr int PARAM_G54_Y = 5222;
constexpr int PARAM_G54_Z = 5223;
constexpr int PARAM_G55_X = 5224;
constexpr int PARAM_G55_Y = 5225;
constexpr int PARAM_G55_Z = 5226;
constexpr int PARAM_G56_X = 5227;
constexpr int PARAM_G56_Y = 5228;
constexpr int PARAM_G56_Z = 5229;
constexpr int PARAM_G57_X = 5230;
constexpr int PARAM_G57_Y = 5231;
constexpr int PARAM_G57_Z = 5232;
constexpr int PARAM_G58_X = 5233;
constexpr int PARAM_G58_Y = 5234;
constexpr int PARAM_G58_Z = 5235;
constexpr int PARAM_G59_X = 5236;
constexpr int PARAM_G59_Y = 5237;
constexpr int PARAM_G59_Z = 5238;

// Tool offset
constexpr int PARAM_TOOL_OFFSET_X = 5400;
constexpr int PARAM_TOOL_OFFSET_Y = 5401;
constexpr int PARAM_TOOL_OFFSET_Z = 5402;

// Current position
constexpr int PARAM_CURRENT_X = 5420;
constexpr int PARAM_CURRENT_Y = 5421;
constexpr int PARAM_CURRENT_Z = 5422;

// Feed rate
constexpr int PARAM_FEED_RATE = 5000;

// Spindle speed
constexpr int PARAM_SPINDLE_SPEED = 5001;

// Tool in spindle
constexpr int PARAM_TOOL_IN_SPINDLE = 5400;

} // namespace InterpParams

#endif // CPP_PARAMCLASS_HH