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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <EmexFoundation/EFPage.h>
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/runtime/EFBase.h>

typedef struct __EFPageGroup *EFPageGroupRef;

EFTypeID EFPageGroupGetTypeID(void);

EFPageGroupRef EFPageGroupCreate(EFAllocatorRef allocatorRef);
EFPageGroupRef EFPageGroupCreateWithPage(EFAllocatorRef allocatorRef, EFPageRef pageRef);
EFPageGroupRef EFPageGroupCreateWithPages(EFAllocatorRef allocatorRef, EFArrayRef pagesArrayRef);
EFPageGroupRef EFPageGroupCreateCopy(EFAllocatorRef allocatorRef, EFPageGroupRef groupRef);

EFArrayRef EFPageGroupCopyPages(EFAllocatorRef allocatorRef, EFPageGroupRef groupRef);

EFIndex EFPageGroupGetLength(EFPageGroupRef groupRef);
Boolean EFPageGroupExtend(EFPageGroupRef groupRef);
Boolean EFPageGroupMerge(EFPageGroupRef groupRef);

EFIndex EFPageGroupWrite(EFPageGroupRef groupRef, EFIndex off, const UInt8 *b, EFIndex length);
EFIndex EFPageGroupRead(EFPageGroupRef groupRef, EFIndex off, UInt8 *b, EFIndex length);

#endif /* EFPAGE_GROUP_H */
