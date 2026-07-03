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

#ifndef EFFILEHANDLE_H
#define EFFILEHANDLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fcntl.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/EFPageGroup.h>
#include <EmexFoundation/runtime/EFBase.h>

typedef enum: UInt8 {
    kEFFileHandleSeekTypeSet,
    kEFFileHandleSeekTypeCur,
    kEFFileHandleSeekTypeEnd,
    /* fuck darwin no SEEK_HOLE */
} kEFFileHandleSeekType;

typedef struct __EFFileHandle *EFFileHandleRef;

EFTypeID EFFileHandleGetTypeID(void);

EFFileHandleRef EFFileHandleCreate(EFAllocatorRef allocatorRef);
EFFileHandleRef EFFileHandleCreateWithFileDescriptor(EFAllocatorRef allocatorRef, int fd);
EFFileHandleRef EFFileHandleCreateWithOptions(EFAllocatorRef allocatorRef, EFStringRef pathStringRef, int flg, ...);

EFIndex EFFileHandleRead(EFFileHandleRef fileHandleRef, UInt8 *buffer, EFIndex length);
EFIndex EFFileHandleWrite(EFFileHandleRef fileHandleRef, const UInt8 *buffer, EFIndex length);
EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandleRef, EFIndex length);

EFIndex EFFileHandleSeek(EFFileHandleRef fileHandleRef, EFIndex offset, kEFFileHandleSeekType seekType);
void EFFileHandleSync(EFFileHandleRef fileHandleRef);

EFIndex EFFileHandleGetLength(EFFileHandleRef fileHandleRef);
Boolean EFFileHandleIsReadable(EFFileHandleRef fileHandleRef);
Boolean EFFileHandleIsWritable(EFFileHandleRef fileHandleRef);

EFDataRef EFFileHandleCopyDataForRange(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef, EFRange range);

EFPageGroupRef EFFIleHandleCopyPageGroup(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef);

#endif /* EFFILEHANDLE_H */
