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
    UInt8 bytes[16];
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
    return NULL;
}

EFStringRef EFUUIDCreateString(EFAllocatorRef allocatorRef,
                               EFUUIDRef uuidRef)
{
    return NULL;
}
