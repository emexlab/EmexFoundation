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

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stddef.h>
#include <fcntl.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/EFPageGroup.h>
#include <EmexFoundation/EFURL.h>

typedef enum: UInt8 {
    kEFFileHandleTypeBSD,
    kEFFileHandleTypeVirtual,
} EFFileHandleType;

typedef enum: UInt8 {
    kEFFileHandleSeekTypeSet,
    kEFFileHandleSeekTypeCur,
    kEFFileHandleSeekTypeEnd,
    /* fuck darwin no SEEK_HOLE (only Torvalds should seek the hole of his mom right back into the birth canal he came out of) */
} EFFileHandleSeekType;

typedef struct __EFFileHandle *EFFileHandleRef;

EF_EXTERN EFTypeID EFFileHandleGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFFileHandleRef EFFileHandleCreate(EFAllocatorRef allocatorRef);
EF_EXTERN EF_RETURNS_RETAINED EFFileHandleRef EFFileHandleCreateWithFileDescriptor(EFAllocatorRef allocatorRef, int fd);
EF_EXTERN EF_RETURNS_RETAINED EFFileHandleRef EFFileHandleCreateWithPathAndOptions(EFAllocatorRef allocatorRef, EFStringRef pathStringRef, int flg, ...);
EF_EXTERN EF_RETURNS_RETAINED EFFileHandleRef EFFileHandleCreateWithURLAndOptions(EFAllocatorRef allocatorRef, EFURLRef urlRef, int flg, ...);
EF_EXTERN EF_RETURNS_RETAINED EFFileHandleRef EFFileHandleCreateCopy(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef);

EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFFileHandleReadData(EFFileHandleRef fileHandleRef, EFIndex length);
EF_EXTERN Boolean EFFileHandleWriteData(EFFileHandleRef fileHandleRef, EFDataRef dataRef);

EF_EXTERN EFIndex EFFileHandleRead(EFFileHandleRef fileHandleRef, UInt8 *buffer, EFIndex length);
EF_EXTERN EFIndex EFFileHandleWrite(EFFileHandleRef fileHandleRef, const UInt8 *buffer, EFIndex length);
EF_EXTERN EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandleRef, EFIndex length);

EF_EXTERN EFIndex EFFileHandleSeek(EFFileHandleRef fileHandleRef, EFIndex offset, EFFileHandleSeekType seekType);
EF_EXTERN void EFFileHandleSync(EFFileHandleRef fileHandleRef);

EF_EXTERN EFIndex EFFileHandleGetLength(EFFileHandleRef fileHandleRef);
EF_EXTERN Boolean EFFileHandleIsReadable(EFFileHandleRef fileHandleRef);
EF_EXTERN Boolean EFFileHandleIsWritable(EFFileHandleRef fileHandleRef);

EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFFileHandleCopyDataForRange(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef, EFRange range);
EF_EXTERN EF_RETURNS_RETAINED EFPageGroupRef EFFIleHandleCopyPageGroup(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef);

EF_EXTERN char *EFFileHandleGets(EFFileHandleRef fileHandle, char *s, int n);

#endif /* EFFILEHANDLE_H */
