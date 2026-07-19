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

#ifndef EFARRAY_H
#define EFARRAY_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>

typedef struct __EFArray *EFArrayRef;
typedef struct __EFArray *EFMutableArrayRef;

typedef Boolean (*evarray_append_callback)(void *ptr);
typedef void (*evarray_remove_callback)(void *ptr);
typedef Boolean (*evarray_equal_callback)(void *ptr1, void *ptr2);
typedef EFStringRef (*evarray_copyDescription_callback)(EFAllocatorRef allocatorRef, void *ptr);

typedef struct EFArrayCallbacks {
    evarray_append_callback append;
    evarray_remove_callback remove;
    evarray_equal_callback equal;
    evarray_copyDescription_callback copyDescription;
} *EFArrayCallbacks;

EF_EXTERN EFArrayCallbacks kEFArrayCallbacksDefaultCallbacks;
EF_EXTERN EFArrayCallbacks kEFArrayCallbacksObjectCallbacks;

EF_EXTERN EFTypeID EFArrayGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFArrayCreate(EFAllocatorRef allocatorRef, EFArrayCallbacks callbacks, void **values, EFIndex valuesCount);
EF_EXTERN EF_RETURNS_RETAINED EFMutableArrayRef EFArrayCreateMutable(EFAllocatorRef allocatorRef, EFArrayCallbacks callbacks, EFIndex capacity);
EF_EXTERN EF_RETURNS_RETAINED EFMutableArrayRef EFArrayCreateMutableCopy(EFAllocatorRef allocatorRef, EFArrayRef arrayRef);
EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFArrayCreateCopy(EFAllocatorRef allocatorRef, EFArrayRef arrayRef);

EF_EXTERN EFIndex EFArrayGetCount(EFArrayRef arrayRef);
EF_EXTERN EF_RETURNS_NOT_RETAINED void *EFArrayGetValueAtIndex(EFArrayRef arrayRef, EFIndex index);

EF_EXTERN Boolean EFArrayAppendValue(EFMutableArrayRef mutableArrayRef, void *value);
EF_EXTERN Boolean EFArrayInsertValueAtIndex(EFMutableArrayRef mutableArrayRef, EFIndex index, void *ptr);
EF_EXTERN void EFArrayRemoveValueAtIndex(EFMutableArrayRef mutableArrayRef, EFIndex index);

EF_EXTERN Boolean EFArrayAppendValuesOfArray(EFMutableArrayRef mutableArrayRef, EFArrayRef otherArrayRef);
EF_EXTERN Boolean EFArrayInsertValuesOfArrayAtIndex(EFMutableArrayRef mutableArrayRef, EFIndex index, EFArrayRef otherArrayRef);    /* unimplemented */
EF_EXTERN Boolean EFArrayRemoveValuesInRange(EFMutableArrayRef mutableArrayRef, EFRange range);                                     /* unimplemented */

#endif /* EFARRAY_H */
