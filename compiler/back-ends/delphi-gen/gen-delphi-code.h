#if !defined(DELPHIGENCODE_H_INCLUDED)
#define DELPHIGENCODE_H_INCLUDED

#include "../../../snacc.h"
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

void PrintDelphiCode(FILE* src, ModuleList* mods, Module* m);


#endif //DELPHIGENCODE_H_INCLUDED