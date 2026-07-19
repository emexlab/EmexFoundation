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

#ifndef EFPAGE_H
#define EFPAGE_H

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdio.h>
#include <stddef.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>

typedef struct __EFPage *EFPageRef;

EF_EXTERN EFIndex __EFPageGetPageLength(void);

EF_EXTERN EFTypeID EFPageGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFPageRef EFPageCreate(EFAllocatorRef allocatorRef);
EF_EXTERN EF_RETURNS_RETAINED EFPageRef EFPageCreateWithOptions(EFAllocatorRef allocatorRef, void *addr, size_t length, int prot, int flags, int fd, off_t offset);

EF_EXTERN EFIndex EFPageGetLength(EFPageRef pageRef);
EF_EXTERN void *EFPageGetPtr(EFPageRef pageRef);

#endif /* EFPAGE_H */
