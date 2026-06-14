/********************************************************************
* Description: pycallback_stub.hh
*   Stub header to replace Boost.Python types used in rs274ngc.
*   These stubs allow compilation without Boost.Python.
*   The actual callback mechanism is reimplemented as pure C++
*   in linuxQtCnc to keep the project dependency-free.
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef PYCALLBACK_STUB_HH
#define PYCALLBACK_STUB_HH

#include <string>
#include <vector>
#include <stdexcept>

// Minimal stub namespace to replace boost::python for compilation
namespace pycallback {

// Stub object - represents a Python object reference
class object {
public:
    object() = default;
    object(const object&) = default;
    ~object() = default;

    template<typename T>
    T extract() const { throw std::runtime_error("Python not available"); }
};

// Stub list
class list {
public:
    list() = default;
    void append(const object&) {}
    template<typename T>
    void append(const T&) {}
};

// Stub dict
class dict {
public:
    dict() = default;
};

// Stub tuple
class tuple {
public:
    tuple() = default;
};

} // namespace pycallback

#endif // PYCALLBACK_STUB_HH
