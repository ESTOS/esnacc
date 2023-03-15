#if !defined(TSROSE_H_INCLUDED)
#define TSROSE_H_INCLUDED

#define INTERFACETYPE_SERVER 0
#define INTERFACETYPE_CLIENT_NODE 1
#define INTERFACETYPE_CLIENT_BROWSER 2

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"

void SaveTSROSEFilesToOutputDirectory(const int genRoseStubs, const char* szPath);
void PrintTSROSECode(FILE* src, ModuleList* mods, Module* m);
void PrintTSROSEInterfaceCode(FILE* src, ModuleList* mods, Module* m);

#endif // TSROSE_H_INCLUDED