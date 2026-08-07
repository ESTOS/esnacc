#ifndef _SnaccModuleVersions_h_
#define _SnaccModuleVersions_h_

#include <string>
#include <map>

/*
 * Starting with esnacc 6 you may use a special syntax to get versioned asn1 files
 * When the compiler parses the asn1 files it looks for a deprecatedbaseline.txt file which
 * contains the API baseline for the asn1 files (YYYYMMDD date or legacy integer major version)
 * Within the different asn1 files you may add asn1 comments like this
 * -- @added 06.03.2024
 *
 * This tells the compiler that a certain operation, sequence, member was added at a certain date
 * The compiler then collects these information and takes the highest value as version information
 * for the asn1 module
 *
 * The full version uses semver MAJOR.MINOR.PATCH:
 *   {baseline}.{0}.{maxAdded}
 *
 * MAJOR (MODULE_MAJOR_VERSION) - API baseline from deprecatedbaseline.txt or -nodeprecated
 *   (YYYYMMDD when dated, legacy integer major, or 0 when no dated baseline).
 * MINOR (MODULE_MINOR_VERSION) - semver placeholder; always 0 in current esnacc releases.
 * PATCH (MODULE_PATCH_VERSION) - newest @added in the module as YYYYMMDD (module activity,
 *   not a traditional semver bugfix increment).
 *
 * MODULE_LASTCHANGE / MODULE_BASELINE - ISO dates for @added activity and API baseline.
 * MODULE_MAJOR_VERSION / MODULE_MINOR_VERSION / MODULE_PATCH_VERSION / MODULE_VERSION -
 * semver fields with identical names in all generated backends.
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
	// The major/baseline version of the interface (from deprecatedbaseline.txt or -nodeprecated baseline)
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