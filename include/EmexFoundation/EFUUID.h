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

#ifndef EFUUID_H
#define EFUUID_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>

typedef struct __EFUUID *EFUUIDRef;

typedef struct {
    UInt8 byte0;
    UInt8 byte1;
    UInt8 byte2;
    UInt8 byte3;
    UInt8 byte4;
    UInt8 byte5;
    UInt8 byte6;
    UInt8 byte7;
    UInt8 byte8;
    UInt8 byte9;
    UInt8 byte10;
    UInt8 byte11;
    UInt8 byte12;
    UInt8 byte13;
    UInt8 byte14;
    UInt8 byte15;
} EFUUIDBytes;

EF_EXTERN EFTypeID EFUUIDGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFUUIDRef EFUUIDCreate(EFAllocatorRef allocatorRef);
EF_EXTERN EF_RETURNS_RETAINED EFUUIDRef EFUUIDCreateWithBytes(EFAllocatorRef allocatorRef, UInt8 byte0, UInt8 byte1, UInt8 byte2, UInt8 byte3, UInt8 byte4, UInt8 byte5, UInt8 byte6, UInt8 byte7, UInt8 byte8, UInt8 byte9, UInt8 byte10, UInt8 byte11, UInt8 byte12, UInt8 byte13, UInt8 byte14, UInt8 byte15);
EF_EXTERN EF_RETURNS_RETAINED EFUUIDRef EFUUIDCreateWithUUIDBytes(EFAllocatorRef allocatorRef, EFUUIDBytes uuidBytes);

EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFUUIDCreateString(EFAllocatorRef allocatorRef, EFUUIDRef uuidRef);

#endif /* EFUUID_H */
