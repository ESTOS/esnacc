#include <ctype.h>
#include <stdio.h>

char* mytolower(char* s)
{
    char* tmp = s;

    for (;*tmp;++tmp) {
        *tmp = (char)tolower((unsigned char) *tmp);
    }

    return s;
}

int myfopen(FILE** pFile, const char *filename, const char *mode) {
    #ifdef _WIN32
        errno_t err = fopen_s(pFile, filename, mode);
        if (err != 0)
            *pFile = NULL;
    #else
        *pFile = fopen(filename, mode);
    #endif
    return *pFile == NULL;
}
