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

#ifndef EFCLASS_H
#define EFCLASS_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFBase.h>

#define EF_MAX_CLASSES  1024

typedef struct {
    /* properties  */
    const char *name;
    EFTypeID typeID;

    /* handlers */
    EFObjectInitCallback init;
    EFObjectDeinitCallback deinit;
    EFObjectEqualCallback equal;
    EFObjectCopyDescriptionCallback copyDescription;
    EFObjectHashCallback hash;
} EFClass;

#endif /* EFCLASS_H */
