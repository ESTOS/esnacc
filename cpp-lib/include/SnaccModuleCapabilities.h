#ifndef _SnaccModuleCapabilities_h_
#define _SnaccModuleCapabilities_h_

/*! Helpers to build SnaccLoadedModuleMap snapshots from negotiate payloads. */

#include "SnaccRoseOperationLookup.h"

#include <cstddef>

/*! One module entry extracted from asnNegotiateInterface (or equivalent). */
struct SnaccRemoteModuleDetailInput
{
	const char* m_szModuleName = nullptr;
	const char* m_szVersion = nullptr;
	const int* m_pInvokeOpIds = nullptr;
	size_t m_stInvokeOpIdCount = 0;
	const int* m_pEventOpIds = nullptr;
	size_t m_stEventOpIdCount = 0;
};

/*! Merges one module detail into @p inOutRemote (creates the module entry when missing). */
void SnaccApplyModuleDetailToRemoteCapabilities(
	const char* szModuleName,
	const char* szVersion,
	const int* pInvokeOpIds,
	size_t stInvokeOpIdCount,
	const int* pEventOpIds,
	size_t stEventOpIdCount,
	SnaccLoadedModuleMap& inOutRemote);

/*! Builds a remote capability map from an array of module detail inputs. Clears @p outRemote first. */
void SnaccBuildRemoteModuleCapabilities(const SnaccRemoteModuleDetailInput* pDetails, size_t stDetailCount, SnaccLoadedModuleMap& outRemote);

#endif // _SnaccModuleCapabilities_h_
