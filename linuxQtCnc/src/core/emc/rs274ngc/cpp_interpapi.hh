/********************************************************************
* Description: cpp_interpapi.hh
*   C++ Interpreter API for linuxQtCnc
*
*   This file provides a pure C++ replacement for the Python-based
*   interpreter module that was used in LinuxCNC for external access
*   to the G-code interpreter.
*
*   Original Python interpreter module features:
*   - interp.execute("G0 X10") - execute G-code string
*   - interp.load("file.ngc") - load G-code file
*   - interp.call("subname")  - call subroutine
*   - interp.params           - access parameters
*   - interp.blocks           - access blocks
*   - interp.tooltable        - access tool table
*
*   C++ Replacement:
*   - InterpApi::execute("G0 X10") - execute G-code
*   - InterpApi::load("file.ngc")  - load file
*   - InterpApi::call("subname")   - call subroutine
*   - InterpApi::params()          - get parameter accessor
*   - InterpApi::blocks()          - get block accessor
*   - InterpApi::toolTable()       - get tool table accessor
*
*   Derived from a work by Michael Haberler
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef CPP_INTERPAPI_HH
#define CPP_INTERPAPI_HH

#include "cpp_paramclass.hh"
#include "cpp_blockwrapper.hh"
#include "interp_fwd.hh"
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
struct Interp;
struct setup_struct;
struct CANON_TOOL_TABLE;

namespace InterpApi {

// ========================================================================
// Interpreter Status Codes
// ========================================================================

enum class InterpStatus {
    OK = 0,
    ERROR = -1,
    EXIT = 1,
    ENDFILE = 2,
    PAUSE = 3,
    RESUME = 4,
    OPTIONAL_STOP = 5
};

// ========================================================================
// Interpreter Mode
// ========================================================================

enum class InterpMode {
    MANUAL = 0,
    AUTO = 1,
    MDI = 2
};

// ========================================================================
// Interpreter State
// ========================================================================

enum class InterpState {
    IDLE = 0,
    RUNNING = 1,
    PAUSED = 2,
    WAITING_FOR_MOTION = 3,
    WAITING_FOR_SPINDLE = 4,
    WAITING_FOR_TOOL = 5,
    WAITING_FOR_HOLD = 6
};

// ========================================================================
// Interpreter API Class
// ========================================================================

class InterpApiWrapper {
public:
    // Construct with an existing interpreter
    explicit InterpApiWrapper(Interp* interp);

    // Create a new interpreter
    explicit InterpApiWrapper();

    // Destructor
    ~InterpApiWrapper();

    // ---- Initialization ----

    // Initialize the interpreter
    bool init();

    // Initialize with INI file
    bool initWithIni(const std::string& ini_file);

    // Close and cleanup
    void close();

    // ---- G-code Execution ----

    // Execute a G-code string
    InterpStatus execute(const std::string& gcode);

    // Execute a G-code string with optional stop handling
    InterpStatus execute(const std::string& gcode, bool handle_optional_stop);

    // Execute from a file
    InterpStatus executeFile(const std::string& filename);

    // Load a G-code file without executing
    bool load(const std::string& filename);

    // ---- Subroutine Calls ----

    // Call a subroutine by name
    InterpStatus call(const std::string& subname);

    // Call a subroutine with parameters
    InterpStatus call(const std::string& subname, const std::vector<double>& params);

    // Call an O-word subroutine
    InterpStatus callOWord(const std::string& o_name);

    // Return from subroutine
    InterpStatus returnFromCall();

    // ---- Parameter Access ----

    // Get parameter accessor
    InterpParams::ParamAccess& params();

    // Get parameter by name
    double getParam(const std::string& name) const;

    // Set parameter by name
    void setParam(const std::string& name, double value);

    // Get parameter by index
    double getParam(int index) const;

    // Set parameter by index
    void setParam(int index, double value);

    // ---- Block Access ----

    // Get current block wrapper
    InterpBlocks::BlockWrapper currentBlock();

    // Get block at call level
    InterpBlocks::BlockWrapper blockAtLevel(int level);

    // Get call level
    int callLevel() const;

    // ---- Tool Table Access ----

    // Get tool entry by tool number
    struct ToolEntry {
        int toolno;
        int pocket;
        double x_offset;
        double y_offset;
        double z_offset;
        double a_offset;
        double b_offset;
        double c_offset;
        double u_offset;
        double v_offset;
        double w_offset;
        double diameter;
        double front_angle;
        double back_angle;
        int orientation;
        std::string comment;
    };

    ToolEntry getTool(int toolno) const;

    // Set tool entry
    void setTool(int toolno, const ToolEntry& entry);

    // Get tool in spindle
    int toolInSpindle() const;

    // Set tool in spindle
    void setToolInSpindle(int toolno);

    // Load tool table from file
    bool loadToolTable(const std::string& filename);

    // Save tool table to file
    bool saveToolTable(const std::string& filename);

    // ---- Position Access ----

    // Get current position
    struct Position {
        double x;
        double y;
        double z;
        double a;
        double b;
        double c;
        double u;
        double v;
        double w;
    };

    Position currentPosition() const;

    // Get work offset position (G54-G59)
    Position workOffsetPosition(int system) const;

    // Get machine position
    Position machinePosition() const;

    // ---- Modal State Access ----

    // Get active G codes
    std::vector<int> activeGCodes() const;

    // Get active M codes
    std::vector<int> activeMCodes() const;

    // Get active settings (F, S, T)
    struct ActiveSettings {
        double feed_rate;
        double spindle_speed;
        int tool_number;
    };
    ActiveSettings activeSettings() const;

    // ---- Feed and Speed ----

    // Get current feed rate
    double feedRate() const;

    // Set feed rate
    void setFeedRate(double rate);

    // Get current spindle speed
    double spindleSpeed() const;

    // Set spindle speed
    void setSpindleSpeed(double speed);

    // ---- Program Control ----

    // Start program execution
    void start();

    // Pause program execution
    void pause();

    // Resume program execution
    void resume();

    // Stop program execution
    void stop();

    // Abort program execution
    void abort();

    // Get interpreter state
    InterpState state() const;

    // Get interpreter mode
    InterpMode mode() const;

    // ---- Error Handling ----

    // Get last error message
    std::string lastError() const;

    // Clear error
    void clearError();

    // Set error callback
    void setErrorCallback(std::function<void(const std::string&)> callback);

    // ---- File Information ----

    // Get current file name
    std::string currentFile() const;

    // Get current line number
    int currentLine() const;

    // Get sequence number (N-word)
    int sequenceNumber() const;

    // ---- Logging ----

    // Set logging level
    void setLogLevel(int level);

    // Get logging level
    int logLevel() const;

    // ---- Callback Registration ----

    // Register remap handler
    void registerRemapHandler(
        const std::string& code,
        std::function<int(InterpApiWrapper&, const std::string&)> handler
    );

    // ---- Raw Interpreter Access ----

    // Get raw interpreter pointer (for advanced use)
    Interp* rawInterp() { return m_interp; }
    const Interp* rawInterp() const { return m_interp; }

private:
    Interp* m_interp;
    bool m_owns_interp;
    std::unique_ptr<InterpParams::ParamAccess> m_params;
    std::function<void(const std::string&)> m_error_callback;
    int m_log_level;

    // Helper to convert status code
    InterpStatus convertStatus(int status) const;
};

// ========================================================================
// Global Interpreter Instance
// ========================================================================

// Get the global interpreter instance (singleton)
InterpApiWrapper& globalInterp();

// Create a new interpreter instance
std::unique_ptr<InterpApiWrapper> createInterp();

// ========================================================================
// Utility Functions
// ========================================================================

// Convert G-code string to canonical commands
std::vector<std::string> parseGCode(const std::string& gcode);

// Validate G-code syntax
bool validateGCode(const std::string& gcode);

// Get G-code word from string
bool extractWord(const std::string& gcode, char letter, double& value);

} // namespace InterpApi

#endif // CPP_INTERPAPI_HH