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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFString.h>

EFRange EFRangeZero = {
    .location = 0,
    .length = 0,
};

static _Atomic(EFClass *) ev_class_table[EF_MAX_CLASSES];
static _Atomic(uint64_t) ev_class_next = 1;

EFTypeID EFGetTypeID(EFObjectRef ref)
{
    return ((EFObject*)ref)->typeID;
}

Boolean EFEqual(EFObjectRef ref1,
                EFObjectRef ref2)
{
    if(ref1 == ref2)
    {
        return true;
    }
    if(ref1 == NULL || ref2 == NULL)
    {
        return false;
    }

    /* types must match */
    EFTypeID typeID = EFGetTypeID(ref1);
    if(typeID != EFGetTypeID(ref2))
    {
        return false;
    }

    EFClass *class = EFClassGetByID(typeID);
    if(class->equal != NULL)
    {
        return class->equal(ref1, ref2);
    }

    /* no handler == not equal */
    return false;
}

EFObjectRef EFRetain(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    if(object->isStatic)
    {
        return ref;
    }
    atomic_fetch_add_explicit(&object->refcount, 1, memory_order_relaxed);
    return ref;
}

void EFRelease(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    if(object->isStatic)
    {
        return;
    }

    /* releasing and trying to get the old reference count */
    EFIndex old = atomic_fetch_sub_explicit(&object->refcount, 1, memory_order_release);
    if(old == 1)
    {
        atomic_thread_fence(memory_order_acquire);
        /* trigger handler */
        EFClass *class = EFClassGetByID(object->typeID);
        assert(class != NULL);
        if(class->deinit != NULL)
        {
            class->deinit(ref);
        }
        EFObjectDealloc(object);
    }
    else if(old <= 0)
    {
        fprintf(stderr, "EFRelease: fatal error occured, reference underflow\n");
        exit(1);
    }
}

EFObjectRef EFRetainTry(EFObjectRef ref)
{
    if(ref == NULL)
    {
        return NULL;
    }
    return EFRetain(ref);
}

Boolean EFReleaseTry(EFObjectRef ref)
{
    if(ref == NULL)
    {
        return false;
    }
    EFRelease(ref);
    return true;
}

Boolean EFReleaseTryHelper(void *ref)
{
    EFObjectRef *refp = (EFObjectRef*)ref;
    if(refp == NULL || *refp == NULL)
    {
        return false;
    }
    EFRelease(*refp);
    return true;
}

EFIndex EFGetRetainCount(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    if(object->isStatic)
    {
        return 1;   /* static.. */
    }
    return atomic_load(&object->refcount);
}

EFTypeID EFClassRegister(EFClass *cls)
{
    assert(cls != NULL);
    uint64_t id = atomic_fetch_add_explicit(&ev_class_next, 1, memory_order_relaxed);
    if(id >= EF_MAX_CLASSES)
    {
        return kEFNotATypeID;
    }

    cls->typeID = id;
    atomic_store_explicit(&ev_class_table[id], cls, memory_order_release);
    return id;
}

EFClass *EFClassGetByID(EFTypeID id)
{
    if(id == kEFNotATypeID || id >= EF_MAX_CLASSES)
    {
        return NULL;
    }
    return atomic_load_explicit(&ev_class_table[id], memory_order_acquire);
}

EFAllocatorRef EFGetAllocator(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    return object->allocatorRef;
}

EFStringRef EFCopyDescription(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    if(object == NULL)
    {
        return EFSTR("<nil>");
    }

    EFClass *cls = EFClassGetByID(object->typeID);
    if(cls == NULL)
    {
        return EFSTR("<nil>");
    }

    if(cls->copyDescription)
    {
        EFStringRef descriptionRef = cls->copyDescription(ref);
        if(descriptionRef != NULL)
        {
            return descriptionRef;
        }
    }

    EFStringRef descriptionFallbackRef = EFStringCreateWithFormat(object->allocatorRef, EFSTR("<%s %p>"), cls->name, ref);
    if(descriptionFallbackRef == NULL)
    {
        return EFSTR("<nil>");
    }

    return descriptionFallbackRef;
}

void EFLog(EFStringRef formatStringRef, ...)
{
    va_list arguments;
    va_start(arguments, formatStringRef);
    EFStringRef resultRef = EFStringCreateWithFormatAndArguments(NULL, formatStringRef, arguments);
    va_end(arguments);
    if(resultRef == NULL)
    {
        return;
    }

    const char *resultCptr = EFStringGetCStringPtr(resultRef, kEFStringEncodingUTF8);
    if(resultCptr)
    {
        fprintf(stderr, "%s", resultCptr);
    }

    EFRelease(resultRef);
}
