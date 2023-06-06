#if !defined(STRUCTURE_UTIL_INCLUDE)
#define STRUCTURE_UTIL_INCLUDE

#include "../../c-lib/include/asn-incl.h"
#include "../core/asn1module.h"
#include "str-util.h"

bool IsROSEValueDef(Module* mod, ValueDef* vd);
// Resolves the argument, result and error value for a ROSE operation
// if specified also resolves the types of these references
// if bResolveToRoot is specified a local or imported type ref is resolved to the root, otherwise a type ref is only resolved one level
bool GetROSEDetails(Module* mod, ValueDef* vd, char** ppszArgument, char** ppszResult, char** ppszError, Type** argumentType, Type** resultType, Type** errorType, bool bResolveToRoot);
// Wenn es sich um ein localtyperef oder importedtyperef handelt wandert es so lange nach oben bis es kein imported oder localtyperef mehr ist
BasicType* ResolveBasicTypeReferences(BasicType* type, const char** szName);
// Resolveds a type reference one level down (if imported or local type ref)
Type* ResolveTypeReferencesOneLevel(Type* type, char** szName);
// Resolveds a type reference all levels down (until it's no longer imported or local type ref)
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

// An element may be flagged deprecated with a timestamp or without
// It's important to mention that when the compiler is running it can remove elements from the output which are flagged as deprecated
// This allows to remove elements e.g. on the client side but not on the server side.
// A method is flagged like this:
// -- @deprecated 01.01.2023 Superseeded by XYZ

// You may now call the compiler with:
// -nodeprecated
// which will remove all deprecated flagg elements
// IsDeprecatedFlagged... returns false, IsDeprecatedNoOutput... returns true

// -nodeprecated:01.01.2023
// which will remove only the elements that are flagg prior or including the mentioned date
// IsDeprecatedFlagged... returns false, IsDeprecatedNoOutput... returns true

// -nodeprecated:31.12.2022
// which will not remove the element but will annotate that the element has been flagged deprecated
// IsDeprecatedFlagged... returns true, IsDeprecatedNoOutput... returns false

// Returns true when an element is flagged as deprecated (and is not set as no output)
bool IsDeprecatedFlaggedModule(Module* mod);
bool IsDeprecatedFlaggedMember(Module* mod, const TypeDef* td, const char* szElement);
bool IsDeprecatedFlaggedSequence(Module* mod, const char* szSequenceName);
bool IsDeprecatedFlaggedOperation(Module* mod, const char* szOperationName);

// Returns true when an element is flagged as deprecated AND shall not be written to the output
bool IsDeprecatedNoOutputModule(Module* mod);
bool IsDeprecatedNoOutputMember(Module* mod, const TypeDef* td, const char* szElement);
bool IsDeprecatedNoOutputSequence(Module* mod, const char* szSequenceName);
bool IsDeprecatedNoOutputOperation(Module* mod, const char* szOperationName);

#endif