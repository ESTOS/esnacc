#if !defined(GENVALS_H_INCLUDED)
#define GENVALS_H_INCLUDED

void PrintCxxValueDef(FILE *src, CxxRules *r, ValueDef *v);
void PrintCxxValueExtern(FILE *hdr, CxxRules *r, ValueDef *v);
void PrintROSEOperationDefines(FILE *hdr, CxxRules *r, ValueDef *v, int bCS);
int PrintROSEOperationRegistration(FILE *src, CxxRules *r, ValueDef *v);

#endif