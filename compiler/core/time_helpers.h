#ifndef TIME_HELPERS_H
#define TIME_HELPERS_H

/**
 * Converts a date in different notations into the seconds based on unix time 1.1.1970
 * DD.MM.YYYY
 * MM/DD/YYYY
 * MM-DD-YYYY
 * YYYYMMDD
 *
 * Returns -1 on error
 */
long long ConvertDateToUnixTime(const char* szDate);

/**
 * Converts a unix time into something readable
 *
 * Returns a pointer to a buffer that needs to get released with Free
 */
char* ConvertUnixTimeToReadable(const long long tmUnixTime);

/**
 * Converts a unix time into an ISO timestamp
 *
 * Returns a pointer to a buffer that needs to get released with Free
 */
char* ConvertUnixTimeToISO(const long long tmUnixTime);

#endif