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

#ifndef EFBASE_H
#define EFBASE_H

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>

#define kEFNotATypeID   ((uint64_t)0)

/* normal types */
typedef unsigned long EFOptionFlags;
typedef unsigned long EFHashCode;
typedef long EFIndex;
typedef struct {
    EFIndex location;
    EFIndex length;
} EFRange;
typedef unsigned char Boolean;
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned int UInt32;
typedef signed int SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;
typedef float Float32;
typedef double Float64;
typedef double EFTimeInterval;
typedef EFTimeInterval EFAbsoluteTime;
/* no OSStatus??? this is modern, aint macintosh carbon era shit, my water aint carbonised */

/* special types */
typedef unsigned long EFTypeID;
typedef void *EFObjectRef;      /* so the compiler shuts up */
typedef void *EFAllocatorRef;

typedef struct __EFString *EFStringRef;

typedef void (*evobject_init_handler_t)(EFObjectRef ref);
typedef void (*evobject_deinit_handler_t)(EFObjectRef ref);
typedef EFObjectRef (*evobject_copy_handler_t)(EFObjectRef ref);
typedef Boolean (*evobject_equal_handler_t)(EFObjectRef ref1, EFObjectRef ref2);
typedef EFStringRef (*evobject_copy_description_handler_t)(EFObjectRef ref);

typedef EFObjectRef (*evallocator_alloc_handler_t)(EFAllocatorRef allocatorRef, EFTypeID typeID, size_t size);
typedef void (*evallocator_dealloc_handler_t)(EFAllocatorRef allocatorRef, EFObjectRef ref);

static inline EFRange EFRangeMake(EFIndex location,
                                  EFIndex length)
{
    return (EFRange){
        .location = location,
        .length = length,
    };
}

#endif /* EFBASE_H */
