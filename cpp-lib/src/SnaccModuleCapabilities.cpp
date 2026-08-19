#include "../include/SnaccModuleCapabilities.h"

namespace
{
void ApplyOpIds(const int* pOpIds, const size_t stOpIdCount, std::unordered_map<unsigned int, SnaccOpVersionInfo>& inOutOps)
{
	if (!pOpIds)
		return;

	for (size_t i = 0; i < stOpIdCount; ++i)
	{
		const unsigned int uiOpId = static_cast<unsigned int>(pOpIds[i]);
		if (uiOpId == 0)
			continue;
		inOutOps.emplace(uiOpId, SnaccOpVersionInfo{});
	}
}
} // namespace

void SnaccApplyModuleDetailToRemoteCapabilities(
	const char* szModuleName,
	const char* szVersion,
	const int* pInvokeOpIds,
	const size_t stInvokeOpIdCount,
	const int* pEventOpIds,
	const size_t stEventOpIdCount,
	SnaccLoadedModuleMap& inOutRemote)
{
	if (!szModuleName || !szVersion)
		return;

	auto& module = inOutRemote[szModuleName];
	module.m_strModuleName = szModuleName;
	module.m_strVersion = szVersion;
	ApplyOpIds(pInvokeOpIds, stInvokeOpIdCount, module.m_invokes);
	ApplyOpIds(pEventOpIds, stEventOpIdCount, module.m_events);
}

void SnaccBuildRemoteModuleCapabilities(const SnaccRemoteModuleDetailInput* pDetails, const size_t stDetailCount, SnaccLoadedModuleMap& outRemote)
{
	outRemote.clear();
	if (!pDetails)
		return;

	for (size_t i = 0; i < stDetailCount; ++i)
	{
		const SnaccRemoteModuleDetailInput& detail = pDetails[i];
		SnaccApplyModuleDetailToRemoteCapabilities(
			detail.m_szModuleName,
			detail.m_szVersion,
			detail.m_pInvokeOpIds,
			detail.m_stInvokeOpIdCount,
			detail.m_pEventOpIds,
			detail.m_stEventOpIdCount,
			outRemote);
	}
}
