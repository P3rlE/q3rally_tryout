/*
 * Q3Rally / Q3LCC-friendly string.h
 * ----------------------------------
 * This header is designed to work even when the toolchain (or Q3LCC) lacks
 * some libc functions. It tries to include the system <string.h> if supported,
 * then supplies portable fallbacks for missing pieces.
 *
 * Notes:
 * - All function implementations here use static/internal linkage to avoid
 *   duplicate symbol issues across translation units.
 * - C89-compatible: no C99-only features used. "inline" is guarded.
 * - Safe helpers included: strlcpy, strlcat, strnlen, strdup, case-insensitive compares.
 * - If you already have project-specific wrappers (e.g., Q_stricmp), you can ignore
 *   the helpers here or #define Q3_STRING_NO_HELPERS before including.
 *
 * WARNING: Placing this file as "string.h" in your include path will shadow the system
 * header. Where supported, we use #include_next to still pull in the real declarations.
 */

#ifndef Q3_STRING_H_
#define Q3_STRING_H_

/* Detect and include the real system header when possible */
#if defined(__has_include)
  #if __has_include_next(<string.h>)
    #define Q3_HAVE_INCLUDE_NEXT 1
  #endif
#endif

#ifdef Q3_HAVE_INCLUDE_NEXT
  #include_next <string.h>
#else
  /* Minimal forward declarations if no system header is found */
  #include <stddef.h> /* size_t */
  void *memcpy(void *dest, const void *src, size_t n);
  void *memset(void *s, int c, size_t n);
  int   memcmp(const void *s1, const void *s2, size_t n);
  void *memmove(void *dest, const void *src, size_t n);

  size_t strlen(const char *s);
  char  *strcpy(char *dest, const char *src);
  char  *strncpy(char *dest, const char *src, size_t n);
  char  *strcat(char *dest, const char *src);
  char  *strncat(char *dest, const char *src, size_t n);
  int    strcmp(const char *s1, const char *s2);
  int    strncmp(const char *s1, const char *s2, size_t n);
  char  *strchr(const char *s, int c);
  char  *strrchr(const char *s, int c);
  char  *strstr(const char *haystack, const char *needle);
#endif /* Q3_HAVE_INCLUDE_NEXT */

#include <stddef.h>
#include <stdlib.h> /* malloc, free */
#include <ctype.h>  /* tolower */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  #define Q3_INLINE static inline
#else
  #define Q3_INLINE static
#endif

/* -------------------------------
 * Implementations / Fallbacks
 * Only compiled if corresponding symbol appears missing or when no system header.
 * We detect presence by allowing the user to define Q3_NO_* macros if desired.
 * ------------------------------- */

#ifndef Q3_STRING_HAVE_MEMCPY
Q3_INLINE void *q3__memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) { *d++ = *s++; }
    return dest;
}
/* Only provide memcpy if not provided by system header */
#ifndef Q3_SUPPRESS_EXPORT_memcpy
#define memcpy q3__memcpy
#endif
#endif

#ifndef Q3_STRING_HAVE_MEMSET
Q3_INLINE void *q3__memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) { *p++ = (unsigned char)c; }
    return s;
}
#ifndef Q3_SUPPRESS_EXPORT_memset
#define memset q3__memset
#endif
#endif

#ifndef Q3_STRING_HAVE_MEMCMP
Q3_INLINE int q3__memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--) {
        if (*a != *b) return (int)(*a - *b);
        ++a; ++b;
    }
    return 0;
}
#ifndef Q3_SUPPRESS_EXPORT_memcmp
#define memcmp q3__memcmp
#endif
#endif

#ifndef Q3_STRING_HAVE_MEMMOVE
Q3_INLINE void *q3__memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) return dest;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}
#ifndef Q3_SUPPRESS_EXPORT_memmove
#define memmove q3__memmove
#endif
#endif

#ifndef Q3_STRING_HAVE_STRLEN
Q3_INLINE size_t q3__strlen(const char *s) {
    const char *p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}
#ifndef Q3_SUPPRESS_EXPORT_strlen
#define strlen q3__strlen
#endif
#endif

#ifndef Q3_STRING_HAVE_STRCPY
Q3_INLINE char *q3__strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0') {}
    return dest;
}
#ifndef Q3_SUPPRESS_EXPORT_strcpy
#define strcpy q3__strcpy
#endif
#endif

#ifndef Q3_STRING_HAVE_STRNCPY
Q3_INLINE char *q3__strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*d++ = *src++) != '\0') n--;
    if (n) while (--n) *d++ = '\0';
    return dest;
}
#ifndef Q3_SUPPRESS_EXPORT_strncpy
#define strncpy q3__strncpy
#endif
#endif

#ifndef Q3_STRING_HAVE_STRCAT
Q3_INLINE char *q3__strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) ++d;
    while ((*d++ = *src++) != '\0') {}
    return dest;
}
#ifndef Q3_SUPPRESS_EXPORT_strcat
#define strcat q3__strcat
#endif
#endif

#ifndef Q3_STRING_HAVE_STRNCAT
Q3_INLINE char *q3__strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) ++d;
    if (n) {
        while (n-- && (*d = *src++) != '\0') ++d;
        *d = '\0';
    }
    return dest;
}
#ifndef Q3_SUPPRESS_EXPORT_strncat
#define strncat q3__strncat
#endif
#endif

#ifndef Q3_STRING_HAVE_STRCMP
Q3_INLINE int q3__strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { ++s1; ++s2; }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}
#ifndef Q3_SUPPRESS_EXPORT_strcmp
#define strcmp q3__strcmp
#endif
#endif

#ifndef Q3_STRING_HAVE_STRNCMP
Q3_INLINE int q3__strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    while (n-- && *s1 && (*s1 == *s2)) { if (n == 0) return 0; ++s1; ++s2; }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}
#ifndef Q3_SUPPRESS_EXPORT_strncmp
#define strncmp q3__strncmp
#endif
#endif

#ifndef Q3_STRING_HAVE_STRCHR
Q3_INLINE char *q3__strchr(const char *s, int c) {
    char ch = (char)c;
    for (; *s; ++s) if (*s == ch) return (char *)s;
    return (ch == '\0') ? (char *)s : NULL;
}
#ifndef Q3_SUPPRESS_EXPORT_strchr
#define strchr q3__strchr
#endif
#endif

#ifndef Q3_STRING_HAVE_STRRCHR
Q3_INLINE char *q3__strrchr(const char *s, int c) {
    const char *last = NULL;
    char ch = (char)c;
    for (; *s; ++s) if (*s == ch) last = s;
    return (char *)( (ch == '\0') ? s : last );
}
#ifndef Q3_SUPPRESS_EXPORT_strrchr
#define strrchr q3__strrchr
#endif
#endif

#ifndef Q3_STRING_HAVE_STRSTR
Q3_INLINE char *q3__strstr(const char *haystack, const char *needle) {
    size_t nlen = 0;
    const char *n = needle;
    while (*n++) ++nlen;
    if (nlen == 0) return (char *)haystack;
    for (; *haystack; ++haystack) {
        if (*haystack == *needle) {
            if (q3__strncmp(haystack, needle, nlen) == 0) return (char *)haystack;
        }
    }
    return NULL;
}
#ifndef Q3_SUPPRESS_EXPORT_strstr
#define strstr q3__strstr
#endif
#endif

/* ---------- Safe helpers & extras (non-standard) ---------- */
#ifndef Q3_STRING_NO_HELPERS

/* strnlen */
#ifndef Q3_STRING_HAVE_STRNLEN
Q3_INLINE size_t q3_strnlen(const char *s, size_t maxlen) {
    size_t i = 0;
    while (i < maxlen && s[i] != '\0') ++i;
    return i;
}
#endif

/* strlcpy / strlcat (OpenBSD semantics) */
#ifndef Q3_STRING_HAVE_STRLCPY
Q3_INLINE size_t q3_strlcpy(char *dst, const char *src, size_t siz) {
    size_t srclen = q3__strlen(src);
    if (siz) {
        size_t copy = (srclen >= siz) ? (siz - 1) : srclen;
        if (copy) q3__memcpy(dst, src, copy);
        dst[copy] = '\0';
    }
    return srclen;
}
#endif

#ifndef Q3_STRING_HAVE_STRLCAT
Q3_INLINE size_t q3_strlcat(char *dst, const char *src, size_t siz) {
    size_t dlen = q3__strlen(dst);
    size_t slen = q3__strlen(src);
    if (dlen >= siz) return siz + slen;
    size_t space = siz - dlen - 1;
    size_t copy = (slen > space) ? space : slen;
    if (copy) q3__memcpy(dst + dlen, src, copy);
    dst[dlen + copy] = '\0';
    return dlen + slen;
}
#endif

/* strdup */
#ifndef Q3_STRING_HAVE_STRDUP
Q3_INLINE char *q3_strdup(const char *s) {
    size_t len = q3__strlen(s) + 1;
    char *d = (char *)malloc(len);
    if (d) q3__memcpy(d, s, len);
    return d;
}
#endif

/* Case-insensitive compares (portable) */
#ifndef Q3_STRING_HAVE_STRICMP
Q3_INLINE int q3_stricmp(const char *a, const char *b) {
    unsigned char ca, cb;
    for (;;) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);
        if (ca != cb || ca == '\0' || cb == '\0') return (int)(ca - cb);
    }
}
#endif

#ifndef Q3_STRING_HAVE_STRNICMP
Q3_INLINE int q3_strnicmp(const char *a, const char *b, size_t n) {
    unsigned char ca, cb;
    if (n == 0) return 0;
    do {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);
        if (ca != cb) return (int)(ca - cb);
        if (ca == '\0') return 0;
    } while (--n);
    return 0;
}
#endif

#endif /* Q3_STRING_NO_HELPERS */

#endif /* Q3_STRING_H_ */
