#ifndef FNMATCH_H
#define FNMATCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#define FNM_NOMATCH 1 /* Match failed. */

#define FNM_NOESCAPE 0x01 /* Disable backslash escaping. */
#define FNM_PATHNAME 0x02 /* Slash must be matched by slash. */
#define FNM_PERIOD 0x04   /* Period must be matched by period. */
#define FNM_CASEFOLD 0x08 /* Fold cases */

    extern int fnmatch(const char* pattern, const char* string, int flags);

#ifdef __cplusplus
}
#endif

#endif
