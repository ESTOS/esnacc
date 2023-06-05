#if !defined(TSCONVERTER_H_INCLUDED)
#define TSCONVERTER_H_INCLUDED

#include "snacc.h"
#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"

void SaveTSConverterFilesToOutputDirectory(const char* szPath);

void PrintTSConverterCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printJSONEncDec, int novolatilefuncs);

#endif // TSCONVERTER_H_INCLUDED