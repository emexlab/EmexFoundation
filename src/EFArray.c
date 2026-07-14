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
#include <pthread.h>
#include <string.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/EFString.h>

typedef struct __EFArray {
    EFObject header;

    Boolean isMutable;
    EFArrayCallbacks callbacks;

    EFIndex items_cap;
    EFIndex items_cnt;
    void **items;
} *__EFArray;

EFArrayCallbacks kEFArrayCallbacksDefaultCallbacks = &(struct EFArrayCallbacks){
    .append = NULL,
    .remove = NULL,
    .equal = NULL,
    .copyDescription = NULL,
};

static Boolean __EFArrayAppendObjectCallback(void *ptr)
{
    EFObjectRef ref = EFRetain((EFObjectRef)ptr);
    return (ref != NULL);
}

static void __EFArrayRemoveObjectCallback(void *ptr)
{
    EFRelease((EFObjectRef)ptr);
}

static Boolean __EFArrayEqualObjectCallback(void *ptr1,
                                            void *ptr2)
{
    return EFEqual((EFObjectRef)ptr1, (EFObjectRef)ptr2);
}

static EFStringRef __EFArrayCopyDescriptionObjectCallback(EFAllocatorRef allocatorRef,
                                                          void *ptr)
{
    return EFStringCreateWithFormat(allocatorRef, EFSTR("%@"), (EFObjectRef)ptr);
}

EFArrayCallbacks kEFArrayCallbacksObjectCallbacks = &(struct EFArrayCallbacks){
    .append = __EFArrayAppendObjectCallback,
    .remove = __EFArrayRemoveObjectCallback,
    .equal = __EFArrayEqualObjectCallback,
    .copyDescription = __EFArrayCopyDescriptionObjectCallback,
};

static void __EFArrayClassDeinit(EFObjectRef arrayRef)
{
    __EFArray array = (__EFArray)arrayRef;
    if(array->callbacks->remove)
    {
        for(EFIndex index = 0; index < array->items_cnt; index++)
        {
            array->callbacks->remove(array->items[index]);
        }
    }
    free(array->items);
}

static Boolean __EFArrayClassEqual(EFObjectRef arrayRef1,
                                   EFObjectRef arrayRef2)
{
    __EFArray array1 = (__EFArray)arrayRef1;
    __EFArray array2 = (__EFArray)arrayRef2;

    if(array1->callbacks != array2->callbacks ||
       array1->items_cnt != array2->items_cnt)
    {
        return false;
    }
    EFArrayCallbacks callbacks = array1->callbacks;

    for(EFIndex index = 0; index < array1->items_cnt; index++)
    {
        if(array1->items[index] == array2->items[index])
        {
            continue;
        }

        if(callbacks->equal && !callbacks->equal(array1->items[index], array2->items[index]))
        {
            return false;
        }
    }

    return true;
}

static EFStringRef __EFArrayCopyDescription(EFObjectRef arrayRef)
{
    __EFArray array = (__EFArray)arrayRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(arrayRef);
    EFClass *cls = EFClassGetByID(array->header.typeID);

    EFStringRef baseStringRef = EFStringCreateWithFormat(allocatorRef, EFSTR("<%s %p>{count = %ld, items = {"), cls->name, arrayRef, array->items_cnt);
    if(baseStringRef == NULL)
    {
        return NULL;
    }

    EFMutableStringRef mutableStringRef = EFStringCreateMutableCopy(allocatorRef, baseStringRef);
    EFRelease(baseStringRef);
    if(mutableStringRef == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < array->items_cnt; index++)
    {
        if(index > 0)
        {
            if(!EFStringAppendString(mutableStringRef, EFSTR(", ")))
            {
                EFRelease(mutableStringRef);
                return NULL;
            }
        }

        void *ptr = EFArrayGetValueAtIndex(arrayRef, index);
        EFStringRef stringRef = NULL;
        if(array->callbacks->copyDescription)
        {
            stringRef = array->callbacks->copyDescription(allocatorRef, ptr);
        }

        if(stringRef == NULL)
        {
            stringRef = EFStringCreateWithFormat(allocatorRef, EFSTR("%p"), ptr);
        }

        if(stringRef == NULL)
        {
            EFRelease(mutableStringRef);
            return NULL;
        }

        Boolean success = EFStringAppendString(mutableStringRef, stringRef);
        EFRelease(stringRef);
        if(!success)
        {
            EFRelease(mutableStringRef);
            return NULL;
        }
    }

    if(!EFStringAppendString(mutableStringRef, EFSTR("}}")))
    {
        EFRelease(mutableStringRef);
        return NULL;
    }

    return mutableStringRef;
}

static EFClass EFArrayClass = {
    .name = "EFArray",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFArrayClassDeinit,
    .equal = __EFArrayClassEqual,
    .copyDescription = __EFArrayCopyDescription,
};

static void EFArrayRegisterClass(void)
{
    EFClassRegister(&EFArrayClass);
}

EFTypeID EFArrayGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFArrayRegisterClass);
    return EFArrayClass.typeID;
}

EFArrayRef EFArrayCreate(EFAllocatorRef allocatorRef,
                         EFArrayCallbacks callbacks,
                         void **values,
                         EFIndex valuesCount)
{
    if((values == NULL && valuesCount != 0) || valuesCount < 0)
    {
        return NULL;
    }

    if(callbacks == NULL)
    {
        callbacks = kEFArrayCallbacksDefaultCallbacks;
    }

    /* for now not its own creator, meaning it hacks around */
    EFMutableArrayRef mutableArrayRef = EFArrayCreateMutable(allocatorRef, callbacks, valuesCount);
    if(mutableArrayRef == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < valuesCount; index++)
    {
        if(!EFArrayAppendValue(mutableArrayRef, values[index]))
        {
            EFRelease(mutableArrayRef);
            return NULL;
        }
    }

    __EFArray mutableArray = (__EFArray)mutableArrayRef;

    /* immutabilize the array */
    mutableArray->isMutable = false;

    return (EFArrayRef)mutableArrayRef;
}

EFMutableArrayRef EFArrayCreateMutable(EFAllocatorRef allocatorRef,
                                       EFArrayCallbacks callbacks,
                                       EFIndex capacity)
{
    if(callbacks == NULL || capacity < 0)
    {
        callbacks = kEFArrayCallbacksDefaultCallbacks;
    }

    void *items = NULL; /* freeing NULL is allowed as a UNIX semantic */
    if(capacity > 0)
    {
        items = calloc(capacity, sizeof(void*));
        if(items == NULL)
        {
            return NULL;
        }
    }

    __EFArray array = (__EFArray)EFObjectAlloc(allocatorRef, EFArrayGetTypeID(), sizeof(struct __EFArray));
    if(array == NULL)
    {
        free(items);
        return NULL;
    }

    array->isMutable = true;
    array->callbacks = callbacks;
    array->items_cnt = 0;
    array->items_cap = capacity;
    array->items = items;

    return (EFMutableArrayRef)array;
}

static EFArrayRef __EFArrayCreateCopy(EFAllocatorRef allocatorRef,
                                      EFArrayRef arrayRef,
                                      Boolean isMutable)
{
    if(arrayRef == NULL)
    {
        return NULL;
    }

    __EFArray srcArray = (__EFArray)arrayRef;
    EFMutableArrayRef copyArrayRef = EFArrayCreateMutable(allocatorRef, srcArray->callbacks, srcArray->items_cnt);
    if(copyArrayRef == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < srcArray->items_cnt; index++)
    {
        if(!EFArrayAppendValue(copyArrayRef, srcArray->items[index]))
        {
            EFRelease((EFObjectRef)copyArrayRef);
            return NULL;
        }
    }

    ((__EFArray)copyArrayRef)->isMutable = isMutable;

    return (EFArrayRef)copyArrayRef;
}

EFMutableArrayRef EFArrayCreateMutableCopy(EFAllocatorRef allocatorRef,
                                           EFArrayRef arrayRef)
{
    return (EFMutableArrayRef)__EFArrayCreateCopy(allocatorRef, arrayRef, true);
}

EFArrayRef EFArrayCreateCopy(EFAllocatorRef allocatorRef,
                             EFArrayRef arrayRef)
{
    return (EFMutableArrayRef)__EFArrayCreateCopy(allocatorRef, arrayRef, false);
}

EFIndex EFArrayGetCount(EFArrayRef arrayRef)
{
    if(arrayRef == NULL)
    {
        return 0;
    }

    __EFArray array = (__EFArray)arrayRef;
    return array->items_cnt;
}

void *EFArrayGetValueAtIndex(EFArrayRef arrayRef,
                             EFIndex index)
{
    if(arrayRef == NULL || index < 0)
    {
        return NULL;
    }

    /* wrap around check */
    __EFArray array = (__EFArray)arrayRef;
    if(array->items_cnt <= index)
    {
        return NULL;
    }

    return array->items[index];
}

Boolean __EFArrayResizeIfNeededForOneMoreIndex(__EFArray array)
{
    EFIndex needed_cap = array->items_cnt + 1;
    if(array->items_cap >= needed_cap)
    {
        return true;
    }

    /* need to reallocate */
    EFIndex new_cap = array->items_cap + 5;
    if(new_cap < array->items_cap)
    {
        /* wrapped around */
        return false;
    }

    /* actual reallocation */
    void *new_ptr = realloc(array->items, (size_t)(new_cap * sizeof(void*)));
    if(new_ptr == NULL)
    {
        return false;
    }
    array->items = new_ptr;
    array->items_cap = new_cap;

    return true;
}

Boolean EFArrayAppendValue(EFMutableArrayRef mutableArrayRef,
                           void *ptr)
{
    __EFArray mutableArray = (__EFArray)mutableArrayRef;
    if(mutableArray == NULL || ptr == NULL || !mutableArray->isMutable || mutableArray->items_cnt >= __LONG_MAX__ || !__EFArrayResizeIfNeededForOneMoreIndex(mutableArray))
    {
        return false;
    }

    if(mutableArray->callbacks->append != NULL)
    {
        if(!mutableArray->callbacks->append(ptr))
        {
            return false;
        }
    }

    /* append */
    EFIndex idx = (mutableArray->items_cnt)++;
    mutableArray->items[idx] = ptr;

    return true;
}

Boolean EFArrayInsertValueAtIndex(EFMutableArrayRef mutableArrayRef,
                                  EFIndex index,
                                  void *ptr)
{
    __EFArray mutableArray = (__EFArray)mutableArrayRef;
    if(mutableArray == NULL || ptr == NULL || !mutableArray->isMutable || index > mutableArray->items_cnt || mutableArray->items_cnt >= __LONG_MAX__ || index < 0 || !__EFArrayResizeIfNeededForOneMoreIndex(mutableArray))
    {
        return false;
    }

    if(mutableArray->callbacks->append != NULL)
    {
        if(!mutableArray->callbacks->append(ptr))
        {
            return false;
        }
    }

    /* insert */
    memmove(&mutableArray->items[index + 1], &mutableArray->items[index], (size_t)((mutableArray->items_cnt - index) * sizeof(void*)));
    mutableArray->items[index] = ptr;
    mutableArray->items_cnt++;

    return true;
}

void EFArrayRemoveValueAtIndex(EFMutableArrayRef mutableArrayRef,
                               EFIndex index)
{
    __EFArray mutableArray = (__EFArray)mutableArrayRef;
    if(mutableArray == NULL || !mutableArray->isMutable || index >= mutableArray->items_cnt || index < 0)
    {
        return;
    }

    if(mutableArray->callbacks->remove != NULL)
    {
        mutableArray->callbacks->remove(mutableArray->items[index]);
    }

    memmove(&mutableArray->items[index], &mutableArray->items[index + 1], (size_t)((mutableArray->items_cnt - index - 1) * sizeof(void*)));
    mutableArray->items_cnt--;
}
