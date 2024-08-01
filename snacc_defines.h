#ifndef SNACC_DEFINES_H
#define SNACC_DEFINES_H

#ifdef __cplusplus

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#if !BOOL_BUILTIN
#ifndef true
/* enum bool { false, true }; */
/* the above looks elegant, but leads to anachronisms (<, ==, !=, ... return value of type int, not enum bool), therefore: */
typedef int bool;
enum
{
	false,
	true
};
#endif
#endif

#else /* !__cplusplus */

/* Type Definitions */
#ifndef bool
typedef char bool;
#endif

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#endif /* __cplusplus */

#endif