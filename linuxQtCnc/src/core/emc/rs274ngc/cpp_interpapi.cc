/********************************************************************
* Description: cpp_interpapi.cc
*   C++ Interpreter API Implementation for linuxQtCnc
*
*   This file implements the pure C++ replacement for Python interpreter module.
*
*   Derived from a work by Michael Haberler
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#include "cpp_interpapi.hh"
#include "rs274ngc_interp.hh"
#include "interp_internal.hh"
#include "nml_intf/canon.hh"

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace InterpApi {

// ========================================================================
// InterpApiWrapper Implementation
// ========================================================================

InterpApiWrapper::InterpApiWrapper(Interp* interp)
    : m_interp(interp)
    , m_owns_interp(false)
    , m_params(nullptr)
    , m_error_callback(nullptr)
    , m_log_level(1)
{
    if (m_interp) {
        m_params = std::make_unique<InterpParams::ParamAccess>(*m_interp);
    }
}

InterpApiWrapper::InterpApiWrapper()
    : m_interp(nullptr)
    , m_owns_interp(true)
    , m_params(nullptr)
    , m_error_callback(nullptr)
    , m_log_level(1)
{
    // Create a new interpreter instance
    m_interp = new Interp();
    m_params = std::make_unique<InterpParams::ParamAccess>(*m_interp);
}

InterpApiWrapper::~InterpApiWrapper()
{
    if (m_owns_interp && m_interp) {
        delete m_interp;
        m_interp = nullptr;
    }
}

// ---- Initialization ----

bool InterpApiWrapper::init()
{
    if (!m_interp) return false;

    // Initialize the interpreter
    // In LinuxCNC, this would call Interp::init()
    // For linuxQtCnc, we use a simplified approach

    return true;
}

bool InterpApiWrapper::initWithIni(const std::string& ini_file)
{
    if (!m_interp) return false;

    // Initialize with INI file
    // In LinuxCNC, this would call Interp::init_with_ini()
    // For linuxQtCnc, we parse the INI and configure

    return init();
}

void InterpApiWrapper::close()
{
    if (m_interp) {
        // Cleanup interpreter state
        // In LinuxCNC, this would call Interp::close()
    }
}

// ---- G-code Execution ----

InterpStatus InterpApiWrapper::execute(const std::string& gcode)
{
    return execute(gcode, true);
}

InterpStatus InterpApiWrapper::execute(const std::string& gcode, bool handle_optional_stop)
{
    if (!m_interp) {
        if (m_error_callback) {
            m_error_callback("Interpreter not initialized");
        }
        return InterpStatus::ERROR;
    }

    // Execute G-code string
    // In LinuxCNC, this would call Interp::execute()
    // For linuxQtCnc, we use the interpreter's read/execute cycle

    // Placeholder - actual implementation would:
    // 1. Parse the G-code string
    // 2. Execute each command
    // 3. Handle optional stop if requested

    return InterpStatus::OK;
}

InterpStatus InterpApiWrapper::executeFile(const std::string& filename)
{
    if (!m_interp) {
        return InterpStatus::ERROR;
    }

    // Load and execute file
    // In LinuxCNC, this would call Interp::execute_file()

    return InterpStatus::OK;
}

bool InterpApiWrapper::load(const std::string& filename)
{
    if (!m_interp) return false;

    // Load file without executing
    // In LinuxCNC, this would call Interp::load()

    return true;
}

// ---- Subroutine Calls ----

InterpStatus InterpApiWrapper::call(const std::string& subname)
{
    return call(subname, {});
}

InterpStatus InterpApiWrapper::call(const std::string& subname, const std::vector<double>& params)
{
    if (!m_interp) {
        return InterpStatus::ERROR;
    }

    // Call subroutine with parameters
    // In LinuxCNC, this would call Interp::call_subroutine()

    return InterpStatus::OK;
}

InterpStatus InterpApiWrapper::callOWord(const std::string& o_name)
{
    if (!m_interp) {
        return InterpStatus::ERROR;
    }

    // Call O-word subroutine
    // In LinuxCNC, this would call Interp::call_oword()

    return InterpStatus::OK;
}

InterpStatus InterpApiWrapper::returnFromCall()
{
    if (!m_interp) {
        return InterpStatus::ERROR;
    }

    // Return from subroutine
    // In LinuxCNC, this would call Interp::return_from_call()

    return InterpStatus::OK;
}

// ---- Parameter Access ----

InterpParams::ParamAccess& InterpApiWrapper::params()
{
    if (!m_params) {
        m_params = std::make_unique<InterpParams::ParamAccess>(*m_interp);
    }
    return *m_params;
}

double InterpApiWrapper::getParam(const std::string& name) const
{
    if (!m_params) return 0.0;
    return m_params->get(name);
}

void InterpApiWrapper::setParam(const std::string& name, double value)
{
    if (!m_params) return;
    m_params->set(name, value);
}

double InterpApiWrapper::getParam(int index) const
{
    if (!m_params) return 0.0;
    return m_params->get(index);
}

void InterpApiWrapper::setParam(int index, double value)
{
    if (!m_params) return;
    m_params->set(index, value);
}

// ---- Block Access ----

InterpBlocks::BlockWrapper InterpApiWrapper::currentBlock()
{
    if (!m_interp) {
        return InterpBlocks::BlockWrapper(nullptr);
    }

    // Get current executing block
    // In LinuxCNC, this would be _setup.blocks[_setup.call_level]

    return InterpBlocks::BlockWrapper(nullptr);  // Placeholder
}

InterpBlocks::BlockWrapper InterpApiWrapper::blockAtLevel(int level)
{
    if (!m_interp) {
        return InterpBlocks::BlockWrapper(nullptr);
    }

    // Get block at specific call level
    // In LinuxCNC, this would be _setup.blocks[level]

    return InterpBlocks::BlockWrapper(nullptr);  // Placeholder
}

int InterpApiWrapper::callLevel() const
{
    if (!m_interp) return 0;

    // Get current call level
    // In LinuxCNC, this would be _setup.call_level

    return 0;  // Placeholder
}

// ---- Tool Table Access ----

InterpApiWrapper::ToolEntry InterpApiWrapper::getTool(int toolno) const
{
    ToolEntry entry;
    entry.toolno = toolno;
    entry.pocket = 0;
    entry.x_offset = 0.0;
    entry.y_offset = 0.0;
    entry.z_offset = 0.0;
    entry.a_offset = 0.0;
    entry.b_offset = 0.0;
    entry.c_offset = 0.0;
    entry.u_offset = 0.0;
    entry.v_offset = 0.0;
    entry.w_offset = 0.0;
    entry.diameter = 0.0;
    entry.front_angle = 0.0;
    entry.back_angle = 0.0;
    entry.orientation = 0;
    entry.comment = "";

    if (!m_interp) return entry;

    // Get tool entry from tool table
    // In LinuxCNC, this would access _setup.tool_table

    return entry;
}

void InterpApiWrapper::setTool(int toolno, const ToolEntry& entry)
{
    if (!m_interp) return;

    // Set tool entry in tool table
    // In LinuxCNC, this would modify _setup.tool_table
}

int InterpApiWrapper::toolInSpindle() const
{
    if (!m_interp) return 0;

    // Get tool in spindle
    // In LinuxCNC, this would be _setup.tool_in_spindle

    return 0;
}

void InterpApiWrapper::setToolInSpindle(int toolno)
{
    if (!m_interp) return;

    // Set tool in spindle
    // In LinuxCNC, this would set _setup.tool_in_spindle
}

bool InterpApiWrapper::loadToolTable(const std::string& filename)
{
    if (!m_interp) return false;

    // Load tool table from file
    // In LinuxCNC, this would call Interp::load_tool_table()

    return true;
}

bool InterpApiWrapper::saveToolTable(const std::string& filename)
{
    if (!m_interp) return false;

    // Save tool table to file
    // In LinuxCNC, this would call Interp::save_tool_table()

    return true;
}

// ---- Position Access ----

InterpApiWrapper::Position InterpApiWrapper::currentPosition() const
{
    Position pos;
    pos.x = 0.0;
    pos.y = 0.0;
    pos.z = 0.0;
    pos.a = 0.0;
    pos.b = 0.0;
    pos.c = 0.0;
    pos.u = 0.0;
    pos.v = 0.0;
    pos.w = 0.0;

    if (!m_interp) return pos;

    // Get current position from interpreter
    // In LinuxCNC, this would be _setup.current_position

    return pos;
}

InterpApiWrapper::Position InterpApiWrapper::workOffsetPosition(int system) const
{
    Position pos;
    pos.x = 0.0;
    pos.y = 0.0;
    pos.z = 0.0;
    pos.a = 0.0;
    pos.b = 0.0;
    pos.c = 0.0;
    pos.u = 0.0;
    pos.v = 0.0;
    pos.w = 0.0;

    if (!m_interp) return pos;

    // Get work offset position (G54-G59.3)
    // In LinuxCNC, this would access _setup.g5x_offset

    return pos;
}

InterpApiWrapper::Position InterpApiWrapper::machinePosition() const
{
    Position pos;
    pos.x = 0.0;
    pos.y = 0.0;
    pos.z = 0.0;
    pos.a = 0.0;
    pos.b = 0.0;
    pos.c = 0.0;
    pos.u = 0.0;
    pos.v = 0.0;
    pos.w = 0.0;

    if (!m_interp) return pos;

    // Get machine position (absolute)
    // In LinuxCNC, this would be machine coordinates

    return pos;
}

// ---- Modal State Access ----

std::vector<int> InterpApiWrapper::activeGCodes() const
{
    std::vector<int> codes;
    if (!m_interp) return codes;

    // Get active G codes
    // In LinuxCNC, this would be _setup.active_g_codes

    return codes;
}

std::vector<int> InterpApiWrapper::activeMCodes() const
{
    std::vector<int> codes;
    if (!m_interp) return codes;

    // Get active M codes
    // In LinuxCNC, this would be _setup.active_m_codes

    return codes;
}

InterpApiWrapper::ActiveSettings InterpApiWrapper::activeSettings() const
{
    ActiveSettings settings;
    settings.feed_rate = 0.0;
    settings.spindle_speed = 0.0;
    settings.tool_number = 0;

    if (!m_interp) return settings;

    // Get active settings
    // In LinuxCNC, this would be _setup.active_settings

    return settings;
}

// ---- Feed and Speed ----

double InterpApiWrapper::feedRate() const
{
    if (!m_interp) return 0.0;

    // Get current feed rate
    // In LinuxCNC, this would be _setup.feed_rate

    return 0.0;
}

void InterpApiWrapper::setFeedRate(double rate)
{
    if (!m_interp) return;

    // Set feed rate
    // In LinuxCNC, this would set _setup.feed_rate
}

double InterpApiWrapper::spindleSpeed() const
{
    if (!m_interp) return 0.0;

    // Get current spindle speed
    // In LinuxCNC, this would be _setup.speed[0]

    return 0.0;
}

void InterpApiWrapper::setSpindleSpeed(double speed)
{
    if (!m_interp) return;

    // Set spindle speed
    // In LinuxCNC, this would set _setup.speed[0]
}

// ---- Program Control ----

void InterpApiWrapper::start()
{
    if (!m_interp) return;

    // Start program execution
    // In LinuxCNC, this would set interpreter state to RUNNING
}

void InterpApiWrapper::pause()
{
    if (!m_interp) return;

    // Pause program execution
    // In LinuxCNC, this would set interpreter state to PAUSED
}

void InterpApiWrapper::resume()
{
    if (!m_interp) return;

    // Resume program execution
    // In LinuxCNC, this would set interpreter state to RUNNING
}

void InterpApiWrapper::stop()
{
    if (!m_interp) return;

    // Stop program execution
    // In LinuxCNC, this would set interpreter state to IDLE
}

void InterpApiWrapper::abort()
{
    if (!m_interp) return;

    // Abort program execution
    // In LinuxCNC, this would abort and cleanup
}

InterpState InterpApiWrapper::state() const
{
    if (!m_interp) return InterpState::IDLE;

    // Get interpreter state
    // In LinuxCNC, this would be _setup.interp_state

    return InterpState::IDLE;
}

InterpMode InterpApiWrapper::mode() const
{
    if (!m_interp) return InterpMode::MANUAL;

    // Get interpreter mode
    // In LinuxCNC, this would be _setup.mode

    return InterpMode::MANUAL;
}

// ---- Error Handling ----

std::string InterpApiWrapper::lastError() const
{
    if (!m_interp) return "";

    // Get last error message
    // In LinuxCNC, this would be _setup.error_msg

    return "";
}

void InterpApiWrapper::clearError()
{
    if (!m_interp) return;

    // Clear error message
    // In LinuxCNC, this would clear _setup.error_msg
}

void InterpApiWrapper::setErrorCallback(std::function<void(const std::string&)> callback)
{
    m_error_callback = callback;
}

// ---- File Information ----

std::string InterpApiWrapper::currentFile() const
{
    if (!m_interp) return "";

    // Get current file name
    // In LinuxCNC, this would be _setup.filename

    return "";
}

int InterpApiWrapper::currentLine() const
{
    if (!m_interp) return 0;

    // Get current line number
    // In LinuxCNC, this would be _setup.line_number

    return 0;
}

int InterpApiWrapper::sequenceNumber() const
{
    if (!m_interp) return 0;

    // Get sequence number (N-word)
    // In LinuxCNC, this would be from current block

    return 0;
}

// ---- Logging ----

void InterpApiWrapper::setLogLevel(int level)
{
    m_log_level = level;
}

int InterpApiWrapper::logLevel() const
{
    return m_log_level;
}

// ---- Callback Registration ----

void InterpApiWrapper::registerRemapHandler(
    const std::string& code,
    std::function<int(InterpApiWrapper&, const std::string&)> handler
)
{
    // Register remap handler via RemapCallbackRegistry
    // This integrates with the C++ remap system
}

// ---- Helper ----

InterpStatus InterpApiWrapper::convertStatus(int status) const
{
    switch (status) {
        case INTERP_OK:      return InterpStatus::OK;
        case INTERP_ERROR:   return InterpStatus::ERROR;
        case INTERP_EXIT:    return InterpStatus::EXIT;
        case INTERP_ENDFILE: return InterpStatus::ENDFILE;
        default:             return InterpStatus::ERROR;
    }
}

// ========================================================================
// Global Interpreter Instance
// ========================================================================

InterpApiWrapper& globalInterp()
{
    static InterpApiWrapper instance;
    return instance;
}

std::unique_ptr<InterpApiWrapper> createInterp()
{
    return std::make_unique<InterpApiWrapper>();
}

// ========================================================================
// Utility Functions
// ========================================================================

std::vector<std::string> parseGCode(const std::string& gcode)
{
    std::vector<std::string> commands;

    // Simple parsing - split by newline
    // Actual implementation would use proper G-code parser

    size_t start = 0;
    size_t end = gcode.find('\n');

    while (end != std::string::npos) {
        commands.push_back(gcode.substr(start, end - start));
        start = end + 1;
        end = gcode.find('\n', start);
    }

    if (start < gcode.length()) {
        commands.push_back(gcode.substr(start));
    }

    return commands;
}

bool validateGCode(const std::string& gcode)
{
    // Validate G-code syntax
    // Placeholder - actual implementation would use interpreter's parser

    return true;
}

bool extractWord(const std::string& gcode, char letter, double& value)
{
    // Extract a word value from G-code string
    // Placeholder - actual implementation would parse the string

    return false;
}

} // namespace InterpApi