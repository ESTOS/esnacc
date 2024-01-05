#if !defined(TSCOMBINED_H_INCLUDED)
#define TSCOMBINED_H_INCLUDED

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"

// When working on the typescript files itself enable the first line instead of the global eslint-disable
// #define ESLINT_DISABLE "/* eslint-disable curly, max-len, max-lines, no-unused-vars, jsdoc/require-jsdoc, @typescript-eslint/naming-convention, @typescript-eslint/no-use-before-define, @typescript-eslint/no-unused-vars */\n"
#define ESLINT_DISABLE "/* eslint-disable */\n"

void PrintTSImports(FILE* src, ModuleList* mods, Module* m, bool bIncludeConverters, bool bIncludeasn1ts, bool bIncludeTASN1Base);
void PrintTSRootTypes(FILE* src, Module* mod, const char* szSuffix);

const char* GetBERType(const enum BasicTypeChoiceId basicTypeChoiseId);
int GetContextID(struct Type* type);

const char* getCommonJSFileExtension();

#endif // TSCOMBINED_H_INCLUDED