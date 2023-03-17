#ifndef GENSWIFTCOMBINED_H
#define GENSWIFTCOMBINED_H

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define PRINTCOMMENT(src, sect, ...) {src; sect; __VA_ARGS__;}
#define PRINTCOMMENT(src, sect, ...) fprintf(src, sect, __VA_ARGS__)
// #define ADDSWIFTCOMMENTS

void PrintSwiftImports(FILE* src, ModuleList* mods, Module* m, bool bIncludeConverters);
const char* GetSwiftType(const enum BasicTypeChoiceId basicTypeChoiseId);

#endif // GENSWIFTCOMBINED_H