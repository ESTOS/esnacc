//
// asn-stringType.cpp
//

#include "../include/asn-incl.h"
#include "../include/asn-stringconvert.h"

#if defined(HPUX) || defined(HPUX32)

/* for some strange reason GCC 3.2.1 on HPUX has trouble specializing the
 * basic_string<> template for wchar_t.  Do the following is necessary on HPUX
 * until GCC is fixed or we figure out another work around.
 */

namespace std
{
	const basic_string<wchar_t>::size_type basic_string<wchar_t>::_Rep::_S_max_size = (((npos - sizeof(_Rep)) / sizeof(wchar_t)) - 1) / 4;

	const wchar_t basic_string<wchar_t>::_Rep::_S_terminal = wchar_t();
} // namespace std

#endif // HPUX

#if _WIN32
#include <windows.h>
#endif

using namespace SNACC;

/*
** This lookup table is used to help decode the first byte of
** a multi-byte UTF8 character.
*/
/*
static const unsigned char snaccUtf8Trans1[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
};
*/

#define MAX_UTF8_OCTS_PER_CHAR 6

typedef struct
{
	unsigned char mask;
	unsigned char value;
	unsigned short bits;
	unsigned long maxCharValue;
} MaskValue;

/*
const MaskValue gUTF8Masks[6] = {
	{ 0x80, 0x00, 1, 0x0000007F },		// one-byte encoding
	{ 0xE0, 0xC0, 3, 0x000007FF },		// two-byte encoding
	{ 0xF0, 0xE0, 4, 0x0000FFFF },		// three-byte encoding
	{ 0xF8, 0xF0, 5, 0x0001FFFF },		// four-byte encoding
	{ 0xFC, 0xF8, 6, 0x03FFFFFF },		// five-byte encoding
	{ 0xFE, 0xFC, 7, 0x07FFFFFF }		// six-byte encoding
};
*/

char* AsnString::getChar(long offset) const
{
	return (char*)&(*this)[offset];
}

const char* AsnString::PermittedAlphabet(int& sizeAlpha) const
{
	sizeAlpha = 256;
	static const unsigned char baseAlpha[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	};

	return (char*)baseAlpha;
}

int AsnString::findB2(int B) const
{
	int power = 0;
	int B2 = 0;
	int B2NotFound = 1;

	while (B2NotFound == 1)
	{
		B2 = (int)(1 << power);
		if (B <= B2)
			B2NotFound = 0;
		power++;
	}

	return B2;
}

int AsnString::numBits() const
{
	int B = 0;
	int N;

	PermittedAlphabet(N);

	N -= 1;
	while (N > 0)
	{
		N -= (long)(1 << B);
		B += 1;
	}

	return B;
}

long AsnString::FindSizeConstraintBounds(int& iSCLowerBound, int& iSCUpperBound) const
{
	int count = 0;
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);

	while (count < numSizeConstraints)
	{
		if ((unsigned)iSCUpperBound < sizeConstraints[count].lowerBound)
			iSCUpperBound = sizeConstraints[count].lowerBound;

		if (sizeConstraints[count].upperBoundExists == 1 && (unsigned)iSCUpperBound < sizeConstraints[count].upperBound)
			iSCUpperBound = sizeConstraints[count].upperBound;

		if ((unsigned)iSCLowerBound > sizeConstraints[count].lowerBound && sizeConstraints[count].lowerBound >= 0)
			iSCLowerBound = sizeConstraints[count].lowerBound;

		count++;
	}

	return ((iSCUpperBound - iSCLowerBound) + 1);
}

AsnLen AsnString::EncodeWithSizeConstraint(AsnBufBits& b) const
{
	FUNC("AsnString::EncodeWithSizeConstraint");

	AsnLen len = 0;
	int B = numBits();
	int B2 = findB2(B);
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	int iSCLowerBound = sizeConstraints[0].lowerBound;
	int iSCUpperBound = iSCLowerBound;
	int minBitsNeeded = 0;
	int minBytesNeeded = 0;
	long Range = FindSizeConstraintBounds(iSCLowerBound, iSCUpperBound);
	long tempRange = Range - 1;
	long size = (long)length();
	long count = 0;
	unsigned char* pStr = new unsigned char[1];

	while (tempRange > 0)
	{
		tempRange -= (long)(1 << minBitsNeeded);
		minBitsNeeded += 1;
	}

	if (size < iSCLowerBound || size > iSCUpperBound)
		throw EXCEPT("String size not withing restricted bounds", RESTRICTED_TYPE_ERROR);

	if (Range > 1)
	{
		if (((iSCUpperBound * B2) > 16) && b.IsAligned())
			len += b.OctetAlignWrite();

		minBytesNeeded = minBitsNeeded / 8;
		minBitsNeeded = minBitsNeeded % 8;
		size -= iSCLowerBound;

		if (minBytesNeeded > 0)
		{
			pStr[0] = (unsigned char)(size >> minBitsNeeded);
			len += b.PutBits(pStr, 8);
		}

		pStr[0] = (unsigned char)size;
		pStr[0] <<= 8 - minBitsNeeded;
		len += b.PutBits(pStr, minBitsNeeded);
	}

	if (((iSCUpperBound * B2) > 16) && b.IsAligned())
		len += b.OctetAlignWrite();

	while ((unsigned)count < length())
	{
		len += Interpret(b, count);
		count++;
	}

	delete[] pStr;
	return len;
}

void AsnString::DecodeWithSizeConstraint(AsnBufBits& b, AsnLen& bitsDecoded)
{
	FUNC("AsnString::DecodeWithSizeConstraint");

	int B = numBits();
	int B2 = findB2(B);
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	int iSCLowerBound = sizeConstraints[0].lowerBound;
	int iSCUpperBound = iSCLowerBound;
	int minBitsNeeded = 0;
	int minBytesNeeded = 0;
	long Range = FindSizeConstraintBounds(iSCLowerBound, iSCUpperBound);
	long tempRange = Range - 1;
	long count = 0;
	long decodeSize = 0;
	unsigned char* pStr = new unsigned char[1];

	Clear();

	while (tempRange > 0)
	{
		tempRange -= (long)(1 << minBitsNeeded);
		minBitsNeeded += 1;
	}

	if (Range > 1)
	{
		if (((iSCUpperBound * B2) > 16) && b.IsAligned())
			bitsDecoded += b.OctetAlignRead();

		minBytesNeeded = minBitsNeeded / 8;
		minBitsNeeded = minBitsNeeded % 8;

		if (minBytesNeeded > 0)
		{
			delete[] pStr;
			;
			pStr = b.GetBits(8);
			bitsDecoded += 8;
			decodeSize <<= 8;
			decodeSize |= (long)pStr[0];
		}

		delete[] pStr;
		;
		pStr = b.GetBits(minBitsNeeded);
		bitsDecoded += minBitsNeeded;

		if (minBitsNeeded > 0)
		{
			decodeSize <<= minBitsNeeded;
			pStr[0] >>= (8 - minBitsNeeded);
			decodeSize |= (long)pStr[0];
		}
	}

	decodeSize += iSCLowerBound;

	if (decodeSize > iSCUpperBound)
		throw EXCEPT("String size not withing restricted bounds", RESTRICTED_TYPE_ERROR);

	if (((iSCUpperBound * B2) > 16) && b.IsAligned())
		bitsDecoded += b.OctetAlignRead();

	while (count < decodeSize)
	{
		Deterpret(b, bitsDecoded, count);
		count++;
	}

	delete[] pStr;
}

AsnLen AsnString::Interpret(AsnBufBits& b, long offset) const
{
	AsnLen len;
	int B = numBits();
	int B2 = findB2(B);
	int sizepermittedalpha;
	const char* permittedAlphabet = PermittedAlphabet(sizepermittedalpha);
	int ub = (int)permittedAlphabet[sizepermittedalpha - 1];
	bool bNotFound = true;
	int count = 0;

	if (b.IsAligned())
		len = B2;
	else
		len = B;

	unsigned char* seg = (unsigned char*)getChar(offset);

	if (ub <= ((1 << len) - 1))
	{
		len = (sizeof(char) * 8);
	}
	else
	{
		while (bNotFound)
		{
			if (permittedAlphabet[count] == seg[0])
			{
				seg[0] = (char)count;
				bNotFound = false;
			}
			count++;
		}
	}

	seg[0] <<= ((sizeof(char) * 8) - len);
	b.PutBits(seg, len);

	return len;
}

AsnString& AsnString::operator=(const char* str)
{
	if (str == NULL)
		erase();
	else
		assign(str);

	return *this;
}

void AsnString::Deterpret(AsnBufBits& b, AsnLen& bitsDecoded, long offset)
{
	AsnLen len;
	int B = numBits();
	int B2 = findB2(B);
	int count = 0;
	bool bNotFound = true;
	int sizePermittedAlpha;
	const char* permittedAlphabet = PermittedAlphabet(sizePermittedAlpha);
	int ub = (int)permittedAlphabet[sizePermittedAlpha - 1];

	if (b.IsAligned())
		len = B2;
	else
		len = B;

	if (ub <= ((1 << len) - 1))
		len = (sizeof(char) * 8);

	unsigned char* seg = b.GetBits(len);
	bitsDecoded += len;
	seg[0] >>= ((sizeof(char) * 8) - len);

	if (!(ub <= ((1 << len) - 1)))
	{
		while (bNotFound)
		{
			if (count == (int)seg[0])
			{
				seg[0] = permittedAlphabet[count];
				bNotFound = false;
			}
			count++;
		}
	}

	putChar((char*)seg);
	free(seg);
}

void AsnString::PDec(AsnBufBits& b, AsnLen& bitsDecoded)
{
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);

	if (sizeConstraints == NULL && numSizeConstraints == 0)
		DecodeGeneral(b, bitsDecoded);
	else
		DecodeWithSizeConstraint(b, bitsDecoded);
}

AsnLen AsnString::PEnc(AsnBufBits& b) const
{
	FUNC("AsnString::PEnc");
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);

	if (checkConstraints(NULL))
		throw ConstraintException("String not within constraints", STACK_ENTRY);

	if (sizeConstraints == NULL && numSizeConstraints == 0)
		return EncodeGeneral(b);
	else
		return EncodeWithSizeConstraint(b);
}

AsnLen AsnString::BEnc(AsnBuf& b) const
{
	FUNC("AsnString::BEnc");

	if (checkConstraints(NULL))
		throw ConstraintException("String not within constraints", STACK_ENTRY);

	AsnLen l = BEncContent(b);
	l += BEncDefLen(b, l);
	l += BEncTag1(b, UNIV, PRIM, tagCode());
	return l;
}

void AsnString::BDec(const AsnBuf& b, AsnLen& bytesDecoded)
{
	FUNC("AsnStringType::BDec()");

	AsnTag tag = BDecTag(b, bytesDecoded);
	if ((tag != MAKE_TAG_ID(UNIV, PRIM, tagCode())) && (tag != MAKE_TAG_ID(UNIV, CONS, tagCode())))
		throw InvalidTagException(typeName(), tag, STACK_ENTRY);

	AsnLen elmtLen1 = BDecLen(b, bytesDecoded);
	BDecContent(b, tag, elmtLen1, bytesDecoded);
}

void AsnString::JEnc(SJson::Value& b) const
{
	b = SJson::Value(c_str());
}

bool AsnString::JDec(const SJson::Value& b)
{
	clear();
	if (b.isString())
	{
		operator=(b.asCString());
		return true;
	}
	return false;
}

AsnLen AsnString::BEncContent(AsnBuf& b) const
{
	FUNC("AsnString::BEncContent()");

#ifndef DISABLE_STRING_CHECK
	if (!check())
		throw EXCEPT("Invalid character present", RESTRICTED_TYPE_ERROR);
#endif

	b.PutSegRvs(c_str(), length());
	return (int)length();
}

void AsnString::BDecContent(const AsnBuf& b, AsnTag tagId, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("AsnString::BDecContent()");

	// Erase the existing characters
	erase();

	// If tag is constructed...
	if (TAG_IS_CONS(tagId))
	{
		BDecConsString(b, elmtLen, bytesDecoded);
	}
	else // primitive string

		if (elmtLen != INDEFINITE_LEN) // RWC; TMP disable -1,indefinite length value
									   // RWC;  FOUND through NISCC tests for EnvelopedData
		{
			b.GetSeg(*this, elmtLen);
			bytesDecoded += elmtLen;
		}
		else
		{
			throw BoundsException("Indefinite length not allowed on primitive", STACK_ENTRY);
		}

	/*RWC;DISABLE; USER CAN CALL SEPARATELY;#ifndef DISABLE_STRING_CHECK
		if (!check())
			throw EXCEPT("Invalid character present", RESTRICTED_TYPE_ERROR);
	#endif;RWC;*/
}

void AsnString::Print(std::ostream& os, unsigned short indent) const
{
	os << c_str();
	if (!indent)
		os << std::endl;
}

void AsnString::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	const char* title = lpszTitle;
	if (title == NULL)
		title = typeName();

	os << "<" << title << ">" << c_str() << "</" << title << ">";
}

void AsnString::BDecConsString(const AsnBuf& b, AsnLen elmtLen, AsnLen& bytesDecoded)

{
	FUNC("AsnString::BDecConsString()");

	AsnLen totalElmtsLen = 0;

	while ((totalElmtsLen < elmtLen) || (elmtLen == INDEFINITE_LEN))

	{
		AsnTag innerTag = BDecTag(b, totalElmtsLen);

		if ((innerTag == EOC_TAG_ID) && (elmtLen == INDEFINITE_LEN))
		{
			BDEC_2ND_EOC_OCTET(b, totalElmtsLen);

			break;
		}

		AsnLen innerLen = BDecLen(b, totalElmtsLen);

		if (innerTag == MAKE_TAG_ID(UNIV, PRIM, OCTETSTRING_TAG_CODE))
		{
			char* seg = new char[innerLen];

			if (seg == NULL)
				throw MemoryException(elmtLen, "seg", STACK_ENTRY);

			b.GetSeg(seg, innerLen);

			totalElmtsLen += innerLen;

			append(seg, innerLen);

			delete[] seg;
		}
		else if (innerTag == MAKE_TAG_ID(UNIV, CONS, OCTETSTRING_TAG_CODE))
		{
			BDecConsString(b, innerLen, totalElmtsLen);
		}
		else // wrong tag
			throw InvalidTagException(typeName(), innerTag, STACK_ENTRY);
	}

	bytesDecoded += totalElmtsLen;
} // end of AsnString::BDecConsString()

WideAsnString::WideAsnString()
{
}

WideAsnString::WideAsnString(const wchar_t* szUTF16)
	: std::wstring(szUTF16)
{
}

WideAsnString::WideAsnString(const std::wstring& strUTF16)
	: std::wstring(strUTF16)
{
}

WideAsnString::WideAsnString(const char* szASCII)
{
	setASCII(szASCII);
}

WideAsnString::WideAsnString(const std::string& strASCII)
{
	setASCII(strASCII.c_str());
}

void WideAsnString::setASCII(const char* strAscii)
{
	assign(AsnStringConvert::AsciiToUTF16(strAscii));
}

void WideAsnString::setASCII(const std::string& strAscii)
{
	assign(AsnStringConvert::AsciiToUTF16(strAscii.c_str()));
}

std::string WideAsnString::getASCII() const
{
	return AsnStringConvert::UTF16ToAscii(c_str());
}

void WideAsnString::getASCII(std::string& strAscii) const
{
	strAscii = AsnStringConvert::UTF16ToAscii(c_str());
}

void WideAsnString::setUTF8(const char* szUTF8)
{
	assign(AsnStringConvert::UTF8ToUTF16(szUTF8));
}

void WideAsnString::setUTF8(const std::string& strUTF8)
{
	assign(AsnStringConvert::UTF8ToUTF16(strUTF8.c_str()));
}

std::string WideAsnString::getUTF8() const
{
	return AsnStringConvert::UTF16ToUTF8(c_str());
}

void WideAsnString::getUTF8(std::string& strUTF8) const
{
	strUTF8 = AsnStringConvert::UTF16ToUTF8(c_str());
}

void WideAsnString::setUTF16(const wchar_t* szUTF16)
{
	assign(szUTF16);
}

void WideAsnString::setUTF16(const std::wstring& strUTF16)
{
	assign(strUTF16);
}

std::wstring WideAsnString::getUTF16() const
{
	return *this;
}

void WideAsnString::getUTF16(std::wstring& strUTF16) const
{
	strUTF16 = *this;
}

void WideAsnString::PDec(AsnBufBits& b, AsnLen& bitsDecoded)
{
	DecodeGeneral(b, bitsDecoded);
}

AsnLen WideAsnString::PEnc(AsnBufBits& b) const
{
	FUNC("WideAsnString::PEnc");

	if (checkConstraints(NULL))
		throw ConstraintException("Wide string not within constraints", STACK_ENTRY);

	return EncodeGeneral(b);
}

AsnLen WideAsnString::BEnc(AsnBuf& b) const
{
	FUNC("WideAsnString::BEnc");

	if (checkConstraints(NULL))
		throw ConstraintException("Wide string not within constraints", STACK_ENTRY);

	AsnLen l = BEncContent(b);
	l += BEncDefLen(b, l);
	l += BEncTag1(b, UNIV, PRIM, tagCode());
	return l;
}

void WideAsnString::BDec(const AsnBuf& b, AsnLen& bytesDecoded)
{
	FUNC("WideAsnString::BDec()");

	AsnTag tag = BDecTag(b, bytesDecoded);
	if ((tag != MAKE_TAG_ID(UNIV, PRIM, tagCode())) && (tag != MAKE_TAG_ID(UNIV, CONS, tagCode())))
		throw InvalidTagException(typeName(), tag, STACK_ENTRY);

	AsnLen elmtLen1 = BDecLen(b, bytesDecoded);
	BDecContent(b, tag, elmtLen1, bytesDecoded);
}

void WideAsnString::JEnc(SJson::Value& b) const
{
	b = SJson::Value(c_str());
}

bool WideAsnString::JDec(const SJson::Value& b)
{
	clear();
	if (b.isConvertibleTo(SJson::stringValue))
	{
		setUTF8(b.asString().c_str());
		return true;
	}
	return false;
}

void WideAsnString::Print(std::ostream& os, unsigned short indent) const
{
	std::string asciiForm;
	getASCII(asciiForm);
	os << asciiForm.c_str();
	if (!indent)
		os << std::endl;
}

void WideAsnString::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	const char* title = lpszTitle;
	if (title == NULL)
		title = typeName();

	std::string utf8Form;
	getUTF8(utf8Form);
	os << "<" << title << ">" << utf8Form.c_str() << "</" << title << ">";
}

void WideAsnString::Deterpret(AsnBufBits& b, AsnLen& bitsDecoded, long offset)
{
	wchar_t* seg = (wchar_t*)b.GetBits(sizeof(wchar_t));
	bitsDecoded += (sizeof(wchar_t));

	putWideChar(seg);
}

AsnLen WideAsnString::CombineConsString(const AsnBuf& b, AsnLen elmtLen, std::string& encStr)
{
	FUNC("WideAsnString::CombineConsString()");

	AsnLen totalElmtsLen = 0;
	while ((totalElmtsLen < elmtLen) || (elmtLen == INDEFINITE_LEN))
	{
		AsnTag innerTag = BDecTag(b, totalElmtsLen);

		if ((innerTag == EOC_TAG_ID) && (elmtLen == INDEFINITE_LEN))
		{
			BDEC_2ND_EOC_OCTET(b, totalElmtsLen);
			break;
		}

		AsnLen innerLen = BDecLen(b, totalElmtsLen);
		if (innerTag == MAKE_TAG_ID(UNIV, PRIM, OCTETSTRING_TAG_CODE))
		{
			char* seg = new char[elmtLen];
			if (seg == NULL)
				throw MemoryException(elmtLen, "seg", STACK_ENTRY);
			b.GetSeg(seg, elmtLen);
			totalElmtsLen += elmtLen;

			encStr.append(seg, elmtLen);
			innerLen -= elmtLen;
			delete[] seg;
		}
		else if (innerTag == MAKE_TAG_ID(UNIV, CONS, OCTETSTRING_TAG_CODE))
		{
			totalElmtsLen += CombineConsString(b, innerLen, encStr);
		}
		else // wrong tag
			throw InvalidTagException(typeName(), innerTag, STACK_ENTRY);
	}

	return totalElmtsLen;
} // end of WideAsnString::CombineConsString()

bool NumericString::check() const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		// Check for 0-9
		if ((*i < '0') || (*i > '9'))
		{
			// Check for space
			if (*i != ' ')
				return false;
		}
	}
	return true;
}

const char* NumericString::PermittedAlphabet(int& alphaSize) const
{
	alphaSize = 11;
	static const char kNumAlpha[] = " 0123456789";
	return kNumAlpha;
}

const SizeConstraint* NumericString::SizeConstraints(int& sizeList) const
{
	sizeList = 0;
	return NULL;
}

const char* PrintableString::PermittedAlphabet(int& sizeAlpha) const
{
	sizeAlpha = 74;
	static char pPbleAlpha[] = {0x20, 0x27, 0x28, 0x29, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3D, 0x3F, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A};

	return pPbleAlpha;
}

bool PrintableString::check() const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		// Check for A-Z
		if ((*i < 'A') || (*i > 'Z'))
		{
			// Check for a-z
			if ((*i < 'a') || (*i > 'z'))
			{
				// Check for 0-9
				if ((*i < '0') || (*i > '9'))
				{
					switch (*i)
					{
						case ' ':  // space
						case '\'': // apostrophe
						case '(':  // left parenthesis
						case ')':  // right parenthesis
						case '+':  // plus sign
						case ',':  // comma
						case '-':  // hyphen-minus
						case '.':  // full stop (period)
						case '/':  // solidus
						case ':':  // colon
						case '=':  // equal sign
						case '?':  // question mark
							break;

						default:
							return false;
					}
				}
			}
		}
	}
	return true;
}

wchar_t* WideAsnString::getWideChar(long offset) const
{
	return (wchar_t*)&(*this)[offset];
}

AsnLen WideAsnString::Interpret(AsnBufBits& b, long offset) const
{
	AsnLen len = sizeof(wchar_t);

	unsigned char* seg = (unsigned char*)getWideChar(offset);

	b.PutBits(seg, len);

	return len;
}

const char* WideAsnString::checkStringTypPermittedAlpha(const char* permittedAlphabet, long permittedAlphabetSize) const
{
	char* pError = NULL;
	const char* cstr;
	int found = 0;
	int x = 0;
	int count = (int)length();

	if (count > 0)
	{
		std::string utf8Form;
		getUTF8(utf8Form);

		cstr = utf8Form.c_str();

		while (count)
		{
			found = 0;
			for (x = 0; x < permittedAlphabetSize; x++)
				if (permittedAlphabet[x] == cstr[count - 1])
					found = 1;

			if (found == 0)
				break;

			count--;
		}

		if (found == 1)
		{
			return pError;
		}
		else
		{
			return ConstraintErrorStringList[WIDE_STRING_PERMITTED_ALPHA];
			;
		}
	}
	return pError;
}

int WideAsnString::checkConstraints(ConstraintFailList* pConstraintFails) const
{
	int count = 0;
	int sizefailed = 1;
	int alphafailed = 1;
	std::string ptr;
	const char* tmpptr = NULL;
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	int sizePermittedAlpha;
	const char* permittedAlphabet = PermittedAlphabet(sizePermittedAlpha);

	if (sizeConstraints)
	{
		for (count = 0; count < numSizeConstraints; count++)
		{
			tmpptr = NULL;
			if (sizeConstraints[count].upperBoundExists == 1)
			{
				if ((sizeConstraints[count].lowerBound > (getUTF8().size())) || (sizeConstraints[count].upperBound < (getUTF8().size())))
					tmpptr = ConstraintErrorStringList[WIDE_STRING_SIZE_VALUE_RANGE];
			}
			else
			{
				if (sizeConstraints[count].lowerBound != getUTF8().size())
					tmpptr = ConstraintErrorStringList[WIDE_STRING_SIZE_SINGLE_VALUE];
			}

			if (tmpptr)
				ptr += tmpptr;
			else
				sizefailed = 0;
		}
	}
	else
	{
		sizefailed = 0;
	}

	if (sizePermittedAlpha > 0)
	{
		tmpptr = NULL;
		tmpptr = checkStringTypPermittedAlpha(permittedAlphabet, sizePermittedAlpha);

		if (tmpptr)
			ptr += tmpptr;
		else
			alphafailed = 0;
	}
	else
	{
		alphafailed = 0;
	}

	if (sizefailed || alphafailed)
	{
		if (pConstraintFails != NULL)
			pConstraintFails->push_back(ptr);
		return 1;
	}

	return 0;
}

/*
char* WideAsnString::checkStringTypSize(unsigned int m_Size)const
{

	char* pError=NULL;
	char cTmperr[300];
	std::string utf8Form;
	getUTF8(utf8Form);

   if(m_Size>=(strlen(utf8Form.c_str())))
   {

	return pError;
   }
   else
   {

		snprintf(cTmperr, 300, "_______\nRestricted Char String Type--Size Constraint:\n_______\nError: --String length must be less < or = to Size Constraint--\nValue: %d is not < or = to Size Constraint: %d \n", strlen(utf8Form.c_str()), m_Size);
		pError = strdup(cTmperr);
		return pError;

   }

   return pError;
}

char* WideAsnString::checkStringTypSize(unsigned int m_LowerSize, unsigned int m_UpperSize)const
{

	char* pError=NULL;
	char cTmperr[500];
	std::string utf8Form;
	getUTF8(utf8Form);

   if(m_UpperSize>=(strlen(utf8Form.c_str())) && m_LowerSize<=(strlen(utf8Form.c_str())))
   {

	return pError;
   }
   else
   {

		snprintf(cTmperr, 500, "_______\nRestricted Char String Type--Size Constraint:\n_______\nError: --String length must be between bounds--\nValue: %d is not between Lower Size Constraint: %d\n             and Upper Size Constraint %d \n", strlen(utf8Form.c_str()), m_LowerSize, m_UpperSize);
		pError = strdup(cTmperr);
		return pError;

   }

   return pError;
}

*/

const char* AsnString::checkStringTypPermittedAlpha(const char* permittedAlphabet, long permittedAlphabetSize) const
{
	const char* pError = NULL;
	const char* cstr;
	int found = 0;
	int x = 0;
	int count = (int)length();

	if (count > 0)
	{
		cstr = c_str();

		while (count)
		{
			found = 0;
			for (x = 0; x < permittedAlphabetSize; x++)
				if (permittedAlphabet[x] == cstr[count - 1])
					found = 1;

			if (found == 0)
				break;

			count--;
		}

		if (found == 1)
			return pError;
		else
			return ConstraintErrorStringList[STRING_PERMITTED_ALPHA];
	}
	return pError;
}

int AsnString::checkConstraints(ConstraintFailList* pConstraintFails) const
{
	int count = 0;
	int sizefailed = 1;
	int alphafailed = 1;
	std::string ptr;
	const char* tmpptr = NULL;
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	int sizePermittedAlpha;
	const char* permittedAlphabet = PermittedAlphabet(sizePermittedAlpha);

	if (sizeConstraints)
	{
		for (count = 0; count < numSizeConstraints; count++)
		{
			tmpptr = NULL;
			if (sizeConstraints[count].upperBoundExists == 1)
			{
				if ((sizeConstraints[count].lowerBound > (strlen(c_str()))) || sizeConstraints[count].upperBound < (strlen(c_str())))
					tmpptr = ConstraintErrorStringList[STRING_SIZE_VALUE_RANGE];
			}
			else
			{
				if (sizeConstraints[count].lowerBound != (strlen(c_str())))
					tmpptr = ConstraintErrorStringList[STRING_SIZE_SINGLE_VALUE];
			}

			if (tmpptr)
				ptr += tmpptr;
			else
				sizefailed = 0;
		}
	}
	else
	{
		sizefailed = 0;
	}

	if (sizePermittedAlpha > 0)
	{
		tmpptr = NULL;
		tmpptr = checkStringTypPermittedAlpha(permittedAlphabet, sizePermittedAlpha);

		if (tmpptr)
			ptr += tmpptr;
		else
			alphafailed = 0;
	}

	if (sizefailed || alphafailed)
	{
		if (pConstraintFails != NULL)
			pConstraintFails->push_back(ptr);
		return 1;
	}

	return 0;
}

const char* IA5String::PermittedAlphabet(int& sizeAlpha) const
{
	sizeAlpha = 128;
	static const char IA5Alpha[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
									0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f};

	return IA5Alpha;
}

bool IA5String::check() const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		// Check that character is less than 128
		if (((unsigned)*i < 0) || ((unsigned)*i > 127))
			return false;
	}

	return true;
}

const char* VisibleString::PermittedAlphabet(int& sizeAlpha) const
{
	sizeAlpha = 95;

	static const char VisibleAlpha[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
										0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e};

	return VisibleAlpha;
}

bool VisibleString::check() const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		// Check that character is between ' ' and '~' inclusive
		if ((*i < ' ') || (*i > '~'))
			return false;
	}

	return true;
}

// Encode BMPString octets into byte stream
//
AsnLen BMPString::BEncContent(AsnBuf& b) const
{
	FUNC("BMPString::BEncContent");

	size_t wcharSize = sizeof(wchar_t);

	AsnLen len = 0;
	const_reverse_iterator wstrI;
	for (wstrI = rbegin(); wstrI != rend(); ++wstrI)
	{
		wchar_t wc = *wstrI;
		for (unsigned int i = 0; i < 2; ++i)
		{
			b.PutByteRvs((unsigned char)wc);
			wc >>= 8;
			++len;
		}

		// Check that the upper bytes, if any, are zero
		if ((wcharSize > 2) && (wc != 0))
			throw EXCEPT("Invalid BMPString", RESTRICTED_TYPE_ERROR);
	}
	return len;
}

void BMPString::BDecContent(const AsnBuf& b, AsnTag tagId, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("BMPString::BDecContent");

	// Erase the existing characters
	erase();

	if (elmtLen == INDEFINITE_LEN || elmtLen > b.length())
		throw MemoryException(elmtLen, "elmtLen requests for too much data", STACK_ENTRY);

	// If tag is constructed, decode and combine the segments
	std::string encStr;
	if (TAG_IS_CONS(tagId))
	{
		bytesDecoded += CombineConsString(b, elmtLen, encStr);
	}
	else // tag is primitive, just combine the one segment
	{
		char* seg = new char[elmtLen];
		if (seg == NULL)
			throw MemoryException(elmtLen, "seg", STACK_ENTRY);
		b.GetSeg(seg, elmtLen);
		bytesDecoded += elmtLen;

		encStr.append(seg, elmtLen);
		delete[] seg;
	}

	// encoding length must be a multiple of two since BMPString uses
	// 2 bytes to represent a character.
	//
	if (encStr.length() % 2 != 0)
		throw EXCEPT("Invalid BMPString length not multiple of 2", RESTRICTED_TYPE_ERROR);

	// decode BMPString into wide string
	//
	resize(encStr.length() / 2);
	std::string::const_iterator iEnc = encStr.begin();
	for (size_type i = 0; i < size(); ++i)
	{
		wchar_t wtmpCh = *iEnc++;
		wtmpCh <<= 8;
		wtmpCh |= *iEnc++;
		at(i) = wtmpCh;
	}
}

// Encode UniversalString octets into byte stream
//
AsnLen UniversalString::BEncContent(AsnBuf& b) const
{
	FUNC("UniversalString::BEncContent");

	size_t wcharSize = sizeof(wchar_t);

	AsnLen len = 0;
	const_reverse_iterator wstrI;
	for (wstrI = rbegin(); wstrI != rend(); ++wstrI)
	{
		wchar_t wc = *wstrI;
		for (unsigned int i = 0; i < 4; ++i)
		{
			if (i < wcharSize)
				b.PutByteRvs((unsigned char)wc);
			else
				b.PutByteRvs(0);
			wc >>= 8;
			++len;
		}

		// Check that the upper bytes, if any, are zero
		if ((wcharSize > 4) && (wc != 0))
			throw EXCEPT("Invalid UniversalString", RESTRICTED_TYPE_ERROR);
	}
	return len;
}

void UniversalString::BDecContent(const AsnBuf& b, AsnTag tagId, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("UniversalString::BDecContent");

	// Erase the existing characters
	erase();

	if (elmtLen == INDEFINITE_LEN || elmtLen > b.length())
		throw MemoryException(elmtLen, "elmtLen requests for too much data", STACK_ENTRY);

	// If tag is constructed, decode and combine the segments
	std::string encStr;
	if (TAG_IS_CONS(tagId))
	{
		bytesDecoded += CombineConsString(b, elmtLen, encStr);
	}
	else // tag is primitive, just combine the one segment
	{
		char* seg = new char[elmtLen];
		if (seg == NULL)
			throw MemoryException(elmtLen, "seg", STACK_ENTRY);
		b.GetSeg(seg, elmtLen);
		bytesDecoded += elmtLen;

		encStr.append(seg, elmtLen);
		delete[] seg;
	}

	// encoding length must be a multiple of four since UniversalString
	// uses 4 bytes to represent a character.
	//
	if (encStr.length() % 4 != 0)
		throw EXCEPT("Invalid UniversalString length not multiple of 4", RESTRICTED_TYPE_ERROR);

	const size_t wcharSize = sizeof(wchar_t);

	// decode UniversalString into wide string
	//
	resize(encStr.length() / 4);
	std::string::iterator iEnc = encStr.begin();
	for (size_type i = 0; i < size(); ++i)
	{
		wchar_t wtmpCh = 0;
		for (unsigned int iByte = 4; iByte > 0; --iByte)
		{
			// Check that the wchar_t won't overflow
			if ((wcharSize - iByte) < 0)
			{
				if (*iEnc++ != 0)
					throw EXCEPT("UniversalString not supported by platform wchar_t size", RESTRICTED_TYPE_ERROR);
				// else just skip the zero byte
			}
			else
			{
				wtmpCh <<= 8;
				wtmpCh |= *iEnc++;
			}
		}
		at(i) = wtmpCh;
	}
}

UTF8String::UTF8String()
{
}

UTF8String::UTF8String(const wchar_t* szUTF16)
	: WideAsnString(szUTF16)
{
}

UTF8String::UTF8String(const std::wstring& strUTF16)
	: WideAsnString(strUTF16)
{
}

UTF8String UTF8String::CreateFromUTF8(const char* szUTF8)
{
	return UTF8String(AsnStringConvert::UTF8ToUTF16(szUTF8));
}

UTF8String UTF8String::CreateFromUTF8(const std::string& strUTF8)
{
	return UTF8String(AsnStringConvert::UTF8ToUTF16(strUTF8.c_str()));
}

UTF8String UTF8String::CreateFromASCII(const char* szAscii)
{
	return UTF8String(AsnStringConvert::AsciiToUTF16(szAscii));
}

UTF8String UTF8String::CreateFromASCII(const std::string& strAscii)
{
	return UTF8String(AsnStringConvert::AsciiToUTF16(strAscii.c_str()));
}

UTF8String* UTF8String::CreateNewFromUTF8(const char* szUTF8)
{
	return new UTF8String(AsnStringConvert::UTF8ToUTF16(szUTF8));
}

UTF8String* UTF8String::CreateNewFromUTF8(const std::string& strUTF8)
{
	return new UTF8String(AsnStringConvert::UTF8ToUTF16(strUTF8.c_str()));
}

UTF8String* UTF8String::CreateNewFromASCII(const char* szAscii)
{
	return new UTF8String(AsnStringConvert::AsciiToUTF16(szAscii));
}

UTF8String* UTF8String::CreateNewFromASCII(const std::string& strAscii)
{
	return new UTF8String(AsnStringConvert::AsciiToUTF16(strAscii.c_str()));
}

void UTF8String::JEnc(SJson::Value& b) const
{
	b = SJson::Value(getUTF8().c_str());
}

bool UTF8String::JDec(const SJson::Value& b)
{
	clear();
	if (b.isString())
	{
		setUTF8(b.asCString());
		return true;
	}
	return false;
}

AsnLen UTF8String::BEncContent(AsnBuf& b) const
{
	std::string utf8;
	getUTF8(utf8);

	AsnLen len = (int)utf8.length();
	b.PutSegRvs(utf8.data(), len);
	return len;
}

void UTF8String::BDecContent(const AsnBuf& b, AsnTag tagId, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("UTF8String::BDecContent()");

	// Erase the existing characters
	erase();

	try
	{
		if (elmtLen == INDEFINITE_LEN || elmtLen > b.length())
			throw MemoryException(elmtLen, "elmtLen requests for too much data", STACK_ENTRY);

		// If tag is constructed, decode and combine the segments
		std::string encStr;
		if (TAG_IS_CONS(tagId))
		{
			bytesDecoded += CombineConsString(b, elmtLen, encStr);
		}
		else // tag is primitive, just combine the one segment
		{
			char* seg = new char[elmtLen];
			if (seg == NULL)
				throw MemoryException(elmtLen, "seg", STACK_ENTRY);
			b.GetSeg(seg, elmtLen);
			bytesDecoded += elmtLen;

			encStr.append(seg, elmtLen);
			delete[] seg;
		}

		// Decode this UTF-8 string and assign it to this object
		setUTF8(encStr.c_str());
	}
	catch (SnaccException& snaccE)
	{
		snaccE.push(STACK_ENTRY);
		throw;
	}
}
