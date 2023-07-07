#include "../include/SnaccROSEHelper.h"
#include "../include/SnaccROSEInterfaces.h"
#include "../include/SNACCROSE.h"

bool SnaccROSEHelper::DecodeInvoke(const SNACC::ROSEMessage* pMsg, SNACC::AsnType* type, SnaccROSESender* pBase) {
	if (pMsg->invoke->argument)
	{
		if (pMsg->invoke->argument->anyBuf)
		{
			SNACC::AsnLen len;
			type->BDec(*pMsg->invoke->argument->anyBuf, len);
			// pBase->LogTransportData(false, SNACC::TransportEncoding::BER, nullptr, 1, );
		}
		else if (pMsg->invoke->argument->jsonBuf)
			type->JDec(*pMsg->invoke->argument->jsonBuf);
	}



	return false;
}