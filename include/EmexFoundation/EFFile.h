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

#ifndef EFFILE_H
#define EFFILE_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFURL.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFBitWalker.h>

typedef enum: UInt8 {
    kEFFileTypeUnknown,
    kEFFileTypeDirectory,
    kEFFileTypeAssembly,
    kEFFileTypeAssemblyIncludations,
    kEFFileTypeC,
    kEFFileTypeCHeader,
    kEFFileTypeCXX,
    kEFFileTypeCXXHeader,
    kEFFileTypeObjC,
    kEFFileTypeObjCXX,
    kEFFileTypeObject
} EFFileType;

typedef enum: UInt8 {
    kEFFilePolicyPermissionRead =    0b00000001,
    kEFFilePolicyPermissionWrite =   0b00000010,
    kEFFilePolicyPermissionExecute = 0b00000100,
} EFFilePolicyPermission;

typedef struct EFFilePolicy {
    EFFilePolicyPermission neededPermission;
    Boolean mustExist;
    Boolean mustBeAFile;
    Boolean createOnOpen;
} EFFilePolicy;

extern EFFilePolicy EFFilePolicyInData;
extern EFFilePolicy EFFilePolicyOutData;
extern EFFilePolicy EFFilePolicyInNoCreate;

typedef struct __EFFile *EFFileRef;

extern EFTypeID EFFileGetTypeID(void);

extern EFFileRef EFFileCreateWithPath(EFAllocatorRef allocatorRef, EFFilePolicy policy, EFStringRef stringRef);
extern EFFileRef EFFileCreateWithURL(EFAllocatorRef allocatorRef, EFFilePolicy policy, EFURLRef urlRef);
extern EFFileRef EFFileCreateWithString(EFAllocatorRef allocatorRef, EFFilePolicy policy, EFURLRef urlRef, EFStringRef stringRef);

extern Boolean EFFileOpen(EFFileRef fileRef);
extern void EFFileClose(EFFileRef fileRef);

extern EFFileHandleRef EFFileCopyFileHandle(EFAllocatorRef allocatorRef, EFFileRef fileRef);
extern EFBitWalkerRef EFFileCopyBitWalker(EFAllocatorRef allocatorRef, EFFileRef fileRef, EFEndian endian);

extern EFFileType EFFileGetType(EFFileRef fileRef);

extern EFFileType EFFileTypeForPath(EFStringRef path, Boolean mustExist);

extern EFURLRef EFFileGetURL(EFFileRef fileRef);

#endif /* EFFILE_H */
