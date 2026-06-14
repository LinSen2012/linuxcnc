/********************************************************************
* Description: remap_callback.cc
*   C++ Remap Callback Registry Implementation for linuxQtCnc
*
*   This file implements the pure C++ replacement for the Python-based
*   remap mechanism in LinuxCNC.
*
*   Derived from a work by Michael Haberler, Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#include "remap_callback.hh"
#include "rs274ngc_interp.hh"
#include "interp_internal.hh"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>

namespace RemapCallbacks {

// ========================================================================
// Logging Helper
// ========================================================================

static void log_remap(int level, const char* fmt, ...) {
    if (level > RemapCallbackRegistry::instance().logLevel()) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[remap] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#define LOG_ERROR(fmt, ...) \
    log_remap(1, "ERROR: " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    log_remap(2, "INFO: " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    log_remap(3, "DEBUG: " fmt, ##__VA_ARGS__)

// ========================================================================
// RemapCallbackRegistry Implementation
// ========================================================================

bool RemapCallbackRegistry::registerHandler(
    const std::string& code,
    const std::string& argspec,
    PrologFunc prolog,
    BodyFunc body,
    EpilogFunc epilog
) {
    RemapHandler handler(code);
    handler.argspec = argspec;
    handler.prolog = prolog;
    handler.body = body;
    handler.epilog = epilog;
    handler.has_cpp_handler = true;

    m_handlers[code] = handler;
    LOG_INFO("Registered C++ remap handler: %s (argspec=%s)", code.c_str(), argspec.c_str());
    return true;
}

bool RemapCallbackRegistry::registerHandler(
    const std::string& code,
    const std::string& argspec,
    RemapHandlerFunc handler
) {
    RemapHandler h(code);
    h.argspec = argspec;
    h.combined = handler;
    h.has_cpp_handler = true;

    m_handlers[code] = h;
    LOG_INFO("Registered C++ combined remap handler: %s (argspec=%s)", code.c_str(), argspec.c_str());
    return true;
}

bool RemapCallbackRegistry::registerHandler(const std::string& code, RemapHandler handler) {
    handler.has_cpp_handler = true;
    m_handlers[code] = handler;
    LOG_INFO("Registered C++ remap handler: %s", code.c_str());
    return true;
}

bool RemapCallbackRegistry::unregisterHandler(const std::string& code) {
    auto it = m_handlers.find(code);
    if (it != m_handlers.end()) {
        m_handlers.erase(it);
        LOG_INFO("Unregistered remap handler: %s", code.c_str());
        return true;
    }
    return false;
}

RemapHandler* RemapCallbackRegistry::getHandler(const std::string& code) {
    auto it = m_handlers.find(code);
    if (it != m_handlers.end()) {
        return &it->second;
    }
    return nullptr;
}

const RemapHandler* RemapCallbackRegistry::getHandler(const std::string& code) const {
    auto it = m_handlers.find(code);
    if (it != m_handlers.end()) {
        return &it->second;
    }
    return nullptr;
}

bool RemapCallbackRegistry::hasHandler(const std::string& code) const {
    return m_handlers.find(code) != m_handlers.end();
}

std::vector<std::string> RemapCallbackRegistry::getRegisteredHandlers() const {
    std::vector<std::string> handlers;
    for (const auto& pair : m_handlers) {
        handlers.push_back(pair.first);
    }
    return handlers;
}

int RemapCallbackRegistry::executeProlog(
    Interp* interp,
    block_struct* block,
    RemapParams& params,
    const std::string& code
) {
    RemapHandler* handler = getHandler(code);
    if (!handler) {
        LOG_ERROR("executeProlog: no handler found for %s", code.c_str());
        return -1;
    }

    if (handler->combined) {
        // Combined handler handles all phases
        return handler->combined(interp, block, params, Phase::PROLOG);
    }

    if (handler->prolog) {
        LOG_DEBUG("Executing prolog for %s", code.c_str());
        return handler->prolog(interp, block, params);
    }

    // No prolog - parameters should already be set
    LOG_DEBUG("No prolog for %s - using default parameter extraction", code.c_str());
    return 0;
}

int RemapCallbackRegistry::executeBody(
    Interp* interp,
    RemapParams& params,
    const std::string& code
) {
    RemapHandler* handler = getHandler(code);
    if (!handler) {
        LOG_ERROR("executeBody: no handler found for %s", code.c_str());
        return -1;
    }

    if (handler->combined) {
        return handler->combined(interp, nullptr, params, Phase::BODY);
    }

    if (handler->body) {
        LOG_DEBUG("Executing body for %s", code.c_str());
        return handler->body(interp, params);
    }

    if (!handler->ngc_file.empty()) {
        // NGC file execution is handled by interp_o_word.cc
        LOG_DEBUG("Body for %s delegated to NGC file: %s", code.c_str(), handler->ngc_file.c_str());
        return 0;  // Interp will handle NGC file execution
    }

    LOG_ERROR("executeBody: no body handler or NGC file for %s", code.c_str());
    return -1;
}

int RemapCallbackRegistry::executeEpilog(
    Interp* interp,
    RemapParams& params,
    const std::string& code,
    int body_status
) {
    RemapHandler* handler = getHandler(code);
    if (!handler) {
        LOG_ERROR("executeEpilog: no handler found for %s", code.c_str());
        return -1;
    }

    if (handler->combined) {
        return handler->combined(interp, nullptr, params, Phase::EPILOG);
    }

    if (handler->epilog) {
        LOG_DEBUG("Executing epilog for %s (body_status=%d)", code.c_str(), body_status);
        return handler->epilog(interp, params, body_status);
    }

    // No epilog - that's OK
    LOG_DEBUG("No epilog for %s", code.c_str());
    return body_status;  // Return body status unchanged
}

int RemapCallbackRegistry::executeRemap(
    Interp* interp,
    block_struct* block,
    RemapParams& params,
    const std::string& code
) {
    LOG_DEBUG("Executing complete remap for %s", code.c_str());

    // Execute prolog
    int status = executeProlog(interp, block, params, code);
    if (status != 0) {
        LOG_ERROR("Prolog failed for %s: status=%d", code.c_str(), status);
        return status;
    }

    // Execute body
    status = executeBody(interp, params, code);
    if (status != 0) {
        LOG_ERROR("Body failed for %s: status=%d", code.c_str(), status);
    }

    // Execute epilog (always, even if body failed)
    int epilog_status = executeEpilog(interp, params, code, status);
    if (epilog_status != status) {
        LOG_DEBUG("Epilog modified status: %d -> %d", status, epilog_status);
        status = epilog_status;
    }

    return status;
}

void RemapCallbackRegistry::clear() {
    m_handlers.clear();
    LOG_INFO("Cleared all remap handlers");
}

// ========================================================================
// INI File Parser Implementation
// ========================================================================

// Maximum tokens per REMAP= line
#define MAX_REMAPOPTS 16

int parseRemapIniLine(const char* inistring, int lineno) {
    char iniline[LINELEN];
    char* argv[MAX_REMAPOPTS];
    int argc = 0;
    const char* code = nullptr;
    RemapHandler r;
    bool errored = false;
    int g1 = 0, g2 = 0;
    int mcode = -1;
    int gcode = -1;
    char* s;

    memset(&r, 0, sizeof(RemapHandler));
    r.modal_group = -1;
    r.motion_code = INT_MIN;

    rtapi_strxcpy(iniline, inistring);

    // Strip trailing comments
    if ((s = strchr(iniline, '#')) != nullptr) {
        *s = '\0';
    }

    char* saveptr;
    s = strtok_r(iniline, " \t", &saveptr);

    while (s != nullptr && argc < MAX_REMAPOPTS - 1) {
        argv[argc++] = s;
        s = strtok_r(nullptr, " \t", &saveptr);
    }

    if (argc == MAX_REMAPOPTS) {
        LOG_ERROR("parse_remap: too many arguments (max %d)", MAX_REMAPOPTS);
        return -1;
    }

    argv[argc] = nullptr;
    code = argv[0];
    r.name = code;

    // Parse key=value pairs
    for (int i = 1; i < argc; i++) {
        int kwlen = 0;
        char* kw = argv[i];
        char* arg = strchr(argv[i], '=');
        if (arg != nullptr) {
            kwlen = arg - argv[i];
            arg++;
            if (!strlen(arg)) {
                LOG_ERROR("option '%s' - zero length value: %d:REMAP = %s",
                          kw, lineno, inistring);
                errored = true;
                continue;
            }
        } else {
            LOG_ERROR("option '%s' - missing '=<value>: %d:REMAP = %s",
                      kw, lineno, inistring);
            errored = true;
            continue;
        }

        // Parse modalgroup
        if (!strncasecmp(kw, "modalgroup", kwlen)) {
            r.modal_group = atoi(arg);
            continue;
        }

        // Parse argspec
        if (!strncasecmp(kw, "argspec", kwlen)) {
            // Validate argspec characters
            size_t pos = strspn(arg, "ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz>^@");
            if (pos != strlen(arg)) {
                LOG_ERROR("argspec: illegal word '%c' - %d:REMAP = %s",
                          arg[pos], lineno, inistring);
                errored = true;
                continue;
            }
            r.argspec = arg;
            continue;
        }

        // Parse prolog (C++ function name)
        if (!strncasecmp(kw, "prolog", kwlen)) {
            // In linuxQtCnc, prolog should be a registered C++ function
            // For now, just store the name - actual registration happens elsewhere
            LOG_INFO("Prolog function specified: %s (will look up in registry)", arg);
            continue;
        }

        // Parse epilog (C++ function name)
        if (!strncasecmp(kw, "epilog", kwlen)) {
            LOG_INFO("Epilog function specified: %s (will look up in registry)", arg);
            continue;
        }

        // Parse ngc file
        if (!strncasecmp(kw, "ngc", kwlen)) {
            r.ngc_file = arg;
            LOG_INFO("NGC remap file: %s", arg);
            continue;
        }

        // Parse body (C++ function name)
        if (!strncasecmp(kw, "body", kwlen)) {
            LOG_INFO("Body function specified: %s (will look up in registry)", arg);
            continue;
        }

        // Skip python= option (not supported in linuxQtCnc)
        if (!strncasecmp(kw, "python", kwlen)) {
            LOG_ERROR("python= option not supported in linuxQtCnc (use C++ handlers instead)");
            errored = true;
            continue;
        }

        LOG_ERROR("unrecognized option '%.*s' in %d:REMAP = %s",
                  kwlen, kw, lineno, inistring);
        errored = true;
    }

    if (errored) {
        return -1;
    }

    // Check if code is already registered
    if (RemapCallbackRegistry::instance().hasHandler(code)) {
        LOG_ERROR("code '%s' already remapped: %d:REMAP = %s",
                  code, lineno, inistring);
        return -1;
    }

    // Validate: must have either NGC file or C++ handler
    // The actual handler registration happens elsewhere (typically at startup)
    // Here we just validate the INI syntax
    if (r.ngc_file.empty()) {
        // Check if a C++ handler will be registered by name
        LOG_INFO("No NGC file specified for %s - C++ handler expected", code);
    }

    // Register the remap (with empty callbacks - will be set later)
    RemapCallbackRegistry::instance().registerHandler(code, r);

    LOG_INFO("Parsed REMAP=%s from INI line %d", code, lineno);
    return 0;
}

// ========================================================================
// Built-in C++ Remap Handlers
// ========================================================================

namespace BuiltInHandlers {

// M6 - Tool Change Handler
// This is an example implementation of a tool change remap
int M6_prolog(Interp* interp, block_struct* block, RemapParams& params) {
    LOG_DEBUG("M6_prolog called");

    // Extract T word (tool number)
    if (block->t_flag) {
        params.t = block->t_number;
        params.t_flag = true;
        LOG_INFO("M6: tool number T%.0f", params.t);
    } else {
        // If no T word, use current tool
        LOG_INFO("M6: no T word, using current tool");
    }

    // Q word for pocket number (if specified)
    if (block->q_flag) {
        params.q = block->q_number;
        params.q_flag = true;
        LOG_DEBUG("M6: pocket number Q%.0f", params.q);
    }

    return 0;  // Success
}

int M6_body(Interp* interp, RemapParams& params) {
    LOG_DEBUG("M6_body called with T=%.0f", params.t);

    // In a real implementation, this would:
    // 1. Signal the motion controller to stop
    // 2. Execute tool change sequence
    // 3. Wait for tool change complete
    // 4. Update tool table

    // For now, just log the tool change
    if (params.t_flag) {
        LOG_INFO("Executing tool change to tool T%.0f", params.t);
    }

    // Call the canon layer to execute tool change
    // This would integrate with the actual machine
    // canon_tool_change(params.t);

    return 0;  // Success
}

int M6_epilog(Interp* interp, RemapParams& params, int body_status) {
    LOG_DEBUG("M6_epilog called, body_status=%d", body_status);

    if (body_status == 0) {
        LOG_INFO("M6: tool change completed successfully");
    } else {
        LOG_ERROR("M6: tool change failed with status %d", body_status);
    }

    return body_status;
}

// M61 - Set Tool Number (without tool change)
int M61_prolog(Interp* interp, block_struct* block, RemapParams& params) {
    LOG_DEBUG("M61_prolog called");

    if (block->q_flag) {
        params.q = block->q_number;
        params.q_flag = true;
        LOG_INFO("M61: setting tool number to Q%.0f", params.q);
    } else {
        LOG_ERROR("M61: Q word (tool number) required");
        return -1;
    }

    return 0;
}

int M61_body(Interp* interp, RemapParams& params) {
    LOG_DEBUG("M61_body called with Q=%.0f", params.q);

    // Set the current tool number without executing a tool change
    // canon_set_tool_number(params.q);

    LOG_INFO("M61: current tool set to Q%.0f", params.q);
    return 0;
}

// Register all built-in handlers
void registerBuiltInHandlers() {
    auto& registry = RemapCallbackRegistry::instance();

    // Register M6 tool change handler
    registry.registerHandler(
        "M6",
        "Tq",  // argspec: T=tool number, q=pocket number
        M6_prolog,
        M6_body,
        M6_epilog
    );

    // Register M61 set tool number handler
    registry.registerHandler(
        "M61",
        "Q",  // argspec: Q=tool number
        M61_prolog,
        M61_body,
        nullptr  // No epilog needed
    );

    LOG_INFO("Registered %zu built-in C++ remap handlers",
             registry.handlerCount());
}

} // namespace BuiltInHandlers

} // namespace RemapCallbacks
