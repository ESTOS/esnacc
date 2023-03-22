#include "../include/SNACCDeprecated.h"

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
#else
    #include <execinfo.h>
    #include <unistd.h>
    #include <signal.h>
#endif

SNACCDeprecatedNotify* SNACCDeprecated::m_pCallback = NULL;

void SNACCDeprecated::SetCallback(SNACCDeprecatedNotify* pCallBack) {
    m_pCallback = pCallBack;
}

void SNACCDeprecated::DeprecatedObject(const char* szModuleName, const char* szObjectName) {
    if (!m_pCallback)
        return;

    std::list<std::string> callStack = GetStackTrace();

    m_pCallback->DeprecatedObject(szModuleName, szObjectName, callStack);
}

void SNACCDeprecated::DeprecatedMethod(const char* szModuleName, const char* szMethodName, const SNACCDeprecatedNotifyCallDirection direction) {
    if (!m_pCallback)
        return;

    std::list<std::string> callStack = GetStackTrace();

    m_pCallback->DeprecatedMethod(szModuleName, szMethodName, direction, callStack);
}

std::list<std::string> SNACCDeprecated::GetStackTrace(int remove /*= 1*/) {
    std::list<std::string> stackTrace;
#ifdef _WIN32
    void* trace[256];
    int size = CaptureStackBackTrace(0, 256, trace, NULL);
    for (int i = 0; i < size; i++) {
        const char* symbol = new char[MAX_PATH];
        SymFromAddr(GetCurrentProcess(), (DWORD64)(trace[i]), NULL, (SYMBOL_INFO*)&symbol);
        stackTrace.push_back(symbol);
        delete [] symbol;
    }
#elif defined(_WIN32) || defined(WIN32)
    void* trace[256];
    int size = backtrace(trace, 256);
    char** symbols = backtrace_symbols(trace, size);
    for (int i = 0; i < size; i++) {
        stackTrace.push_back(symbols[i]);
    }
    free(symbols);
#endif
    return stackTrace;
}
