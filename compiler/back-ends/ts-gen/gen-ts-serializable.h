#if !defined(TS_SERIALIZABLE_H_INCLUDED)
#define TS_SERIALIZABLE_H_INCLUDED

#include "snacc.h"
#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"

bool ContainsSerializables(Module* m);

void PrintTSSerializableCode(FILE* src, ModuleList* mods, Module* m, int novolatilefuncs);

#endif // TS_SERIALIZABLE_H_INCLUDED
