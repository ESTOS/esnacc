/* JKG -- 7/31/03 --  */
/*  ../SNACC/compiler/back-ends/c++-gen/cxxconstraints.h  */
/*  This file contains prototypes of functions for        */
/*  checking constraints and converting .asn1             */
/*  primitives (with constraints) into valid C++ code     */
/*  Their respective implementations are in:              */
/*       'cxxconstraints.c' in the pwd                    */

#ifndef _CXXCONSTRAINTS_H_
#define _CXXCONSTRAINTS_H_

#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "snacc.h"
#include "limits.h"
#include "rules.h"
#include "../../core/snacc-util.h"

// void PrintCxxSetOfSizeValRangeConstraints PROTO((FILE* hdr, FILE* src, TypeDef* td));

// void PrintCxxSetOfSizeSingleValConstraints PROTO((FILE* hdr, FILE* src, TypeDef* td));

void PrintTypeDefDefault PROTO((FILE * hdr, FILE* src, TypeDef* td));

void PrintCxxSetOfSizeConstraint(FILE* hdr, SubtypeValue* sizeConstraint, Module* m, Type* type);

#endif
