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

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/EFString.h>

struct __EFData {
    EFObject super;
    Boolean isMutable;
    Boolean isInlined;    /* meaning the object has the buffer in it self */
    UInt8 *buffer;        /* it is neither inlined nor undeallocatable if mutable */
    EFIndex length;
};

static void __EFDataDeinit(EFObjectRef dataRef)
{
    EFDataRef data = (EFDataRef)dataRef;
    if(data->isMutable)
    {
        EFAllocatorDeallocate(EFGetAllocator(dataRef), data->buffer);
    }
}

static EFStringRef __EFDataCopyDebugDescription(EFObjectRef dataRef)
{
    EFDataRef data = (EFDataRef)dataRef;
    return EFStringCreateWithFormat(EFGetAllocator(dataRef), EFSTR("<%@ %p>{buffer = %p, length = %ld}"), data->isMutable ? EFSTR("EFMutableData") : EFSTR("EFData"), dataRef, data->buffer, data->length);
}

EF_HIDDEN EFClassDefinitionNewest EFDataClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDData,
        .name = EFSTR_FILESCOPE("EFData"),
    },
    .init = NULL,
    .deinit = __EFDataDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = __EFDataCopyDebugDescription,
};

EFTypeID EFDataGetTypeID(void)
{
    return kEFTypeIDData;
}

static inline EFDataRef __EFDataCreate(EFAllocatorRef allocator,
                                       const UInt8 *buffer,
                                       EFIndex length,
                                       Boolean isInlined,
                                       Boolean isMutable)
{
    if((buffer == NULL && !isMutable) || length < 0)
    {
        return NULL;
    }

    EFAUTOREL EFDataRef data = (EFDataRef)EFObjectCreate(allocator, EFDataGetTypeID(), (EFIndex)(sizeof(struct __EFData) + (isInlined ? length : 0)));
    if(data == NULL)
    {
        return NULL;
    }

    if(isMutable)
    {
        data->buffer = EFAllocatorAllocate(allocator, length, 0);
        if(data->buffer == NULL)
        {
            return NULL;
        }

        if(buffer != NULL)
        {
            goto needs_copy;
        }
        bzero(data->buffer, (EFSize)length);
    }
    else if(isInlined)
    {
        data->buffer = (UInt8*)((const char*)data + sizeof(struct __EFData));
needs_copy:
        memcpy(data->buffer, buffer, (EFSize)length);
    }
    else
    {
        data->buffer = (UInt8*)buffer;
    }

    /* always assigned with the same values */
    data->length = length;
    data->isInlined = !isMutable && isInlined;  /* isInlined is only possible when isMutable is not enabled */
    data->isMutable = isMutable;

    return EFAUTOTRANSFER(data);
}

static inline EFDataRef __EFDataCreateCopy(EFAllocatorRef allocator,
                                           EFDataRef data,
                                           Boolean isMutable)
{
    if(data == NULL)
    {
        return NULL;
    }

    if(allocator == NULL)
    {
        allocator = EFGetAllocator(data);
    }
    
    return __EFDataCreate(allocator, data->buffer, data->length, true, isMutable);
}

EFDataRef EFDataCreateWithBuffer(EFAllocatorRef allocator,
                                 const UInt8 *buffer,
                                 EFIndex length)
{
    if(buffer == NULL)
    {
        return NULL;
    }

    return __EFDataCreate(allocator, buffer, length, true, false);
}

EFDataRef EFDataCreateWithBufferNoCopy(EFAllocatorRef allocator,
                                       const UInt8 *buffer,
                                       EFIndex length)
{
    if(buffer == NULL)
    {
        return NULL;
    }

    return __EFDataCreate(allocator, buffer, length, false, false);
}

EFMutableDataRef EFDataCreateMutable(EFAllocatorRef allocator,
                                     EFIndex capacity)
{
    return __EFDataCreate(allocator, NULL, capacity, true, true);
}

EFDataRef EFDataCreateCopy(EFAllocatorRef allocator,
                           EFDataRef data)
{
    return __EFDataCreateCopy(allocator, data, false);
}

EFMutableDataRef EFDataCreateMutableCopy(EFAllocatorRef allocator,
                                         EFDataRef data)
{
    return __EFDataCreateCopy(allocator, data, true);
}

EFIndex EFDataGetLength(EFDataRef data)
{
    if(data == NULL)
    {
        return 0;
    }

    return data->length;
}

const UInt8 *EFDataGetPtr(EFDataRef data)
{
    if(data == NULL)
    {
        return NULL;
    }

    return data->buffer;
}

UInt8 *EFDataGetMutablePtr(EFMutableDataRef mutableData)
{
    if(mutableData == NULL || !mutableData->isMutable)
    {
        return NULL;
    }

    return mutableData->buffer;
}

Boolean EFDataCopyRangeToBuffer(EFDataRef data,
                                EFRange range,
                                UInt8 *buffer)
{
    if(data == NULL || data->length < range.location || data->length < (range.location + range.length))
    {
        return false;
    }

    memcpy(buffer, data->buffer + (EFSize)range.location, (EFSize)range.length);
    return true;
}

Boolean EFDataSetLength(EFMutableDataRef mutableData,
                        EFIndex length)
{
    if(mutableData == NULL || !mutableData->isMutable || length < 0)
    {
        return false;
    }

    if(mutableData->length == length)
    {
        return true;
    }

    void *newp = EFAllocatorReallocate(EFGetAllocator(mutableData), mutableData->buffer, length, 0);
    if(newp == NULL)
    {
        return false;
    }

    mutableData->buffer = newp;
    if(mutableData->length < length)
    {
        bzero(mutableData->buffer + (EFSize)mutableData->length, length - mutableData->length);
    }
    mutableData->length = length;
    return true;
}

Boolean EFDataIncreaseLength(EFMutableDataRef mutableData,
                             EFIndex extraLength)
{
    if(mutableData == NULL || !mutableData->isMutable || extraLength < 0)
    {
        return false;
    }

    EFIndex newLength = mutableData->length + extraLength;
    if(newLength < mutableData->length)
    {
        /* integer overflow! */
        return false;
    }

    return EFDataSetLength(mutableData, newLength);
}

Boolean EFDataAppendBuffer(EFMutableDataRef mutableData,
                           const UInt8 *buffer,
                           EFIndex length)
{
    if(mutableData == NULL || !mutableData->isMutable)
    {
        return false;
    }

    if(!EFDataIncreaseLength(mutableData, length))
    {
        return false;
    }

    UInt8 *ptr = mutableData->buffer + (EFSize)(mutableData->length - length);
    memcpy(ptr, buffer, (EFSize)length);

    return true;
}
