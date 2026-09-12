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

EF_EXTERN EFTypeID EFURLGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateWithString(EFAllocatorRef allocator, EFStringRef string);
EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateByAppendingPathComponent(EFAllocatorRef allocator, EFURLRef url, EFStringRef pathComponent);
EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateByDeletingLastPathComponent(EFAllocatorRef allocator, EFURLRef url);
EF_EXTERN EF_RETURNS_RETAINED EFURLRef EFURLCreateByReplacingLastPathComponent(EFAllocatorRef allocator, EFURLRef url, EFStringRef pathComponent);

EF_EXTERN EFURLType EFURLGetType(EFURLRef url);
EF_EXTERN EF_RETURNS_NOT_RETAINED EFArrayRef EFURLGetPathComponents(EFURLRef url);

EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFURLCopyPath(EFAllocatorRef allocator, EFURLRef url) EFDEPRECATED("use EFURLGetPath() instead, which is better since a URL object can't mutate anyways, which is the realization I had.");
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFURLCopyPathWithoutPrefix(EFAllocatorRef allocator, EFURLRef url);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFURLCopyPathWithoutHostname(EFAllocatorRef allocator, EFURLRef url);

EF_EXTERN EF_RETURNS_NOT_RETAINED EFStringRef EFURLGetPath(EFURLRef url);

#endif /* EFURL_H */
