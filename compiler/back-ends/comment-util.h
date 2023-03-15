#pragma once

typedef struct TypeDef TypeDef;

void printMemberComment(FILE* src, const TypeDef* td, const char* szElement);
void printSequenceComment(FILE* src, const TypeDef* td);
void printModuleComment(FILE* src, const char* szModuleName);
void printComment(FILE* src, const char* szPrefix, const char* szString, const char* szSuffix);

