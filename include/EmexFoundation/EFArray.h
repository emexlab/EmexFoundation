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
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>

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

extern EFArrayCallbacks kEFArrayCallbacksDefaultCallbacks;
extern EFArrayCallbacks kEFArrayCallbacksObjectCallbacks;

EFTypeID EFArrayGetTypeID(void);

EFArrayRef EFArrayCreate(EFAllocatorRef allocatorRef, EFArrayCallbacks callbacks, void **values, EFIndex valuesCount);
EFMutableArrayRef EFArrayCreateMutable(EFAllocatorRef allocatorRef, EFArrayCallbacks callbacks, EFIndex capacity);
EFMutableArrayRef EFArrayCreateMutableCopy(EFAllocatorRef allocatorRef, EFArrayRef arrayRef);
EFArrayRef EFArrayCreateCopy(EFAllocatorRef allocatorRef, EFArrayRef arrayRef);

EFIndex EFArrayGetCount(EFArrayRef arrayRef);
void *EFArrayGetValueAtIndex(EFArrayRef arrayRef, EFIndex index);

Boolean EFArrayAppendValue(EFMutableArrayRef arrayRef, void *value);

Boolean EFArrayInsertValueAtIndex(EFMutableArrayRef arrayRef, EFIndex index, void *ptr);
void EFArrayRemoveValueAtIndex(EFMutableArrayRef arrayRef, EFIndex index);

#endif /* EFARRAY_H */
