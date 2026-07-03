/*
 * MIT License
 *
 * Copyright (c) 2026 emexLabs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EFENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>

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

static EFClass EFDataClass = {
    .name = "EFData",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFDataDeinit,
    .equal = NULL,
    .copyDescription = NULL,
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

    __EFData data = (__EFData)EFObjectAlloc(allocatorRef, EFDataGetTypeID(), sizeof(struct __EFData) + (isInlined ? length + 1 : 0));
    if(data == NULL)
    {
        return NULL;
    }

    if(isMutable)
    {
        data->buffer = malloc((size_t)length);
        bzero(data->buffer, (size_t)length);
        isInlined = false; /* must be false */
    }
    else if(isInlined)
    {
        data->buffer = (UInt8*)((const char*)data + sizeof(struct __EFData));
        memcpy(data->buffer, buffer, (size_t)length);
        data->buffer[length] = '\0';
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

EFDataRef EFDataCreateCopy(EFAllocatorRef allocatorRef,
                           EFDataRef dataRef)
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
    
    return __EFDataCreate(allocatorRef, data->buffer, data->length, true, false);
}

EFMutableDataRef EFDataCreateMutable(EFAllocatorRef allocatorRef,
                                     EFIndex capacity)
{
    return __EFDataCreate(allocatorRef, NULL, capacity, true, true);
}

EFMutableDataRef EFDataCreateMutableCopy(EFAllocatorRef allocatorRef,
                                         EFDataRef dataRef)
{
    return NULL;
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
