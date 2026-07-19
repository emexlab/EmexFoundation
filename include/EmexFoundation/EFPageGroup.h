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

#ifndef EFPAGE_GROUP_H
#define EFPAGE_GROUP_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFPage.h>
#include <EmexFoundation/EFArray.h>

typedef struct __EFPageGroup *EFPageGroupRef;

EF_EXTERN EFTypeID EFPageGroupGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFPageGroupRef EFPageGroupCreate(EFAllocatorRef allocatorRef);
EF_EXTERN EF_RETURNS_RETAINED EFPageGroupRef EFPageGroupCreateWithPage(EFAllocatorRef allocatorRef, EFPageRef pageRef);
EF_EXTERN EF_RETURNS_RETAINED EFPageGroupRef EFPageGroupCreateWithPages(EFAllocatorRef allocatorRef, EFArrayRef pagesArrayRef);
EF_EXTERN EF_RETURNS_RETAINED EFPageGroupRef EFPageGroupCreateCopy(EFAllocatorRef allocatorRef, EFPageGroupRef groupRef);

EF_EXTERN EF_RETURNS_NOT_RETAINED EFArrayRef EFPageGroupGetPages(EFPageGroupRef groupRef);

EF_EXTERN EFIndex EFPageGroupGetLength(EFPageGroupRef groupRef);
EF_EXTERN Boolean EFPageGroupExtend(EFPageGroupRef groupRef);
EF_EXTERN Boolean EFPageGroupMerge(EFPageGroupRef groupRef);

EF_EXTERN EFIndex EFPageGroupWrite(EFPageGroupRef groupRef, EFIndex off, const UInt8 *b, EFIndex length);
EF_EXTERN EFIndex EFPageGroupRead(EFPageGroupRef groupRef, EFIndex off, UInt8 *b, EFIndex length);

EF_EXTERN EFIndex EFPageGroupGetLastWrittenOffset(EFPageGroupRef groupRef);

#endif /* EFPAGE_GROUP_H */
