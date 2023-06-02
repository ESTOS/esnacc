#if !defined(SWIFTGENCODE_H_INCLUDED)
#define SWIFTGENCODE_H_INCLUDED

#include "snacc.h"
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <string.h>
#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "../../core/snacc-util.h"

void PrintSwiftROSECodeOLD(FILE* src, ModuleList* mods, Module* m);

void PrintSwiftCodeOLD(FILE* src, ModuleList* mods, Module* m, long longJmpVal,
	int printTypes, int printValues, int printEncoders,
	int printDecoders, int printJSONEncDec,
	int novolatilefuncs);

void PrintSwiftOperationFactoryOLD(FILE* src, ModuleList* mods);


#endif //SWIFTGENCODE_H_INCLUDED