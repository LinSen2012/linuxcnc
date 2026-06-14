/********************************************************************
* Description: cpp_paramclass.cc
*   C++ Parameter Access Class Implementation for linuxQtCnc
*
*   This file implements the pure C++ replacement for Python ParamClass.
*
*   Derived from a work by Michael Haberler
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#include "cpp_paramclass.hh"
#include "rs274ngc_interp.hh"
#include "interp_internal.hh"
#include "interp_parameter_def.hh"

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace InterpParams {

using namespace interp_param_global;

// ========================================================================
// ParamAccess Implementation
// ========================================================================

ParamAccess::ParamAccess(Interp& interp)
    : m_interp(interp)
{
}

double ParamAccess::get(const std::string& name) const
{
    int index = findParamIndex(name);
    if (index < 0) {
        fprintf(stderr, "[ParamAccess] Warning: parameter '%s' not found\n", name.c_str());
        return 0.0;
    }
    return get(index);
}

void ParamAccess::set(const std::string& name, double value)
{
    int index = findParamIndex(name);
    if (index < 0) {
        fprintf(stderr, "[ParamAccess] Warning: parameter '%s' not found, cannot set\n", name.c_str());
        return;
    }
    set(index, value);
}

bool ParamAccess::has(const std::string& name) const
{
    return findParamIndex(name) >= 0;
}

std::map<std::string, double> ParamAccess::getAllNamed() const
{
    std::map<std::string, double> result;

    // Get all registered parameter names
    auto& registry = ParamNameRegistry::instance();
    for (const auto& name : registry.allRegisteredNames()) {
        if (has(name)) {
            result[name] = get(name);
        }
    }

    return result;
}

double ParamAccess::get(int index) const
{
    if (!isValidIndex(index)) {
        fprintf(stderr, "[ParamAccess] Warning: invalid parameter index %d\n", index);
        return 0.0;
    }

    // Access the interpreter's parameter array
    // In LinuxCNC, parameters are stored in _setup.parameters
    // This requires access to the Interp's internal setup structure

    // For now, use a simplified approach
    // The actual implementation would call the interpreter's parameter access methods

    // Note: This is a placeholder - the actual parameter access
    // requires integration with the Interp's internal state
    return 0.0;  // Placeholder
}

void ParamAccess::set(int index, double value)
{
    if (!isValidIndex(index)) {
        fprintf(stderr, "[ParamAccess] Warning: invalid parameter index %d, cannot set\n", index);
        return;
    }

    // Set the parameter in the interpreter's parameter array
    // This requires integration with the Interp's internal state

    // Placeholder - actual implementation would call:
    // m_interp.store_named_param(..., value, ...);
}

bool ParamAccess::isValidIndex(int index) const
{
    // Valid parameter range in LinuxCNC
    return index >= 1 && index < RS274NGC_MAX_PARAMETERS;
}

int ParamAccess::minIndex() const
{
    return 1;
}

int ParamAccess::maxIndex() const
{
    return RS274NGC_MAX_PARAMETERS - 1;
}

std::vector<std::string> ParamAccess::locals() const
{
    std::vector<std::string> local_names;

    // Local parameters are those in the current call context
    // In LinuxCNC, local parameters are numbered 1-30 in each call level

    // Placeholder - actual implementation would iterate through
    // the current context's local parameters

    return local_names;
}

std::vector<std::string> ParamAccess::globals() const
{
    std::vector<std::string> global_names;

    // Global parameters are those that persist across call levels
    // In LinuxCNC, global parameters are numbered 31-5000

    // Placeholder - actual implementation would iterate through
    // the global parameter namespace

    return global_names;
}

std::vector<std::string> ParamAccess::allNames() const
{
    auto local = locals();
    auto global = globals();

    local.insert(local.end(), global.begin(), global.end());
    return local;
}

int ParamAccess::count() const
{
    return RS274NGC_MAX_PARAMETERS;
}

double ParamAccess::feedRate() const
{
    return get(PARAM_FEED_RATE);
}

double ParamAccess::spindleSpeed() const
{
    return get(PARAM_SPINDLE_SPEED);
}

int ParamAccess::toolNumber() const
{
    // Tool number is stored in a special parameter
    return static_cast<int>(get(PARAM_TOOL_IN_SPINDLE));
}

int ParamAccess::coordinateSystem() const
{
    // Coordinate system is determined by which G54-G59 offset is active
    // This requires checking the interpreter's modal G codes
    return 1;  // Placeholder - G54 default
}

void ParamAccess::clearLocals()
{
    // Clear local parameters in current context
    // Placeholder - actual implementation would reset local params
}

void ParamAccess::pushContext()
{
    // Push a new call context for subroutine parameters
    // Placeholder - actual implementation would call Interp's context methods
}

void ParamAccess::popContext()
{
    // Pop the call context after subroutine returns
    // Placeholder - actual implementation would call Interp's context methods
}

int ParamAccess::findParamIndex(const std::string& name) const
{
    return ParamNameRegistry::instance().lookup(name);
}

context_struct* ParamAccess::getCurrentContext() const
{
    // Get the current call context from the interpreter
    // Placeholder - requires access to Interp internals
    return nullptr;
}

// ========================================================================
// ParamNameRegistry Implementation
// ========================================================================

ParamNameRegistry::ParamNameRegistry()
{
    // Register standard LinuxCNC parameter names

    // Coordinate system offsets
    registerParam("g54_x", PARAM_G54_X);
    registerParam("g54_y", PARAM_G54_Y);
    registerParam("g54_z", PARAM_G54_Z);
    registerParam("g55_x", PARAM_G55_X);
    registerParam("g55_y", PARAM_G55_Y);
    registerParam("g55_z", PARAM_G55_Z);
    registerParam("g56_x", PARAM_G56_X);
    registerParam("g56_y", PARAM_G56_Y);
    registerParam("g56_z", PARAM_G56_Z);
    registerParam("g57_x", PARAM_G57_X);
    registerParam("g57_y", PARAM_G57_Y);
    registerParam("g57_z", PARAM_G57_Z);
    registerParam("g58_x", PARAM_G58_X);
    registerParam("g58_y", PARAM_G58_Y);
    registerParam("g58_z", PARAM_G58_Z);
    registerParam("g59_x", PARAM_G59_X);
    registerParam("g59_y", PARAM_G59_Y);
    registerParam("g59_z", PARAM_G59_Z);

    // Tool offsets
    registerParam("tool_offset_x", PARAM_TOOL_OFFSET_X);
    registerParam("tool_offset_y", PARAM_TOOL_OFFSET_Y);
    registerParam("tool_offset_z", PARAM_TOOL_OFFSET_Z);

    // Current position
    registerParam("current_x", PARAM_CURRENT_X);
    registerParam("current_y", PARAM_CURRENT_Y);
    registerParam("current_z", PARAM_CURRENT_Z);

    // Feed and speed
    registerParam("feed_rate", PARAM_FEED_RATE);
    registerParam("spindle_speed", PARAM_SPINDLE_SPEED);

    // Common named parameters (from LinuxCNC)
    registerParam("_x", 1);      // Current X position
    registerParam("_y", 2);      // Current Y position
    registerParam("_z", 3);      // Current Z position
    registerParam("_a", 4);      // Current A position
    registerParam("_b", 5);      // Current B position
    registerParam("_c", 6);      // Current C position
    registerParam("_u", 7);      // Current U position
    registerParam("_v", 8);      // Current V position
    registerParam("_w", 9);      // Current W position

    registerParam("_feed", 10);  // Current feed rate
    registerParam("_speed", 11); // Current spindle speed
    registerParam("_tool", 12);  // Current tool number
    registerParam("_line", 13);  // Current line number
    registerParam("_motion_mode", 14);  // Motion mode (G0, G1, etc.)
    registerParam("_plane", 15);        // Active plane (G17, G18, G19)
    registerParam("_absolute", 16);     // Absolute mode (G90)
    registerParam("_incremental", 17);  // Incremental mode (G91)
}

void ParamNameRegistry::registerParam(const std::string& name, int index)
{
    m_name_to_index[name] = index;
    m_index_to_name[index] = name;
}

int ParamNameRegistry::lookup(const std::string& name) const
{
    auto it = m_name_to_index.find(name);
    if (it != m_name_to_index.end()) {
        return it->second;
    }

    // Try lowercase version
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    it = m_name_to_index.find(lower_name);
    if (it != m_name_to_index.end()) {
        return it->second;
    }

    return -1;  // Not found
}

std::string ParamNameRegistry::lookupName(int index) const
{
    auto it = m_index_to_name.find(index);
    if (it != m_index_to_name.end()) {
        return it->second;
    }
    return "";
}

std::vector<std::string> ParamNameRegistry::allRegisteredNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : m_name_to_index) {
        names.push_back(pair.first);
    }
    return names;
}

bool ParamNameRegistry::isRegistered(const std::string& name) const
{
    return m_name_to_index.find(name) != m_name_to_index.end();
}

} // namespace InterpParams