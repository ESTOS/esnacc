// file: .../c++-lib/src/asn-systemtime- AsnSystemTimeReal (ASN.1 REAL) type
//
// ste: 04.11.2014
// Addded AsnSystemTime as native type for different JSON Encoding

#include "../include/asn-incl.h"
#include "math.h"

#ifdef _WIN32
#include <Windows.h>
#endif

_BEGIN_SNACC_NAMESPACE

#ifdef _WIN32

// Copied from EDate.
#define ONEDAY 1.0
#define ONEHOUR (ONEDAY / 24)
#define ONEMINUTE (ONEHOUR / 60)
#define ONESECOND (ONEMINUTE / 60)
#define ONEMILLISECOND (ONESECOND / 1000)

// Copied from EDate.
bool VariantTimeToSystemTimeWithMilliseconds(const DATE& dVariantTime, SYSTEMTIME& st)
{
	// Take off 0.5 seconds from the original variant time to avoid the rounding off problem.
	// If the milliseconds part is larger than 500 ms then VariantTimeToSystemTime will round off the seconds.
	if (!VariantTimeToSystemTime(dVariantTime - (ONESECOND / 2), &st))
		return false;

	double fraction = dVariantTime - (int)dVariantTime; // extracts the fraction part

	double hours;
	hours = fraction = (fraction - (int)fraction) * 24;

	double minutes;
	minutes = (hours - (int)hours) * 60;

	double seconds;
	seconds = (minutes - (int)minutes) * 60;

	double milliseconds;
	milliseconds = (seconds - (int)seconds) * 1000;

	if (milliseconds > 999.5L)
		VariantTimeToSystemTime(dVariantTime, &st);
	else
		st.wMilliseconds = (WORD)round(milliseconds);

	return true;
}

std::string DateToISO8601(DATE dt)
{
	// we discard such old data immediately
	if (dt < 10)
		return "";

	// 2012-04-23T18:25:43.511Z
	// 2012-04-23T18:25:43.500Z		// Zeros at the end are technically superfluous, but make them easier to read.
	char szDateTime[40] = {0};
	SYSTEMTIME sysTime;
	memset(&sysTime, 0x00, sizeof(sysTime));
	if (VariantTimeToSystemTimeWithMilliseconds(dt, sysTime))
	{
		if (sysTime.wMilliseconds == 0)
			sprintf_s(szDateTime, 40, "%04u-%02u-%02uT%02u:%02u:%02uZ", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
		else
			sprintf_s(szDateTime, 40, "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	}
	return szDateTime;
}

// Copied from EDate.
bool SystemTimeToVariantTimeWithMilliseconds(const SYSTEMTIME& st, DATE& dVariantTime)
{
	WORD wMilliseconds = st.wMilliseconds;

	// The SystemTimeToVariantTime function normally discards the milliseconds.
	// To be on the safe side (you never know if Microsoft will change its mind), the
	// Clear milliseconds. This means that we are always guaranteed the value without milliseconds.
	SYSTEMTIME st_temp = st;
	st_temp.wMilliseconds = 0;
	if (!SystemTimeToVariantTime(&st_temp, &dVariantTime))
		return false;

	// Manually convert the millisecond information into variant
	// fraction and add it to system converted value
	dVariantTime += (wMilliseconds * ONEMILLISECOND);
	return true;
}

double ISO8601ToDATE(const char* szDateTime)
{
	double dblValue = 0;
	SYSTEMTIME sysTime;
	memset(&sysTime, 0x00, sizeof(sysTime));

	int iLen = (int)strlen(szDateTime);

	if (iLen == 20)
	{
		// 2012-04-23T18:25:43Z
#if _MSC_VER < 1900
		sscanf(szDateTime, "%04u-%02u-%02uT%02u:%02u:%02uZ", &sysTime.wYear, &sysTime.wMonth, &sysTime.wDay, &sysTime.wHour, &sysTime.wMinute, &sysTime.wSecond);
#else
		sscanf_s(szDateTime, "%04hu-%02hu-%02huT%02hu:%02hu:%02huZ", &sysTime.wYear, &sysTime.wMonth, &sysTime.wDay, &sysTime.wHour, &sysTime.wMinute, &sysTime.wSecond);
#endif
	}
	else if (iLen > 20)
	{
		// 2012-04-23T18:25:43.511Z
		// 2012-04-23T18:25:43.500Z
		// 2012-04-23T18:25:43.5Z
		float fSecs = 0.0f;
#if _MSC_VER < 1900
		sscanf(szDateTime, "%04u-%02u-%02uT%02u:%02u:%fZ", &sysTime.wYear, &sysTime.wMonth, &sysTime.wDay, &sysTime.wHour, &sysTime.wMinute, &fSecs);
#else
		sscanf_s(szDateTime, "%04hu-%02hu-%02huT%02hu:%02hu:%fZ", &sysTime.wYear, &sysTime.wMonth, &sysTime.wDay, &sysTime.wHour, &sysTime.wMinute, &fSecs);
#endif
		float fSecsPart = 0.0f;
		float fFracts = modff(fSecs, &fSecsPart);
		sysTime.wSecond = (WORD)floor(fSecsPart);
		sysTime.wMilliseconds = (WORD)round(fFracts * 1000.0f);
	}

	if (sysTime.wYear && sysTime.wMonth && sysTime.wDay)
		SystemTimeToVariantTimeWithMilliseconds(sysTime, dblValue);

	return dblValue;
}
#endif

time_t AsnSystemTime::get_time_t() const
{
	// 25569 days offset, 86400 seconds per day
	return (time_t)((value - 25569) * 86400);
}

void AsnSystemTime::set_time_t(time_t tim)
{
	// 25569 days offset, 86400 seconds per day
	double dbltmp = (double)tim;
	dbltmp = dbltmp / 86400;
	value = dbltmp + 25569;
}

void AsnSystemTime::JEnc(SJson::Value& b) const
{
	// ISO 8601
	// 2012-04-23T18:25:43.511Z
#ifdef _WIN32
	b = SJson::Value(DateToISO8601(value));
#else
	b = SJson::Value(value);
#endif
}

bool AsnSystemTime::JDec(const SJson::Value& b)
{
	value = 0;
	if (b.isConvertibleTo(SJson::realValue))
	{
		value = b.asDouble();
		return true;
	}
#ifdef _WIN32
	else if (b.isString())
	{
		// ISO 8601
		// 2012-04-23T18:25:43.511Z
		value = ISO8601ToDATE(b.asCString());
		return true;
	}
#endif
	return false;
}

void AsnSystemTime::Print(std::ostream& os, unsigned short /*indent*/) const
{
	os << value;
}

void AsnSystemTime::PrintXML(std::ostream& os, const char* lpszTitle) const
{
	os << "<REAL>";
	if (lpszTitle)
		os << lpszTitle;
	os << "-";
	Print(os);
	os << "</REAL>\n";
}

_END_SNACC_NAMESPACE
