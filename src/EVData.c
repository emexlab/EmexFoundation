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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <evObj/EVData.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>

typedef struct EVData {
    EVObject header;
    bool is_mutable;
    bool is_inlined;    /* meaning the object has the buffer in it self */
    uint8_t *buf;       /* it is neither inlined nor undeallocatable if mutable */
    size_t len;
} *EVData;

static void __EVDataInit(EVDataRef dataRef)
{
    EVData data = (EVData)dataRef;

    /* we first automatically expect it to be at the inline */
    data->is_mutable = false;
    data->is_inlined = true;
    data->buf = (uint8_t*)((const char*)data + sizeof(struct EVData));
}

static void __EVDataDeinit(EVDataRef dataRef)
{
    EVData data = (EVData)dataRef;
    if(data->is_mutable)
    {
        free(data->buf);
    }
}

static EVClass EVDataClass = {
    .name = "EVData",
    .typeID = kEVNotATypeID,
    .init = __EVDataInit,
    .deinit = __EVDataDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

static void EVDataRegisterClass(void)
{
    EVClassRegister(&EVDataClass);
}

EVTypeID EVDataGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EVDataRegisterClass);
    return EVDataClass.typeID;
}

EVDataRef EVDataCreateWithCBuffer(EVAllocatorRef allocatorRef,
                                  const uint8_t *bytes,
                                  size_t length)
{
    if(bytes == NULL)
    {
        return NULL;
    }

    EVData data = (EVData)EVObjectAlloc(allocatorRef, EVDataGetTypeID(), sizeof(struct EVData) + length);
    memcpy(data->buf, bytes, length);
    data->len = length;

    return (EVDataRef)data;
}

EVDataRef EVDataCreateWithCBufferNoCopy(EVAllocatorRef allocatorRef,
                                        const uint8_t *bytes,
                                        size_t length)
{
    if(bytes == NULL)
    {
        return NULL;
    }

    EVData data = (EVData)EVObjectAlloc(allocatorRef, EVDataGetTypeID(), sizeof(struct EVData));
    data->is_inlined = false;
    data->buf = (uint8_t*)bytes;
    data->len = length;

    return (EVDataRef)data;
}

EVMutableDataRef EVDataCreateMutable(EVAllocatorRef allocatorRef,
                                     size_t capacity)
{
    return NULL;
}

EVDataRef EVDataCreateCopy(EVAllocatorRef allocatorRef,
                           EVDataRef dataRef)
{
    EVData data = (EVData)dataRef;
    if(data == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EVGetAllocator(dataRef);
    }
    
    return EVDataCreateWithCBuffer(allocatorRef, data->buf, data->len);
}

EVMutableDataRef EVDataCreateMutableCopy(EVAllocatorRef allocatorRef, EVDataRef dataRef)
{
    return NULL;
}
