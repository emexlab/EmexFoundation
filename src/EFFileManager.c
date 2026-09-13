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
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFFileManager.h>

struct __EFFileManager {
    EFObject super;
};

EF_HIDDEN EFClassDefinitionNewest EFFileManagerClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDFileManager,
        .name = EFSTR_FILESCOPE("EFFileManager"),
    },
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

EFTypeID EFFileManagerGetTypeID(void)
{
    return kEFTypeIDFileManager;
}

EFFileManagerRef EFFileManagerCreate(EFAllocatorRef allocator)
{
    EFFileManagerRef fileManager = (EFFileManagerRef)EFObjectCreate(allocator, EFFileManagerGetTypeID(), (EFIndex)sizeof(struct __EFFileManager));
    if(fileManager == NULL)
    {
        return NULL;
    }
    return fileManager;
}

Boolean EFFileManagerFileExistsAtPath(EFFileManagerRef manager,
                                      EFStringRef path,
                                      Boolean *isDirectory)
{
    if(manager == NULL || path == NULL)
    {
        return false;
    }

    /* we need it's c buffer */
    char cPath[PATH_MAX];
    if(!EFStringGetCString(path, cPath, PATH_MAX, kEFStringEncodingUTF8))
    {
        return false;
    }

    /* proves it's existence and if it is a directory */
    struct stat fdstat;
    if(stat(cPath, &fdstat) != 0)
    {
        return false;
    }

    /* writing back */
    if(isDirectory)
    {
        *isDirectory = S_ISDIR(fdstat.st_mode);
    }
    
    return true;
}

Boolean EFFileManagerFileExistsAtURL(EFFileManagerRef manager,
                                     EFURLRef url,
                                     Boolean *isDirectory)
{
    if(manager == NULL || url == NULL)
    {
        return false;
    }

    EFStringRef path = EFURLGetPath(url);
    if(path == NULL)
    {
        return false;
    }

    return EFFileManagerFileExistsAtPath(manager, path, isDirectory);
}

Boolean EFFileManagerIsReadableAtPath(EFFileManagerRef manager,
                                      EFStringRef path)
{
    if(manager == NULL || path == NULL)
    {
        return false;
    }

    /* we need it's c buffer for the access() call */
    char cPath[PATH_MAX];
    if(!EFStringGetCString(path, cPath, PATH_MAX, kEFStringEncodingUTF8))
    {
        return false;
    }

    return access(cPath, R_OK) == 0;
}

Boolean EFFileManagerIsWritableAtPath(EFFileManagerRef manager,
                                      EFStringRef path)
{
    if(manager == NULL || path == NULL)
    {
        return false;
    }

    /* we need it's c buffer for the access() call */
    char cPath[PATH_MAX];
    if(!EFStringGetCString(path, cPath, PATH_MAX, kEFStringEncodingUTF8))
    {
        return false;
    }

    return access(cPath, W_OK) == 0;
}

Boolean EFFileManagerIsExecutableAtPath(EFFileManagerRef manager,
                                        EFStringRef path)
{
    if(manager == NULL || path == NULL)
    {
        return false;
    }

    /* we need it's c buffer for the access() call */
    char cPath[PATH_MAX];
    if(!EFStringGetCString(path, cPath, PATH_MAX, kEFStringEncodingUTF8))
    {
        return false;
    }

    return access(cPath, X_OK) == 0;
}

Boolean EFFileManagerIsReadableAtURL(EFFileManagerRef manager,
                                     EFURLRef url)
{
    if(manager == NULL || url == NULL)
    {
        return false;
    }

    EFStringRef path = EFURLGetPath(url);
    if(path == NULL)
    {
        return false;
    }

    return EFFileManagerIsReadableAtPath(manager, path);
}

Boolean EFFileManagerIsWritableAtURL(EFFileManagerRef manager,
                                     EFURLRef url)
{
    if(manager == NULL || url == NULL)
    {
        return false;
    }

    EFStringRef path = EFURLGetPath(url);
    if(path == NULL)
    {
        return false;
    }

    return EFFileManagerIsWritableAtPath(manager, path);
}

Boolean EFFileManagerIsExecutableAtURL(EFFileManagerRef manager,
                                       EFURLRef url)
{
    if(manager == NULL || url == NULL)
    {
        return false;
    }

    EFStringRef path = EFURLGetPath(url);
    if(path == NULL)
    {
        return false;
    }

    return EFFileManagerIsExecutableAtPath(manager, path);
}

EFArrayRef EFFileManagerContentsOfDirectoryAtPath(EFFileManagerRef manager,
                                                  EFStringRef path)
{
    if(manager == NULL || path == NULL)
    {
        return NULL;
    }

    /* we need it's c buffer for the opendir() call */
    char cPath[PATH_MAX];
    if(!EFStringGetCString(path, cPath, PATH_MAX, kEFStringEncodingUTF8))
    {
        return NULL;
    }

    DIR *dir = opendir(cPath);
    if(dir == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef contents = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, 0);
    if(contents == NULL)
    {
        closedir(dir);
        return NULL;
    }

    struct dirent *direntry = readdir(dir);
    while(direntry != NULL)
    {
        EFStringRef entryName = EFStringCreateWithCString(kEFAllocatorDefault, direntry->d_name, kEFStringEncodingUTF8);
        if(entryName == NULL)
        {
            continue;
        }

        /* just append, if it doesn't succeed it doesn't create a new reference */
        EFArrayAppendValue(contents, entryName);
        EFRelease(entryName);

        direntry = readdir(dir);
    }

    return EFAUTOTRANSFER(contents);
}

EFArrayRef EFFileManagerContentsOfDirectoryAtURL(EFFileManagerRef manager,
                                                 EFURLRef url)
{
    if(manager == NULL || url == NULL)
    {
        return NULL;
    }

    EFStringRef path = EFURLGetPath(url);
    if(path == NULL)
    {
        return NULL;
    }

    /* we need it's c buffer for the opendir() call */
    char cPath[PATH_MAX];
    if(!EFStringGetCString(path, cPath, PATH_MAX, kEFStringEncodingUTF8))
    {
        return NULL;
    }

    DIR *dir = opendir(cPath);
    if(dir == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef contents = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, 0);
    if(contents == NULL)
    {
        closedir(dir);
        return NULL;
    }

    struct dirent *direntry = readdir(dir);
    while(direntry != NULL)
    {
        EFStringRef entryName = EFStringCreateWithCString(kEFAllocatorDefault, direntry->d_name, kEFStringEncodingUTF8);
        if(entryName == NULL)
        {
            continue;
        }

        EFURLRef entryURL = EFURLCreateByAppendingPathComponent(kEFAllocatorDefault, url, entryName);
        EFRelease(entryName);
        if(entryURL == NULL)
        {
            continue;
        }

        /* just append, if it doesn't succeed it doesn't create a new reference */
        EFArrayAppendValue(contents, entryURL);
        EFRelease(entryURL);

        direntry = readdir(dir);
    }

    return EFAUTOTRANSFER(contents);
}
