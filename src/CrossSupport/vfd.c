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
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/CrossSupport/vfd.h>

SInt32 vfd_create(const char *name,
                  UInt32 flags)
{
    /* need to fix the flags */
    SInt32 accessMode = (flags & O_ACCMODE);
    SInt32 fileDescriptor = -1;

    /*
#if defined(__linux__) || defined(__FreeBSD__)
     * first method thing is memfd_create *
    fileDescriptor = memfd_create(name, accessMode);
    if(fileDescriptor >= 0)
    {
        return fileDescriptor;
    }
#endif * __linux__ || __FreeBSD__ *
    */

    /* fallback method */
    char tempBuf[PATH_MAX];
    strncpy(tempBuf, name, PATH_MAX);

    const char *tempFile = mktemp(tempBuf);
    if(tempFile == NULL)
    {
        return -1;
    }

    fileDescriptor = open(tempFile, accessMode | O_CREAT | O_TRUNC, 0777);
    unlink(tempFile);   /* unlinking immediately keeps it in memory */
    return fileDescriptor;
}
