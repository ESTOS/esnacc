#ifndef _SnaccModuleVersions_h_
#define _SnaccModuleVersions_h_

#include <string>
#include <map>

/*
 * Starting with esnacc 6 you may use a special syntax to get versioned asn1 files
 * When the compiler parses the asn1 files it looks for an interfaceversion.txt file which
 * contains the major interface version for the asn1 files
 * Within the different asn1 files you may add asn1 comments like this
 * -- @added 06.03.2024
 *
 * This tells the compiler that a certain operation, sequence, member was added at a certain date
 * The compiler then collects these information and takes the highest value as version information
 * for the asn1 module
 *
 * The full version is build using
 * majorversion.0.patchversion
 *
 * majorversion is a simple integer
 * patchversion is the reverse date representation as numbers e.g. 31.12.2024 would be 20241231
 */

/*
 * Version information for one asn1 file
 */
class SnaccModuleVersion
{
public:
	SnaccModuleVersion();
	SnaccModuleVersion(const char* szModuleName, const int iMajorVersion, long long llPatchVersion);
	// Retrieve the major version of the interface -> major
	int GetMajorVersion() const;
	// Retrieve the patch version of the interface -> patch
	long long GetPatchVersion() const;

private:
	// Name of the module
	std::string m_strModuleName;
	// The patch version as __int64 value (value of the @added timestamp, so unix time since 1970)
	long long m_llPatchVersion = 0;
	// The major version of the interface (as defined by the interfaceversion.txt)
	int m_iMajorVersion = 0;
};

/*
 * Version information for all asn1 files
 */
class SnaccModuleVersions
{
public:
	static bool addModuleVersion(const char* szModuleName, int iMajorVersion, long long i64PatchVersion);
	static bool getModuleVersion(const char* szModuleName, SnaccModuleVersion& version);
	static bool getHighestModuleVersion(SnaccModuleVersion& version);

private:
	static std::map<std::string, SnaccModuleVersion> m_ModuleVersions;
};

#endif // _SnaccModuleVersions_h_