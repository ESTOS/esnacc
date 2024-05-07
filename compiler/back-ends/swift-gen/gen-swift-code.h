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

void PrintSwiftCode(ModuleList* allMods, long longJmpVal, int genTypes, int genValues, int genEncoders, int genDecoders, int genJSONEncDec, int genPrinters, int genPrintersXML, int genFree, int novolatilefuncs, int genROSEDecoders);

#endif // GENSWIFTCODE_H
