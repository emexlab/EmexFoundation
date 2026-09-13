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
#include <sys/mman.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFMapping.h>
#include <EmexFoundation/EFString.h>

struct __EFMapping {
    EFObject super;
    Boolean unmap;
    void *addr;
    EFSize size;
};

static EFStringRef __EFMappingCopyDebugDescription(EFObjectRef objectRef)
{
    EFMappingRef mapping = (EFMappingRef)objectRef;
    return EFStringCreateWithFormat(EFGetAllocator(objectRef), EFSTR("<EFMapping %p>{addr = %p, length = %llu}"), objectRef, mapping->addr, mapping->size);
}

static void __EFMappingDeinit(EFObjectRef objectRef)
{
    EFMappingRef mapping = (EFMappingRef)objectRef;
    if(mapping->unmap)
    {
        munmap(mapping->addr, mapping->size);
    }
}

EF_HIDDEN EFClassDefinitionNewest EFMappingClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDMapping,
        .name = EFSTR_FILESCOPE("EFMapping"),
    },
    .init = NULL,
    .deinit = __EFMappingDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = __EFMappingCopyDebugDescription,
};

EFTypeID EFMappingGetTypeID(void)
{
    return kEFTypeIDMapping;
}

EFMappingRef EFMappingCreate(EFAllocatorRef allocator,
                             void *addr,
                             EFSize size,
                             SInt32 prot,
                             SInt32 flags,
                             SInt32 fd,
                             EFOffset offset)
{
    EFAUTOREL EFMappingRef mapping = (EFMappingRef)EFObjectCreate(allocator, EFMappingGetTypeID(), (EFIndex)sizeof(struct __EFMapping));
    if(mapping == NULL)
    {
        return NULL;
    }

    mapping->addr = mmap(addr, size, prot, flags, fd, offset);
    if(mapping->addr == MAP_FAILED)
    {
        return NULL;
    }
    mapping->size = size;
    mapping->unmap = true;

    return EFAUTOTRANSFER(mapping);
}

void *EFMappingGetAddress(EFMappingRef mapping)
{
    if(mapping == NULL)
    {
        return NULL;
    }

    return mapping->addr;
}

EFSize EFMappingGetSize(EFMappingRef mapping)
{
    if(mapping == NULL)
    {
        return -1;
    }

    return mapping->size;
}

void EFMappingDisableUnmap(EFMappingRef mapping)
{
    if(mapping == NULL)
    {
        return;
    }

    mapping->unmap = false;
}
