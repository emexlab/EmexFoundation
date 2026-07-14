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
#include <string.h>
#include <pthread.h>
#include <assert.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFNumber.h>
#include <EmexFoundation/EFString.h>

typedef __int128_t SInt128;

typedef struct __EVNumber {
    EFObject header;
    EFNumberType type;
    SInt128 s128;
} *__EVNumber;

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
    return a->s128 == b->s128;
}


static EFStringRef __EFNumberCopyDescription(EFObjectRef numberRef)
{
    /* since it is not a string we need to still somehow display it lol */
    __EVNumber number = (__EVNumber)numberRef;
    switch(number->type)
    {
        case kEFNumberTypeOverflow:
            return EFStringCreateWithFormat(kEFAllocatorDefault, EFSTR("overflown"));
        case kEFNumberTypeUInt8:
        case kEFNumberTypeUInt16:
        case kEFNumberTypeUInt32:
        case kEFNumberTypeUInt64:
        case kEFNumberTypeSInt8:
        case kEFNumberTypeSInt16:
        case kEFNumberTypeSInt32:
        case kEFNumberTypeSInt64:
            if(number->s128 > SINT64_MAX)
            {
                return EFStringCreateWithFormat(kEFAllocatorDefault, EFSTR("%llu"), (UInt64)number->s128);
            }
            else
            {
                return EFStringCreateWithFormat(kEFAllocatorDefault, EFSTR("%lld"), (SInt64)number->s128);
            }
        default:
            return NULL;
    }
}

static EFClass EFNumberClass = {
    .name = "EFNumber",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = __EFNumberEqual,
    .copyDescription = __EFNumberCopyDescription,
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
                           EFNumberType type,
                           const void *value)
{
    if(value == NULL && type != kEFNumberTypeOverflow)
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
        case kEFNumberTypeOverflow:
            /* as a error marker */
            break;
        case kEFNumberTypeSInt8:
            num->s128 = (SInt128)*(const SInt8 *)value;
            break;
        case kEFNumberTypeSInt16:
            num->s128 = (SInt128)*(const SInt16 *)value;
            break;
        case kEFNumberTypeSInt32:
            num->s128 = (SInt128)*(const SInt32 *)value;
            break;
        case kEFNumberTypeSInt64:
            num->s128 = (SInt128)*(const SInt64 *)value;
            break;
        case kEFNumberTypeUInt8:
            num->s128 =(SInt128)*(const UInt8 *)value;
            break;
        case kEFNumberTypeUInt16:
            num->s128 = (SInt128)*(const UInt16 *)value;
            break;
        case kEFNumberTypeUInt32:
            num->s128 = (SInt128)*(const UInt32 *)value;
            break;
        case kEFNumberTypeUInt64:
            num->s128 = (SInt128)*(const UInt64 *)value;
            break;
        default:
            /* possibly bad type */
            EFRelease(num);
            return NULL;
    }

    num->type = type;

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
        case kEFNumberTypeOverflow:
            return 0;
        case kEFNumberTypeSInt8:
        case kEFNumberTypeUInt8:
            return sizeof(SInt8);
        case kEFNumberTypeSInt16:
        case kEFNumberTypeUInt16:
            return sizeof(SInt16);
        case kEFNumberTypeSInt32:
        case kEFNumberTypeUInt32:
            return sizeof(SInt32);
        case kEFNumberTypeSInt64:
        case kEFNumberTypeUInt64:
            return sizeof(SInt64);
        default:
            return 0;
    }
}

EFNumberType EFNumberGetType(EFNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return kEFNumberTypeOverflow;
    }

    __EVNumber num = (__EVNumber)numberRef;
    return num->type;
}

Boolean EFNumberGetValue(EFNumberRef numberRef,
                         EFNumberType type,
                         void *value)
{
    if(numberRef == NULL || value == NULL)
    {
        return false;
    }

    __EVNumber num = (__EVNumber)numberRef;
    switch(num->type)
    {
        case kEFNumberTypeOverflow:
            return false;
        case kEFNumberTypeSInt8:
            *(SInt8 *)value = (SInt8)num->s128;
            return true;
        case kEFNumberTypeSInt16:
            *(SInt16 *)value = (SInt16)num->s128;
            return true;
        case kEFNumberTypeSInt32:
            *(SInt32 *)value = (SInt32)num->s128;
            return true;
        case kEFNumberTypeSInt64:
            *(SInt64 *)value = (SInt64)num->s128;
            return true;
        case kEFNumberTypeUInt8:
            *(UInt8 *)value = (UInt8)num->s128;
            return true;
        case kEFNumberTypeUInt16:
            *(UInt16 *)value = (UInt16)num->s128;
            return true;
        case kEFNumberTypeUInt32:
            *(UInt32 *)value = (UInt32)num->s128;
            return true;
        case kEFNumberTypeUInt64:
            *(UInt64 *)value = (UInt64)num->s128;
            return true;
        default:
            return false;
    }
}

EFComparisonResult EFNumberCompare(EFNumberRef numberRef,
                                   EFNumberRef otherNumberRef)
{
    __EVNumber num = (__EVNumber)numberRef;
    __EVNumber otherNum = (__EVNumber)otherNumberRef;

    if(num == NULL || otherNum == NULL || num->type == kEFNumberTypeOverflow || otherNum->type == kEFNumberTypeOverflow)
    {
        /* nothing to compare */
        return kEFComparisonResultEqualTo;
    }

    return (num->s128 == otherNum->s128) ? kEFComparisonResultEqualTo : (num->s128 > otherNum->s128) ? kEFComparisonResultGreaterThan : kEFComparisonResultLessThan;
}
