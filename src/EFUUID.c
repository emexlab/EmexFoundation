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
 *  Systems Headers
 * -------------------------------------------------------------------- */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFUUID.h>
#include <EmexFoundation/EFString.h>

static void _intToHexChars(UInt32 in,
                           char *out,
                           int digits)
{
    int shift;
    UInt32 d;
    while(--digits >= 0)
    {
        shift = digits << 2;
        d = 0x0FL & (in >> shift);
        if(d <= 9)
        {
            *out++ = (char)'0' + d;
        }
        else
        {
            *out++ = (char)'A' + (d - 10);
        }
    }
}

typedef struct __EFUUID {
    EFObject header;
    EFUUIDBytes bytes;
} *__EFUUID;

EFClass EFUUIDClass = {
    .name = "EFUUID",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
    .copyDescription = NULL,
    .hash = NULL,
};

static void EFUUIDRegisterClass(void)
{
    EFClassRegister(&EFUUIDClass);
}

EFTypeID EFUUIDGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFUUIDRegisterClass);
    return EFUUIDClass.typeID;
}

EFUUIDRef EFUUIDCreate(EFAllocatorRef allocatorRef)
{
    __EFUUID uuid = (__EFUUID)EFObjectCreate(allocatorRef, EFUUIDGetTypeID(), (EFIndex)sizeof(struct __EFUUID));
    if(uuid == NULL)
    {
        return NULL;
    }

    arc4random_buf(&(uuid->bytes), sizeof(uuid->bytes));
    return (EFUUIDRef)uuid;
}

EFUUIDRef EFUUIDCreateWithBytes(EFAllocatorRef allocatorRef,
                                UInt8 byte0,
                                UInt8 byte1,
                                UInt8 byte2,
                                UInt8 byte3,
                                UInt8 byte4,
                                UInt8 byte5,
                                UInt8 byte6,
                                UInt8 byte7,
                                UInt8 byte8,
                                UInt8 byte9,
                                UInt8 byte10,
                                UInt8 byte11,
                                UInt8 byte12,
                                UInt8 byte13,
                                UInt8 byte14,
                                UInt8 byte15)
{
    __EFUUID uuid = (__EFUUID)EFObjectCreate(allocatorRef, EFUUIDGetTypeID(), (EFIndex)sizeof(struct __EFUUID));
    if(uuid == NULL)
    {
        return NULL;
    }

    uuid->bytes.byte0 = byte0;
    uuid->bytes.byte1 = byte1;
    uuid->bytes.byte2 = byte2;
    uuid->bytes.byte3 = byte3;
    uuid->bytes.byte4 = byte4;
    uuid->bytes.byte5 = byte5;
    uuid->bytes.byte6 = byte6;
    uuid->bytes.byte7 = byte7;
    uuid->bytes.byte8 = byte8;
    uuid->bytes.byte9 = byte9;
    uuid->bytes.byte10 = byte10;
    uuid->bytes.byte11 = byte11;
    uuid->bytes.byte12 = byte12;
    uuid->bytes.byte13 = byte13;
    uuid->bytes.byte14 = byte14;
    uuid->bytes.byte15 = byte15;

    return (EFUUIDRef)uuid;
}

EFUUIDRef EFUUIDCreateWithUUIDBytes(EFAllocatorRef allocatorRef,
                                    EFUUIDBytes uuidBytes)
{
    return EFUUIDCreateWithBytes(allocatorRef, uuidBytes.byte0, uuidBytes.byte1, uuidBytes.byte2, uuidBytes.byte3, uuidBytes.byte4, uuidBytes.byte5, uuidBytes.byte6, uuidBytes.byte7, uuidBytes.byte8, uuidBytes.byte9, uuidBytes.byte10, uuidBytes.byte11, uuidBytes.byte12, uuidBytes.byte13, uuidBytes.byte14, uuidBytes.byte15);
}

EFStringRef EFUUIDCreateString(EFAllocatorRef allocatorRef,
                               EFUUIDRef uuidRef)
{
    __EFUUID uuid = (__EFUUID)uuidRef;
    if(uuid == NULL)
    {
        return NULL;
    }

    char buf[37];
    memset(buf, '0', sizeof(buf));
    _intToHexChars(uuid->bytes.byte0, &buf[0], 2);
    _intToHexChars(uuid->bytes.byte1, &buf[2], 2);
    _intToHexChars(uuid->bytes.byte2, &buf[4], 2);
    _intToHexChars(uuid->bytes.byte3, &buf[6], 2);
    buf[8] = '-';
    _intToHexChars(uuid->bytes.byte4, &buf[9], 2);
    _intToHexChars(uuid->bytes.byte5, &buf[11], 2);
    buf[13] = '-';
    _intToHexChars(uuid->bytes.byte6, &buf[14], 2);
    _intToHexChars(uuid->bytes.byte7, &buf[16], 2);
    buf[18] = '-';
    _intToHexChars(uuid->bytes.byte8, &buf[19], 2);
    _intToHexChars(uuid->bytes.byte9, &buf[21], 2);
    buf[23] = '-';
    _intToHexChars(uuid->bytes.byte10, &buf[24], 2);
    _intToHexChars(uuid->bytes.byte11, &buf[26], 2);
    _intToHexChars(uuid->bytes.byte12, &buf[28], 2);
    _intToHexChars(uuid->bytes.byte13, &buf[30], 2);
    _intToHexChars(uuid->bytes.byte14, &buf[32], 2);
    _intToHexChars(uuid->bytes.byte15, &buf[34], 2);
    buf[36] = '\0';

    return EFStringCreateWithCString(allocatorRef, buf, kEFStringEncodingUTF8);
}
