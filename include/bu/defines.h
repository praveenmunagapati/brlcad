/*                      D E F I N E S . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** @file defines.h
 *
 * Commonly used definitions for the BRL-CAD Utility Library, LIBBU.
 *
 * The two letters "BU" stand for "BRL-CAD" and "Utility".  This
 * library provides several layers of low-level utility routines,
 * providing features that make cross-platform coding easier.
 *
 * Parallel processing support:  threads, semaphores, parallel-malloc.
 * Consolidated logging support:  bu_log(), bu_exit(), and bu_bomb().
 *
 * The intention is that these routines are general extensions to the
 * data types offered by the C language itself, and to the basic C
 * runtime support provided by the system LIBC.  All routines in LIBBU
 * are designed to be "parallel-safe" (sometimes called "mp-safe" or
 * "thread-safe" if parallelism is via threading) to greatly ease code
 * development for multiprocessor systems.
 *
 * All of the data types provided by this library are defined in bu.h;
 * none of the routines in this library will depend on data types
 * defined in other BRL-CAD header files, such as vmath.h.  Look for
 * those routines in LIBBN.
 *
 * All truly fatal errors detected by the library use bu_bomb() to
 * exit with a status of 12.  The LIBBU variants of system calls
 * (e.g., bu_malloc()) do not return to the caller (unless there's a
 * bomb hook defined) unless they succeed, thus sparing the programmer
 * from constantly having to check for NULL return codes.
 *
 */
#ifndef BU_DEFINES_H
#define BU_DEFINES_H

#include "common.h"
#include <stddef.h>
#include <sys/types.h>

#ifndef BU_EXPORT
#  if defined(BU_DLL_EXPORTS) && defined(BU_DLL_IMPORTS)
#    error "Only BU_DLL_EXPORTS or BU_DLL_IMPORTS can be defined, not both."
#  elif defined(BU_DLL_EXPORTS)
#    define BU_EXPORT __declspec(dllexport)
#  elif defined(BU_DLL_IMPORTS)
#    define BU_EXPORT __declspec(dllimport)
#  else
#    define BU_EXPORT
#  endif
#endif

/* NOTE: do not rely on these values */
#define BRLCAD_OK 0
#define BRLCAD_ERROR 1

 /**
 * BU_IGNORE provides a common mechanism for innocuously ignoring a
 * parameter that is sometimes used and sometimes not.  It should
 * "practically" result in nothing of concern happening.  It's
 * commonly used by macros that disable functionality based on
 * compilation settings (e.g., BU_ASSERT()) and shouldn't normally
 * need to be used directly by code.
 */
#define BU_IGNORE(_parm) (void)(_parm)

/**
 * @def BU_DIR_SEPARATOR
 * the default directory separator character
 */
#ifdef DIR_SEPARATOR
#  define BU_DIR_SEPARATOR DIR_SEPARATOR
#else
#  ifdef DIR_SEPARATOR_2
#    define BU_DIR_SEPARATOR DIR_SEPARATOR_2
#  else
#    ifdef _WIN32
#      define BU_DIR_SEPARATOR '\\'
#    else
#      define BU_DIR_SEPARATOR '/'
#    endif  /* _WIN32 */
#  endif  /* DIR_SEPARATOR_2 */
#endif  /* DIR_SEPARATOR */

/**
 * Maximum length of a filesystem path.  Typically defined in a system
 * file but if it isn't set, we create it.
 */
#ifndef MAXPATHLEN
#  ifdef PATH_MAX
#    define MAXPATHLEN PATH_MAX
#  else
#    ifdef _MAX_PATH
#      define MAXPATHLEN _MAX_PATH
#    else
#      define MAXPATHLEN 1024
#    endif
#  endif
#endif

/**
 * set to the path list separator character
 */
#if defined(PATH_SEPARATOR)
#  define BU_PATH_SEPARATOR PATH_SEPARATOR
#else
#  if defined(_WIN32)
#    define BU_PATH_SEPARATOR ';'
#  else
#    define BU_PATH_SEPARATOR ':'
#  endif  /* _WIN32 */
#endif  /* PATH_SEPARATOR */


/**
 * @def BU_FLSTR
 *
 * Macro for getting a concatenated string of the current file and
 * line number.  Produces something of the form: "filename.c"":""1234"
 */
#define bu_cpp_str(s) # s
#define bu_cpp_xstr(s) bu_cpp_str(s)
#define bu_cpp_glue(a, b) a ## b
#define bu_cpp_xglue(a, b) bu_cpp_glue(a, b)
#define BU_FLSTR __FILE__ ":" bu_cpp_xstr(__LINE__)


/**
 * shorthand declaration of a printf-style functions
 */
#if !defined(_BU_ATTR_PRINTF12)
#define _BU_ATTR_PRINTF12 __attribute__ ((__format__ (__printf__, 1, 2)))
#endif
#if !defined(_BU_ATTR_PRINTF23)
#define _BU_ATTR_PRINTF23 __attribute__ ((__format__ (__printf__, 2, 3)))
#endif
#if !defined(_BU_ATTR_SCANF23)
#define _BU_ATTR_SCANF23 __attribute__ ((__format__ (__scanf__, 2, 3)))
#endif

/**
 * shorthand declaration of a function that doesn't return
 */
#define _BU_ATTR_NORETURN __attribute__ ((__noreturn__))

/**
 * shorthand declaration of a function that should always be inline
 */
#define _BU_ATTR_ALWAYS_INLINE __attribute__ ((always_inline))

/**
 *  If we're compiling strict, turn off "format string vs arguments"
 *  checks - BRL-CAD customizes the arguments to some of these
 *  function types (adding bu_vls support) and that is a problem with
 *  strict checking.
 */
#if defined(STRICT_FLAGS)
#  undef _BU_ATTR_PRINTF12
#  undef _BU_ATTR_PRINTF23
#  undef _BU_ATTR_SCANF23
#  undef _BU_ATTR_NORETURN
#  define _BU_ATTR_PRINTF12
#  define _BU_ATTR_PRINTF23
#  define _BU_ATTR_SCANF23
#  define _BU_ATTR_NORETURN
#endif

/**
 * This macro is used to take the 'C' function name, and convert it at
 * compile time to the FORTRAN calling convention.
 *
 * Lower case, with a trailing underscore.
 */
#define BU_FORTRAN(lc, uc) lc ## _


/**
 * Fast dynamic memory allocation macro for small pointer allocations.
 * Memory is automatically initialized to zero and, similar to
 * bu_calloc(), is guaranteed to return non-NULL (or bu_bomb()).
 *
 * Memory acquired with BU_GET() should be returned with BU_PUT(), NOT
 * with bu_free().
 *
 * Use BU_ALLOC() for dynamically allocating structures that are
 * relatively large, infrequently allocated, or otherwise don't need
 * to be fast.
 */
#if 0
#define BU_GET(_ptr, _type) _ptr = (_type *)bu_heap_get(sizeof(_type))
#else
#define BU_GET(_ptr, _type) _ptr = (_type *)bu_calloc(1, sizeof(_type), #_type " (BU_GET) " BU_FLSTR)
#endif

/**
 * Handy dynamic memory deallocator macro.  Deallocated memory has the
 * first byte zero'd for sanity (and potential early detection of
 * double-free crashing code) and the pointer is set to NULL.
 *
 * Memory acquired with bu_malloc()/bu_calloc() should be returned
 * with bu_free(), NOT with BU_PUT().
 */
#if 0
#define BU_PUT(_ptr, _type) *(uint8_t *)(_type *)(_ptr) = /*zap*/ 0; bu_heap_put(_ptr, sizeof(_type)); _ptr = NULL
#else
#define BU_PUT(_ptr, _type) do { *(uint8_t *)(_type *)(_ptr) = /*zap*/ 0; bu_free(_ptr, #_type " (BU_PUT) " BU_FLSTR); _ptr = NULL; } while (0)
#endif

/**
 * Convenience macro for allocating a single structure on the heap.
 * Not intended for performance-critical code.  Release memory
 * acquired with bu_free() or BU_FREE() to dealloc and set NULL.
 */
#define BU_ALLOC(_ptr, _type) _ptr = (_type *)bu_calloc(1, sizeof(_type), #_type " (BU_ALLOC) " BU_FLSTR)

/**
 * Convenience macro for deallocating a single structure allocated on
 * the heap (with bu_malloc(), bu_calloc(), BU_ALLOC()).
 */
#define BU_FREE(_ptr, _type) do { bu_free(_ptr, #_type " (BU_FREE) " BU_FLSTR); _ptr = (_type *)NULL; } while (0)


/**
 * @def BU_ASSERT(eqn)
 * Quick and easy macros to generate an informative error message and
 * abort execution if the specified condition does not hold true.
 *
 * @def BU_ASSERT_PTR(eqn)
 * Quick and easy macros to generate an informative error message and
 * abort execution if the specified condition does not hold true.
 *
 * @def BU_ASSERT_LONG(eqn)
 * Quick and easy macros to generate an informative error message and
 * abort execution if the specified condition does not hold true.
 *
 * @def BU_ASSERT_DOUBLE(eqn)
 * Quick and easy macros to generate an informative error message and
 * abort execution if the specified condition does not hold true.
 *
 * Example: BU_ASSERT_LONG(j+7, <, 42);
 */
#ifdef NO_BOMBING_MACROS
#  define BU_ASSERT(_equation) BU_IGNORE((_equation))
#else
#  define BU_ASSERT(_equation)	\
    if (UNLIKELY(!(_equation))) { \
	bu_log("BU_ASSERT(" #_equation ") failed, file %s, line %d\n", \
	       __FILE__, __LINE__); \
	bu_bomb("BU_ASSERT failure\n"); \
    }
#endif

#ifdef NO_BOMBING_MACROS
#  define BU_ASSERT_PTR(_lhs, _relation, _rhs) BU_IGNORE((_lhs)); BU_IGNORE((_rhs))
#else
#  define BU_ASSERT_PTR(_lhs, _relation, _rhs)	\
    if (UNLIKELY(!((_lhs) _relation (_rhs)))) { \
	bu_log("BU_ASSERT_PTR(" #_lhs #_relation #_rhs ") failed, lhs=%p, rhs=%p, file %s, line %d\n", \
	       (void *)(_lhs), (void *)(_rhs), \
	       __FILE__, __LINE__); \
	bu_bomb("BU_ASSERT_PTR failure\n"); \
    }
#endif


#ifdef NO_BOMBING_MACROS
#  define BU_ASSERT_LONG(_lhs, _relation, _rhs) BU_IGNORE((_lhs)); BU_IGNORE((_rhs))
#else
#  define BU_ASSERT_LONG(_lhs, _relation, _rhs)	\
    if (UNLIKELY(!((_lhs) _relation (_rhs)))) { \
	bu_log("BU_ASSERT_LONG(" #_lhs #_relation #_rhs ") failed, lhs=%ld, rhs=%ld, file %s, line %d\n", \
	       (long)(_lhs), (long)(_rhs), \
	       __FILE__, __LINE__); \
	bu_bomb("BU_ASSERT_LONG failure\n"); \
    }
#endif


#ifdef NO_BOMBING_MACROS
#  define BU_ASSERT_SIZE_T(_lhs, _relation, _rhs) BU_IGNORE((_lhs)); BU_IGNORE((_rhs))
#else
#  define BU_ASSERT_SIZE_T(_lhs, _relation, _rhs)	\
    if (UNLIKELY(!((_lhs) _relation (_rhs)))) { \
	bu_log("BU_ASSERT_SIZE_T(" #_lhs #_relation #_rhs ") failed, lhs=%zd, rhs=%zd, file %s, line %d\n", \
	       (size_t)(_lhs), (size_t)(_rhs), \
	       __FILE__, __LINE__); \
	bu_bomb("BU_ASSERT_SIZE_T failure\n"); \
    }
#endif


#ifdef NO_BOMBING_MACROS
#  define BU_ASSERT_SSIZE_T(_lhs, _relation, _rhs) BU_IGNORE((_lhs)); BU_IGNORE((_rhs))
#else
#  define BU_ASSERT_SSIZE_T(_lhs, _relation, _rhs)	\
    if (UNLIKELY(!((_lhs) _relation (_rhs)))) { \
	bu_log("BU_ASSERT_SSIZE_T(" #_lhs #_relation #_rhs ") failed, lhs=%zd, rhs=%zd, file %s, line %d\n", \
	       (ssize_t)(_lhs), (ssize_t)(_rhs), \
	       __FILE__, __LINE__); \
	bu_bomb("BU_ASSERT_SSIZE_T failure\n"); \
    }
#endif


#ifdef NO_BOMBING_MACROS
#  define BU_ASSERT_DOUBLE(_lhs, _relation, _rhs) BU_IGNORE((_lhs)); BU_IGNORE((_rhs))
#else
#  define BU_ASSERT_DOUBLE(_lhs, _relation, _rhs)	\
    if (UNLIKELY(!((_lhs) _relation (_rhs)))) { \
	bu_log("BU_ASSERT_DOUBLE(" #_lhs #_relation #_rhs ") failed, lhs=%lf, rhs=%lf, file %s, line %d\n", \
	       (double)(_lhs), (double)(_rhs), \
	       __FILE__, __LINE__); \
	bu_bomb("BU_ASSERT_DOUBLE failure\n"); \
    }
#endif

/**
 * fastf_t - Intended to be the fastest floating point data type on
 * the current machine, with at least 64 bits of precision.  On 16 and
 * 32 bit machines, this is typically "double", but on 64 bit machines,
 * it is often "float".  Virtually all floating point variables (and
 * more complicated data types, like vect_t and mat_t) are defined as
 * fastf_t.  The one exception is when a subroutine return is a
 * floating point value; that is always declared as "double".
 *
 * TODO: If used pervasively, it should eventually be possible to make
 * fastf_t a GMP C++ type for fixed-precision computations.
 */
typedef double fastf_t;

/**
 * Definitions about limits of floating point representation
 * Eventually, should be tied to type of hardware (IEEE, IBM, Cray)
 * used to implement the fastf_t type.
 *
 * MAX_FASTF - Very close to the largest value that can be held by a
 * fastf_t without overflow.  Typically specified as an integer power
 * of ten, to make the value easy to spot when printed.  TODO: macro
 * function syntax instead of constant (DEPRECATED)
 *
 * SQRT_MAX_FASTF - sqrt(MAX_FASTF), or slightly smaller.  Any number
 * larger than this, if squared, can be expected to * produce an
 * overflow.  TODO: macro function syntax instead of constant
 * (DEPRECATED)
 *
 * SMALL_FASTF - Very close to the smallest value that can be
 * represented while still being greater than zero.  Any number
 * smaller than this (and non-negative) can be considered to be
 * zero; dividing by such a number can be expected to produce a
 * divide-by-zero error.  All divisors should be checked against
 * this value before actual division is performed.  TODO: macro
 * function syntax instead of constant (DEPRECATED)
 *
 * SQRT_SMALL_FASTF - sqrt(SMALL_FASTF), or slightly larger.  The
 * value of this is quite a lot larger than that of SMALL_FASTF.  Any
 * number smaller than this, when squared, can be expected to produce
 * a zero result.  TODO: macro function syntax instead of constant
 * (DEPRECATED)
 *
 */
#if defined(vax)
/* DEC VAX "D" format, the most restrictive */
#  define MAX_FASTF		1.0e37	/* Very close to the largest number */
#  define SQRT_MAX_FASTF	1.0e18	/* This squared just avoids overflow */
#  define SMALL_FASTF		1.0e-37	/* Anything smaller is zero */
#  define SQRT_SMALL_FASTF	1.0e-18	/* This squared gives zero */
#else
/* IBM format, being the next most restrictive format */
#  define MAX_FASTF		1.0e73	/* Very close to the largest number */
#  define SQRT_MAX_FASTF	1.0e36	/* This squared just avoids overflow */
#  define SMALL_FASTF		1.0e-77	/* Anything smaller is zero */
#  if defined(aux)
#    define SQRT_SMALL_FASTF	1.0e-40 /* _doprnt error in libc */
#  else
#    define SQRT_SMALL_FASTF	1.0e-39	/* This squared gives zero */
#  endif
#endif

/** DEPRECATED, do not use */
#define SMALL SQRT_SMALL_FASTF


/**
 * It is necessary to have a representation of 1.0/0.0 or log(0),
 * i.e., "infinity" that fits within the dynamic range of the machine
 * being used.  This constant places an upper bound on the size object
 * which can be represented in the model.  With IEEE 754 floating
 * point, this may print as 'inf' and is represented with all 1 bits
 * in the biased-exponent field and all 0 bits in the fraction with
 * the sign indicating positive (0) or negative (1) infinity.
 */
#ifndef INFINITY
#  if defined(HUGE_VAL)
#    define INFINITY ((fastf_t)HUGE_VAL)
#  elif defined(HUGE_VALF)
#    define INFINITY ((fastf_t)HUGE_VALF)
#  elif defined(HUGE)
#    define INFINITY ((fastf_t)HUGE)
#  elif defined(MAXDOUBLE)
#    define INFINITY ((fastf_t)MAXDOUBLE)
#  elif defined(MAXFLOAT)
#    define INFINITY ((fastf_t)MAXFLOAT)
#  else
     /* all else fails, just pick something big slightly over 32-bit
      * single-precision floating point that has worked well before.
      */
#    define INFINITY ((fastf_t)1.0e40)
#  endif
#endif


#endif  /* BU_DEFINES_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
