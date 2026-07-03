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

#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>
#include <EmexFoundation/EFNumber.h>

typedef struct __EVNumber {
    EFObject header;
    kEFNumberType type;
    union {
        int8_t s8;
        int16_t s16;
        int32_t s32;
        int64_t s64;
    };
} *__EVNumber;

static int64_t __EFNumberGetSInt64Value(__EVNumber number)
{
    if(number == NULL)
    {
        return 0;
    }
    
    switch(number->type)
    {
        case kEFNumberTypeSInt8:
            return (int64_t)number->s8;
        case kEFNumberTypeSInt16:
            return(int64_t)number->s16;
        case kEFNumberTypeSInt32:
            return(int64_t)number->s32;
        case kEFNumberTypeSInt64:
            return(int64_t)number->s64;
        default:
            return 0;
    }
}

static Boolean __EFNumberEqual(EFObjectRef ref1,
                               EFObjectRef ref2)
{
    __EVNumber a = (__EVNumber)ref1;
    __EVNumber b = (__EVNumber)ref2;
    
    if(a->type != b->type)
    {
        /* not the same type of number =3 */
        return false;
    }

    /* comparing the number it self */
    switch(a->type)
    {
        case kEFNumberTypeSInt8:
            return a->s8 == b->s8;
        case kEFNumberTypeSInt16:
            return a->s16 == b->s16;
        case kEFNumberTypeSInt32:
            return a->s32 == b->s32;
        case kEFNumberTypeSInt64:
            return a->s64 == b->s64;
        default:
            return false;
    }
}

static EFClass EFNumberClass = {
    .name = "EFNumber",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = __EFNumberEqual,
    .copyDescription = NULL,
};

static void EFNumberRegisterClass(void)
{
    EFClassRegister(&EFNumberClass);
}

EFTypeID EFNumberGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFNumberRegisterClass);
    return EFNumberClass.typeID;
}

EFNumberRef EFNumberCreate(EFAllocatorRef allocatorRef,
                           kEFNumberType type,
                           const void *value)
{
    if(value == NULL)
    {
        return NULL;
    }

    __EVNumber num = (__EVNumber)EFObjectAlloc(allocatorRef, EFNumberGetTypeID(), sizeof(struct __EVNumber));
    if(num == NULL)
    {
        return NULL;
    }

    switch(type)
    {
        case kEFNumberTypeSInt8:
            num->s8  = *(const int8_t *)value;
            break;
        case kEFNumberTypeSInt16:
            num->s16 = *(const int16_t *)value;
            break;
        case kEFNumberTypeSInt32:
            num->s32 = *(const int32_t *)value;
            break;
        case kEFNumberTypeSInt64:
            num->s64 = *(const int64_t *)value;
            break;
        default:
            /* possibly bad type */
            EFRelease(num);
            return NULL;
    }

    return num;
}

EFIndex EFNumberGetByteSize(EFNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return 0;
    }

    __EVNumber num = (__EVNumber)numberRef;
    switch(num->type)
    {
        case kEFNumberTypeSInt8:
            return sizeof(UInt8);
        case kEFNumberTypeSInt16:
            return sizeof(uint16_t);
        case kEFNumberTypeSInt32:
            return sizeof(uint32_t);
        case kEFNumberTypeSInt64:
            return sizeof(uint64_t);
        default:
            return 0;
    }
}

kEFNumberType EFNumberGetType(EFNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return kEFNumberTypeSInt8;
    }

    __EVNumber num = (__EVNumber)numberRef;
    return num->type;
}

Boolean EFNumberGetValue(EFNumberRef numberRef,
                         kEFNumberType type,
                         void *value)
{
    if(numberRef == NULL || value == NULL)
    {
        return false;
    }

    __EVNumber num = (__EVNumber)numberRef;
    switch(num->type)
    {
        case kEFNumberTypeSInt8:
            *(int8_t *)value = (UInt8)__EFNumberGetSInt64Value(num);
            return true;
        case kEFNumberTypeSInt16:
            *(int16_t *)value = (uint16_t)__EFNumberGetSInt64Value(num);
            return true;
        case kEFNumberTypeSInt32:
            *(int32_t *)value = (uint32_t)__EFNumberGetSInt64Value(num);
            return true;
        case kEFNumberTypeSInt64:
            *(int64_t *)value = (uint64_t)__EFNumberGetSInt64Value(num);
            return true;
        default:
            return false;
    }
}

kEFNumberComparisonResult EFNumberCompare(EFNumberRef numberRef,
                                          EFNumberRef otherNumberRef)
{
    __EVNumber num = (__EVNumber)numberRef;
    __EVNumber otherNum = (__EVNumber)otherNumberRef;

    if(num == NULL || otherNum == NULL)
    {
        /* nothing to compare */
        return kEFNumberComparisonResultEqualTo;
    }

    int64_t int1 = __EFNumberGetSInt64Value(num);
    int64_t int2 = __EFNumberGetSInt64Value(otherNum);

    return (int1 == int2) ? kEFNumberComparisonResultEqualTo : (int1 > int2) ? kEFNumberComparisonResultGreaterThan : kEFNumberComparisonResultLessThan;
}
