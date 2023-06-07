/*#include <string>*/
#include "../include/asn-incl.h"

_BEGIN_SNACC_NAMESPACE

// Base64 Functions
// Decoder
typedef enum
{
	step_a,
	step_b,
	step_c,
	step_d
} base64_decodestep;

typedef struct
{
	base64_decodestep step;
	char plainchar;
} base64_decodestate;

void base64_init_decodestate(base64_decodestate* state_in);

int base64_decode_value(char value_in);

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in);

int base64_decode_value(char value_in)
{
	static const char decoding[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
	static const char decoding_size = sizeof(decoding);
	value_in -= 43;
	if (value_in < 0 || value_in >= decoding_size)
		return -1;
	return decoding[(int)value_in];
}

void base64_init_decodestate(base64_decodestate* state_in)
{
	state_in->step = step_a;
	state_in->plainchar = 0;
}

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in)
{
	const char* codechar = code_in;
	char* plainchar = plaintext_out;
	char fragment;

	*plainchar = state_in->plainchar;

	switch (state_in->step)
	{
		while (1)
		{
			[[fallthrough]];
			case step_a:
				do
				{
					if (codechar == code_in + length_in)
					{
						state_in->step = step_a;
						state_in->plainchar = *plainchar;
						return (int)(plainchar - plaintext_out);
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while (fragment < 0);
				*plainchar = (fragment & 0x03f) << 2;
				[[fallthrough]];
			case step_b:
				do
				{
					if (codechar == code_in + length_in)
					{
						state_in->step = step_b;
						state_in->plainchar = *plainchar;
						return (int)(plainchar - plaintext_out);
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while (fragment < 0);
				*plainchar++ |= (fragment & 0x030) >> 4;
				*plainchar = (fragment & 0x00f) << 4;
				[[fallthrough]];
			case step_c:
				do
				{
					if (codechar == code_in + length_in)
					{
						state_in->step = step_c;
						state_in->plainchar = *plainchar;
						return (int)(plainchar - plaintext_out);
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while (fragment < 0);
				*plainchar++ |= (fragment & 0x03c) >> 2;
				*plainchar = (fragment & 0x003) << 6;
				[[fallthrough]];
			case step_d:
				do
				{
					if (codechar == code_in + length_in)
					{
						state_in->step = step_d;
						state_in->plainchar = *plainchar;
						return (int)(plainchar - plaintext_out);
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while (fragment < 0);
				*plainchar++ |= (fragment & 0x03f);
		}
	}
	/* control should not reach here */
	return (int)(plainchar - plaintext_out);
}

// Encoder
typedef enum
{
	step_A,
	step_B,
	step_C
} base64_encodestep;

typedef struct
{
	base64_encodestep step;
	char result;
	int stepcount;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate* state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in);

int base64_encode_blockend(char* code_out, base64_encodestate* state_in);

const int CHARS_PER_LINE = 72;

void base64_init_encodestate(base64_encodestate* state_in)
{
	state_in->step = step_A;
	state_in->result = 0;
	state_in->stepcount = 0;
}

char base64_encode_value(char value_in)
{
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63)
		return '=';
	return encoding[(int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
	const char* plainchar = plaintext_in;
	const char* const plaintextend = plaintext_in + length_in;
	char* codechar = code_out;
	char result;
	char fragment;

	result = state_in->result;

	switch (state_in->step)
	{
		while (1)
		{
			[[fallthrough]];
			case step_A:
				if (plainchar == plaintextend)
				{
					state_in->result = result;
					state_in->step = step_A;
					return (int)(codechar - code_out);
				}
				fragment = *plainchar++;
				result = (fragment & 0x0fc) >> 2;
				*codechar++ = base64_encode_value(result);
				result = (fragment & 0x003) << 4;
				[[fallthrough]];
			case step_B:
				if (plainchar == plaintextend)
				{
					state_in->result = result;
					state_in->step = step_B;
					return (int)(codechar - code_out);
				}
				fragment = *plainchar++;
				result |= (fragment & 0x0f0) >> 4;
				*codechar++ = base64_encode_value(result);
				result = (fragment & 0x00f) << 2;
				[[fallthrough]];
			case step_C:
				if (plainchar == plaintextend)
				{
					state_in->result = result;
					state_in->step = step_C;
					return (int)(codechar - code_out);
				}
				fragment = *plainchar++;
				result |= (fragment & 0x0c0) >> 6;
				*codechar++ = base64_encode_value(result);
				result = (fragment & 0x03f) >> 0;
				*codechar++ = base64_encode_value(result);

				++(state_in->stepcount);
				if (state_in->stepcount == CHARS_PER_LINE / 4)
				{
					//*codechar++ = '\n';
					// state_in->stepcount = 0;
				}
		}
	}
	/* control should not reach here */
	return (int)(codechar - code_out);
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
	char* codechar = code_out;

	switch (state_in->step)
	{
		case step_B:
			*codechar++ = base64_encode_value(state_in->result);
			*codechar++ = '=';
			*codechar++ = '=';
			break;
		case step_C:
			*codechar++ = base64_encode_value(state_in->result);
			*codechar++ = '=';
			break;
		case step_A:
			break;
	}
	//*codechar++ = '\n';

	return (int)(codechar - code_out);
}

// Copy Constructor
//
// copy m_str and pFileSeg
AsnOcts::AsnOcts(const AsnOcts& o)
{
	m_str = o.m_str;
	if (o.m_pFileSeg != NULL)
		m_pFileSeg = new AsnFileSeg(*o.m_pFileSeg);
	else
		m_pFileSeg = NULL;
}

AsnOcts::~AsnOcts()
{
	if (m_pFileSeg != NULL)
		delete m_pFileSeg;
}

// Returns the length of the AsnOcts.
size_t AsnOcts::Len() const
{
	size_t result = 0;

	result = m_str.length();

	if (m_pFileSeg != NULL)
		result = m_pFileSeg->size();

	return result;
}

// Returns the data
const std::string& AsnOcts::data() const
{
	char ch;

	// IF the octet string is not in an AsnFileSeg just return the string
	//
	if (m_pFileSeg == NULL)
	{
		return m_str;
	}
	// ELSE if the octet string is in an AsnFileSeg read it into the
	// string then return it. Destroy the AsnFileSeg in the process.
	//
	else
	{
		m_pFileSeg->pubseekoff(0, std::ios_base::beg, std::ios_base::in);
		while ((ch = (char)m_pFileSeg->snextc()) != EOF)
		{
			m_str += ch;
			ch = (char)m_pFileSeg->snextc();
		}
		delete m_pFileSeg;
		m_pFileSeg = NULL;
		return m_str;
	}
}

// Returns the data
const char* AsnOcts::c_str() const
{
	return data().c_str();
}

const unsigned char* AsnOcts::c_ustr() const
{
	return (unsigned char*)data().data();
}

// Initialize the AsnOcts with a char * and length.
// copies the string str.
void AsnOcts::Set(const char* str, size_t len)
{
	m_str.assign(str, len);
}

// Prints the AsnOcts to the given ostream in Value Notation.
void AsnOcts::PrintXML(std::ostream& os, const char* lpszTitle, const char* lpszType) const
{
	if (lpszType)
		os << "<" << lpszType << ">";
	else
		os << "<OCTET_STRING>";
	if (lpszTitle)
		os << lpszTitle;
	os << "-";
	Print(os);
	// PrintXMLSupport(&os, ((AsnOcts *)this)->Access(), octetLen);
	if (lpszType)
		os << "</" << lpszType << ">\n";
	else
		os << "</OCTET_STRING>\n";
}

// Prints the AsnOcts to the given ostream in Value Notation.
void AsnOcts::Print(std::ostream& os, unsigned short /*indent*/) const
{
	int i;
	os << "'";
	for (i = 0; i < (int)Len(); i++)
		os << TO_HEX(c_ustr()[i] >> 4) << (TO_HEX(c_ustr()[i]));

	os << "'H  -- \"";

	/* put printable parts in ASN.1 comment */
	for (i = 0; i < (int)Len(); i++)
		if (isspace((unsigned char)c_ustr()[i]) || !isprint(c_ustr()[i]))
			os << "."; /* newlines->space (so don't screw up ASN.1 comment) */
		else
			os << c_ustr()[i];
	os << "\" --";
} /* AsnOcts::Print */

AsnLen AsnOcts::BEncContent(AsnBuf& b) const
{
	if (m_pFileSeg != NULL)
		b.PutFileSeg(m_pFileSeg);
	else
		b.PutSegRvs(m_str.data(), m_str.length());

	return (int)Len();
}

// Decodes a BER OCTET STRING value and puts it in this object.
// Constructed OCTET STRINGs are always concatenated into primitive ones.
void AsnOcts::BDecContent(const AsnBuf& b, AsnTag tagId, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("AsnOcts::BDecContent()");

	if (elmtLen != 0)
	{
		/*
		 * tagId is encoded tag shifted into long int.
		 * if CONS bit is set then constructed octet string
		 */
		if (tagId & 0x20000000)
		{
			BDecConsOcts(b, elmtLen, bytesDecoded);
		}
		else /* primitive octet string */
		{
			if (elmtLen == INDEFINITE_LEN)
				throw BoundsException("indefinite length on primitive", STACK_ENTRY);

			/** PIERCE: commented out until further testing is done
		 else if (elmtLen > MAX_OCTS)  // use a filebuf instead of memory
		  {
			 m_pFileSeg = b.GetFileSeg(elmtLen);
		  }
		  else                          // use memory
		  {
			b.GetSeg(m_str, elmtLen);
		  }
			**/

			b.GetSeg(m_str, elmtLen);

			bytesDecoded += elmtLen;
		}
	}

} /* AsnOcts::BDecContent */

void AsnOcts::JEnc(SJson::Value& b) const
{
	if (m_str.size() == 0)
	{
		b = SJson::Value("");
	}
	else
	{
		/* set up a destination buffer large enough to hold the encoded data */
		const size_t sizeDataIn = m_str.size();
		const size_t sizeBufferOut =
			// encoding every first and second byte need one byte each in the output buffer, every 3rd byte needs two bytes in the output buffer.
			((sizeDataIn / 3) * 4) + sizeDataIn % 3 + 3 + // base64_encode_blockend addes up to 3 bytes.
			1;											  // null byte string termination for use with printf.

		char* output = (char*)malloc(sizeBufferOut);
		if (output)
		{
			/* keep track of our encoded position */
			char* c = output;
			/* we need an encoder state */
			base64_encodestate s;

			/*---------- START ENCODING ----------*/
			/* initialise the encoder state */
			base64_init_encodestate(&s);
			/* gather data from the input and send it to the output */
			/* store the number of bytes encoded by a single call */
			unsigned int cnt = base64_encode_block(m_str.data(), (int)m_str.size(), c, &s);
			c += cnt;
			/* since we have encoded the entire input string, we know that
			   there is no more input data; finalise the encoding */
			/* store the number of bytes encoded by a single call */
			unsigned int cntend = base64_encode_blockend(c, &s);
			c += cntend;
			/*---------- STOP ENCODING  ----------*/

			if (sizeBufferOut < cnt + cntend + 1 /* including null byte string termination */)
				throw EXCEPT("AsnOcts size not withing restricted bounds", RESTRICTED_TYPE_ERROR);

			/* we want to print the encoded data, so null-terminate it: */
			*c = 0;
			b = SJson::Value(output);
			free(output);
		}
		else
		{
			b = SJson::Value("");
		}
	}
}

bool AsnOcts::JDec(const SJson::Value& b)
{
	clear();
	if (b.isConvertibleTo(SJson::stringValue))
	{
		std::string str = b.asString();

		/* set up a destination buffer large enough to hold the decoded data */
		char* output = (char*)malloc(str.length() + 1);
		if (output)
		{
			/* keep track of our decoded position */
			char* c = output;
			/* store the number of bytes decoded by a single call */
			int cnt = 0;
			/* we need a decoder state */
			base64_decodestate s;

			/*---------- START DECODING ----------*/
			/* initialise the decoder state */
			base64_init_decodestate(&s);
			/* decode the input data */
			cnt = base64_decode_block(str.c_str(), (int)str.length(), c, &s);
			c += cnt;
			/* note: there is no base64_decode_blockend! */
			/*---------- STOP DECODING  ----------*/

			m_str = std::string(output, cnt);
			free(output);
			return true;
		}
	}
	return false;
}

AsnLen AsnOcts::EncodeWithSizeConstraint(AsnBufBits& b) const
{
	FUNC("AsnOcts::EncodeWithSizeConstraint");

	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	AsnLen len = 0;
	int iSCLowerBound = sizeConstraints[0].lowerBound;
	int iSCUpperBound = iSCLowerBound;
	int minBitsNeeded = 0;
	int minBytesNeeded = 0;
	long Range = FindSizeConstraintBounds(iSCLowerBound, iSCUpperBound);
	long tempRange = Range - 1;
	long size = length();
	unsigned char* pStr = new unsigned char[1];

	while (tempRange > 0)
	{
		tempRange -= (long)(1 << minBitsNeeded);
		minBitsNeeded += 1;
	}

	if (size < iSCLowerBound || size > iSCUpperBound)
		throw EXCEPT("AsnOcts size not withing restricted bounds", RESTRICTED_TYPE_ERROR);

	if (Range > 1)
	{
		if ((iSCUpperBound <= 2) && b.IsAligned())
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

	if (iSCUpperBound > 0)
	{
		if ((iSCUpperBound <= 2) && b.IsAligned())
			len += b.OctetAlignWrite();

		len += b.PutBits((unsigned char*)c_ustr(), (length() * 8));
	}

	delete[] pStr;

	return len;
}

void AsnOcts::DecodeWithSizeConstraint(AsnBufBits& b, AsnLen& bitsDecoded)
{
	FUNC("AsnString::DecodeWithSizeConstraint");

	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	int iSCLowerBound = sizeConstraints[0].lowerBound;
	int iSCUpperBound = iSCLowerBound;
	int minBitsNeeded = 0;
	int minBytesNeeded = 0;
	long Range = FindSizeConstraintBounds(iSCLowerBound, iSCUpperBound);
	long tempRange = Range - 1;
	long decodeSize = 0;
	unsigned char* seg;
	unsigned char* pStr = new unsigned char[1];

	clear();

	while (tempRange > 0)
	{
		tempRange -= (long)(1 << minBitsNeeded);
		minBitsNeeded += 1;
	}

	if (Range > 1)
	{
		if ((iSCUpperBound <= 2) && b.IsAligned())
			bitsDecoded += b.OctetAlignRead();

		minBytesNeeded = minBitsNeeded / 8;
		minBitsNeeded = minBitsNeeded % 8;

		if (minBytesNeeded > 0)
		{
			delete[] pStr;
			pStr = b.GetBits(8);
			bitsDecoded += 8;
			decodeSize <<= 8;
			decodeSize |= (long)pStr[0];
		}

		delete[] pStr;
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

	if (iSCUpperBound > 0)
	{
		if ((iSCUpperBound <= 2) && b.IsAligned())
			bitsDecoded += b.OctetAlignRead();

		seg = b.GetBits(decodeSize * 8);
		m_str.append((const char*)seg, decodeSize);
		bitsDecoded += (decodeSize * 8);
		free(seg);
	}

	delete[] pStr;
}

long AsnOcts::FindSizeConstraintBounds(int& iSCLowerBound, int& iSCUpperBound) const
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

AsnLen AsnOcts::EncodeGeneral(AsnBufBits& b) const
{
	AsnLen len = 0;
	unsigned long l_64kFrag = l_16k * 4;
	unsigned long count = 0;
	unsigned long x = 0;
	unsigned long tempLen = length();
	unsigned char ch = 0x00;
	unsigned char* c = NULL;
	long offset = 0;

	if (tempLen >= l_16k)
	{
		/*there is more than 16k bytes of data*/
		count = (tempLen / l_64kFrag);

		for (x = 0; x < count; x++)
		{
			len += b.OctetAlignWrite();

			len += PEncLen_16kFragment(b, 4);

			len += b.OctetAlignWrite();

			len += b.PutBits((unsigned char*)&m_str[offset], (l_64kFrag * 8));

			offset += l_64kFrag;
		}

		tempLen -= count * l_64kFrag;

		count = tempLen / l_16k;

		if (count != 0)
		{
			len += b.OctetAlignWrite();

			len += PEncLen_16kFragment(b, count);

			len += b.OctetAlignWrite();

			len += b.PutBits((unsigned char*)&m_str[offset], (count * l_16k * 8));

			offset += (count * l_16k);
		}

		tempLen -= (l_16k * count);

		if (tempLen == 0)
		{
			ch = 0x00;
			c = &ch;

			len += b.OctetAlignWrite();

			len += b.PutBits(c, 8);

			return len;
		}
	}

	/*if there are less than 128 bytes of data*/
	if (tempLen < 128)
	{
		len += b.OctetAlignWrite();

		len += PEncDefLenTo127(b, tempLen);

		len += b.OctetAlignWrite();

		len += b.PutBits((unsigned char*)&m_str[offset], (tempLen * 8));

		offset += tempLen;
	}
	else if (tempLen >= 128 && tempLen < l_16k)
	{
		len += b.OctetAlignWrite();
		/*if there is less than 16k bytes of data*/
		/*and more than 127 bytes of data*/
		len += PEncLen_1to16k(b, tempLen);

		len += b.OctetAlignWrite();

		len += b.PutBits((unsigned char*)&m_str[offset], (tempLen * 8));

		offset += tempLen;
	}

	return len;
}

void AsnOcts::DecodeGeneral(AsnBufBits& b, AsnLen& bitsDecoded)
{
	unsigned char* seg;
	unsigned long templen = 0;

	clear();

	bitsDecoded += b.OctetAlignRead();

	seg = (unsigned char*)b.GetBits(8);
	bitsDecoded += 8;

	while ((seg[0] & 0xC0) == 0xC0)
	{
		seg[0] &= 0x3F;
		templen = (unsigned long)seg[0];
		templen *= l_16k;

		b.OctetAlignRead();

		m_str.append((const char*)b.GetBits(templen * 8), templen);
		bitsDecoded += (templen * 8);

		bitsDecoded += b.OctetAlignRead();

		free(seg);
		seg = (unsigned char*)b.GetBits(8);
		bitsDecoded += 8;
	}

	if ((seg[0] & 0xC0) == 0x80)
	{
		seg[0] &= 0x3F;
		templen = (unsigned long)seg[0];
		templen <<= 8;
		free(seg);
		seg = (unsigned char*)b.GetBits(8);
		bitsDecoded += 8;
		templen |= (unsigned long)seg[0];

		bitsDecoded += b.OctetAlignRead();

		free(seg);
		seg = b.GetBits(templen * 8);
		m_str.append((const char*)seg, templen);
		free(seg);
		bitsDecoded += (templen * 8);
	}
	else if ((seg[0] & 0x80) == 0x00)
	{
		seg[0] &= 0x7F;
		templen = (unsigned long)seg[0];

		bitsDecoded += b.OctetAlignRead();

		free(seg);
		seg = b.GetBits(templen * 8);
		m_str.append((const char*)seg, templen);
		free(seg);
		bitsDecoded += (templen * 8);
	}
}

AsnLen AsnOcts::PEnc(AsnBufBits& b) const
{
	/*if there are no constraints, a default lower bound of zero is set */
	AsnLen len = 0;

	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);
	if (sizeConstraints == NULL && numSizeConstraints == 0)
		len = EncodeGeneral(b);
	else
		len = EncodeWithSizeConstraint(b);

	return len;
}

void AsnOcts::PDec(AsnBufBits& b, AsnLen& bitsDecoded)
{
	int numSizeConstraints;
	const SizeConstraint* sizeConstraints = SizeConstraints(numSizeConstraints);

	if (sizeConstraints == NULL && numSizeConstraints == 0)
		DecodeGeneral(b, bitsDecoded);
	else
		DecodeWithSizeConstraint(b, bitsDecoded);
}

AsnLen AsnOcts::BEnc(AsnBuf& b) const
{
	AsnLen l;
	l = BEncContent(b);
	l += BEncDefLen(b, l);
	l += BEncTag1(b, UNIV, PRIM, OCTETSTRING_TAG_CODE);
	return l;
}

void AsnOcts::BDec(const AsnBuf& b, AsnLen& bytesDecoded)
{
	FUNC("AsnOcts::BDec()");

	AsnLen elmtLen;
	AsnTag tag;

	tag = BDecTag(b, bytesDecoded);
	if ((tag != MAKE_TAG_ID(UNIV, PRIM, OCTETSTRING_TAG_CODE)) && (tag != MAKE_TAG_ID(UNIV, CONS, OCTETSTRING_TAG_CODE)))
		throw InvalidTagException(typeName(), tag, STACK_ENTRY);
	elmtLen = BDecLen(b, bytesDecoded);
	BDecContent(b, tag, elmtLen, bytesDecoded);
}

/*
 * decodes a seq of universally tagged octets until either EOC is
 * encountered or the given len decoded.  Return them in a
 * single concatenated octet string
 */
void AsnOcts::BDecConsOcts(const AsnBuf& b, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	ConsStringDeck strDeck;

	strDeck.Fill(b, elmtLen, bytesDecoded);
	strDeck.Collapse(m_str);

} /* BDecConsOcts */

class RefNode
{
public:
	~RefNode()
	{ /***RWC;TBD; ***/
	}
	AsnLen length;
	AsnLen count;

	RefNode()
	{
		length = (unsigned long)-1;
		count = (unsigned long)-1;
	}
	RefNode(AsnLen l, AsnLen c)
	{
		length = l;
		count = c;
	}
};

void ConsStringDeck::Fill(const AsnBuf& b, AsnLen elmtLen, AsnLen& bytesDecoded)
{
	FUNC("ConsStringDeck::Fill()");

	AsnLen totalElmtsLen1 = 0;
	std::list<RefNode> refList;
	refList.insert(refList.begin(), RefNode(elmtLen, totalElmtsLen1));
	std::list<RefNode>::iterator curr = refList.begin();

	bool done = false;
	unsigned char* strPtr;
	unsigned long tagId1;

	while (!done)
	{
		for (; (curr->count < curr->length) || (curr->length == INDEFINITE_LEN);)
		{
			tagId1 = BDecTag(b, curr->count);

			if (tagId1 == EOC_TAG_ID && curr->length == INDEFINITE_LEN)
			{
				// We may have found a EOC TAG
				// if next byte is a 0 then is an EOC tag
				if (b.GetByte() == 0)
				{
					++curr->count;
					break;
				}
				else
				{
					throw EXCEPT("Partial EOC tag found", DECODE_ERROR);
				}
			}
			else if (tagId1 == MAKE_TAG_ID(UNIV, PRIM, m_baseTag))
			{
				/*
				 * primitive part of string, put references to piece (s) in
				 * str stack
				 */
				totalElmtsLen1 = BDecLen(b, curr->count);

				if (totalElmtsLen1 == INDEFINITE_LEN)
					throw InvalidTagException("Primitive String can not have INDEFINITE_LEN", tagId1, STACK_ENTRY);
				strPtr = (unsigned char*)b.GetSeg(totalElmtsLen1);
				push_back(StringPair(strPtr, totalElmtsLen1));
				curr->count += totalElmtsLen1;
			}
			else if (tagId1 == MAKE_TAG_ID(UNIV, CONS, m_baseTag))
			{
				/*
				 * primitive part of string, put references to piece (s) in
				 * str stack
				 */

				totalElmtsLen1 = BDecLen(b, curr->count);

				if ((totalElmtsLen1 != INDEFINITE_LEN) && (totalElmtsLen1 + curr->count) > curr->length /*elmtLen*/)
					throw BoundsException("Invalid constructed object", STACK_ENTRY);

				curr = refList.insert(refList.end(), RefNode(totalElmtsLen1, 0));
				// curr = curr->next;

				// Fill(b, curr->length, curr->count);
			}
			else if (m_baseTag == 0 && TAG_IS_CONS(tagId1))
			{
				/* Handle set and sequence
				 */
				totalElmtsLen1 = BDecLen(b, curr->count);

				if ((totalElmtsLen1 != INDEFINITE_LEN) && (totalElmtsLen1 + curr->count) > curr->length /*elmtLen*/)
					throw BoundsException("Invalid constructed object", STACK_ENTRY);

				curr = refList.insert(refList.end(), RefNode(totalElmtsLen1, 0));
				// Fill(b, curr->length, curr->count);
			}
			else if (m_baseTag == 0)
			{
				totalElmtsLen1 = BDecLen(b, curr->count);

				if (totalElmtsLen1 == INDEFINITE_LEN)
					throw InvalidTagException("Primitive String can not have INDEFINITE_LEN", tagId1, STACK_ENTRY);

				if (totalElmtsLen1 > b.length())
					throw InvalidTagException("Primitive String, length", tagId1, STACK_ENTRY);

				strPtr = (unsigned char*)b.GetSeg(totalElmtsLen1);
				push_back(StringPair(strPtr, totalElmtsLen1));
				curr->count += totalElmtsLen1;
			}
			else
			{
				throw InvalidTagException("Constructed String", tagId1, STACK_ENTRY);
			}
		} /* end of for */

		if (curr != refList.begin())
		{
			int iTmpCount = curr->count;
			refList.erase(curr);
			curr = refList.end();
			--curr;
			curr->count += iTmpCount;
		}
		else
		{
			done = true;
		}
	}

	bytesDecoded += refList.begin()->count;
}

void ConsStringDeck::Collapse(std::string& str)
{
	iterator i;
	i = begin();
	for (; i != end(); i++)
		str.append((char*)i->first, i->second);
}

ConsStringDeck::~ConsStringDeck()
{
	iterator i;
	i = begin();
	for (; i != end(); i++)
	{
		delete[] i->first;
		i->first = NULL;
	}
}

bool AsnOcts::operator==(const AsnOcts& o) const
{
	if ((o.Len() == Len()) && (memcmp(o.c_ustr(), c_ustr(), Len()) == 0))
		return true;
	return false;
}

bool AsnOcts::operator!=(const AsnOcts& o) const
{
	return !operator==(o);
}

_END_SNACC_NAMESPACE
