//
// File:		asn-listset.h
//
// Contents:	AsnList, AsnSeqOf, AsnSetOf template classes
//
//

#ifndef SNACC_ASN_LISTSET_H
#define SNACC_ASN_LISTSET_H

#include <list>
#include "asn-incl.h"

/////////////////////////////////
// Abstract AsnList template  //
///////////////////////////////
template <class T> class AsnList : public std::list<T>, public SNACC::AsnType, public SNACC::PERGeneral
{
public:
	// Appends newly inserted element to list and returns its iterator
	typename std::list<T>::iterator append(const T& x = T())
	{
		return std::list<T>::insert(std::list<T>::end(), x);
	}

	// encode and decode routines
	virtual SNACC::AsnLen PEnc(SNACC::AsnBufBits& b) const override;
	// virtual SNACC::AsnLen BEnc(SNACC::AsnBuf& b) const = 0;
	virtual SNACC::AsnLen BEncContent(SNACC::AsnBuf& b) const;
	virtual void PDec(SNACC::AsnBufBits& b, SNACC::AsnLen& bytesDecoded) override;
	// virtual void BDec(const SNACC::AsnBuf& b, SNACC::AsnLen& bytesDecoded) = 0;
	virtual void BDecContent(const SNACC::AsnBuf& b, SNACC::AsnTag tagId, SNACC::AsnLen elmtLen, SNACC::AsnLen& bytesDecoded);

	virtual void JEnc(SJson::Value& b) const override;
	virtual bool JDec(const SJson::Value& b) override;

	// const char* typeName() const = 0;
	virtual int checkConstraints(SNACC::ConstraintFailList* pConstraintFails) const override;
	
	// A list may be empty
	virtual bool mayBeEmpty() const
	{
		return true;
	}

	void PrintXML(std::ostream& os, const char* lpszTitle = NULL) const;

	void Print(std::ostream& os, unsigned short indent) const override;

	//	int checkListConstraints(SNACC::ConstraintFailList* pConstraintFails) const; /*JKG--8/12/03--*/
	char* checkSOfVRange(long m_Lower, long m_Upper) const;
	char* checkSOfSingleVal(long m_SingleVal) const;

	virtual SNACC::SizeConstraint* SizeConstraints() const
	{
		return NULL;
	}
	//	virtual SNACC::SizeConstraint* SizeConstraints()		{ return NULL; }

	virtual void Allocate(long size) override
	{
	}
	virtual void Clear() override
	{
		std::list<T>::clear();
	}
	virtual SNACC::AsnLen Interpret(SNACC::AsnBufBits& b, long offset) const override;
	virtual void Deterpret(SNACC::AsnBufBits& b, SNACC::AsnLen& bitsDecoded, long offset) override;

	//	virtual long lEncLen() const							{ return size(); }
};

//////////////////////////////////
// Abstract AsnList template   //
//          CODE              //
///////////////////////////////
template <class T> void AsnList<T>::Deterpret(SNACC::AsnBufBits& b, SNACC::AsnLen& bitsDecoded, long offset)
{
	append()->PDec(b, bitsDecoded);
}

template <class T> SNACC::AsnLen AsnList<T>::Interpret(SNACC::AsnBufBits& b, long offset) const
{
	FUNC("AsnList<T>::Interpret()");

	auto i = AsnList<T>::cbegin();
	while ((i != AsnList<T>::end()) && (offset > 0))
	{
		++i;
		--offset;
	}

	if (i != AsnList<T>::end())
		return i->PEnc(b);
	else
		throw SNACC_EXCEPT("Null element found in List");
}

template <class T> SNACC::AsnLen AsnList<T>::PEnc(SNACC::AsnBufBits& b) const
{
	FUNC("AsnList<T>::PEnc()");

	if (!SizeConstraints())
		return EncodeGeneral(b);
	else
	{
		SNACC::AsnLen sum = 0;
		if (SizeConstraints()->upperBoundExists == 0)
		{
			if (AsnList<T>::size() != (unsigned int)SizeConstraints()->lowerBound)
				throw EXCEPT("Number of elements in AsnList does not match singlevalue constraint", ENCODE_ERROR);

			for (auto i = AsnList<T>::cbegin(); i != AsnList<T>::cend(); ++i)
				sum += i->PEnc(b);
		}
		else if (SizeConstraints()->upperBoundExists == 1)
		{
			if ((AsnList<T>::size() < SizeConstraints()->lowerBound) || (AsnList<T>::size() > SizeConstraints()->upperBound))
				throw EXCEPT("Number of elements in AsnList is not within the size constraint", ENCODE_ERROR);

			unsigned char pStr[] = {0x00, 0x00, 0x00, 0x00};
			long tempRange = SizeConstraints()->upperBound - SizeConstraints()->lowerBound;
			int minBitsNeeded = 0;
			while (tempRange > 0)
			{
				tempRange -= (long)(1 << minBitsNeeded);
				minBitsNeeded += 1;
			}

			sum += b.OctetAlignWrite();

			long minBytesNeeded = minBitsNeeded / 8;
			minBitsNeeded %= 8;
			long numExtra = (long)AsnList<T>::size() - SizeConstraints()->lowerBound;
			if (minBytesNeeded > 0)
			{
				pStr[0] = (unsigned char)(numExtra >> minBitsNeeded);
				sum += b.PutBits(pStr, 8);
			}

			pStr[0] = (unsigned char)numExtra;
			pStr[0] <<= 8 - minBitsNeeded;
			sum += b.PutBits(pStr, minBitsNeeded);

			sum += b.OctetAlignWrite();
		}
		else
		{
			if (AsnList<T>::size() < SizeConstraints()->lowerBound)
				throw EXCEPT("Number of elements in AsnList is below the minimum size constraint", ENCODE_ERROR);

			SNACC::AsnInt listLen((long)AsnList<T>::size() - SizeConstraints()->lowerBound);
			sum += b.OctetAlignWrite();
			listLen.PEnc(b);
			sum += b.OctetAlignWrite();
		}

		// Encode the elements
		for (auto i = AsnList<T>::cbegin(); i != AsnList<T>::cend(); ++i)
			sum += i->PEnc(b);

		return sum;
	}
}

//
template <class T> void AsnList<T>::PDec(SNACC::AsnBufBits& b, SNACC::AsnLen& bitsDecoded)
{
	// Remove existing elements
	AsnList<T>::clear();

	if (!SizeConstraints())
	{
		DecodeGeneral(b, bitsDecoded);
	}
	else if (SizeConstraints()->upperBoundExists == 0)
	{
		for (unsigned int i = 0; i < SizeConstraints()->lowerBound; i++)
			append()->PDec(b, bitsDecoded);
	}
	else if (SizeConstraints()->upperBoundExists == 1)
	{
		long tempRange = SizeConstraints()->upperBound - SizeConstraints()->lowerBound;
		int minBitsNeeded = 0;
		int minBytesNeeded = 0;
		long decodeSize = 0;
		unsigned char* pStr;

		while (tempRange > 0)
		{
			tempRange -= (long)(1 << minBitsNeeded);
			minBitsNeeded += 1;
		}

		minBytesNeeded = minBitsNeeded / 8;
		minBitsNeeded = minBitsNeeded % 8;

		bitsDecoded += b.OctetAlignRead();
		if (minBytesNeeded > 0)
		{
			pStr = b.GetBits(8);
			bitsDecoded += 8;
			decodeSize <<= 8;
			decodeSize |= (long)pStr[0];
		}

		pStr = b.GetBits(minBitsNeeded);
		bitsDecoded += minBitsNeeded;

		if (minBitsNeeded > 0)
		{
			decodeSize <<= minBitsNeeded;
			pStr[0] >>= (8 - minBitsNeeded);
			decodeSize |= (long)pStr[0];
		}

		decodeSize += SizeConstraints()->lowerBound;

		bitsDecoded += b.OctetAlignRead();
		for (int i = 0; i < decodeSize; i++)
			append()->PDec(b, bitsDecoded);

		free(pStr);
	}
	else
	{
		bitsDecoded += b.OctetAlignRead();
		SNACC::AsnInt listLen;
		listLen.PDec(b, bitsDecoded);
		bitsDecoded += b.OctetAlignRead();

		int tempLen = listLen + SizeConstraints()->lowerBound;
		for (int i = 0; i < tempLen; i++)
			append()->PDec(b, bitsDecoded);
	}
}

template <class T> char* AsnList<T>::checkSOfSingleVal(long m_SingleVal) const
{
	char* pError = NULL;
	if (AsnList<T>::size() != m_SingleVal)
	{
		char cTmperr[200];
		snprintf(cTmperr, 200, "_______\nList--SingleValue Constraints:\n_______\nError: --Invalid Number of Elements in List--\nNumber of Elements: %d must match the Constraint Single Value: %d \n", AsnList<T>::size(), m_SingleVal);
		pError = strdup(cTmperr);
	}

	return pError;
}

template <class T> char* AsnList<T>::checkSOfVRange(long m_Lower, long m_Upper) const
{
	char* pError = NULL;

	if (AsnList<T>::size() < m_Lower)
	{
		char cTmperr[200];
		snprintf(cTmperr, 200, "_______\nList--Valuerange Constraints:\n_______\nError: --Not Enough Elements In List--\nNumber of Elements: %d is below the Lower Limit: %d \n", AsnList<T>::size(), m_Lower);
		pError = strdup(cTmperr);
	}

	if (AsnList<T>::size() > m_Upper)
	{
		char cTmperr[200];
		snprintf(cTmperr, 200, "_______\nList--Valuerange Constraints:\n_______\nError: --Too Many Elements In List--\nNumber of Elements: %d is above the Upper Limit: %d \n", AsnList<T>::size(), m_Upper);
		pError = strdup(cTmperr);
	}

	return pError;
}

template <class T> int AsnList<T>::checkConstraints(SNACC::ConstraintFailList* pConstraintFails) const
{
	for (typename AsnList<T>::const_iterator i = AsnList<T>::begin(); i != AsnList<T>::end(); ++i)
		i->checkConstraints(pConstraintFails);
	return 0;
}

template <class T> SNACC::AsnLen AsnList<T>::BEncContent(SNACC::AsnBuf& b) const
{
	SNACC::AsnLen sum = 0;
	for (typename AsnList<T>::const_reverse_iterator i = AsnList<T>::rbegin(); i != AsnList<T>::rend(); ++i)
		sum += i->BEnc(b);
	return sum;
}

template <class T> void AsnList<T>::BDecContent(const SNACC::AsnBuf& b, SNACC::AsnTag, SNACC::AsnLen elmtLen, SNACC::AsnLen& bytesDecoded)
{
	SNACC::AsnLen localBytesDecoded = 0;

	while ((elmtLen == INDEFINITE_LEN) || (localBytesDecoded < elmtLen))
	{
		if (elmtLen == INDEFINITE_LEN)
		{
			if (b.PeekByte() == EOC_TAG_ID)
			{
				SNACC::BDecEoc(b, localBytesDecoded);
				break;
			}
		}
		append()->BDec(b, localBytesDecoded);
	}

	bytesDecoded += localBytesDecoded;
} // AsnList<T>::BDecContent()

template <class T> void AsnList<T>::JEnc(SJson::Value& b) const
{
	b = SJson::Value(SJson::arrayValue);
	SJson::Value tmp;

	for (auto i = AsnList<T>::cbegin(); i != AsnList<T>::cend(); ++i)
	{
		i->JEnc(tmp);
		b.append(tmp);
	}
}

template <class T> bool AsnList<T>::JDec(const SJson::Value& b)
{
	Clear();

	if (b.isArray())
	{
		SJson::Value::const_iterator it = b.begin();
		while (it != b.end())
		{
			if (append()->JDec(*it) == false)
				return false;

			it++;
		}
		return true;
	}
	return false;
}

//
template <class T> void AsnList<T>::Print(std::ostream& os, unsigned short indent) const
{
	os << "OF ";
	++indent;

	if (AsnList<T>::empty())
		os << "is EMPTY\n";
	else
	{
		os << AsnList<T>::front().typeName() << " \n";

		for (auto i = AsnList<T>::cbegin(); i != AsnList<T>::cend(); ++i)
		{
			SNACC::Indent(os, indent);
			i->Print(os, indent);
		}
		SNACC::Indent(os, --indent);
	}
	os << "}\n";
} // AsnList<T>::Print()

template <class T> void AsnList<T>::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	if (AsnList<T>::empty())
		os << "-- Void --\n";
	else
	{
		// os << "<" << first->elmt->typeName() << ">" << endl;
		for (auto i = AsnList<T>::cbegin(); i != AsnList<T>::cend(); ++i)
			i->PrintXML(os, lpszTitle);
		// os << "</" << first->elmt->typeName() << ">" << endl;
	}
} // AsnList<T>::PrintXML()

//////////////////////////
// AsnSeqOf template   //
////////////////////////
template <class T> class AsnSeqOf : public AsnList<T>
{
public:
	SNACC::AsnLen BEnc(SNACC::AsnBuf& b) const override;
	void BDec(const SNACC::AsnBuf& b, SNACC::AsnLen& bytesDecoded) override;

	void Print(std::ostream& os, unsigned short indent = 0) const override;
	void PrintXML(std::ostream& os, const char* lpszTitle = NULL) const;

	// virtual int checkConstraints(ConstraintFailList* pConstraintFails)const;
};

//////////////////////////
// AsnSeqOf template   //
//       CODE          //
////////////////////////
template <class T> SNACC::AsnLen AsnSeqOf<T>::BEnc(SNACC::AsnBuf& b) const
{
	SNACC::AsnLen l = AsnSeqOf<T>::BEncContent(b);
	l += SNACC::BEncDefLen(b, l);
	l += BEncTag1(b, SNACC::UNIV, SNACC::CONS, SNACC::SEQ_TAG_CODE);
	return l;
}

template <class T> void AsnSeqOf<T>::BDec(const SNACC::AsnBuf& b, SNACC::AsnLen& bytesDecoded)
{
	// Clear the existing contents
	AsnSeqOf<T>::clear();

	SNACC::AsnTag tagId = SNACC::BDecTag(b, bytesDecoded);
	if (tagId != MAKE_TAG_ID(SNACC::UNIV, SNACC::CONS, SNACC::SEQ_TAG_CODE))
		throw SNACC::InvalidTagException(this->typeName(), tagId, __FILE__, __LINE__, "AsnSeqOf<T>:BDec()");
	SNACC::AsnLen elmtLen;
	elmtLen = SNACC::BDecLen(b, bytesDecoded);
	AsnSeqOf<T>::BDecContent(b, tagId, elmtLen, bytesDecoded);
}

template <class T> void AsnSeqOf<T>::Print(std::ostream& os, unsigned short indent) const
{
	os << "\n";
	SNACC::Indent(os, indent);
	os << "{ -- " << this->typeName() << " SEQUENCE OF ";
	++indent;

	if (AsnSeqOf<T>::empty())
		os << "is EMPTY\n";
	else
	{
		os << this->front().typeName() << " \n";

		for (auto i = AsnSeqOf<T>::cbegin(); i != AsnSeqOf<T>::cend(); ++i)
		{
			SNACC::Indent(os, indent);
			i->Print(os, indent);
		}
	}
	SNACC::Indent(os, --indent);
	os << "}\n";
}

template <class T> void AsnSeqOf<T>::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	if (lpszTitle)
	{
		os << "<" << lpszTitle;
		os << " typeName=\"" << this->typeName() << "\"";
	}
	else
		os << "<" << this->typeName();

	os << " type=\"SEQUENCE_OF\">\n";
	AsnList<T>::PrintXML(os, lpszTitle);

	if (lpszTitle)
		os << "</" << lpszTitle << ">";
	else
		os << "</" << this->typeName() << ">";
}

////////////////////////
// AsnSetOf template //
//////////////////////
template <class T> class AsnSetOf : public AsnList<T>
{
public:
	SNACC::AsnLen PEnc(SNACC::AsnBufBits& b) const;
	SNACC::AsnLen BEnc(SNACC::AsnBuf& b) const;
	SNACC::AsnLen BEncContent(SNACC::AsnBuf& b) const;
	void BDec(const SNACC::AsnBuf& b, SNACC::AsnLen& bytesDecoded);
	void PDec(SNACC::AsnBufBits& b, SNACC::AsnLen& bitsDecoded);

	void Print(std::ostream& os, unsigned short indent = 0) const;
	void PrintXML(std::ostream& os, const char* lpszTitle = NULL) const;

	// virtual int checkCosntraints(ConstraintFailList* pConstraintFails);
};

////////////////////////
// AsnSetOf template //
//     CODE          //
//////////////////////
template <class T> SNACC::AsnLen AsnSetOf<T>::BEnc(SNACC::AsnBuf& b) const
{
	SNACC::AsnLen l = BEncContent(b);
	l += SNACC::BEncDefLen(b, l);
	l += BEncTag1(b, SNACC::UNIV, SNACC::CONS, SNACC::SET_TAG_CODE);
	return l;
}

template <class T> SNACC::AsnLen AsnSetOf<T>::BEncContent(SNACC::AsnBuf& b) const
{
	SNACC::AsnLen totalLen = BEncEocIfNec(b);

	// Encode each component of the SET OF into the AsnBuf list;
	std::list<SNACC::AsnBuf> bufList;
	for (auto i = AsnSetOf<T>::cbegin(); i != AsnSetOf<T>::cend(); ++i)
	{
		SNACC::AsnBuf& bufRef = *bufList.insert(bufList.end(), SNACC::AsnBuf());
		totalLen += i->BEnc(bufRef);
	}

	// Sort array of AsnBuf according to DER rules for SET OF
	//
	bufList.sort();

	std::list<SNACC::AsnBuf>::reverse_iterator ri;
	for (ri = bufList.rbegin(); ri != bufList.rend(); ++ri)
		b.splice(*ri);

	return totalLen;
}

template <class T> void AsnSetOf<T>::BDec(const SNACC::AsnBuf& b, SNACC::AsnLen& bytesDecoded)
{
	// Clear the existing elements
	AsnSetOf<T>::clear();

	SNACC::AsnTag tagId = SNACC::BDecTag(b, bytesDecoded);
	if (tagId != MAKE_TAG_ID(SNACC::UNIV, SNACC::CONS, SNACC::SET_TAG_CODE))
		throw SNACC::InvalidTagException(this->typeName(), tagId, __FILE__, __LINE__, "AsnSetOf<T>::BDec()");

	SNACC::AsnLen elmtLen;
	elmtLen = SNACC::BDecLen(b, bytesDecoded);
	AsnSetOf<T>::BDecContent(b, tagId, elmtLen, bytesDecoded);
}

//
//
template <class T> void AsnSetOf<T>::PDec(SNACC::AsnBufBits& b, SNACC::AsnLen& bitsDecoded)
{
	// Clear the existing elements
	AsnSetOf<T>::clear();

	SNACC::AsnInt intSetOfLength;
	intSetOfLength.PDec(b, bitsDecoded);
	for (int i = 0; i < intSetOfLength; i++)
		AsnSetOf<T>::append()->PDec(b, bitsDecoded);
}

template <class T> SNACC::AsnLen AsnSetOf<T>::PEnc(SNACC::AsnBufBits& b) const
{
	SNACC::AsnInt intSetOfLength((int)AsnSetOf<T>::size());
	intSetOfLength.PEnc(b);

	SNACC::AsnLen totalLen = 0;
	std::list<SNACC::AsnBufBits> bufList;

	// Encode each component of the SET OF into the AsnBuf list
	for (auto i = AsnSetOf<T>::begin(); i != AsnSetOf<T>::end(); ++i)
	{
		SNACC::AsnBufBits& bufRef = *bufList.insert(bufList.end(), SNACC::AsnBufBits(b.IsAligned()));
		totalLen += i->PEnc(bufRef);
	}

	bufList.sort();

	std::list<SNACC::AsnBufBits>::iterator iBuf;
	for (iBuf = bufList.begin(); iBuf != bufList.end(); ++iBuf)
		iBuf->AppendTo(b);

	return totalLen;
} // end of AsnSetOf<T>::PEnc()

template <class T> void AsnSetOf<T>::Print(std::ostream& os, unsigned short indent) const
{
	os << "{ -- " << this->typeName() << " SET OF ";
	++indent;

	if (AsnSetOf<T>::empty())
		os << "is EMPTY\n";
	else
	{
		os << this->front().typeName() << " \n";

		for (auto i = AsnSetOf<T>::begin(); i != AsnSetOf<T>::end(); ++i)
		{
			SNACC::Indent(os, indent);
			i->Print(os, indent);
		}
		SNACC::Indent(os, --indent);
	}
	os << "}\n";
}

template <class T> void AsnSetOf<T>::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	if (lpszTitle)
	{
		os << "<" << lpszTitle;
		os << " typeName=\"" << this->typeName() << "\"";
	}
	else
		os << "<" << this->typeName();

	os << " type=\"SET_OF\">";
	AsnList<T>::PrintXML(os);

	if (lpszTitle)
		os << "</" << lpszTitle << ">";
	else
		os << "</" << this->typeName() << ">";
}

namespace SNACC
{

	// Implementierung von JEnc und JDec ist in asn-optionalparam.cpp
	class AsnOptionalParameters : public AsnSeqOf<AsnOptionalParam>
	{
	public:
		const char* typeName() const override
		{
			return "AsnOptionalParameters";
		}
		AsnType* Clone() const override
		{
			return new AsnOptionalParameters(*this);
		}
		void JEnc(SJson::Value& b) const override;
		bool JDec(const SJson::Value& b) override;
	};

} // namespace SNACC
/*
template <class T>
int ListsEquiv (AsnList<T>& l1, AsnList<U>& l2)
{
	if (l1.Count() != l2.Count())
		return false;

	l1.SetCurrToFirst();
	l2.SetCurrToFirst();

	for (; l1.Curr() != NULL; l1.GoNext(), l2.GoNext())
	{
		if (*l1.Curr() !=  *l2.Curr())
		{
			return false;
		}
	}
	return true;
}
*/

#endif // SNACC_ASN_LISTSET_H
