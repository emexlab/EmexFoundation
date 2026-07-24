/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of EmexFoundation.
 *
 * EmexFoundation is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EmexFoundation is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with EmexFoundation. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EFRUNTIME_H
#define EFRUNTIME_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFEndian.h>
#include <EmexFoundation/EFRuntime/EFBase.h>
#include <EmexFoundation/EFRuntime/EFAllocator.h>
#include <EmexFoundation/EFRuntime/EFClass.h>
#include <EmexFoundation/EFRuntime/EFObject.h>

/* automated object holding (less powerful than ARC, but very powerful) */
#define EFAUTOREL __attribute__((cleanup(EFReleaseTryHelper)))
#define EFAUTOTRANSFER(var) \
    __extension__ ({ \
        __typeof__(var) _temp = (var); \
        (var) = NULL; \
        _temp; \
    })
#define EFAUTOSWAP(var, newvar) \
    __extension__ ({ \
        EFReleaseTry((var)); \
        (var) = (newvar); \
    })

/* cross platform deprecation management */
#if defined(__GNUC__) || defined(__clang__)
#define EFDEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define EFDEPRECATED(msg) __declspec(deprecated(msg))
#else
#define EFDEPRECATED(msg)
#endif /* __GNUC__ || __clang__ */

#if defined(__GNUC__) || defined(__clang__)
#define EFSUPPRESS_DEPRECATED_START _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define EFSUPPRESS_DEPRECATED_END _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define EFSUPPRESS_DEPRECATED_START __pragma(warning(push)) __pragma(warning(disable: 4996))
#define EFSUPPRESS_DEPRECATED_END __pragma(warning(pop))
#else
#define EFSUPPRESS_DEPRECATED_START
#define EFSUPPRESS_DEPRECATED_END
#endif /* __GNUC__ || __clang__ */

EF_EXTERN EFTypeID EFClassRegister(EFClass *cls);
EF_EXTERN EFClass *EFClassGetByID(EFTypeID id);

EF_EXTERN EFTypeID EFGetTypeID(EFObjectRef ref);
EF_EXTERN EFRootType EFGetRootType(EFObjectRef ref);

EF_EXTERN Boolean EFEqual(EFObjectRef ref1, EFObjectRef ref2);

EF_EXTERN EF_RETURNS_RETAINED EFObjectRef EFRetain(EFObjectRef ref);
EF_EXTERN void EFRelease(EF_CONSUMED EFObjectRef ref);
EF_EXTERN EF_RETURNS_RETAINED EFObjectRef EFRetainTry(EFObjectRef ref);
EF_EXTERN Boolean EFReleaseTry(EF_CONSUMED EFObjectRef ref);
EF_EXTERN Boolean EFReleaseTryHelper(void *ref);    /* technically consumes */
EF_EXTERN EFIndex EFGetRetainCount(EFObjectRef ref);

EF_EXTERN EFAllocatorRef EFGetAllocator(EFObjectRef ref);

EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFCopyDescription(EFObjectRef ref);

EF_EXTERN void EFLog(EFStringRef formatStringRef, ...);

#endif /* EFRUNTIME_H */
