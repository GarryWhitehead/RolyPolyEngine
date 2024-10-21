/* Copyright (c) 2024 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

// Adapted from the gcc wiki (by Niall Douglas originally)
// This allows for better code optimisation through PLT indirections. Also
// reduces the size and load times of DSO.

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_DLL
#ifdef __GNUC__
#define YAVE_PUBLIC __attribute__((dllexport))
#else
#define YAVE_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define YAVE_PUBLIC __attribute__((dllimport))
#else
#define YAVE_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#else
#if __GNUC__ >= 4
#define RPE_PUBLIC __attribute__((visibility("default")))
#else
#define RPE_PUBLIC
#endif
#endif

// Used mainly for silencing warnings.
#define RPE_UNUSED(var) (void)(var)

#ifdef __GNUC__
#define RPE_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define YAVE_FORCE_INLINE inline
#endif

#ifdef __GNUC__
#define RPE_PACKED __attribute__((packed))
#else
#define RPE_PACKED
#endif

#ifdef __GNUC__
#define AUTO __auto_type
#elif _WIN32
#define AUTO auto
#endif
