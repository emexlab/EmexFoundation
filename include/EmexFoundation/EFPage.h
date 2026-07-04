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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>

typedef struct __EFPage *EFPageRef;

EFIndex __EFPageGetPageLength(void);

EFTypeID EFPageGetTypeID(void);

EFPageRef EFPageCreate(EFAllocatorRef allocatorRef);
EFPageRef EFPageCreateWithOptions(EFAllocatorRef allocatorRef, void *addr, size_t length, int prot, int flags, int fd, off_t offset);

EFIndex EFPageGetLength(EFPageRef pageRef);
void *EFPageGetPtr(EFPageRef pageRef);

#endif /* EFPAGE_H */
