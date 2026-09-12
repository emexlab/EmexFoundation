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

EF_EXTERN EFFilePolicy EFFilePolicyInData;
EF_EXTERN EFFilePolicy EFFilePolicyOutData;
EF_EXTERN EFFilePolicy EFFilePolicyInNoCreate;

EF_EXTERN EFTypeID EFFileGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFFileRef EFFileCreateWithPath(EFAllocatorRef allocator, EFFilePolicy policy, EFStringRef string);
EF_EXTERN EF_RETURNS_RETAINED EFFileRef EFFileCreateWithURL(EFAllocatorRef allocator, EFFilePolicy policy, EFURLRef url);
EF_EXTERN EF_RETURNS_RETAINED EFFileRef EFFileCreateUnsavedWithString(EFAllocatorRef allocator, EFFilePolicy policy, EFURLRef url, EFStringRef string);

EF_EXTERN Boolean EFFileOpen(EFFileRef file);
EF_EXTERN void EFFileClose(EFFileRef file);

EF_EXTERN EF_RETURNS_RETAINED EFFileHandleRef EFFileCopyFileHandle(EFAllocatorRef allocator, EFFileRef file);
EF_EXTERN EF_RETURNS_RETAINED EFBitWalkerRef EFFileCopyBitWalker(EFAllocatorRef allocator, EFFileRef file, EFEndian endian);
EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFFileCopyData(EFAllocatorRef allocator, EFFileRef file);

EF_EXTERN EFFileType EFFileGetType(EFFileRef file);

EF_EXTERN EFFileType EFFileTypeForPath(EFStringRef path, Boolean mustExist);
EF_EXTERN void EFFileUnlink(EFFileRef file);

EF_EXTERN EF_RETURNS_NOT_RETAINED EFURLRef EFFileGetURL(EFFileRef file);

#endif /* EFFILE_H */
