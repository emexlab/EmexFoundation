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
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/EFString.h>

struct __EFArray {
    EFObject super;

    Boolean isMutable;
    EFArrayCallbacks callbacks;

    EFIndex itemsCapacity;
    EFIndex itemsCount;
    void **items;
};

EFArrayCallbacks kEFArrayCallbacksDefaultCallbacks = &(struct EFArrayCallbacks){
    .retain = NULL,
    .release = NULL,
    .equal = NULL,
    .copyDescription = NULL,
};

static Boolean __EFArrayRetainObjectCallback(void *ptr)
{
    EFObjectRef ref = EFRetain((EFObjectRef)ptr);
    return (ref != NULL);
}

static void __EFArrayReleaseObjectCallback(void *ptr)
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
    .retain = __EFArrayRetainObjectCallback,
    .release = __EFArrayReleaseObjectCallback,
    .equal = __EFArrayEqualObjectCallback,
    .copyDescription = __EFArrayCopyDescriptionObjectCallback,
};

static void __EFArrayClassDeinit(EFObjectRef arrayRef)
{
    EFArrayRef array = (EFArrayRef)arrayRef;
    if(array->callbacks->release)
    {
        for(EFIndex index = 0; index < array->itemsCount; index++)
        {
            array->callbacks->release(array->items[index]);
        }
    }
    EFAllocatorDeallocate(EFGetAllocator(arrayRef), array->items);
}

static Boolean __EFArrayClassEqual(EFObjectRef arrayRef1,
                                   EFObjectRef arrayRef2)
{
    EFArrayRef array1 = (EFArrayRef)arrayRef1;
    EFArrayRef array2 = (EFArrayRef)arrayRef2;

    if(array1->callbacks != array2->callbacks ||
       array1->itemsCount != array2->itemsCount)
    {
        return false;
    }
    EFArrayCallbacks callbacks = array1->callbacks;

    for(EFIndex index = 0; index < array1->itemsCount; index++)
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

static EFStringRef __EFArrayCopyDebugDescription(EFObjectRef arrayRef)
{
    EFArrayRef array = (EFArrayRef)arrayRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(arrayRef);

    EFAUTOREL EFStringRef baseStringRef = EFStringCreateWithFormat(allocatorRef, EFSTR("<%@ %p>{count = %ld, items = {"), array->isMutable ? EFSTR("EFMutableArray") : EFSTR("EFArray"), arrayRef, array->itemsCount);
    EFAUTOREL EFMutableStringRef mutableStringRef = EFStringCreateMutableCopy(allocatorRef, baseStringRef);
    if(mutableStringRef == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < array->itemsCount; index++)
    {
        if(index > 0 && !EFStringAppendString(mutableStringRef, EFSTR(", ")))
        {
            return NULL;
        }

        void *ptr = EFArrayGetValueAtIndex(arrayRef, index);
        EFAUTOREL EFStringRef stringRef = NULL;
        if(array->callbacks->copyDescription)
        {
            stringRef = array->callbacks->copyDescription(allocatorRef, ptr);
        }

        if(stringRef == NULL)
        {
            stringRef = EFStringCreateWithFormat(allocatorRef, EFSTR("%p"), ptr);
        }

        if(stringRef == NULL || !EFStringAppendString(mutableStringRef, stringRef))
        {
            return NULL;
        }
    }

    if(!EFStringAppendString(mutableStringRef, EFSTR("}}")))
    {
        return NULL;
    }

    return EFAUTOTRANSFER(mutableStringRef);
}

EF_HIDDEN EFClassDefinitionNewest EFArrayClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDArray,
        .name = EFSTR_FILESCOPE("EFArray"),
    },
    .init = NULL,
    .deinit = __EFArrayClassDeinit,
    .equal = __EFArrayClassEqual,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = __EFArrayCopyDebugDescription,
};

EFTypeID EFArrayGetTypeID(void)
{
    return kEFTypeIDArray;
}

EFArrayRef EFArrayCreate(EFAllocatorRef allocator,
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
    EFAUTOREL EFMutableArrayRef mutableArray = EFArrayCreateMutable(allocator, callbacks, valuesCount);
    if(mutableArray == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < valuesCount; index++)
    {
        if(!EFArrayAppendValue(mutableArray, values[index]))
        {
            return NULL;
        }
    }

    /* immutabilize the array */
    mutableArray->isMutable = false;

    return (EFArrayRef)EFAUTOTRANSFER(mutableArray);
}

EFMutableArrayRef EFArrayCreateMutable(EFAllocatorRef allocator,
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
        items = EFAllocatorAllocate(allocator, capacity * sizeof(void*), 0);
        if(items == NULL)
        {
            return NULL;
        }
    }

    EFMutableArrayRef array = (EFMutableArrayRef)EFObjectCreate(allocator, EFArrayGetTypeID(), (EFIndex)sizeof(struct __EFArray));
    if(array == NULL)
    {
        EFAllocatorDeallocate(allocator, items);
        return NULL;
    }

    array->isMutable = true;
    array->callbacks = callbacks;
    array->itemsCount = 0;
    array->itemsCapacity = capacity;
    array->items = items;

    return array;
}

static EFArrayRef __EFArrayCreateCopy(EFAllocatorRef allocator,
                                      EFArrayRef array,
                                      Boolean isMutable)
{
    if(array == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef newArray = EFArrayCreateMutable(allocator, array->callbacks, array->itemsCount);
    if(newArray == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < array->itemsCount; index++)
    {
        if(!EFArrayAppendValue(newArray, array->items[index]))
        {
            return NULL;
        }
    }

    newArray->isMutable = isMutable;

    return EFAUTOTRANSFER(newArray);
}

EFMutableArrayRef EFArrayCreateMutableCopy(EFAllocatorRef allocator,
                                           EFArrayRef array)
{
    return __EFArrayCreateCopy(allocator, array, true);
}

EFArrayRef EFArrayCreateCopy(EFAllocatorRef allocator,
                             EFArrayRef array)
{
    return __EFArrayCreateCopy(allocator, array, false);
}

EFIndex EFArrayGetCount(EFArrayRef array)
{
    if(array == NULL)
    {
        return 0;
    }

    return array->itemsCount;
}

void *EFArrayGetValueAtIndex(EFArrayRef array,
                             EFIndex index)
{
    if(array == NULL || index < 0 || array->itemsCount <= index)
    {
        return NULL;
    }

    return array->items[index];
}

Boolean __EFArrayResizeIfNeededForOneMoreIndex(EFArrayRef array)
{
    EFIndex neededCapacity = array->itemsCount + 1;
    if(array->itemsCapacity >= neededCapacity)
    {
        return true;
    }

    /* need to reallocate */
    EFIndex new_cap = array->itemsCapacity + 5;
    if(new_cap < array->itemsCapacity)
    {
        /* wrapped around */
        return false;
    }

    /* actual reallocation */
    void *new_ptr = EFAllocatorReallocate(EFGetAllocator(array), array->items, (EFSize)(new_cap * sizeof(void*)), 0);
    if(new_ptr == NULL)
    {
        return false;
    }
    array->items = new_ptr;
    array->itemsCapacity = new_cap;

    return true;
}

Boolean EFArrayAppendValue(EFMutableArrayRef mutableArray,
                           void *ptr)
{
    if(mutableArray == NULL || ptr == NULL || !mutableArray->isMutable || mutableArray->itemsCount >= __LONG_MAX__ || !__EFArrayResizeIfNeededForOneMoreIndex(mutableArray))
    {
        return false;
    }

    if(mutableArray->callbacks->retain != NULL && !mutableArray->callbacks->retain(ptr))
    {
        return false;
    }

    /* append */
    EFIndex idx = (mutableArray->itemsCount)++;
    mutableArray->items[idx] = ptr;

    return true;
}

Boolean EFArrayInsertValueAtIndex(EFMutableArrayRef mutableArray,
                                  EFIndex index,
                                  void *ptr)
{
    if(mutableArray == NULL || ptr == NULL || !mutableArray->isMutable || index > mutableArray->itemsCount || mutableArray->itemsCount >= __LONG_MAX__ || index < 0 || !__EFArrayResizeIfNeededForOneMoreIndex(mutableArray))
    {
        return false;
    }

    if(mutableArray->callbacks->retain != NULL && !mutableArray->callbacks->retain(ptr))
    {
        return false;
    }

    /* insert */
    memmove(&mutableArray->items[index + 1], &mutableArray->items[index], (EFSize)((mutableArray->itemsCount - index) * sizeof(void*)));
    mutableArray->items[index] = ptr;
    mutableArray->itemsCount++;

    return true;
}

void EFArrayRemoveValueAtIndex(EFMutableArrayRef mutableArray,
                               EFIndex index)
{
    if(mutableArray == NULL || !mutableArray->isMutable || index >= mutableArray->itemsCount || index < 0)
    {
        return;
    }

    if(mutableArray->callbacks->release != NULL)
    {
        mutableArray->callbacks->release(mutableArray->items[index]);
    }

    memmove(&mutableArray->items[index], &mutableArray->items[index + 1], (EFSize)((mutableArray->itemsCount - index - 1) * sizeof(void*)));
    mutableArray->itemsCount--;
}

Boolean EFArrayAppendValuesOfArray(EFMutableArrayRef mutableArray,
                                   EFArrayRef otherArray)
{
    if(mutableArray == NULL || otherArray == NULL || !mutableArray->isMutable || mutableArray->callbacks != otherArray->callbacks)
    {
        return false;
    }

    EFIndex currentMutableCount = mutableArray->itemsCount;
    EFIndex remaining = otherArray->itemsCount;
    for(; remaining > 0; remaining -= 5)
    {
        if(!__EFArrayResizeIfNeededForOneMoreIndex(mutableArray))
        {
            mutableArray->itemsCount = currentMutableCount;
            return false;
        }
        mutableArray->itemsCount += 5;
    }
    mutableArray->itemsCount = currentMutableCount;

    for(EFIndex index = 0; index < otherArray->itemsCount; index++)
    {
        if(mutableArray->callbacks->retain != NULL && !mutableArray->callbacks->retain(otherArray->items[index]))
        {
            /* TODO: have to revert back, but currently not implemented */
            return false;
        }

        /* append */
        EFIndex idx = (mutableArray->itemsCount)++;
        mutableArray->items[idx] = otherArray->items[index];
    }

    return true;
}

Boolean EFArrayInsertValuesOfArrayAtIndex(EFMutableArrayRef mutableArray,
                                          EFIndex index,
                                          EFArrayRef otherArray)
{
    return false;
}

void EFArrayRemoveValuesInRange(EFMutableArrayRef mutableArray,
                                EFRange range)
{
    /* TODO: make this way more efficient ;-; */
    EFIndex ceiling = range.location + range.length;
    for(EFIndex index = range.location; index < ceiling; index++)
    {
        EFArrayRemoveValueAtIndex(mutableArray, range.location);
    }
}
