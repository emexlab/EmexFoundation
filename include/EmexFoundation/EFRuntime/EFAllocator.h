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

#ifndef EFALLOCATOR_H
#define EFALLOCATOR_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFBase.h>

EF_EXTERN EFAllocatorRef kEFAllocatorDefault;
EF_EXTERN EFAllocatorRef kEFAllocatorMalloc;

typedef struct EFAllocator {
    EFRootType _rt;

    /* properties  */
    const char *name;
    void *info; /* a more complex allocator in the future will need this */

    /* handlers */
    EFAllocatorAllocateCallback allocate;
    EFAllocatorDeallocateCallback deallocate;
    EFAllocatorReallocateCallback reallocate;
} EFAllocator;

EF_EXTERN EF_RETURNS_RETAINED EFObjectRef EFObjectCreate(EFAllocatorRef allocatorRef, EFTypeID typeID, EFIndex size);

EF_EXTERN void *EFAllocatorAllocate(EFAllocatorRef allocatorRef, EFIndex size, EFOptionFlags hint);
EF_EXTERN void *EFAllocatorReallocate(EFAllocatorRef allocatorRef, void *ptr, EFIndex newSize, EFOptionFlags hint);
EF_EXTERN void EFAllocatorDeallocate(EFAllocatorRef allocatorRef, void *ptr);

#endif /* EFALLOCATOR_H */
