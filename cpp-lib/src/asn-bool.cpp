// file: .../c++-lib/src/asn-bool.C - methods for AsnBool (ASN.1 BOOLEAN) class
//
// MS 92/06/16
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
// $Header: /develop30/common/esnacc1.7/SNACC/c++-lib/src/asn-bool.cpp,v 1.2 2005/10/19 12:32:50 \stm Exp $
//

#include "../include/asn-incl.h"

_BEGIN_SNACC_NAMESPACE

AsnLen AsnBool::PEnc(AsnBufBits& b) const
{
	AsnLen len = 1;
	unsigned char c = 0x80;
	unsigned char* cBool;

	if (value)
	{
		cBool = &c;
	}
	else
	{
		c = 0x00;
		cBool = &c;
	}

	b.PutBits(cBool, 1);

	return len;
}

void AsnBool::PDec(AsnBufBits& b, AsnLen& bitsDecoded)
{
	unsigned char* cBool;

	cBool = b.GetBits(1);

	if ((cBool[0] & 0x80) == 0x80)
		value = true;
	else
		value = false;

	bitsDecoded += 1;

	free(cBool);
}

AsnLen AsnBool::BEnc(AsnBuf& b) const
{
	AsnLen l;
	l = BEncContent(b);
	BEncDefLenTo127(b, l);
	l++;
	l += BEncTag1(b, UNIV, PRIM, BOOLEAN_TAG_CODE);
	return l;
}

void AsnBool::BDec(const AsnBuf& b, AsnLen& bytesDecoded)
{
	FUNC("AsnBool::BDec()");

	AsnLen elmtLen;
	AsnTag tagId;

	tagId = BDecTag(b, bytesDecoded);
	if (tagId != MAKE_TAG_ID(UNIV, PRIM, BOOLEAN_TAG_CODE))
		throw InvalidTagException(typeName(), tagId, STACK_ENTRY);
	elmtLen = BDecLen(b, bytesDecoded);

	BDecContent(b, MAKE_TAG_ID(UNIV, PRIM, BOOLEAN_TAG_CODE), elmtLen, bytesDecoded);
}

// Decodes the content of a BOOLEAN and sets this object's value
// to the decoded value. Flags an error if the length is wrong
// or a read error occurs.
void AsnBool::BDecContent(const AsnBuf& b, AsnTag /*tagId*/, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("AsnBool::BDecContent");

	if (elmtLen != 1)
		throw BoundsException("AsnBool max length exceeded", STACK_ENTRY);

	value = (b.GetByte() != 0);
	bytesDecoded++;

	//   if (b.ReadError())
	//   {
	//      throw SNACC_EXCEPT("decoded past end of data ");
	//   }
}

AsnLen AsnBool::BEncContent(AsnBuf& b) const
{
	b.PutByteRvs((unsigned char)(value ? 0xFF : 0));
	return 1;
}

// print the BOOLEAN's value in ASN.1 value notation to the given ostream
void AsnBool::Print(std::ostream& os, unsigned short /*indent*/) const
{
	os << (value ? "TRUE" : "FALSE");
}

void AsnBool::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	os << "<BOOLEAN>";
	if (lpszTitle)
		os << lpszTitle;
	os << "-";
	Print(os);
	os << "</BOOLEAN>\n";
}

char* AsnBool::checkBoolSingleVal(const bool m_SingleVal) const
{
	bool ltemp;
	char* pError = NULL;
	char cTmperr[200];

	ltemp = value;

	if (ltemp == m_SingleVal)
	{
		return pError;
	}
	else
	{
#ifdef _WIN32
		sprintf_s(cTmperr, 200, "_______\nBOOLEAN--SingleValue Constraints:\n_______\nError: --Values must match--\nValue: %u is not equal to the Constraint Single Value: %u \n", ltemp, m_SingleVal);
#else
		snprintf(cTmperr, 200, "_______\nBOOLEAN--SingleValue Constraints:\n_______\nError: --Values must match--\nValue: %u is not equal to the Constraint Single Value: %u \n", ltemp, m_SingleVal);
#endif
		pError = _strdup(cTmperr);
		return pError;
	}
}

#if META

const AsnTypeDesc AsnBool::_desc(NULL, NULL, false, AsnTypeDesc::BOOLEAN, NULL);

const AsnTypeDesc* AsnBool::_getdesc() const
{
	return &_desc;
}

#if TCL

int AsnBool::TclGetVal(Tcl_Interp* interp) const
{
	Tcl_SetResult(interp, value ? "TRUE" : "FALSE", TCL_STATIC);
	return TCL_OK;
}

int AsnBool::TclSetVal(Tcl_Interp* interp, const char* valstr)
{
	int valval;

	if (Tcl_GetBoolean(interp, (char*)valstr, &valval) != TCL_OK)
		return TCL_ERROR;

	value = valval;

	return TCL_OK;
}

#endif /* TCL */
#endif /* META */

void AsnBool::JEnc(SJson::Value& b) const
{
	b = SJson::Value(operator bool());
}

bool AsnBool::JDec(const SJson::Value& b)
{
	if (b.isConvertibleTo(SJson::booleanValue))
	{
		operator=(b.asBool());
		return true;
	}
	else
		return false;
}

_END_SNACC_NAMESPACE
