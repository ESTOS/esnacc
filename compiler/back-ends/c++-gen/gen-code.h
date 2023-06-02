#if !defined(GENCODE_H_INCLUDED)
#define GENCODE_H_INCLUDED

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
#include "rules.h"
#include "../../core/snacc-util.h"

void PrintCxxCode(FILE* src, FILE* hdr, if_META(MetaNameStyle genMeta COMMA
	const Meta* meta COMMA MetaPDU* metapdus COMMA)
	ModuleList* mods, Module* m, CxxRules* r, long longJmpVal,
	int printTypes, int printValues, int printEncoders,
	int printDecoders, int printJSONEncDec, int printPrinters, int printPrintersXML, int printFree,
	if_TCL(int printTcl COMMA) int novolatilefuncs, const char* szCppHeaderIncludePath);

//ROSE Protocol Stub (V2) - ste 22.10.2014
void PrintROSECode(FILE* src, FILE* hdr, FILE* hdrInterface, ModuleList* mods, Module* m, CxxRules* r, const char* szCppHeaderIncludePath);
//Forward declarations
void PrintForwardDeclarationsCode(FILE* hdrForwardDecl, ModuleList* mods, Module* m);

int HasROSEOperations(Module* m);

enum BasicTypeChoiceId ParanoidGetBuiltinType(Type* t);
void PrintConditionalIncludeOpen(FILE* f, char* fileName);
void PrintConditionalIncludeClose(FILE* f, char* fileName);
void PrintCxxAnyCode(FILE* src, FILE* hdr, CxxRules* r,
	ModuleList* mods, Module* m);
void PrintCxxValueDef(FILE* src, CxxRules* r, ValueDef* v);
void PrintCxxValueExtern(FILE* hdr, CxxRules* r, ValueDef* v);
char* LookupNamespace(Type* t, ModuleList* mods);
void PrintROSEAnyCode(FILE* src, FILE* hdr, CxxRules* r, ModuleList* mods, Module* m);
void PrintROSEOperationDefines(FILE* hdr, CxxRules* r, ValueDef* v, int bCS);
int PrintROSEOperationRegistration(FILE* src, CxxRules* r, ValueDef* v);

extern char* bVDAGlobalDLLExport;
extern int gNO_NAMESPACE;
extern const char* gAlternateNamespaceString;
extern int genPERCode;
//extern short ImportedFilesG;
extern int genCodeCPPPrintStdAfxInclude;		//Print stdafx.h includes

static const char	bufTypeNameG[] = "AsnBuf";
static const char	lenTypeNameG[] = "AsnLen";
static const char	tagTypeNameG[] = "AsnTag";
static const char	envTypeNameG[] = "ENV_TYPE";
static const char	baseClassesG[] = "AsnType";

extern long 	longJmpValG;
extern int		printTypesG;
extern int		printEncodersG;
extern int		printDecodersG;
extern int		printJSONEncDecG;
extern int		printPrintersG;
extern int		printPrintersXMLG;
extern int		printFreeG;

#if META
static MetaNameStyle	printMetaG;
static MetaPDU* meta_pdus_G;
#if TCL
static int		printTclG;
#endif
#endif /* META */

void PrintCxxEncodeContaining(Type* t, CxxRules* r, FILE* src, const char* szIndet);
void PrintCxxDecodeContaining(Type* t, CxxRules* r, FILE* src, const char* szIndet);
void PrintCxxPEREncodeContaining(Type* t, CxxRules* r, FILE* src);
void PrintCxxPERDecodeContaining(Type* t, CxxRules* r, FILE* src);
void PrintCxxSetTypeByCode(NamedType* defByNamedType, CxxTRI* cxxtri, FILE* src, const char* szIndet);

#endif //GENCODE_H_INCLUDED