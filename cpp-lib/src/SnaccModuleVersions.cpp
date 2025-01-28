#include "SnaccModuleVersions.h"
#include "asn-config.h"

std::map<std::string, SnaccModuleVersion> SnaccModuleVersions::m_ModuleVersions;

SnaccModuleVersion::SnaccModuleVersion()
{
}

SnaccModuleVersion::SnaccModuleVersion(const char* szModuleName, const int iMajorVersion, long long llPatchVersion)
{
	m_strModuleName = szModuleName;
	m_iMajorVersion = iMajorVersion;
	m_llPatchVersion = llPatchVersion;
}

int SnaccModuleVersion::GetMajorVersion() const
{
	return m_iMajorVersion;
}

long long SnaccModuleVersion::GetPatchVersion() const
{
	return m_llPatchVersion;
}

bool SnaccModuleVersions::addModuleVersion(const char* szModuleName, int iMajorVersion, long long llPatchVersion)
{
	if (m_ModuleVersions.find(szModuleName) != m_ModuleVersions.end())
		return false;

	m_ModuleVersions[szModuleName] = SnaccModuleVersion(szModuleName, iMajorVersion, llPatchVersion);
	return true;
}

bool SnaccModuleVersions::getModuleVersion(const char* szModuleName, SnaccModuleVersion& version)
{
	auto iter = m_ModuleVersions.find(szModuleName);
	if (iter == m_ModuleVersions.end())
		return false;

	version = iter->second;

	return true;
}

bool SnaccModuleVersions::getHighestModuleVersion(SnaccModuleVersion& version)
{
	int iMajor = -1;
	long long llPatch = -1;
	for (const auto& module : m_ModuleVersions)
	{
		if (module.second.GetMajorVersion() > iMajor)
		{
			iMajor = module.second.GetMajorVersion();
			llPatch = module.second.GetPatchVersion();
			version = module.second;
		}
		else if (module.second.GetMajorVersion() == iMajor && module.second.GetPatchVersion() > llPatch)
		{
			iMajor = module.second.GetMajorVersion();
			llPatch = module.second.GetPatchVersion();
			version = module.second;
		}
	}

	if (iMajor != -1)
		return true;

	return false;
}
