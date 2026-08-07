#ifndef MODULE_VERSION_EMIT_H
#define MODULE_VERSION_EMIT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum ModuleVersionEmitStyle
{
	ModuleVersionEmitCppHeader,
	ModuleVersionEmitCppInterfaceVersionStruct,
	ModuleVersionEmitTsExports,
	ModuleVersionEmitTsClassStatic,
	ModuleVersionEmitKotlinObject,
	ModuleVersionEmitSwiftStruct,
	ModuleVersionEmitJavaClass,
	ModuleVersionEmitJsonDocVersionObject,
} ModuleVersionEmitStyle;

/**
 * Emits annotated MODULE_* version fields for generated stubs.
 * patchUnix is the newest @added unix timestamp for the module or compile scope.
 * szModuleNameUpper is required for ModuleVersionEmitCppHeader only (e.g. ENETUC_CER).
 * indent prefixes each emitted line (e.g. "\t", "    ", or "\t\t\t" for JSON).
 */
void EmitAnnotatedModuleVersionFields(
	FILE* out,
	ModuleVersionEmitStyle style,
	const char* szModuleNameUpper,
	const char* indent,
	int majorVersion,
	long long patchUnix);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_VERSION_EMIT_H */
