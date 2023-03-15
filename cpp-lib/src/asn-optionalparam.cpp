// file: .../c++-lib/src/asn-optionalparam- AsnOptionalParam type
//
// ste: 14.11.2014
// Addded AsnOptionalParam as native type for different JSON Encoding

#include "../include/asn-incl.h"
#include "../include/asn-listset.h"

_BEGIN_SNACC_NAMESPACE

/*
OptionalParams:
Old JSON Format (like BER)
"optionalParams": [
		{
		key : "AnonymousLogin",
		value: { integerdata: true }
		},
		{
		key : "AnonymousHintData",
		value: { stringdata: this.endpointID }
		}
]

New JSON Format:
"optionalParams": {
	AnonymousLogin: true,
	AnonymousHintData: "test",
	otherData: { binarydata: "base64" }
}
*/

//Implementation from asn-listset.h
void AsnOptionalParameters::JEnc (EJson::Value &b) const
{
	/*
	b = EJson::Value(EJson::arrayValue);
	EJson::Value tmp;

	for (AsnOptionalParameters::const_reverse_iterator i = rbegin(); i != rend(); ++i)
	{
		i->JEnc(tmp);
		b.append(tmp);
	}
	*/
	b = EJson::Value(EJson::objectValue);
	EJson::Value tmp;

	for (AsnOptionalParameters::const_reverse_iterator i = rbegin(); i != rend(); ++i)
	{
		i->value.JEnc(tmp);
		std::string key;
		i->key.getUTF8(key);
		b[key] = tmp;
	}
}

bool AsnOptionalParameters::JDec (const EJson::Value &b)
{
	Clear();

	//original Implementation (like BER)
	if (b.isArray())
	{
		EJson::Value::const_iterator it = b.begin();
		while (it != b.end())
		{
			if (append()->JDec(*it) == false)
				return false;

			it++;
		}
		return true;
	}
	else if (b.isObject())
	{
		EJson::Value::Members members = b.getMemberNames();
		for (EJson::Value::Members::iterator it = members.begin(); it != members.end(); it++)
		{
			SNACC::AsnOptionalParam param;
			param.key.setUTF8(it->c_str());
			if (!param.value.JDec(b[*it]))
				return false;
			append(param);
		}
		return true;
	}

	return false;
}


void AsnOptionalParam::Init(void)
{
}


int AsnOptionalParam::checkConstraints(ConstraintFailList* pConstraintFails) const{
	key.checkConstraints(pConstraintFails);

	value.checkConstraints(pConstraintFails);

	return 0;
}


void AsnOptionalParam::Clear()
{
	value.Clear();
}

AsnOptionalParam::AsnOptionalParam(const AsnOptionalParam &that)
{
	Init();
	*this = that;
}
AsnType *AsnOptionalParam::Clone() const
{
	return new AsnOptionalParam(*this);
}

AsnOptionalParam &AsnOptionalParam::operator = (const AsnOptionalParam &that)
{
	if (this != &that)
	{
		Clear();
		key = that.key;
		value = that.value;
	}

	return *this;
}

AsnLen
	AsnOptionalParam::BEncContent (AsnBuf &_b) const
{
	AsnLen totalLen = 0;
	AsnLen l=0;

	l = value.BEncContent (_b);
	totalLen += l;

	l = key.BEncContent (_b);
	l += BEncDefLen (_b, l);

	l += BEncTag1 (_b, UNIV, PRIM, UTF8STRING_TAG_CODE);
	totalLen += l;

	return totalLen;
} // AsnOptionalParam::BEncContent


void AsnOptionalParam::BDecContent (const AsnBuf &_b, AsnTag /*tag0*/, AsnLen elmtLen0, AsnLen &bytesDecoded)
{
	FUNC(" AsnOptionalParam::BDecContent");
	Clear();
	AsnLen seqBytesDecoded = 0;
	AsnLen elmtLen1 = 0;
	// Wenn nix da ist, brauchen wir nicht weiter machen --> raus
    if (elmtLen0 == 0) {
        return;
	}

	AsnTag tag1 = BDecTag (_b, seqBytesDecoded);

	if ((tag1 == MAKE_TAG_ID (UNIV, PRIM, UTF8STRING_TAG_CODE))
		|| (tag1 == MAKE_TAG_ID (UNIV, CONS, UTF8STRING_TAG_CODE)))
	{
		elmtLen1 = BDecLen (_b, seqBytesDecoded);
		key.BDecContent (_b, tag1, elmtLen1, seqBytesDecoded);
		tag1 = BDecTag (_b, seqBytesDecoded);
	}
	else
	{
		throw EXCEPT("SEQUENCE is missing non-optional root elmt", DECODE_ERROR);
	}

	if ((tag1 == MAKE_TAG_ID (UNIV, PRIM, UTF8STRING_TAG_CODE))
		|| (tag1 == MAKE_TAG_ID (UNIV, CONS, UTF8STRING_TAG_CODE))
		|| (tag1 == MAKE_TAG_ID (UNIV, PRIM, OCTETSTRING_TAG_CODE))
		|| (tag1 == MAKE_TAG_ID (UNIV, CONS, OCTETSTRING_TAG_CODE))
		|| (tag1 == MAKE_TAG_ID (UNIV, PRIM, INTEGER_TAG_CODE)))
	{
		elmtLen1 = BDecLen (_b, seqBytesDecoded);
		value.BDecContent (_b, tag1, elmtLen1, seqBytesDecoded);
	}
	else
	{
		throw EXCEPT("SEQUENCE is missing non-optional root elmt", DECODE_ERROR);
	}

	bytesDecoded += seqBytesDecoded;
	if (elmtLen0 == INDEFINITE_LEN)
	{
		BDecEoc (_b, bytesDecoded);
		return;
	}
	else if (seqBytesDecoded != elmtLen0)
	{
		throw EXCEPT("Length discrepancy on sequence", DECODE_ERROR);
	}
	else
		return;
} // AsnOptionalParam::BDecContent

AsnLen AsnOptionalParam::BEnc (AsnBuf &_b) const
{
	AsnLen l=0;
	l = BEncContent (_b);
	l += BEncConsLen (_b, l);
	l += BEncTag1 (_b, UNIV, CONS, SEQ_TAG_CODE);
	return l;
}

void AsnOptionalParam::JEnc (EJson::Value &b) const
{
	b = EJson::Value(EJson::objectValue);

	EJson::Value tmp;

	key.JEnc (tmp);
	b["key"] = tmp;

	value.JEnc (tmp);
	b["value"] = tmp;

}


void AsnOptionalParam::BDec (const AsnBuf &_b, AsnLen &bytesDecoded)
{
	FUNC(" AsnOptionalParam::BDec");
	AsnTag tag;
	AsnLen elmtLen1;

	if ((tag = BDecTag (_b, bytesDecoded)) != MAKE_TAG_ID (UNIV, CONS, SEQ_TAG_CODE))
	{
		throw InvalidTagException(typeName(), tag, STACK_ENTRY);
	}
	elmtLen1 = BDecLen (_b, bytesDecoded);
	BDecContent (_b, tag, elmtLen1, bytesDecoded);
}

bool AsnOptionalParam::JDec (const EJson::Value &b){
	Clear();
	if (!b.isObject()) return false;

	EJson::Value tmp;
	if (b.isMember("key")) {
		if (!key.JDec(b["key"])) throw InvalidTagException(typeName(), "decode failed: key", STACK_ENTRY);
	}
	else throw InvalidTagException(typeName(), "not found: key", STACK_ENTRY);
	if (b.isMember("value")) {
		if (!value.JDec(b["value"])) throw InvalidTagException(typeName(), "decode failed: value", STACK_ENTRY);
	}
	else throw InvalidTagException(typeName(), "not found: value", STACK_ENTRY);
	return true;
} // AsnOptionalParam::JDec

void AsnOptionalParam::Print(std::ostream& os, unsigned short indent) const
{
	os << std::endl;
	Indent(os, indent);
	os << "{ -- SEQUENCE --" << std::endl;
	++indent;

	Indent(os, indent);
	os << "key ";
	key.Print(os, indent);
	os << std::endl;

	Indent(os, indent);
	os << "value ";
	value.Print(os, indent);
	os << std::endl;

	--indent;
	Indent(os, indent);
	os << "}\n";
} // end of AsnOptionalParam::Print()


AsnOptionalParamChoice::AsnOptionalParamChoice(const AsnOptionalParamChoice &that)
{
	Init();
	*this = that;
}
void AsnOptionalParamChoice::Init(void)
{
	// initialize choice to no choiceId to first choice and set pointer to NULL
	choiceId = stringdataCid;
	stringdata = NULL;
}


int AsnOptionalParamChoice::checkConstraints(ConstraintFailList* pConstraintFails) const
{
	if (stringdata != NULL)
		stringdata->checkConstraints(pConstraintFails);

	if (binarydata != NULL)
		binarydata->checkConstraints(pConstraintFails);

	if (integerdata != NULL)
		integerdata->checkConstraints(pConstraintFails);

	return 0;
}


void AsnOptionalParamChoice::Clear()
{
	switch (choiceId)
	{
	case stringdataCid:
		delete stringdata;
		stringdata = NULL;
		break;
	case binarydataCid:
		delete binarydata;
		binarydata = NULL;
		break;
	case integerdataCid:
		delete integerdata;
		integerdata = NULL;
		break;
	} // end of switch
} // end of Clear()

AsnType *AsnOptionalParamChoice::Clone() const
{
	return new AsnOptionalParamChoice(*this);
}

AsnOptionalParamChoice &AsnOptionalParamChoice::operator = (const AsnOptionalParamChoice &that)
{
	if (this != &that)
	{
		Clear();
		// Check first type in choice to determine if choice is empty
		if (that.stringdata != NULL)
		{
			switch (choiceId = that.choiceId)
			{
			case stringdataCid:
				stringdata = new UTF8String(*that.stringdata);
				break;
			case binarydataCid:
				binarydata = new AsnOcts(*that.binarydata);
				break;
			case integerdataCid:
				integerdata = new AsnInt(*that.integerdata);
				break;
			}// end of switch
		}// end of if
	}

	return *this;
}

AsnLen
	AsnOptionalParamChoice::BEncContent (AsnBuf &_b) const
{
	FUNC("AsnOptionalParamChoice::BEncContent (AsnBuf &_b)");
	AsnLen l=0;
	switch (choiceId)
	{
	case stringdataCid:
		l = stringdata->BEncContent (_b);
		l += BEncDefLen (_b, l);

		l += BEncTag1 (_b, UNIV, PRIM, UTF8STRING_TAG_CODE);
		break;

	case binarydataCid:
		l = binarydata->BEncContent (_b);
		l += BEncDefLen (_b, l);

		l += BEncTag1 (_b, UNIV, PRIM, OCTETSTRING_TAG_CODE);
		break;

	case integerdataCid:
		l = integerdata->BEncContent (_b);
		l += BEncDefLen (_b, l);

		l += BEncTag1 (_b, UNIV, PRIM, INTEGER_TAG_CODE);
		break;

	default:
		throw EXCEPT("Choice is empty", ENCODE_ERROR);
	} // end switch
	return l;
} // AsnOptionalParamChoice::BEncContent


void AsnOptionalParamChoice::BDecContent (const AsnBuf &_b, AsnTag tag, AsnLen elmtLen0, AsnLen &bytesDecoded /*, s env*/)
{
	FUNC("AsnOptionalParamChoice::BDecContent()");
	Clear();
	switch (tag)
	{
	case MAKE_TAG_ID (UNIV, PRIM, UTF8STRING_TAG_CODE):
	case MAKE_TAG_ID (UNIV, CONS, UTF8STRING_TAG_CODE):
		choiceId = stringdataCid;
		stringdata = new UTF8String;
		stringdata->BDecContent (_b, tag, elmtLen0, bytesDecoded);
		break;

	case MAKE_TAG_ID (UNIV, PRIM, OCTETSTRING_TAG_CODE):
	case MAKE_TAG_ID (UNIV, CONS, OCTETSTRING_TAG_CODE):
		choiceId = binarydataCid;
		binarydata = new AsnOcts;
		binarydata->BDecContent (_b, tag, elmtLen0, bytesDecoded);
		break;

	case MAKE_TAG_ID (UNIV, PRIM, INTEGER_TAG_CODE):
		choiceId = integerdataCid;
		integerdata = new AsnInt;
		integerdata->BDecContent (_b, tag, elmtLen0, bytesDecoded);
		break;

	default:
		{        throw InvalidTagException(typeName(), tag, STACK_ENTRY);
		break;
		}
	} // end switch
} // AsnOptionalParamChoice::BDecContent


AsnLen AsnOptionalParamChoice::BEnc (AsnBuf &_b) const
{
	AsnLen l=0;
	l = BEncContent (_b);
	return l;
}

void AsnOptionalParamChoice::JEnc (EJson::Value &b) const
{
	FUNC("AsnOptionalParamChoice::JEnc()");
	/*
	b = EJson::Value(EJson::objectValue);

	EJson::Value tmp;

	switch (choiceId)
	{
	case stringdataCid:
		stringdata->JEnc (tmp);
		b["stringdata"] = tmp;
		break;

	case binarydataCid:
		binarydata->JEnc (tmp);
		b["binarydata"] = tmp;
		break;

	case integerdataCid:
		integerdata->JEnc (tmp);
		b["integerdata"] = tmp;
		break;

	default:
		throw EXCEPT("Choice is empty", ENCODE_ERROR);
	} // end switch
	*/
	switch (choiceId)
	{
	case stringdataCid:
		stringdata->JEnc (b);
		break;

	case binarydataCid:
		{
			b = EJson::Value(EJson::objectValue);
			EJson::Value tmp;
			binarydata->JEnc (tmp);
			b["binarydata"] = tmp;
			break;
		}

	case integerdataCid:
		integerdata->JEnc (b);
		break;

	default:
		throw EXCEPT("Choice is empty", ENCODE_ERROR);
	} // end switch

} // AsnOptionalParamChoice::JEnc


void AsnOptionalParamChoice::BDec (const AsnBuf &_b, AsnLen &bytesDecoded)
{
	AsnLen elmtLen = 0;
	AsnTag tag;

	/*  CHOICEs are a special case - grab identifying tag */
	/*  this allows easier handling of nested CHOICEs */
	tag = BDecTag (_b, bytesDecoded);
	elmtLen = BDecLen (_b, bytesDecoded);
	BDecContent (_b, tag, elmtLen, bytesDecoded);
}

bool AsnOptionalParamChoice::JDec (const EJson::Value &b){
	FUNC("AsnOptionalParamChoice::JDec()");
	Clear();
	if (b.isObject()) {
		EJson::Value tmp;
		if (b.isMember("stringdata")) {
			choiceId = stringdataCid;
			delete stringdata;
			stringdata = new UTF8String;
			if (!stringdata->JDec(b["stringdata"])) throw InvalidTagException(typeName(), "decode failed: stringdata", STACK_ENTRY);
		}
		else if (b.isMember("binarydata")) {
			choiceId = binarydataCid;
			delete binarydata;
			binarydata = new AsnOcts;
			if (!binarydata->JDec(b["binarydata"])) throw InvalidTagException(typeName(), "decode failed: binarydata", STACK_ENTRY);
		}
		else if (b.isMember("integerdata")) {
			choiceId = integerdataCid;
			delete integerdata;
			integerdata = new AsnInt;
			if (!integerdata->JDec(b["integerdata"])) throw InvalidTagException(typeName(), "decode failed: integerdata", STACK_ENTRY);
		}
		else
			throw InvalidTagException(typeName(), "no valid choice member", STACK_ENTRY);
	}
	else if (b.isConvertibleTo(EJson::intValue))
	{
		choiceId = integerdataCid;
		delete integerdata;
		integerdata = new AsnInt;
		if (!integerdata->JDec(b)) throw InvalidTagException(typeName(), "decode failed: integerdata", STACK_ENTRY);
	}
	else if (b.isString())
	{
		choiceId = stringdataCid;
		delete stringdata;
		stringdata = new UTF8String;
		if (!stringdata->JDec(b)) throw InvalidTagException(typeName(), "decode failed: stringdata", STACK_ENTRY);
	}
	else
	{
		throw InvalidTagException(typeName(), "no valid choice member", STACK_ENTRY);
	}

	return true;
} // AsnOptionalParamChoice::JDec


void AsnOptionalParamChoice::Print(std::ostream& os, unsigned short indent) const
{
	switch (choiceId)
	{
	case stringdataCid:
		os << "stringdata ";
		if (stringdata)
			stringdata->Print(os, indent);
		else
			os << "<CHOICE value is missing>\n";
		break;

	case binarydataCid:
		os << "binarydata ";
		if (binarydata)
			binarydata->Print(os, indent);
		else
			os << "<CHOICE value is missing>\n";
		break;

	case integerdataCid:
		os << "integerdata ";
		if (integerdata)
			integerdata->Print(os, indent);
		else
			os << "<CHOICE value is missing>\n";
		break;

	} // end of switch
} // end of AsnOptionalParamChoice::Print()



_END_SNACC_NAMESPACE
