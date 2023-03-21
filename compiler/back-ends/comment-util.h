#ifndef COMMENT_UTIL_H
#define COMMENT_UTIL_H

#include <stdio.h>

typedef struct TypeDef TypeDef;
typedef struct Module Module;

const char* getDeprecated(const char* szDeprecated);
void printMemberComment(FILE* src, const Module* m, const TypeDef* td, const char* szElement);
void printSequenceComment(FILE* src, const Module* m, const TypeDef* td);
void printModuleComment(FILE* src, const char* szModuleName);
bool printComment(FILE* src, const char* szPrefix, const char* szString, const char* szSuffix);

#endif // COMMENT_UTIL_H