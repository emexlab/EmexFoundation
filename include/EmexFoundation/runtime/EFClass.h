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

#include <EmexFoundation/runtime/EFBase.h>

#define EF_MAX_CLASSES  1024

typedef struct {
    /* properties  */
    const char *name;
    EFTypeID typeID;

    /* handlers */
    evobject_init_handler_t init;
    evobject_deinit_handler_t deinit;
    evobject_equal_handler_t equal;
    evobject_copy_description_handler_t copyDescription;
} EFClass;

#endif /* EFCLASS_H */
