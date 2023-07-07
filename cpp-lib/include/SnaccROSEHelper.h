#ifndef _SnaccROSEHelper_h_
#define _SnaccROSEHelper_h_

namespace SNACC {
	class ROSEMessage;
	class AsnType;
}

class SnaccROSESender;

class SnaccROSEHelper {
public:
	static bool DecodeInvoke(const SNACC::ROSEMessage* pMsg, SNACC::AsnType* type, SnaccROSESender* pBase);
};

#endif