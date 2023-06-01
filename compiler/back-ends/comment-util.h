#ifndef COMMENT_UTIL_H
#define COMMENT_UTIL_H

#include <stdio.h>

typedef struct TypeDef TypeDef;
typedef struct Module Module;
#ifndef bool
typedef char bool;
#endif

enum COMMENTSTYLE
{         
    COMMENTSTYLE_JSON = 0,
    COMMENTSTYLE_CPP = 1,
    COMMENTSTYLE_SWIFT = 2
};

const char* getDeprecated(const char* szDeprecated, enum COMMENTSTYLE style);
void printMemberComment(FILE* src, const Module* m, const TypeDef* td, const char* szElement, const char* szIndent, enum COMMENTSTYLE style);
void printSequenceComment(FILE* src, const Module* m, const TypeDef* td, enum COMMENTSTYLE style);
void printModuleComment(FILE* src, const char* szModuleName, enum COMMENTSTYLE style);
bool printOperationComment(FILE* src, const Module* m, const char* szOperationName, enum COMMENTSTYLE style);
bool printComment(FILE* src, const char* szPrefix, const char* szString, const char* szSuffix);

#endif // COMMENT_UTIL_H