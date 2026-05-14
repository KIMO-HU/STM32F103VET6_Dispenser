/* Minimal stdint.h for embedded ARM GCC */
#ifndef _STDINT_H
#define _STDINT_H

typedef signed char     int8_t;
typedef short int       int16_t;
typedef int             int32_t;
typedef long long int   int64_t;

typedef unsigned char   uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long int uint64_t;

typedef int8_t          int_least8_t;
typedef int16_t         int_least16_t;
typedef int32_t         int_least32_t;
typedef int64_t         int_least64_t;

typedef uint8_t         uint_least8_t;
typedef uint16_t        uint_least16_t;
typedef uint32_t        uint_least32_t;
typedef uint64_t        uint_least64_t;

typedef int32_t         int_fast8_t;
typedef int32_t         int_fast16_t;
typedef int32_t         int_fast32_t;
typedef int64_t         int_fast64_t;

typedef uint32_t        uint_fast8_t;
typedef uint32_t        uint_fast16_t;
typedef uint32_t        uint_fast32_t;
typedef uint64_t        uint_fast64_t;

typedef long int        intptr_t;
typedef unsigned long int uintptr_t;

typedef long long int   intmax_t;
typedef unsigned long long int uintmax_t;

#define INT8_MIN        (-128)
#define INT16_MIN       (-32768)
#define INT32_MIN       (-2147483648)
#define INT64_MIN       (-9223372036854775808LL)

#define INT8_MAX        (127)
#define INT16_MAX       (32767)
#define INT32_MAX       (2147483647)
#define INT64_MAX       (9223372036854775807LL)

#define UINT8_MAX       (255)
#define UINT16_MAX      (65535)
#define UINT32_MAX      (4294967295U)
#define UINT64_MAX      (18446744073709551615ULL)

#define INTPTR_MIN      (-2147483648)
#define INTPTR_MAX      (2147483647)
#define UINTPTR_MAX     (4294967295U)

#define PTRDIFF_MIN     (-2147483648)
#define PTRDIFF_MAX     (2147483647)

#define SIZE_MAX        (4294967295U)

#endif /* _STDINT_H */
