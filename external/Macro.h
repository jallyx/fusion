#pragma once

#if defined(_WIN32)
    #include <Shlwapi.h>
    #define bzero(s,n) memset(s,0,n)
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
    #define strcasestr StrStrIA
    #define getpid GetCurrentProcessId
    #define localtime_r(_clock,_result) localtime_s(_result,_clock)
    #define gmtime_r(_clock,_result) gmtime_s(_result,_clock)
    #define timezone _timezone
    #define PATH_MAX MAX_PATH
#endif

#if defined(_WIN32)
    #include <WinSock2.h>
    #define poll WSAPoll
    #define GET_SOCKET_ERROR WSAGetLastError
    #define ERROR_WOULDBLOCK WSAEWOULDBLOCK
    #define ERROR_INPROGRESS WSAEWOULDBLOCK
#else
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (-1)
    #define SOCKET int
    #define closesocket close
    #define GET_SOCKET_ERROR() errno
    #define ERROR_WOULDBLOCK EWOULDBLOCK
    #define ERROR_INPROGRESS EINPROGRESS
#endif

namespace os {
#if defined(_WIN32)
    static const char sep = '\\';
    static const char * const linesep = "\r\n";
    static const char * const devnull = "NUL";
#else
    static const char sep = '/';
    static const char * const linesep = "\n";
    static const char * const devnull = "/dev/null";
#endif
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define SAFE_FREE(p) do{if(p){free(p);p=nullptr;}}while(0)
#define SAFE_DELETE(p) do{if(p){delete(p);p=nullptr;}}while(0)
#define SAFE_DELETE_ARRAY(p) do{if(p){delete[]p;p=nullptr;}}while(0)

#define COUNT_OF_1(a) (sizeof(a)/sizeof(a[0]))
#define COUNT_OF_2(a) (sizeof(a)/sizeof(a[0][0]))
#define COUNT_OF_3(a) (sizeof(a)/sizeof(a[0][0][0]))
#define ARRAY_SIZE(a) (std::extent<typename std::remove_reference<decltype(a)>::type>::value)

#define IS_INPTR_CONTAIN_VALUE(Array,Size,Value) \
    (std::find(Array,Array+Size,Value)!=Array+Size)
#define IS_ARRAY_CONTAIN_VALUE(Array,Value) \
    IS_INPTR_CONTAIN_VALUE(Array,ARRAY_SIZE(Array),Value)
#define IS_VECTOR_CONTAIN_VALUE(Vector,Value) \
    IS_INPTR_CONTAIN_VALUE(Vector.data(),Vector.size(),Value)
