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

#ifndef EFOBJECT_H
#define EFOBJECT_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFBase.h>

typedef struct {
    /*
     * the root type telling the runtime
     * what flavour the object is.
     */
    EFRootType _rt;

    /*
     * the typeID of the class of that
     * object, similar to CFRuntime.
     */
    EFTypeID typeID;

    /* self explainatory */
    EFAllocatorRef allocatorRef;

    /*
     * reference count of an object if
     * it hits zero it will free
     * automatically.
     */
    _Atomic EFIndex refcount;
} EFObject;

#endif /* EFOBJECT_H */
