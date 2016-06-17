#ifndef LMICE_EAL_COMMON_H
#define LMICE_EAL_COMMON_H

#define UNREFERENCED_PARAM(param) (void)param

/* 32bit -64bit */
#if defined(__LP64__) || defined(_WIN64) || defined(__amd64) || defined(__x86_64__) || defined(_M_X64)
#define arch64 1
#define arch32 0
#define declare_pad8_pointer(type, name) \
    type* name
#define declare_pad8_pointer_array(type, name, n) \
    type* name[n]
#else /* 32bit */
#define arch64 0
#define arch32 1
#define declare_pad8_pointer(type, name) \
    type* name;  \
    uint32_t name##_pad8
#define declare_pad8_pointer_array(type, name, n) \
    type* name[n];  \
    uint32_t name##pad8[n];
#endif

/**
  support gcc msvc
*/
#if defined(__GNUC__)   /** GCC */
#include <unistd.h>
#define forceinline __attribute__((always_inline)) inline static
#define forcepack(x) __attribute__((__aligned__((x))))
#include "sys/types.h"

#if defined(__MINGW32__)
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif

#elif defined(_MSC_VER) /** MSC */

#define forceinline static __forceinline
#define forcepack(x) __declspec(align(x))
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

#else                   /** Other compiler */

#error(Unsupported compiler)
#endif

#endif /** LMICE_EAL_COMMON_H */
