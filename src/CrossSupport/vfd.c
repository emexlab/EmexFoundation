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

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/CrossSupport/vfd.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFUUID.h>

SInt32 vfd_create(UInt32 flags)
{
    /* creates file descriptor that "lives in memory" */
    EFAUTOREL EFUUIDRef uuid = EFUUIDCreate(kEFAllocatorDefault);
    EFAUTOREL EFStringRef string = EFUUIDCreateString(kEFAllocatorDefault, uuid);
    EFAUTOREL EFStringRef pathStr = EFStringCreateWithFormat(kEFAllocatorDefault, EFSTR("%s/%@"), getenv("TMPDIR"), string);
    const char *pathStrC = EFStringGetCStringPtr(pathStr, kEFStringEncodingUTF8);
    SInt32 fileDescriptor = open(pathStrC, flags | O_CREAT | O_TRUNC, 0777);
    unlink(pathStrC);   /* unlinking immediately keeps it in memory */
    return fileDescriptor;
}
