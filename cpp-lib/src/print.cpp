// file: .../c++-lib/src/print.C
//
// MS 92
// Copyright (C) 1992 Michael Sample and the University of British Columbia
//
// This library is free software; you can redistribute it and/or
// modify it provided that this copyright/license information is retained
// in original form.
//
// If you modify this file, you must clearly indicate your changes.
//
// This source code is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// $Header: /develop30/common/esnacc1.7/SNACC/c++-lib/src/print.cpp,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
//

#include "../include/asn-incl.h"


void SNACC::Indent(std::ostream& os, unsigned short i)
{
	while (i-- > 0)
		os << '\t';
}

std::ostream& operator<<(std::ostream& os, const SNACC::AsnType& v)
{
	v.Print(os);
	return os;
}
