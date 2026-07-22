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

#ifndef EFURL_H
#define EFURL_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>

typedef enum: UInt8 {
    kEFURLTypePOSIX,
    kEFURLTypeHTTPS,
    kEFURLTypeHTTP,
} EFURLType;

typedef struct __EFURL *EFURLRef;
typedef struct __EFString *EFStringRef;
typedef struct __EFArray *EFArrayRef;

EF_EXTERN EFTypeID EFURLGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateWithString(EFAllocatorRef allocatorRef, EFStringRef string);
EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateURLByAppendingPathComponent(EFAllocatorRef allocatorRef, EFURLRef urlRef, EFStringRef pathComponent);
EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateURLByDeletingLastPathComponent(EFAllocatorRef allocatorRef, EFURLRef urlRef);
EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateURLByReplacingLastPathComponent(EFAllocatorRef allocatorRef, EFURLRef urlRef, EFStringRef pathComponent);

EF_EXTERN EFURLType EFURLGetType(EFURLRef urlRef);
EF_EXTERN EF_RETURNS_NOT_RETAINED EFArrayRef EFURLGetPathComponents(EFURLRef urlRef);
EF_EXTERN Boolean EFURLIsRelative(EFURLRef urlRef);

EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFURLCopyPath(EFAllocatorRef allocatorRef, EFURLRef urlRef);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFURLCopyPathWithoutPrefix(EFAllocatorRef allocatorRef, EFURLRef urlRef);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFURLCopyPathWithoutHostname(EFAllocatorRef allocatorRef, EFURLRef urlRef);

#endif /* EFURL_H */
