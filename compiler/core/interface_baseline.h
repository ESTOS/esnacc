#ifndef INTERFACE_BASELINE_H
#define INTERFACE_BASELINE_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Set when -nodeprecated is given without a date; resolved after ASN.1 parse.
 */
extern int gNodeprecatedAutoResolve;

/**
 * Baseline file name beside ASN.1 modules (optional).
 */
#define DEPRECATED_BASELINE_FILENAME "deprecatedbaseline.txt"

/**
 * Set when -nodeprecated:date overrides deprecatedbaseline.txt for the deprecated cutoff.
 */
extern int gCliNodeprecatedExplicit;

/**
 * Set after bare -nodeprecated baseline resolution so filter output can be rebuilt.
 */
extern int gInterfaceBaselineAutoResolved;

/**
 * Unix baseline read from deprecatedbaseline.txt (date form) while auto-resolve is pending.
 */
extern long long gi64FileBaselineUnix;

/**
 * Sets gMajorInterfaceVersion to YYYYMMDD derived from a unix baseline timestamp.
 */
void SetMajorVersionFromUnixBaseline(long long i64UnixTime);

/**
 * Applies a deprecated-symbol cutoff and aligns the major/baseline label when using dates.
 */
void ApplyDeprecatedCutoffUnix(long long i64UnixTime);

/**
 * Parses one trimmed deprecatedbaseline.txt line (date or legacy integer major).
 * Returns 1 when a value was accepted.
 */
int ParseInterfaceVersionValue(const char* szTrimmedLine);

/**
 * Reads deprecatedbaseline.txt from an ASN.1 directory when present.
 * Missing files and comment-only files are ignored.
 * Returns 1 when an active baseline value was loaded.
 */
int LoadDeprecatedBaselineFile(const char* szDirectory);

/**
 * Copies deprecatedbaseline.txt into the output directory when the source file exists.
 * Returns 1 on success, 0 when the source file is absent.
 */
int CopyDeprecatedBaselineFile(const char* szSourceDirectory, const char* szTargetDirectory);

/**
 * Returns the unix timestamp of the configured API deprecation baseline for this compile
 * (deprecatedbaseline.txt date, -nodeprecated, or newest @deprecated). Returns 0 when no
 * dated baseline is active.
 */
long long GetInterfaceBaselineUnixTimestamp(void);

/**
 * Finishes bare -nodeprecated handling after all ASN.1 files were parsed.
 */
void ResolveInterfaceBaselineAfterParse(void);

/**
 * Resets baseline globals; for unit tests only.
 */
void ResetInterfaceBaselineStateForTests(void);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_BASELINE_H */
