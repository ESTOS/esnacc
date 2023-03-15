/*#include <string>*/
#include "../include/asn-incl.h"

_BEGIN_SNACC_NAMESPACE

void AsnStringOcts::JEnc(EJson::Value &b) const
{
	b = EJson::Value(m_str.c_str());
}

bool AsnStringOcts::JDec(const EJson::Value &b)
{
	clear();
	if (b.isConvertibleTo(EJson::stringValue))
	{
		m_str = b.asString();
		return true;
	}
	return false;
}




_END_SNACC_NAMESPACE
