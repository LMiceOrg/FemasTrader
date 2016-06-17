#ifndef LMICE_EAL_ALIGN_H
#define LMICE_EAL_ALIGN_H

#if defined(__GNUC__)  && __GNUC__ > 4
#define COMPILE_TIME_ASSERT(X) ({ extern int __attribute__((error("assertion failure: '" #X "' not true"))) compile_time_check(); ((X)?0:compile_time_check()),0; })

#else

#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(!!(COND))*2-1]
#define COMPILE_TIME_ASSERT3(X,L) STATIC_ASSERT(X,static_assertion_at_line_##L)
#define COMPILE_TIME_ASSERT2(X,L) COMPILE_TIME_ASSERT3(X,L)
#define COMPILE_TIME_ASSERT(X)    COMPILE_TIME_ASSERT2(X,__LINE__)

#endif

#define EAL_STRUCT_ALIGN(x) COMPILE_TIME_ASSERT(sizeof(x) % 8 == 0)

#endif /** LMICE_EAL_ALIGN_H */
