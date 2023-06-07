
// The following "SM_NO_THREADS" define allows the application/lib code that
//  uses threads to not require this define (cleaner).  In this source module
//  the thread lock logic is disabled (there is some slight performance loss
//  in the wasted no-op call to the thread lock/unlock, but the logic is
//  cleaner).  This technique also has the advantage that only this source
//  file needs the "SM_NO_THREADS" definition to turn thread locking off.

#include "../include/asn-incl.h"

_BEGIN_SNACC_NAMESPACE

#ifdef NO_THREADS

void threadDestroy()
{
}
void threadLock()
{
}
void threadUnlock()
{
}

#else

#ifdef _WIN32

#include <windows.h>
HANDLE gMutex;

#else

#include <pthread.h>
pthread_mutex_t gMutex;

#endif

#ifndef _WIN32
void initMutex(void)
{
	pthread_mutex_init(&gMutex, NULL);
}
#endif

//
//
void threadDestroy()
{
#ifdef _WIN32
	CloseHandle(gMutex);
#else
	pthread_mutex_destroy(&gMutex);
#endif
}

//
//
void threadLock()
{
#ifdef _WIN32

	gMutex = CreateMutex(NULL, false, "Win32_Mutex");
	if (gMutex)
		WaitForSingleObject(gMutex, INFINITE);

#else // _WIN32

#ifdef SUNOS
	pthread_once_t once_block = {PTHREAD_ONCE_INIT};
#else
	pthread_once_t once_block = PTHREAD_ONCE_INIT;
#endif

	pthread_once(&once_block, initMutex);

	pthread_mutex_lock(&gMutex);
#endif // _WIN32
}

void threadUnlock()
{
#ifdef _WIN32
	ReleaseMutex(gMutex);
#else  // UNIX
	pthread_mutex_unlock(&gMutex);
#endif // _WIN32
}

#endif
_END_SNACC_NAMESPACE
// EOF vda_threads2.cpp
