#include "../include/SnaccROSEInterfaces.h"
#include "../include/SnaccROSEBase.h"
#include "../include/SNACCROSE.h"

using namespace SNACC;

long SnaccROSESender::HandleInvokeResult(long lRoseResult, SNACC::ROSEInvoke* pInvokeMsg, SNACC::ROSEResult* pResultMsg, SNACC::ROSEError* pErrorMsg, SNACC::AsnType* result, SNACC::AsnType* error, SnaccInvokeContext* cxt)
{
	if (lRoseResult == ROSE_NOERROR)
	{
		if (pResultMsg && pResultMsg->result && result)
		{
			AsnLen len;
			try
			{
				if (pResultMsg->result->result.anyBuf)
					result->BDec(*pResultMsg->result->result.anyBuf, len);
				else if (pResultMsg->result->result.jsonBuf)
					result->JDec(*pResultMsg->result->result.jsonBuf);

				if (GetLogLevel(true))
					PrintAsnType(false, result, pInvokeMsg);

			}
			catch (SnaccException&)
			{
				lRoseResult = ROSE_RE_DECODE_FAILED;
			}
		}
	}
	else if (lRoseResult == ROSE_ERROR_VALUE)
	{
		if (pErrorMsg && pErrorMsg->error && error)
		{
			AsnLen len;
			try
			{
				if (pErrorMsg->error->anyBuf)
					error->BDec(*pErrorMsg->error->anyBuf, len);
				else if (pErrorMsg->error->jsonBuf)
					error->JDec(*pErrorMsg->error->jsonBuf);

				if (GetLogLevel(true))
					PrintAsnType(false, error, pInvokeMsg);
			}
			catch (SnaccException&)
			{
				lRoseResult = ROSE_RE_DECODE_FAILED;
			}
		}
	}
	return lRoseResult;
}