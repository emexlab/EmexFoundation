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
#define EFAUTOTRANSFER(var) \
    __extension__ ({ \
        __typeof__(var) _temp = (var); \
        (var) = NULL; \
        _temp; \
    })

extern EFTypeID EFClassRegister(EFClass *cls);
extern EFClass *EFClassGetByID(EFTypeID id);

extern EFTypeID EFGetTypeID(EFObjectRef ref);
extern EFRootType EFGetRootType(EFObjectRef ref);

extern Boolean EFEqual(EFObjectRef ref1, EFObjectRef ref2);

extern EFObjectRef EFRetain(EFObjectRef ref);
extern void EFRelease(EFObjectRef ref);
extern EFObjectRef EFRetainTry(EFObjectRef ref);
extern Boolean EFReleaseTry(EFObjectRef ref);
extern Boolean EFReleaseTryHelper(void *ref);
extern EFIndex EFGetRetainCount(EFObjectRef ref);

extern EFAllocatorRef EFGetAllocator(EFObjectRef ref);

extern EFStringRef EFCopyDescription(EFObjectRef ref);

extern void EFLog(EFStringRef formatStringRef, ...);

#endif /* EFRUNTIME_H */
