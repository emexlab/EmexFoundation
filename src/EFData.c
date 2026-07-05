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
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/EFString.h>

typedef struct __EFData {
    EFObject header;
    Boolean isMutable;
    Boolean isInlined;    /* meaning the object has the buffer in it self */
    UInt8 *buffer;       /* it is neither inlined nor undeallocatable if mutable */
    EFIndex length;
} *__EFData;

static void __EFDataDeinit(EFObjectRef dataRef)
{
    __EFData data = (__EFData)dataRef;
    if(data->isMutable)
    {
        free(data->buffer);
    }
}

static EFStringRef __EFDataCopyDescription(EFObjectRef dataRef)
{
    __EFData data = (__EFData)dataRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(dataRef);
    EFMutableStringRef mutableStringRef = EFStringCreateMutableCopy(allocatorRef, EF_STR("<"));
    if(mutableStringRef == NULL)
    {
        return NULL;
    }

    if(!EFStringAppendString(mutableStringRef, data->isMutable ? EF_STR("EFMutableData") : EF_STR("EFData")))
    {
        EFRelease(mutableStringRef);
        return NULL;
    }

    if(!EFStringAppendFormat(mutableStringRef, EF_STR(" %p>{buffer = %p, length = %ld}"), dataRef, data->buffer, data->length))
    {
        EFRelease(mutableStringRef);
        return NULL;
    }

    return mutableStringRef;
}

static EFClass EFDataClass = {
    .name = "EFData",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFDataDeinit,
    .equal = NULL,
    .copyDescription = __EFDataCopyDescription,
};

static void EFDataRegisterClass(void)
{
    EFClassRegister(&EFDataClass);
}

EFTypeID EFDataGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFDataRegisterClass);
    return EFDataClass.typeID;
}

static inline EFDataRef __EFDataCreate(EFAllocatorRef allocatorRef,
                                       const UInt8 *buffer,
                                       EFIndex length,
                                       Boolean isInlined,
                                       Boolean isMutable)
{
    if((buffer == NULL && !isMutable) || length < 0)
    {
        return NULL;
    }

    __EFData data = (__EFData)EFObjectAlloc(allocatorRef, EFDataGetTypeID(), sizeof(struct __EFData) + (isInlined ? length : 0));
    if(data == NULL)
    {
        return NULL;
    }

    if(isMutable)
    {
        data->buffer = malloc((size_t)length);
        if(data->buffer == NULL)
        {
            EFRelease((EFDataRef)data);
            return NULL;
        }

        if(buffer != NULL)
        {
            goto needs_copy;
        }
        bzero(data->buffer, (size_t)length);
    }
    else if(isInlined)
    {
        data->buffer = (UInt8*)((const char*)data + sizeof(struct __EFData));
needs_copy:
        memcpy(data->buffer, buffer, (size_t)length);
    }
    else
    {
        data->buffer = (UInt8*)buffer;
    }

    /* always assigned with the same values */
    data->length = length;
    data->isInlined = !isMutable && isInlined; /* isInlined is only possible when isMutable is not enabled */
    data->isMutable = isMutable;

    return (EFDataRef)data;
}

static inline EFDataRef __EFDataCreateCopy(EFAllocatorRef allocatorRef,
                                           EFDataRef dataRef,
                                           Boolean isMutable)
{
    __EFData data = (__EFData)dataRef;
    if(data == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EFGetAllocator(dataRef);
    }
    
    return __EFDataCreate(allocatorRef, data->buffer, data->length, true, isMutable);
}

EFDataRef EFDataCreateWithBuffer(EFAllocatorRef allocatorRef,
                                 const UInt8 *buffer,
                                 EFIndex length)
{
    if(buffer == NULL)
    {
        return NULL;
    }

    return (EFDataRef)__EFDataCreate(allocatorRef, buffer, length, true, false);
}

EFDataRef EFDataCreateWithBufferNoCopy(EFAllocatorRef allocatorRef,
                                       const UInt8 *buffer,
                                       EFIndex length)
{
    if(buffer == NULL)
    {
        return NULL;
    }

    return (EFDataRef)__EFDataCreate(allocatorRef, buffer, length, false, false);
}

EFMutableDataRef EFDataCreateMutable(EFAllocatorRef allocatorRef,
                                     EFIndex capacity)
{
    return __EFDataCreate(allocatorRef, NULL, capacity, true, true);
}

EFDataRef EFDataCreateCopy(EFAllocatorRef allocatorRef,
                           EFDataRef dataRef)
{
    return __EFDataCreateCopy(allocatorRef, dataRef, false);
}

EFMutableDataRef EFDataCreateMutableCopy(EFAllocatorRef allocatorRef,
                                         EFDataRef dataRef)
{
    return __EFDataCreateCopy(allocatorRef, dataRef, true);
}

EFIndex EFDataGetLength(EFDataRef dataRef)
{
    __EFData data = (__EFData)dataRef;
    if(data == NULL)
    {
        return 0;
    }

    return data->length;
}

const UInt8 *EFDataGetPtr(EFDataRef dataRef)
{
    __EFData data = (__EFData)dataRef;
    if(data == NULL)
    {
        return NULL;
    }

    return data->buffer;
}

UInt8 *EFDataGetMutablePtr(EFMutableDataRef mutableDataRef)
{
    __EFData mutableData = (__EFData)mutableDataRef;
    if(mutableData == NULL || !mutableData->isMutable)
    {
        return NULL;
    }

    return mutableData->buffer;
}

Boolean EFDataCopyRangeToBuffer(EFDataRef dataRef,
                                EFRange range,
                                UInt8 *buffer)
{
    __EFData data = (__EFData)dataRef;
    if(data == NULL || data->length < range.location || data->length < (range.location + range.length))
    {
        return false;
    }

    memcpy(buffer, data->buffer + (size_t)range.location, (size_t)range.length);
    return true;
}

Boolean EFDataSetLength(EFMutableDataRef mutableDataRef,
                        EFIndex length)
{
    __EFData mutableData = (__EFData)mutableDataRef;
    if(mutableData == NULL || !mutableData->isMutable || length < 0)
    {
        return false;
    }

    if(mutableData->length == length)
    {
        return true;
    }

    void *newp = realloc(mutableData->buffer, (size_t)length);
    if(newp == NULL)
    {
        return false;
    }

    mutableData->buffer = newp;
    if(mutableData->length < length)
    {
        bzero(mutableData->buffer + (size_t)mutableData->length, length - mutableData->length);
    }
    mutableData->length = length;
    return true;
}

Boolean EFDataIncreaseLength(EFMutableDataRef mutableDataRef,
                             EFIndex extraLength)
{
    __EFData mutableData = (__EFData)mutableDataRef;
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

    return EFDataSetLength(mutableDataRef, newLength);
}

Boolean EFDataAppendBuffer(EFMutableDataRef mutableDataRef,
                           const UInt8 *buffer,
                           EFIndex length)
{
    __EFData mutableData = (__EFData)mutableDataRef;
    if(mutableData == NULL || !mutableData->isMutable)
    {
        return false;
    }

    if(!EFDataIncreaseLength(mutableDataRef, length))
    {
        return false;
    }

    UInt8 *ptr = mutableData->buffer + (size_t)(mutableData->length - length);
    memcpy(ptr, buffer, (size_t)length);

    return true;
}

Boolean EFDataReplaceBufferInRange(EFMutableDataRef mutableDataRef,
                                   EFRange range,
                                   const UInt8 *newBytes,
                                   EFIndex newLength)
{
    return false;
}

Boolean EFDataDeleteBufferInRange(EFMutableDataRef mutableDataRef,
                                  EFRange range)
{
    return false;
}
