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
bool SNACCDeprecated::m_bProvideStackTrace = false;

void SNACCDeprecated::SetDeprecatedCallback(SNACCDeprecatedNotify* pCallBack, bool bProvideStackTrace)
{
	m_pCallback = pCallBack;
	m_bProvideStackTrace = bProvideStackTrace;
}

void SNACCDeprecated::DeprecatedASN1Object(const long long i64DeprecatedSince, const char* szModuleName, const char* szObjectName)
{
	if (!m_pCallback)
		return;

	std::list<std::string> callStack = GetStackTrace(2);

	m_pCallback->DeprecatedASN1Object(i64DeprecatedSince, szModuleName, szObjectName, callStack);
}

void SNACCDeprecated::DeprecatedASN1Method(const long long i64DeprecatedSince, const char* szModuleName, const char* szMethodName, const SNACCDeprecatedNotifyCallDirection direction, const SnaccInvokeContext* pContext /* = NULL */)
{
	if (!m_pCallback)
		return;

	std::list<std::string> callStack = GetStackTrace(2);

	m_pCallback->DeprecatedASN1Method(i64DeprecatedSince, szModuleName, szMethodName, direction, callStack, pContext);
}

std::list<std::string> SNACCDeprecated::GetStackTrace(int remove /*= 1*/)
{
	std::list<std::string> stackTrace;

	// If stacktraces are not requested, just leave
	if (!m_bProvideStackTrace)
		return stackTrace;

#ifdef _WIN32
	HANDLE hProcess = GetCurrentProcess();
	try
	{
		SymInitialize(hProcess, NULL, TRUE);
		void* trace[256];
		int size = CaptureStackBackTrace(remove, 256, trace, NULL);
		for (int i = 0; i < size; i++)
		{
			const int kMaxNameLength = 256;
			DWORD_PTR frame = reinterpret_cast<DWORD_PTR>(trace[i]);
			const int iSize = (sizeof(SYMBOL_INFO) + kMaxNameLength * sizeof(wchar_t) + sizeof(ULONG64) - 1) / sizeof(ULONG64);
			ULONG64 buffer[iSize];
			PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(&buffer[0]);
			memset(symbol, 0x00, iSize);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = kMaxNameLength - 1;
			DWORD64 sym_displacement = 0;
			if (SymFromAddr(hProcess, frame, &sym_displacement, symbol))
			{
				if (strcmp(symbol->Name, "wmain") == 0)
					break;
				stackTrace.push_back(symbol->Name);
			}
		}
	}
	catch (...)
	{
	}
	SymCleanup(hProcess);
#elif defined(_WIN32) || defined(WIN32)
	void* trace[256];
	int size = backtrace(trace, 256);
	char** symbols = backtrace_symbols(trace, size);
	for (int i = 0; i < size; i++)
		stackTrace.push_back(symbols[i]);
	free(symbols);
#endif
	return stackTrace;
}
