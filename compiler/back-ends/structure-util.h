#if !defined(STRUCTURE_UTIL_INCLUDE)
#define STRUCTURE_UTIL_INCLUDE

#include "../../c-lib/include/asn-incl.h"
#include "../core/asn1module.h"
#include "str-util.h"

bool IsROSEValueDef(ValueDef* vd);

// Resolves the argument, result and error value for a ROSE operation
// if specified also resolves the types of these references
// if bResolveToRoot is specified a local or imported type ref is resolved to the root, otherwise a type ref is only resolved one level
bool GetROSEDetails(ValueDef* vd, char** ppszArgument, char** ppszResult, char** ppszError, Type** argumentType, Type** resultType, Type** errorType, bool bResolveToRoot);
// Wenn es sich um ein localtyperef oder importedtyperef handelt wandert es so lange nach oben bis es kein imported oder localtyperef mehr ist
BasicType* ResolveBasicTypeReferences(BasicType* type, const char** szName);
// Resolveds a type reference one level down (if imported or local type ref)
Type* ResolveTypeReferencesOneLevel(Type* type, char** szName);
// Resolveds a type reference all levels down (until it´s no longer imported or local type ref)
Type* ResolveTypeReferencesToRoot(Type* type, char** szName);
// Wenn es sich um ein localtyperef oder importedtyperef handelt wandert es so lange nach oben bis es kein imported oder localtyperef mehr ist
Type* GetRootType(Type* type, const char** szName);

int IsSimpleType(const enum BasicTypeChoiceId type);

const char* GetNameSpace(Module* mod);

// Wenn es sich um ein localtyperef oder importedtyperef handelt liefert es den Typ eine ebene weiter oben
BasicType* GetBaseBasicType(BasicType* type, const char** szName);

// Wenn es sich um ein localtyperef oder importedtyperef handelt liefert es den enum Typ eine ebene weiter oben
enum BasicTypeChoiceId GetBaseBasicTypeChoiceId(BasicType* basicType);

// Wenn es sich um eine sequenceof oder setof handelt ermittelt es den zugrundeliegenden typ
BasicType* ResolveArrayRootType(BasicType* type, const char** szName);

char* GetImportFileName(char* Impname, ModuleList* mods);
Module* GetImportModuleRefByClassName(const char* className, ModuleList* mods, Module* m);
Module* GetModuleForImportModule(ModuleList* mods, ImportModule* impMod);

// Returns true when the member can be skipped (only certain members can get removed, check the function for details)
bool IsDeprecatedMember(const TypeDef* td, const char* szElement);
bool IsDeprecatedSequence(const char* szSequenceName);
bool IsDeprecatedOperation(const char* szOperationName);

#endif