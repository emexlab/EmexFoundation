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

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>

static EFObjectRef __EFAllocatorDefaultAllocate(EFAllocatorRef allocatorRef,
                                                EFTypeID typeID,
                                                size_t size)
{
    /*
     * gotta need the class for the typeid
     * and the init handler.
     */
    EFClass *class = EFClassGetByID(typeID);

    /* validating class and passed size */
    assert(class != NULL && size >= sizeof(EFObject));

    EFObject *object = calloc(1, size);
    if(object == NULL)
    {
        return NULL;
    }
    object->_rt = kEFRootTypeObject;
    object->refcount = 1;
    object->typeID = class->typeID;
    object->allocatorRef = allocatorRef;

    /* initilizing when applicable */
    if(class->init != NULL)
    {
        class->init(object);
    }

    return (EFObjectRef)object;
}

static void __EFAllocatorDefaultDeallocate(EFAllocatorRef allocatorRef,
                                           EFObjectRef ref)
{
    free(ref);
}

EFAllocatorRef kEFAllocatorMalloc = (EFAllocatorRef)&(EFAllocator){
    ._rt = kEFRootTypeAllocator,
    .name = "EFAllocatorMalloc",
    .info = NULL,

    .allocate = __EFAllocatorDefaultAllocate,
    .deallocate = __EFAllocatorDefaultDeallocate,
};

EFAllocatorRef kEFAllocatorDefault = NULL;

EFObjectRef EFObjectAlloc(EFAllocatorRef allocatorRef,
                          EFTypeID typeID,
                          size_t size)
{
    if(allocatorRef == NULL)
    {
        allocatorRef = kEFAllocatorDefault;
    }

    /* checking if allocator is correctly configured (it must) */
    EFAllocator *allocator = (EFAllocator*)allocatorRef;
    assert(allocator->allocate != NULL && allocator->deallocate != NULL);

    return allocator->allocate(allocator, typeID, size);
}

void EFObjectDealloc(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    ((EFAllocator*)(object->allocatorRef))->deallocate(object->allocatorRef, object);
}

__attribute__((constructor(101)))
static void EFAllocatorConstructor(void)
{
    /* default is malloc */
    kEFAllocatorDefault = kEFAllocatorMalloc;
}
