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

#ifndef EFRUNTIME_H
#define EFRUNTIME_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>
#include <EmexFoundation/runtime/EFClass.h>
#include <EmexFoundation/runtime/EFObject.h>

/* releases the object when it's out of scope */
#define EFAUTOREL __attribute__((cleanup(EFReleaseTryHelper)))

EFTypeID EFClassRegister(EFClass *cls);
EFClass *EFClassGetByID(EFTypeID id);

EFTypeID EFGetTypeID(EFObjectRef ref);
Boolean EFEqual(EFObjectRef ref1, EFObjectRef ref2);

EFObjectRef EFRetain(EFObjectRef ref);
void EFRelease(EFObjectRef ref);
EFObjectRef EFRetainTry(EFObjectRef ref);
Boolean EFReleaseTry(EFObjectRef ref);
Boolean EFReleaseTryHelper(void *ref);
EFIndex EFGetRetainCount(EFObjectRef ref);

EFAllocatorRef EFGetAllocator(EFObjectRef ref);

EFStringRef EFCopyDescription(EFObjectRef ref);

void EFLog(EFStringRef formatStringRef, ...);

#endif /* EFRUNTIME_H */
