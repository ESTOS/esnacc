#ifndef GENSWIFTCODE_H
#define GENSWIFTCODE_H

#include "../../../snacc.h"
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <string.h>
#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "../../core/snacc-util.h"

void PrintSwiftROSECode(FILE* src, ModuleList* mods, Module* m);

void PrintSwiftCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printJSONEncDec, int novolatilefuncs);

void PrintSwiftOperationFactory(FILE* src, ModuleList* mods, Module* m);

#endif // GENSWIFTCODE_H
