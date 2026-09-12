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

#ifndef EFFILEMANAGER_H
#define EFFILEMANAGER_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFURL.h>
#include <EmexFoundation/EFArray.h>

EF_EXTERN EFTypeID EFFileManagerGetTypeID(void);

/* MARK: later virtual file systems will be supported by those managers */
EF_EXTERN EF_RETURNS_NOT_RETAINED EFFileManagerRef EFFileManagerGetDefaultManager(void);

EF_EXTERN EF_RETURNS_RETAINED EFFileManagerRef EFFileManagerCreate(EFAllocatorRef allocator);

EF_EXTERN Boolean EFFileManagerFileExistsAtPath(EFFileManagerRef manager, EFStringRef path, Boolean *isDirectory);
EF_EXTERN Boolean EFFileManagerFileExistsAtURL(EFFileManagerRef manager, EFURLRef url, Boolean *isDirectory);

EF_EXTERN Boolean EFFileManagerIsReadableAtPath(EFFileManagerRef manager, EFStringRef path);
EF_EXTERN Boolean EFFileManagerIsWritableAtPath(EFFileManagerRef manager, EFStringRef path);
EF_EXTERN Boolean EFFileManagerIsExecutableAtPath(EFFileManagerRef manager, EFStringRef path);
EF_EXTERN Boolean EFFileManagerIsReadableAtURL(EFFileManagerRef manager, EFURLRef url);
EF_EXTERN Boolean EFFileManagerIsWritableAtURL(EFFileManagerRef manager, EFURLRef url);
EF_EXTERN Boolean EFFileManagerIsExecutableAtURL(EFFileManagerRef manager, EFURLRef url);

EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFFileManagerContentsOfDirectoryAtPath(EFFileManagerRef manager, EFStringRef path);
EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFFileManagerContentsOfDirectoryAtURL(EFFileManagerRef manager, EFURLRef url);

EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFFileManagerFilesOfDirectoryAtPath(EFFileManagerRef manager, EFStringRef path);
EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFFileManagerFilesOfDirectoryAtURL(EFFileManagerRef manager, EFURLRef url);

#endif /* EFFILEMANAGER_H */
