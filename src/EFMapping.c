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
#include <pthread.h>
#include <sys/mman.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFMapping.h>
#include <EmexFoundation/EFString.h>

typedef struct __EFMapping {
    EFObject header;
    Boolean unmap;
    void *addr;
    EFIndex length;
} *__EFMapping;

static EFStringRef __EFMappingCopyDescription(EFObjectRef objectRef)
{
    EFMappingRef mapping = (EFMappingRef)objectRef;
    return EFStringCreateWithFormat(EFGetAllocator(objectRef), EFSTR("<EFMapping %p>{addr = %p, length = %lu}"), objectRef, mapping->addr, mapping->length);
}

static void __EFMappingDeinit(EFObjectRef objectRef)
{
    EFMappingRef mapping = (EFMappingRef)objectRef;
    if(mapping->unmap)
    {
        munmap(mapping->addr, mapping->length);
    }
}

EFClass EFMappingClass = {
    .name = "EFMapping",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFMappingDeinit,
    .equal = NULL,
    .copyDescription = __EFMappingCopyDescription,
    .hash = NULL,
};

void EFMappingRegisterClass(void)
{
    EFClassRegister(&EFMappingClass);
}

EFTypeID EFMappingGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFMappingRegisterClass);
    return EFMappingClass.typeID;
}

EFMappingRef EFMappingCreate(EFAllocatorRef allocatorRef,
                             void *addr,
                             size_t length,
                             int prot,
                             int flags,
                             int fd,
                             off_t offset)
{
    EFAUTOREL __EFMapping mapping = (__EFMapping)EFObjectCreate(allocatorRef, EFMappingGetTypeID(), (EFIndex)sizeof(struct __EFMapping));
    if(mapping == NULL)
    {
        return NULL;
    }

    mapping->addr = mmap(addr, length, prot, flags, fd, offset);
    if(mapping->addr == MAP_FAILED)
    {
        return NULL;
    }
    mapping->length = (EFIndex)length;
    mapping->unmap = true;

    return (EFMappingRef)EFAUTOTRANSFER(mapping);
}

void *EFMappingGetAddress(EFMappingRef mappingRef)
{
    __EFMapping mapping = (__EFMapping)mappingRef;
    if(mapping == NULL)
    {
        return NULL;
    }

    return (EFMappingRef)mapping->addr;
}

EFIndex EFMappingGetLength(EFMappingRef mappingRef)
{
    __EFMapping mapping = (__EFMapping)mappingRef;
    if(mapping == NULL)
    {
        return -1;
    }

    return mapping->length;
}

void EFMappingDisableUnmap(EFMappingRef mappingRef)
{
    __EFMapping mapping = (__EFMapping)mappingRef;
    if(mapping == NULL)
    {
        return;
    }

    mapping->unmap = false;
}
