/*#include <string>*/
#include "../include/asn-incl.h"

_BEGIN_SNACC_NAMESPACE

void AsnStringOcts::JEnc(SJson::Value& b) const
{
	b = SJson::Value(m_str.c_str());
}

bool AsnStringOcts::JDec(const SJson::Value& b)
{
	clear();
	if (b.isConvertibleTo(SJson::stringValue))
	{
		m_str = b.asString();
		return true;
	}
	return false;
}

_END_SNACC_NAMESPACE
