#include "XPrintf.h"
#include <stdarg.h>
#include <stdlib.h>
#include <vector>

struct Specifier {
    Specifier(int start, int stop, char type)
            : start(start), stop(stop), type(type) {
        flags[0] = width[0] = precision[0] = length[0] = '\0';
        args[0] = args[1] = args[2] = -1;
    }
    int start, stop;
    char flags[5+1];
    char width[10+1];
    char precision[10+1];
    char length[2+1];
    char type;
    int args[3];
};

static inline bool check_diuoxX_length(int n, const char *ptr) {
    if (n == 1) {
        return *ptr == 'h';
    }
    if (n == 2) {
        return (*ptr == 'h' && ptr[1] == 'h') ||
            (*ptr == 'l' && ptr[1] == 'l');
    }
    return false;
}

static inline bool check_fFeEgG_length(int n, const char *ptr) {
    if (n == 1) {
        return *ptr == 'l';
    }
    return false;
}

static inline bool check_s_length(int n, const char *ptr) {
    if (n == 1) {
        return *ptr == 'n';
    }
    return false;
}

static inline bool check_length_valid(const char *ptr, const char *tail) {
    if (strchr("diuoxX", *tail) != NULL) {
        return check_diuoxX_length(tail - ptr, ptr);
    }
    if (strchr("fFeEgG", *tail) != NULL) {
        return check_fFeEgG_length(tail - ptr, ptr);
    }
    if (strchr("s", *tail) != NULL) {
        return check_s_length(tail - ptr, ptr);
    }
    return false;
}

static std::vector<Specifier> ParseFormatSpecifier(const char *fmt)
{
    std::vector<Specifier> specifiers;
    int stack = -1;
    for (const char *str = fmt;;) {
        const char *ptr = strchr(str, '%');
        if (ptr == NULL) {
            break;
        }
        if (ptr[1] == '%') {
            Specifier specifier(ptr - fmt, ptr - fmt + 1, '%');
            specifiers.push_back(specifier);
            str = ptr + 2;
            continue;
        }
        const char *tail = strpbrk(ptr + 1, "diuoxXfFeEgGcs");
        if (tail == NULL) {
            break;
        }
        Specifier specifier(ptr - fmt, tail - fmt, *tail);
        ++ptr;
        const char *nptr = NULL;
        if (*ptr >= '1' && *ptr <= '9') {
            for (nptr = ptr + 1; *nptr >= '0' && *nptr <= '9'; ++nptr) {
                continue;
            }
            if (*nptr == '$') {
                specifier.args[0] = atoi(ptr) - 1;
                ptr = nptr + 1;
                nptr = NULL;
            }
        }
        if (nptr == NULL) {
            const size_t n = strspn(ptr, "-+ #0");
            if (n != 0) {
                strncpy(specifier.flags, ptr, n);
                specifier.flags[n] = '\0';
                ptr += n;
            }
        }
        if (*ptr == '*') {
            ++ptr;
            if (*ptr >= '1' && *ptr <= '9') {
                for (nptr = ptr + 1; *nptr >= '0' && *nptr <= '9'; ++nptr) {
                    continue;
                }
                if (*nptr == '$') {
                    specifier.args[1] = atoi(ptr) - 1;
                    ptr = nptr + 1;
                }
            } else {
                specifier.args[1] = ++stack;
            }
        } else if (nptr != NULL || (*ptr >= '1' && *ptr <= '9')) {
            if (nptr == NULL) {
                for (nptr = ptr + 1; *nptr >= '0' && *nptr <= '9'; ++nptr) {
                    continue;
                }
            }
            strncpy(specifier.width, ptr, nptr - ptr);
            specifier.width[nptr - ptr] = '\0';
            ptr = nptr;
        }
        if (*ptr == '.') {
            ++ptr;
            if (*ptr == '*') {
                ++ptr;
                if (*ptr >= '1' && *ptr <= '9') {
                    for (nptr = ptr + 1; *nptr >= '0' && *nptr <= '9'; ++nptr) {
                        continue;
                    }
                    if (*nptr == '$') {
                        specifier.args[2] = atoi(ptr) - 1;
                        ptr = nptr + 1;
                    }
                } else {
                    specifier.args[2] = ++stack;
                }
            } else {
                for (nptr = ptr; *nptr >= '0' && *nptr <= '9'; ++nptr) {
                    continue;
                }
                if ((*ptr == '0' && nptr == ptr + 1) || (*ptr != '0' && nptr > ptr)) {
                    strncpy(specifier.precision, ptr, nptr - ptr);
                    specifier.precision[nptr - ptr] = '\0';
                    ptr = nptr;
                }
            }
        }
        if (ptr == tail || check_length_valid(ptr, tail)) {
            if (tail > ptr) {
                strncpy(specifier.length, ptr, tail - ptr);
                specifier.length[tail - ptr] = '\0';
            }
            if (specifier.args[0] == -1) {
                specifier.args[0] = ++stack;
            }
            specifiers.push_back(specifier);
        }
        str = tail + 1;
    }
    return specifiers;
}

struct Parameter {
    Parameter(const Specifier *specifier = NULL) : specifier(specifier) {}
    union {
        s64 _s64;
        u64 _u64;
        double _f64;
        const char *_str;
    };
    std::string str;
    const Specifier *specifier;
};

static inline void InsertFormatParameter(std::vector<Parameter> &parameters,
                                         size_t index, const Parameter &parameter)
{
    if (parameters.size() <= index) {
        parameters.resize(index + 1);
    }
    const Parameter &parameter0 = parameters[index];
    if (parameter0.specifier == NULL && parameter.specifier != NULL) {
        parameters[index] = parameter;
    }
}

static std::vector<Parameter> ParseFormatParameter(std::vector<Specifier> &specifiers)
{
    std::vector<Parameter> parameters;
    parameters.reserve(specifiers.size() * 3);
    for (size_t i = 0, n = specifiers.size(); i < n; ++i) {
        const Specifier &specifier = specifiers[i];
        if (specifier.args[1] >= 0) {
            InsertFormatParameter(parameters, specifier.args[1], NULL);
        }
        if (specifier.args[2] >= 0) {
            InsertFormatParameter(parameters, specifier.args[2], NULL);
        }
        if (specifier.args[0] >= 0) {
            InsertFormatParameter(parameters, specifier.args[0], &specifier);
        }
    }
    return parameters;
}

static inline int Print2Buffer(char *buffer, int offset, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = 0;
    if (buffer != NULL) {
        len = vsprintf(buffer + offset, fmt, ap);
    } else {
#if defined(_WIN32)
        len = _vscprintf(fmt, ap);
#else
        len = vsnprintf(NULL, 0, fmt, ap);
#endif
    }
    va_end(ap);
    return len;
}

int static PrintFormatParameter(char *buffer, const char *str,
                                std::vector<Specifier> &specifiers,
                                std::vector<Parameter> &parameters)
{
    int len = 0, pos = 0;
    for (size_t i = 0, n = specifiers.size(); i < n; ++i) {
        const Specifier &specifier = specifiers[i];
        if (pos < specifier.start) {
            len += Print2Buffer(
                buffer, len, "%.*s", specifier.start - pos, str + pos);
        }
        if (specifier.type == '%') {
            len += Print2Buffer(buffer, len, "%%");
        } else {
            char fmt[32];
            char *ptr = fmt; *ptr++ = '%';
            if (specifier.flags[0] != '\0') {
                ptr += sprintf(ptr, "%s", specifier.flags);
            }
            if (specifier.width[0] != '\0') {
                ptr += sprintf(ptr, "%s", specifier.width);
            } else if (specifier.args[1] >= 0) {
                ptr += sprintf(ptr, "%u", (u32)parameters[specifier.args[1]]._u64);
            }
            if (specifier.precision[0] != '\0' || specifier.args[2] >= 0) {
                *ptr++ = '.';
                if (specifier.precision[0] != '\0') {
                    ptr += sprintf(ptr, "%s", specifier.precision);
                } else if (specifier.args[2] >= 0) {
                    ptr += sprintf(ptr, "%u", (u32)parameters[specifier.args[2]]._u64);
                }
            }
            if (strchr("diuoxX", specifier.type) != NULL) {
                *ptr++ = 'l', *ptr++ = 'l';
            }
            *ptr++ = specifier.type;
            *ptr = '\0';
            const Parameter &value = parameters[specifier.args[0]];
            if (strchr("di", specifier.type) != NULL) {
                len += Print2Buffer(buffer, len, fmt, value._s64);
            } else if (strchr("uoxX", specifier.type) != NULL) {
                len += Print2Buffer(buffer, len, fmt, value._u64);
            } else if (strchr("fFeEgG", specifier.type) != NULL) {
                len += Print2Buffer(buffer, len, fmt, value._f64);
            } else if (specifier.type == 'c') {
                len += Print2Buffer(buffer, len, fmt, (char)value._s64);
            } else if (specifier.type == 's') {
                len += Print2Buffer(buffer, len, fmt, value._str);
            }
        }
        pos = specifier.stop + 1;
    }
    if (str[pos] != '\0') {
        len += Print2Buffer(buffer, len, "%s", str + pos);
    }
    return len;
}

///////////////////////////////////////////////////////////////////////////////
// implement printf functions
///////////////////////////////////////////////////////////////////////////////

static void ReadFormatParameter(std::vector<Parameter> &parameters, INetStream &args)
{
    for (size_t i = 0, n = parameters.size(); i < n; ++i) {
        Parameter &parameter = parameters[i];
        if (parameter.specifier == NULL) {
            parameter._u64 = args.Read<u32>();
        } else {
            const Specifier &specifier = *parameter.specifier;
            if (strchr("di", specifier.type) != NULL) {
                if (specifier.length[0] == '\0') {
                    parameter._s64 = args.Read<s32>();
                } else if (specifier.length[0] == 'h') {
                    if (specifier.length[1] == 'h') {
                        parameter._s64 = args.Read<s8>();
                    } else {
                        parameter._s64 = args.Read<s16>();
                    }
                } else if (specifier.length[0] == 'l') {
                    if (specifier.length[1] == 'l') {
                        parameter._s64 = args.Read<s64>();
                    }
                }
            } else if (strchr("uoxX", specifier.type) != NULL) {
                if (specifier.length[0] == '\0') {
                    parameter._u64 = args.Read<u32>();
                } else if (specifier.length[0] == 'h') {
                    if (specifier.length[1] == 'h') {
                        parameter._u64 = args.Read<u8>();
                    } else {
                        parameter._u64 = args.Read<u16>();
                    }
                } else if (specifier.length[0] == 'l') {
                    if (specifier.length[1] == 'l') {
                        parameter._u64 = args.Read<u64>();
                    }
                }
            } else if (strchr("fFeEgG", specifier.type) != NULL) {
                if (specifier.length[0] == '\0') {
                    parameter._f64 = args.Read<f32>();
                } else if (specifier.length[0] == 'l') {
                    parameter._f64 = args.Read<f64>();
                }
            } else if (specifier.type == 'c') {
                parameter._s64 = args.Read<char>();
            } else if (specifier.type == 's') {
                if (specifier.length[0] == '\0') {
                    parameter.str = args.Read<std::string>();
                    parameter._str = parameter.str.c_str();
                } else if (specifier.length[0] == 'n') {
                    parameter._str = I18N_StrID(args.Read<uint32>());
                }
            }
        }
    }
}

std::string NetStream_Printf(const char *fmt, INetStream &args)
{
    std::vector<Specifier> specifiers = ParseFormatSpecifier(fmt);
    std::vector<Parameter> parameters = ParseFormatParameter(specifiers);
    ReadFormatParameter(parameters, args);
    int len = PrintFormatParameter(NULL, fmt, specifiers, parameters);
    std::string str(len, '\0');
    PrintFormatParameter((char*)str.data(), fmt, specifiers, parameters);
    return str;
}

static void ReadFormatParameter(std::vector<Parameter> &parameters, TextUnpacker &args)
{
    for (size_t i = 0, n = parameters.size(); i < n; ++i) {
        Parameter &parameter = parameters[i];
        if (parameter.specifier == NULL) {
            parameter._u64 = args.Unpack<u32>();
        } else {
            const Specifier &specifier = *parameter.specifier;
            if (strchr("di", specifier.type) != NULL) {
                parameter._s64 = args.Unpack<s64>();
            } else if (strchr("uoxX", specifier.type) != NULL) {
                parameter._u64 = args.Unpack<s64>();
            } else if (strchr("fFeEgG", specifier.type) != NULL) {
                parameter._f64 = args.Unpack<double>();
            } else if (specifier.type == 'c') {
                parameter._s64 = args.Unpack<char>();
            } else if (specifier.type == 's') {
                if (specifier.length[0] == '\0') {
                    parameter.str = args.Unpack<std::string>();
                    parameter._str = parameter.str.c_str();
                } else if (specifier.length[0] == 'n') {
                    parameter._str = I18N_StrID(args.Unpack<uint32>());
                }
            }
        }
    }
}

std::string Text_Printf(const char *fmt, TextUnpacker &args)
{
    std::vector<Specifier> specifiers = ParseFormatSpecifier(fmt);
    std::vector<Parameter> parameters = ParseFormatParameter(specifiers);
    ReadFormatParameter(parameters, args);
    int len = PrintFormatParameter(NULL, fmt, specifiers, parameters);
    std::string str(len, '\0');
    PrintFormatParameter((char*)str.data(), fmt, specifiers, parameters);
    return str;
}
