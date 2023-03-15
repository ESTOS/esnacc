#if !defined(TSCOMBINED_H_INCLUDED)
#define TSCOMBINED_H_INCLUDED

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"

#ifdef _DEBUG
    #define ESLINT_DISABLE "/* eslint-disable curly, max-len, max-lines, no-unused-vars, jsdoc/require-jsdoc, @typescript-eslint/naming-convention, @typescript-eslint/no-use-before-define, @typescript-eslint/no-unused-vars */\n"
#else
    #define ESLINT_DISABLE "/* eslint-disable */\n"
#endif

void printTSImports(FILE* src, ModuleList* mods, Module* m, bool bIncludeConverters, bool bIncludeasn1ts);
const char* GetBERType(const enum BasicTypeChoiceId basicTypeChoiseId);
int getContextID(struct Type* type);

#endif // TSCOMBINED_H_INCLUDED