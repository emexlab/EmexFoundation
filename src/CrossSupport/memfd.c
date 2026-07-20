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
#include <EmexFoundation/CrossSupport/memfd.h>

#ifdef __APPLE__

int memfd_create(const char *name,
                 unsigned int flags)
{
    /* probably doesn't work on iOS, on iOS we'll have to fallback to a temporary file */
    char shm_name[32];
    snprintf(shm_name, sizeof(shm_name), "/mfd_%d_%p", getpid(), (void*)name);
    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, 0600);
    if(fd < 0)
    {
        return -1;
    }
    shm_unlink(shm_name);
    return fd;
}

#endif /* __APPLE__ */
