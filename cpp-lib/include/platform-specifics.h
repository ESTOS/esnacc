#ifndef PLATFORMSPECIFICS_CPP_H
#define PLATFORMSPECIFICS_CPP_H

#ifndef _strdup
    #define _strdup(arg) strdup(arg)
#endif
#ifndef strncpy_s
    #define strncpy_s(dest, len, source, amount) strncpy(dest, source, amount)
#endif

#endif // PLATFORMSPECIFICS_CPP_H