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

static void *__EFAllocatorDefaultAllocate(EFAllocatorRef allocatorRef,
                                          EFIndex size,
                                          EFOptionFlags hint)
{
    return calloc(1, (size_t)size);
}

static void *__EFAllocatorDefaultReallocate(EFAllocatorRef allocatorRef,
                                            void *ptr,
                                            EFIndex newSize,
                                            EFOptionFlags hint)
{
    return realloc(ptr, (size_t)newSize);
}

static void __EFAllocatorDefaultDeallocate(EFAllocatorRef allocatorRef,
                                           void *ptr)
{
    free(ptr);
}

EFAllocatorRef kEFAllocatorMalloc = (EFAllocatorRef)&(EFAllocator){
    ._rt = kEFRootTypeAllocator,
    .name = "EFAllocatorMalloc",
    .info = NULL,

    .allocate = __EFAllocatorDefaultAllocate,
    .reallocate = __EFAllocatorDefaultReallocate,
    .deallocate = __EFAllocatorDefaultDeallocate,
};

EFAllocatorRef kEFAllocatorDefault = NULL;

EFObjectRef EFObjectCreate(EFAllocatorRef allocatorRef,
                           EFTypeID typeID,
                           EFIndex size)
{
    if(allocatorRef == NULL)
    {
        allocatorRef = kEFAllocatorDefault;
    }

    /*
     * gotta need the class for the typeid
     * and the init handler.
     */
    EFClass *class = EFClassGetByID(typeID);

    /* validating class and passed size */
    assert(class != NULL && (size_t)size >= sizeof(EFObject));

    EFObject *object = EFAllocatorAllocate(allocatorRef, size, 0);
    if(object == NULL)
    {
        return NULL;
    }

    object->_rt = kEFRootTypeObject;
    object->refcount = 1;
    object->typeID = class->typeID;
    object->allocatorRef = allocatorRef;

    /* initializing when applicable */
    if(class->init != NULL)
    {
        class->init(object);
    }

    return (EFObjectRef)object;
}

extern void *EFAllocatorAllocate(EFAllocatorRef allocatorRef,
                                 EFIndex size,
                                 EFOptionFlags hint)
{
    EFAllocator *allocator = (EFAllocator*)(allocatorRef?: kEFAllocatorDefault);
    assert(allocator->allocate != NULL);
    return allocator->allocate(allocatorRef, size, hint);
}

extern void *EFAllocatorReallocate(EFAllocatorRef allocatorRef,
                                   void *ptr,
                                   EFIndex newSize,
                                   EFOptionFlags hint)
{
    EFAllocator *allocator = (EFAllocator*)(allocatorRef?: kEFAllocatorDefault);
    assert(allocator->reallocate != NULL);
    return allocator->reallocate(allocatorRef, ptr, newSize, hint);
}

extern void EFAllocatorDeallocate(EFAllocatorRef allocatorRef,
                                  void *ptr)
{
    EFAllocator *allocator = (EFAllocator*)(allocatorRef?: kEFAllocatorDefault);
    assert(allocator->deallocate != NULL);
    allocator->deallocate(allocatorRef, ptr);
}

__attribute__((constructor(101)))
static void EFAllocatorConstructor(void)
{
    /* default is malloc */
    kEFAllocatorDefault = kEFAllocatorMalloc;
}
