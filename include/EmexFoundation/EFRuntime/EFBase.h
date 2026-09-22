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

#ifndef UINT8_MIN
#define UINT8_MIN   0
#endif /* UINT8_MIN*/
#ifndef UINT8_MAX
#define UINT8_MAX   0xFF
#endif /* UINT8_MAX */
#ifndef UINT16_MIN
#define UINT16_MIN  0
#endif /* UINT16_MIN */
#ifndef UINT16_MAX
#define UINT16_MAX  0xFFFF
#endif /* UINT16_MAX */
#ifndef UINT32_MIN
#define UINT32_MIN  0
#endif /* UINT32_MIN */
#ifndef UINT32_MAX
#define UINT32_MAX  0xFFFFFFFF
#endif /* UINT32_MAX */
#ifndef UINT64_MIN
#define UINT64_MIN  0
#endif /* UINT64_MIN */
#ifndef UINT64_MAX
#define UINT64_MAX  0xFFFFFFFFFFFFFFFF
#endif /* UINT64_MAX */
#ifndef SINT8_MIN
#define SINT8_MIN   0x80
#endif /* SINT8_MIN */
#ifndef SINT8_MAX
#define SINT8_MAX   0x7F
#endif /* SINT8_MAX */
#ifndef SINT16_MIN
#define SINT16_MIN  0x8000
#endif /* SINT16_MIN */
#ifndef SINT16_MAX
#define SINT16_MAX  0x7FFF
#endif /* SINT16_MAX */
#ifndef SINT32_MIN
#define SINT32_MIN  0x80000000
#endif /* SINT32_MIN */
#ifndef SINT32_MAX
#define SINT32_MAX  0x7FFFFFFF
#endif /* SINT32_MIN */
#ifndef SINT64_MIN
#define SINT64_MIN  0x8000000000000000LL
#endif /* SINT64_MIN */
#ifndef SINT64_MAX
#define SINT64_MAX  0x7FFFFFFFFFFFFFFFLL
#endif /* SINT64_MAX */

#if defined(__GNUC__) || defined(__clang__)
#define EF_EXTERN __attribute__((visibility("default"))) extern
#define EF_HIDDEN __attribute__((visibility("hidden")))
#else
#define EF_EXTERN
#define EF_HIDDEN
#endif /* __GNUC__ || __clang__ */

/* later for arc in my own objc runtime */
#define EF_RETURNS_RETAINED
#define EF_RETURNS_NOT_RETAINED
#define EF_CONSUMED

/* normal types */
typedef unsigned long EFOptionFlags;
typedef unsigned long EFHashCode;
typedef long EFIndex;
typedef struct {
    EFIndex location;
    EFIndex length;
} EFRange;
typedef struct {
    EFIndex line;
    EFIndex column;
} EFStringLocation;
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
typedef UInt16 UniChar;
typedef double EFTimeInterval;
typedef EFTimeInterval EFAbsoluteTime;
/* no OSStatus??? this is modern, aint macintosh carbon era shit, my water aint carbonised */

typedef uintptr_t EFAddr;
typedef size_t EFSize;
typedef EFIndex EFOffset;

typedef __uint128_t EFUInt128;

typedef enum: UInt8 {
    kEFComparisonResultLessThan,
    kEFComparisonResultEqualTo,
    kEFComparisonResultGreaterThan,
} EFComparisonResult;

/* special types */
typedef enum: UInt8 {
    kEFRootTypeNotARootType,
    kEFRootTypeAllocator,
    kEFRootTypeObject,
    kEFRootTypeStaticObject,
} EFRootType;

typedef enum: unsigned long {
    kEFTypeIDNone =                     0,
    kEFTypeIDString =                   1,
    kEFTypeIDNumber =                   2,
    kEFTypeIDURL =                      3,
    kEFTypeIDUUID =                     4,
    kEFTypeIDData =                     5,
    kEFTypeIDFileHandle =               6,
    kEFTypeIDFile =                     7,
    kEFTypeIDBitWalker =                8,
    kEFTypeIDMapping =                  9,
    kEFTypeIDProcess =                  10,
    kEFTypeIDMallocBlock =              11,
    kEFTypeIDArray =                    12,
    kEFTypeIDDictionary =               13, /* unimplemented */
    kEFTypeIDFileManager =              14,
} EFTypeID;

typedef void *EFObjectRef;      /* so the compiler shuts up */
typedef void *EFAllocatorRef;

typedef struct __EFNumber *EFNumberRef;
typedef struct __EFString *EFStringRef;
typedef struct __EFString *EFMutableStringRef;
typedef struct __EFArray *EFArrayRef;
typedef struct __EFArray *EFMutableArrayRef;
typedef struct __EFDictionary *EFDictionaryRef;
typedef struct __EFDictionary *EFMutableDictionaryRef;
typedef struct __EFBitWalker *EFBitWalkerRef;
typedef struct __EFData *EFDataRef;
typedef struct __EFData *EFMutableDataRef;
typedef struct __EFFile *EFFileRef;
typedef struct __EFFileHandle *EFFileHandleRef;
typedef struct __EFMapping *EFMappingRef;
typedef struct __EFProcess *EFProcessRef;
typedef struct __EFURL *EFURLRef;
typedef struct __EFUUID *EFUUIDRef;
typedef struct __EFMallocBlock *EFMallocBlockRef;
typedef struct __EFFileManager *EFFileManagerRef;

typedef void (*EFObjectInitCallback)(EFObjectRef ref);
typedef void (*EFObjectDeinitCallback)(EFObjectRef ref);
typedef Boolean (*EFObjectEqualCallback)(EFObjectRef ref1, EFObjectRef ref2);
typedef EFStringRef (*EFObjectCopyDescriptionCallback)(EFObjectRef ref);
typedef EFHashCode (*EFObjectHashCallback)(EFObjectRef ref);

typedef void *(*EFAllocatorAllocateCallback)(EFAllocatorRef allocatorRef, EFIndex size, EFOptionFlags hint);
typedef void *(*EFAllocatorReallocateCallback)(EFAllocatorRef allocatorRef, void *ptr, EFIndex newSize, EFOptionFlags hint);
typedef void (*EFAllocatorDeallocateCallback)(EFAllocatorRef allocatorRef, void *ptr);

static inline EFRange EFRangeMake(EFIndex location,
                                  EFIndex length)
{
    return (EFRange){ .location = location, .length = length, };
}

static inline Boolean EFRangeIsEqual(EFRange range1,
                                     EFRange range2)
{
    return (range1.location == range2.location && range1.length == range2.length);
}

static inline EFStringLocation EFStringLocationMake(EFIndex line,
                                                    EFIndex column)
{
    return (EFStringLocation){ .line = line, .column = column };
}

static inline Boolean EFStringLocationIsEqual(EFStringLocation loc1,
                                              EFStringLocation loc2)
{
    return (loc1.line == loc2.line && loc1.column == loc2.column);
}

EF_EXTERN EFRange EFRangeZero;
EF_EXTERN EFStringLocation EFStringLocationZero;

#endif /* EFBASE_H */
