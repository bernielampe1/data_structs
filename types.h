#ifndef __TYPES_H__
#define __TYPES_H__

#define ABS(N) ((N < 0) ? (-N) : (N))

// signed primitive types
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

// unsigned primitive types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

constexpr static double tol = 1e-12;

#endif // __TYPES_H__
