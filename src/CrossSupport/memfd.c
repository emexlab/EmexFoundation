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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <EmexFoundation/CrossSupport/memfd.h>

#ifdef __APPLE__

int memfd_create(const char *name,
                 unsigned int flags)
{
    char tempBuf[PATH_MAX];
    strncpy(tempBuf, name, PATH_MAX);

    const char *tempFile = mktemp(tempBuf);
    if(tempFile == NULL)
    {
        return -1;
    }

    int fd = open(tempFile, flags, 0777);
    unlink(tempFile);   /* unlinking immediately keeps it in memory */
    return fd;
}

#endif /* __APPLE__ */
