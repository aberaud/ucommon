#ifndef	_UCOMMON_CONFIG_H_
#define	_UCOMMON_CONFIG_H_

#define	UCOMMON_NAMESPACE	ucc
#define	NAMESPACE_UCOMMON	namespace ucc {
#define	END_NAMESPACE		};

#ifndef	_REENTRANT
#define _REENTRANT 1
#endif

#ifndef	_POSIX_PTHREAD_SEMANTICS
#define	_POSIX_PTHREAD_SEMANTICS
#endif

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32)
#define	_MSWINDOWS_
#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0501
#undef	_WIN32_WINNT
#define	_WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINNT
#define	_WIN32_WINNT 0x0501
#endif

#include <windows.h>
#ifndef	__EXPORT
#define	__EXPORT	__declspec(dllimport)
#endif
#define	__LOCAL
#elif UCOMMON_VISIBILITY > 0
#define	__EXPORT	__attribute__ ((visibility("default")))
#define	__LOCAL		__attribute__ ((visibility("hidden")))
#else
#define	__EXPORT
#define	__LOCAL
#endif

#include <pthread.h>

#ifdef _MSC_VER
typedef	signed __int8 int8_t;
typedef	unsigned __int8 uint8_t;
typedef	signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef char *caddr_t;

#else

#include <sys/types.h>
#include <stdint.h>

#ifdef	_MSWINDOWS_
typedef char *caddr_t;
#endif

#endif

typedef	int32_t rpcint_t;
typedef	rpcint_t rpcbool_t;
typedef	char *rpcstring_t;
typedef	double rpcdouble_t;

#include <stdlib.h>

__EXPORT void *operator new(size_t size);
__EXPORT void *operator new[](size_t size);
__EXPORT void *operator new(size_t size, size_t extra);
__EXPORT void *operator new(size_t size, caddr_t place, size_t max);
__EXPORT void *operator new[](size_t size, caddr_t place, size_t max);
__EXPORT void *operator new(size_t size, caddr_t place);
__EXPORT void *operator new[](size_t size, caddr_t place);
__EXPORT void operator delete(void *mem);
__EXPORT void operator delete[](void *mem);

#ifdef	__GNUC__
extern "C" __EXPORT void __cxa_pure_virtual(void);
#endif

#ifndef	DEBUG
#ifndef	NDEBUG
#define	NDEBUG
#endif
#endif

#ifdef	DEBUG
#ifdef	NDEBUG
#undef	NDEBUG
#endif
#endif

#include <assert.h>
#ifdef	DEBUG
#define	crit(x)	assert(x)
#else
#define	crit(x) if(!(x)) abort()
#endif

#endif