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
#include <assert.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFMallocBlock.h>
#include <EmexFoundation/EFString.h>

struct __EFMallocBlock {
    EFObject super;
    EFSize size;
    EFObjectDeinitCallback deinitCallback;
};

static void __EFMallocBlockDeinit(EFObjectRef blockRef)
{
    EFMallocBlockRef block = (EFMallocBlockRef)blockRef;
    if(block->deinitCallback != NULL)
    {
        block->deinitCallback(blockRef);
    }
}

EF_HIDDEN EFClassDefinitionNewest EFMallocBlockClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDMallocBlock,
        .name = EFSTR_FILESCOPE("EFMallocBlock"),
    },
    .init = NULL,
    .deinit = __EFMallocBlockDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

EFTypeID EFMallocBlockGetTypeID(void)
{
    return kEFTypeIDMallocBlock;
}

EFMallocBlockRef EFMallocBlockCreate(EFAllocatorRef allocator,
                                     EFSize size)
{
    return EFMallocBlockCreateWithDeinitHandler(allocator, size, NULL);
}

EFMallocBlockRef EFMallocBlockCreateWithDeinitHandler(EFAllocatorRef allocator,
                                                      EFSize size,
                                                      EFObjectDeinitCallback deinitCallback)
{
    assert(size > sizeof(struct __EFMallocBlock));

    EFMallocBlockRef block = (EFMallocBlockRef)EFObjectCreate(allocator, EFMallocBlockGetTypeID(), size);
    if(block == NULL)
    {
        return NULL;
    }

    block->size = size;
    block->deinitCallback = deinitCallback;

    return block;
}

EFSize EFMallocBlockGetSize(EFMallocBlockRef block)
{
    if(block == NULL)
    {
        return 0;
    }

    return block->size;
}
