#include "SnaccModuleVersions.h"

SnaccModuleVersion::SnaccModuleVersion()
{
}

SnaccModuleVersion::SnaccModuleVersion(const char* szModuleName, const int iMajorVersion, long long lMinorVersion)
{
	m_strModuleName = szModuleName;
	m_iMajorVersion = iMajorVersion;
	m_i64MinorVersion = lMinorVersion;

	char szBuffer[100] = {0};
	sprintf_s(szBuffer, 100, "%i.%lld", iMajorVersion, lMinorVersion);
	m_strFullVersion = szBuffer;
}

const char* SnaccModuleVersion::GetVersion() const
{
	return m_strFullVersion.c_str();
}

int SnaccModuleVersion::GetMajorVersion() const
{
	return m_iMajorVersion;
}

long long SnaccModuleVersion::GetMinorVersion() const
{
	return m_i64MinorVersion;
}

bool SnaccModuleVersions::addModuleVersion(const char* szModuleName, int iMajorVersion, long long i64MinorVersion)
{
	if (m_ModuleVersions.find(szModuleName) != m_ModuleVersions.end())
		return false;

	m_ModuleVersions[szModuleName] = SnaccModuleVersion(szModuleName, iMajorVersion, iMajorVersion);
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
	long long lMinor = -1;
	for (const auto module : m_ModuleVersions)
	{
		if (module.second.GetMajorVersion() > iMajor)
		{
			iMajor = module.second.GetMajorVersion();
			lMinor = module.second.GetMinorVersion();
			version = module.second;
		}
		else if (module.second.GetMajorVersion() == iMajor && module.second.GetMinorVersion() > lMinor)
		{
			iMajor = module.second.GetMajorVersion();
			lMinor = module.second.GetMinorVersion();
			version = module.second;
		}
	}

	if (iMajor != -1)
		return true;

	return false;
}
