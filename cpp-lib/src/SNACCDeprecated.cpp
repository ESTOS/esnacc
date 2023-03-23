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

    std::list<std::string> callStack = GetStackTrace(2);

    m_pCallback->DeprecatedObject(szModuleName, szObjectName, callStack);
}

void SNACCDeprecated::DeprecatedMethod(const char* szModuleName, const char* szMethodName, const SNACCDeprecatedNotifyCallDirection direction) {
    if (!m_pCallback)
        return;

    std::list<std::string> callStack = GetStackTrace(2);

    m_pCallback->DeprecatedMethod(szModuleName, szMethodName, direction, callStack);
}

std::list<std::string> SNACCDeprecated::GetStackTrace(int remove /*= 1*/) {
    std::list<std::string> stackTrace;
#ifdef _WIN32
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
    void* trace[256];
    int size = CaptureStackBackTrace(remove, 256, trace, NULL);
    for (int i = 0; i < size; i++) {
        const int kMaxNameLength = 256;
        DWORD_PTR frame = reinterpret_cast<DWORD_PTR>(trace[i]);
        const int iSize = (sizeof(SYMBOL_INFO) + kMaxNameLength * sizeof(wchar_t) + sizeof(ULONG64) - 1) / sizeof(ULONG64);
        ULONG64 buffer[iSize];
        PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(&buffer[0]);
        memset(symbol, 0x00, iSize);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = kMaxNameLength - 1;
        DWORD64 sym_displacement = 0;
        if (SymFromAddr(GetCurrentProcess(), frame, &sym_displacement, symbol)) {
            if (strcmp(symbol->Name, "wmain") == 0)
                break;
            stackTrace.push_back(symbol->Name);
        }
    }
    SymCleanup(GetCurrentProcess());
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
