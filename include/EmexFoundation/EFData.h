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

#ifndef EFDATA_H
#define EFDATA_H

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stddef.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>

typedef struct __EFData *EFDataRef;
typedef struct __EFData *EFMutableDataRef;

EF_EXTERN EFTypeID EFDataGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFDataCreateWithBuffer(EFAllocatorRef allocatorRef, const UInt8 *buffer, EFIndex length);
EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFDataCreateWithBufferNoCopy(EFAllocatorRef allocatorRef, const UInt8 *buffer, EFIndex length);
EF_EXTERN EF_RETURNS_RETAINED EFMutableDataRef EFDataCreateMutable(EFAllocatorRef allocatorRef, EFIndex capacity);
EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFDataCreateCopy(EFAllocatorRef allocatorRef, EFDataRef dataRef);
EF_EXTERN EF_RETURNS_RETAINED EFMutableDataRef EFDataCreateMutableCopy(EFAllocatorRef allocatorRef, EFDataRef dataRef);

EF_EXTERN EFIndex EFDataGetLength(EFDataRef dataRef);
EF_EXTERN const UInt8 *EFDataGetPtr(EFDataRef dataRef);
EF_EXTERN UInt8 *EFDataGetMutablePtr(EFMutableDataRef mutableDataRef);
EF_EXTERN Boolean EFDataCopyRangeToBuffer(EFDataRef dataRef, EFRange range, UInt8 *buffer);

EF_EXTERN Boolean EFDataSetLength(EFMutableDataRef mutableDataRef, EFIndex length);
EF_EXTERN Boolean EFDataIncreaseLength(EFMutableDataRef mutableDataRef, EFIndex extraLength);
EF_EXTERN Boolean EFDataAppendBuffer(EFMutableDataRef mutableDataRef, const UInt8 *buffer, EFIndex length);
EF_EXTERN Boolean EFDataReplaceBufferInRange(EFMutableDataRef mutableDataRef, EFRange range, const UInt8 *newBytes, EFIndex newLength);
EF_EXTERN Boolean EFDataDeleteBufferInRange(EFMutableDataRef mutableDataRef, EFRange range);

#endif /* EFDATA_H */
