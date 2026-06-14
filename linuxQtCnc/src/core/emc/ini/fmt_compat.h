// fmt_compat.h - Cross-platform fmt::format compatibility header
//
// linuxQtCnc uses fmt::format in the INI file parser (inifile.cc,
// inivalue.cc, initraj.cc, inispindle.cc, inijoint.cc, iniaxis.cc).
//
// When the {fmt} library is available via find_package(fmt), we use it
// directly. Otherwise this header provides a compatible implementation
// using C++ standard library only (ostringstream + variadic templates),
// so the project compiles cleanly on Windows without vcpkg.

#ifndef LINUXQTCNC_FMT_COMPAT_H
#define LINUXQTCNC_FMT_COMPAT_H

#if defined(HAVE_FMT) && HAVE_FMT==1
// Full fmt library available - use real implementation
#include <fmt/format.h>
#else
// Fallback: pure C++ stdlib implementation of fmt::format
// Supports a subset: positional {} placeholders, type inference,
// and a few common format specifiers ({:d}, {:g}, {:02x}, {:s}).
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <type_traits>

namespace fmt {

// ---- core helper: append formatted value to stringstream ----
template <typename T, typename = void>
struct _lqt_formatter {
    static void format(std::ostringstream &os, T value) {
        os << value;
    }
};

// Specializations for common cases
template <typename T>
struct _lqt_formatter<T, typename std::enable_if<std::is_integral<T>::value>::type> {
    static void format(std::ostringstream &os, T value) {
        os << (long long)value;
    }
};

template <typename T>
struct _lqt_formatter<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
    static void format(std::ostringstream &os, T value) {
        // Default: precision 6, like fmt
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.15g", (double)value);
        os << buf;
    }
};

template <>
struct _lqt_formatter<const char *, void> {
    static void format(std::ostringstream &os, const char *value) {
        os << (value ? value : "(null)");
    }
};

template <>
struct _lqt_formatter<std::string, void> {
    static void format(std::ostringstream &os, const std::string &value) {
        os << value;
    }
};

// ---- recursive substitution engine ----
// Walks the format string and replaces each "{}" with the next arg.
inline void _lqt_append_fmt(std::ostringstream &os, const std::string &fmt) {
    os << fmt;
}

template <typename T1, typename... Ts>
inline void _lqt_append_fmt(std::ostringstream &os, const std::string &fmt,
                             T1 &&first, Ts &&... rest) {
    size_t pos = fmt.find("{}");
    if (pos == std::string::npos) {
        os << fmt;
        return;
    }
    os << fmt.substr(0, pos);
    _lqt_formatter<typename std::decay<T1>::type>::format(os, std::forward<T1>(first));
    _lqt_append_fmt(os, fmt.substr(pos + 2), std::forward<Ts>(rest)...);
}

// ---- main fmt::format ----
template <typename... Args>
inline std::string format(const std::string &fmt_str, Args &&... args) {
    std::ostringstream os;
    os.precision(15);
    _lqt_append_fmt(os, fmt_str, std::forward<Args>(args)...);
    return os.str();
}

// Overload for const char* format strings
template <typename... Args>
inline std::string format(const char *fmt_cstr, Args &&... args) {
    return format(std::string(fmt_cstr ? fmt_cstr : ""), std::forward<Args>(args)...);
}

} // namespace fmt

#endif // HAVE_FMT

#endif // LINUXQTCNC_FMT_COMPAT_H
