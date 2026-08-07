#include "module_version_emit.h"

#include "interface_baseline.h"
#include "time_helpers.h"

#include <stdlib.h>
#include <string.h>

static void EmitVersionComment(FILE* out, const char* indent, const char* text)
{
	fprintf(out, "%s// %s\n", indent ? indent : "", text);
}

static void EmitModuleVersionIntroComment(FILE* out, const char* indent)
{
	EmitVersionComment(out, indent, "Module version metadata for this ASN.1 compile (see SnaccModuleVersions.h). Semver: MAJOR.MINOR.PATCH.");
}

/**
 * Emits annotated MODULE_* version fields for generated stubs.
 */
void EmitAnnotatedModuleVersionFields(
	FILE* out,
	ModuleVersionEmitStyle style,
	const char* szModuleNameUpper,
	const char* indent,
	int majorVersion,
	long long patchUnix)
{
	const long long lBaselineUnix = GetInterfaceBaselineUnixTimestamp();
	char* szISODate = ConvertUnixTimeToISO(patchUnix);
	char* szBaselineISODate = ConvertUnixTimeToISO(lBaselineUnix);
	char* szNumericDate = ConvertUnixTimeToNumericDate(patchUnix);
	const char* lineIndent = indent ? indent : "";
	char szCppPrefix[520] = {0};

	if (!out || !szISODate || !szBaselineISODate || !szNumericDate)
	{
		free(szISODate);
		free(szBaselineISODate);
		free(szNumericDate);
		return;
	}

	if (style == ModuleVersionEmitCppHeader && szModuleNameUpper)
		snprintf(szCppPrefix, sizeof(szCppPrefix), "%s_", szModuleNameUpper);

	if (style != ModuleVersionEmitJsonDocVersionObject)
		EmitModuleVersionIntroComment(out, lineIndent);

	switch (style)
	{
	case ModuleVersionEmitCppHeader:
		if (!szModuleNameUpper)
			break;
		EmitVersionComment(out, lineIndent, "Newest @added date in this module (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "#define %sMODULE_LASTCHANGE \"%s\"\n", szCppPrefix, szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "#define %sMODULE_BASELINE \"%s\"\n", szCppPrefix, szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "#define %sMODULE_MAJOR_VERSION %i\n", szCppPrefix, majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "#define %sMODULE_MINOR_VERSION 0\n", szCppPrefix);
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this module as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "#define %sMODULE_PATCH_VERSION %s\n", szCppPrefix, szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "#define %sMODULE_VERSION \"%i.0.%s\"\n", szCppPrefix, majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitCppInterfaceVersionStruct:
		EmitVersionComment(out, lineIndent, "Newest @added date in this compile scope (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "%sstatic constexpr const char* MODULE_LASTCHANGE = \"%s\";\n", lineIndent, szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "%sstatic constexpr const char* MODULE_BASELINE = \"%s\";\n", lineIndent, szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "%sstatic constexpr int MODULE_MAJOR_VERSION = %i;\n", lineIndent, majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "%sstatic constexpr int MODULE_MINOR_VERSION = 0;\n", lineIndent);
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this compile scope as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "%sstatic constexpr long long MODULE_PATCH_VERSION = %s;\n", lineIndent, szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "%sstatic constexpr const char* MODULE_VERSION = \"%i.0.%s\";\n", lineIndent, majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitTsExports:
		EmitVersionComment(out, lineIndent, "Newest @added date in this module (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "export const MODULE_LASTCHANGE = \"%s\";\n", szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "export const MODULE_BASELINE = \"%s\";\n", szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "export const MODULE_MAJOR_VERSION = %i;\n", majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "export const MODULE_MINOR_VERSION = 0;\n");
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this module as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "export const MODULE_PATCH_VERSION = %s;\n", szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "export const MODULE_VERSION = \"%i.0.%s\";\n", majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitTsClassStatic:
		EmitVersionComment(out, lineIndent, "Newest @added date in this compile scope (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "%spublic static MODULE_LASTCHANGE = \"%s\";\n", lineIndent, szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "%spublic static MODULE_BASELINE = \"%s\";\n", lineIndent, szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "%spublic static MODULE_MAJOR_VERSION = %i;\n", lineIndent, majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "%spublic static MODULE_MINOR_VERSION = 0;\n", lineIndent);
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this compile scope as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "%spublic static MODULE_PATCH_VERSION = %s;\n", lineIndent, szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "%spublic static MODULE_VERSION = \"%i.0.%s\";\n", lineIndent, majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitKotlinObject:
		EmitVersionComment(out, lineIndent, "Newest @added date in this module (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "%sconst val MODULE_LASTCHANGE: String = \"%s\"\n", lineIndent, szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "%sconst val MODULE_BASELINE: String = \"%s\"\n", lineIndent, szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "%sconst val MODULE_MAJOR_VERSION: Int = %i\n", lineIndent, majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "%sconst val MODULE_MINOR_VERSION: Int = 0\n", lineIndent);
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this module as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "%sconst val MODULE_PATCH_VERSION: Long = %s\n", lineIndent, szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "%sconst val MODULE_VERSION: String = \"%i.0.%s\"\n", lineIndent, majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitSwiftStruct:
		EmitVersionComment(out, lineIndent, "Newest @added date in this module (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "%slet MODULE_LASTCHANGE = \"%s\"\n", lineIndent, szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "%slet MODULE_BASELINE = \"%s\"\n", lineIndent, szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "%slet MODULE_MAJOR_VERSION = %i\n", lineIndent, majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "%slet MODULE_MINOR_VERSION = 0\n", lineIndent);
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this module as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "%slet MODULE_PATCH_VERSION = %s\n", lineIndent, szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "%slet MODULE_VERSION = \"%i.0.%s\"\n", lineIndent, majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitJavaClass:
		EmitVersionComment(out, lineIndent, "Newest @added date in this module (ISO); same source as MODULE_PATCH_VERSION.");
		fprintf(out, "%spublic static final String MODULE_LASTCHANGE = \"%s\";\n", lineIndent, szISODate);
		EmitVersionComment(out, lineIndent, "API deprecation baseline from deprecatedbaseline.txt or -nodeprecated (ISO).");
		fprintf(out, "%spublic static final String MODULE_BASELINE = \"%s\";\n", lineIndent, szBaselineISODate);
		EmitVersionComment(out, lineIndent, "Semver major: baseline as YYYYMMDD or legacy integer; 0 when no dated baseline.");
		fprintf(out, "%spublic static final int MODULE_MAJOR_VERSION = %i;\n", lineIndent, majorVersion);
		EmitVersionComment(out, lineIndent, "Semver minor placeholder; always 0 in current esnacc releases.");
		fprintf(out, "%spublic static final int MODULE_MINOR_VERSION = 0;\n", lineIndent);
		EmitVersionComment(out, lineIndent, "Semver patch: newest @added in this module as YYYYMMDD (module activity, not a bugfix bump).");
		fprintf(out, "%spublic static final long MODULE_PATCH_VERSION = %s;\n", lineIndent, szNumericDate);
		EmitVersionComment(out, lineIndent, "\"{major}.{minor}.{patch}\" label for handshake and logging.");
		fprintf(out, "%spublic static final String MODULE_VERSION = \"%i.0.%s\";\n", lineIndent, majorVersion, szNumericDate);
		break;

	case ModuleVersionEmitJsonDocVersionObject:
		fprintf(out, "%s\"MODULE_LASTCHANGE\": \"%s\"", lineIndent, szISODate);
		fprintf(out, ",\n%s\"MODULE_BASELINE\": \"%s\"", lineIndent, szBaselineISODate);
		fprintf(out, ",\n%s\"MODULE_MAJOR_VERSION\": %i", lineIndent, majorVersion);
		fprintf(out, ",\n%s\"MODULE_MINOR_VERSION\": 0", lineIndent);
		fprintf(out, ",\n%s\"MODULE_PATCH_VERSION\": %s", lineIndent, szNumericDate);
		fprintf(out, ",\n%s\"MODULE_VERSION\": \"%i.0.%s\"", lineIndent, majorVersion, szNumericDate);
		break;
	}

	free(szISODate);
	free(szBaselineISODate);
	free(szNumericDate);
}
