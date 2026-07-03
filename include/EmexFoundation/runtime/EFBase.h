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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>

#define kEFNotATypeID   ((uint64_t)0)
#define EF_MAX_CLASSES  1024

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

typedef struct {
    /* properties  */
    const char *name;
    EFTypeID typeID;

    /* handlers */
    evobject_init_handler_t init;
    evobject_deinit_handler_t deinit;
    evobject_equal_handler_t equal;
    evobject_copy_description_handler_t copyDescription;
} EFClass;

typedef struct EFAllocator {
    /* properties  */
    const char *name;
    void *info; /* a more complex allocator in the future will need this */

    /* handlers */
    evallocator_alloc_handler_t allocate;
    evallocator_dealloc_handler_t deallocate;
} EFAllocator;

typedef struct {
    /*
     * the typeID of the class of that
     * object, similar to CFRuntime.
     */
    EFTypeID typeID;

    /* self explainatory */
    EFAllocatorRef allocatorRef;

    /* is not allocated by a allocator for example */
    Boolean isStatic;

    /*
     * reference count of an object if
     * it hits zero it will free
     * automatically.
     */
    _Atomic EFIndex refcount;
} EFObject;

EFTypeID EFGetTypeID(EFObjectRef ref);
Boolean EFEqual(EFObjectRef ref1, EFObjectRef ref2);

EFObjectRef EFRetain(EFObjectRef ref);
void EFRelease(EFObjectRef ref);
EFIndex EFGetRetainCount(EFObjectRef ref);

EFTypeID EFClassRegister(EFClass *cls);
EFClass *EFClassGetByID(EFTypeID id);

EFAllocatorRef EFGetAllocator(EFObjectRef ref);

EFStringRef EFCopyDescription(EFObjectRef ref);

void EFLog(EFStringRef formatStringRef, ...);

#endif /* EFBASE_H */
