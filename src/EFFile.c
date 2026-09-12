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
#include <sys/mman.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFFile.h>
#include <EmexFoundation/EFBitWalker.h>
#include <EmexFoundation/EFURL.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFData.h>

EFFilePolicy EFFilePolicyInData = {
    .neededPermission = kEFFilePolicyPermissionRead,
    .mustExist = true,
    .mustBeAFile = true,
    .createOnOpen = false,
};

EFFilePolicy EFFilePolicyOutData = {
    .neededPermission = kEFFilePolicyPermissionRead | kEFFilePolicyPermissionWrite,
    .mustExist = false,
    .mustBeAFile = true,
    .createOnOpen = true,
};

EFFilePolicy EFFilePolicyInNoCreate = {
    .neededPermission = kEFFilePolicyPermissionRead | kEFFilePolicyPermissionWrite,
    .mustExist = false,
    .mustBeAFile = true,
    .createOnOpen = false,
};

static inline SInt32 __EFFilePolicyToORW(EFFilePolicyPermission p)
{
    if((p & (kEFFilePolicyPermissionRead | kEFFilePolicyPermissionWrite)) == (kEFFilePolicyPermissionRead | kEFFilePolicyPermissionWrite))
    {
        return O_RDWR;
    }
    if(p & kEFFilePolicyPermissionWrite)
    {
        return O_WRONLY;
    }
    return O_RDONLY;
}

typedef struct __EFFile {
    EFObject super;
    EFURLRef url;
    EFFileType type;
    EFFilePolicy policy;
    EFFileHandleRef fileHandle;
} *__EFFile;

static void __EFBitWalkerDeinit(EFObjectRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    EFReleaseTry(file->url);
    EFReleaseTry(file->fileHandle);
}

EF_HIDDEN EFClassDefinitionNewest EFFileClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDFile,
        .name = EFSTR_FILESCOPE("EFFile"),
    },
    .init = NULL,
    .deinit = __EFBitWalkerDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

EFTypeID EFFileGetTypeID(void)
{
    return kEFTypeIDFile;
}

EFFileRef __EFFileCreate(EFAllocatorRef allocator,
                         EFFilePolicy policy,
                         EFURLRef url,
                         Boolean care_about_file_exist_policy)
{
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFFileRef file = (EFFileRef)EFObjectCreate(allocator, EFFileGetTypeID(), (EFIndex)sizeof(struct __EFFile));
    if(file == NULL)
    {
        return NULL;
    }

    file->url = EFRetain(url);
    if(file->url == NULL)
    {
        return NULL;
    }

    file->policy = policy;

    EFURLType urlType = EFURLGetType(url);
    EFStringRef path = EFURLGetPath(file->url);
    file->type = EFFileTypeForPath(path, urlType == kEFURLTypePOSIX && policy.mustExist);
    if(urlType == kEFURLTypePOSIX)
    {
        /*
         * resolving the true paths is important
         * so errors can reveal the actual file
         * locations.
         */
        if(policy.mustExist && care_about_file_exist_policy && access(EFStringGetCStringPtr(path, kEFStringEncodingUTF8), F_OK) != 0)
        {
            return NULL;
        }

        /* setting standard values */
        if(policy.mustBeAFile && file->type == kEFFileTypeDirectory)
        {
            return NULL;
        }
    }

    return EFAUTOTRANSFER(file);
}

EFFileRef EFFileCreateWithPath(EFAllocatorRef allocator,
                               EFFilePolicy policy,
                               EFStringRef string)
{
    EFAUTOREL EFURLRef urlRef = EFURLCreateWithString(allocator, string);
    return __EFFileCreate(allocator, policy, urlRef, true);
}

EFFileRef EFFileCreateWithURL(EFAllocatorRef allocator,
                              EFFilePolicy policy,
                              EFURLRef urlRef)
{
    return __EFFileCreate(allocator, policy, urlRef, true);
}

EFFileRef EFFileCreateUnsavedWithString(EFAllocatorRef allocator,
                                        EFFilePolicy policy,
                                        EFURLRef urlRef,
                                        EFStringRef string)
{
    EFAUTOREL EFFileRef file = (EFFileRef)__EFFileCreate(allocator, policy, urlRef, false);
    if(file == NULL)
    {
        return NULL;
    }

    /* setting unsaved values */
    file->fileHandle = EFFileHandleCreate(allocator);
    if(file->fileHandle == NULL)
    {
        return NULL;
    }

    /* TODO: shall be validated (wasn't even validated in original C version of emex_file_t) */
    EFFileHandleWrite(file->fileHandle, (const UInt8*)EFStringGetCStringPtr(string, kEFStringEncodingUTF8), EFStringGetLength(string));
    EFFileHandleSeek(file->fileHandle, 0, kEFFileHandleSeekTypeSet);

    return EFAUTOTRANSFER(file);
}

Boolean EFFileOpen(EFFileRef file)
{
    if(file == NULL)
    {
        return false;
    }

    if(file->fileHandle != NULL)
    {
        return true;
    }

    if(file->type == kEFFileTypeDirectory)
    {
        return false;
    }

    /* initial open */
    file->fileHandle = EFFileHandleCreateWithURLAndOptions(EFGetAllocator(file), file->url, __EFFilePolicyToORW(file->policy.neededPermission) | (file->policy.createOnOpen ? (O_CREAT | O_TRUNC) : 0), 0755);
    if(file->fileHandle == NULL)
    {
        return false;
    }

    return true;
}

void EFFileClose(EFFileRef file)
{
    if(file == NULL)
    {
        return;
    }

    EFReleaseTry(file->fileHandle);
    file->fileHandle = NULL;
}

EFFileHandleRef EFFileCopyFileHandle(EFAllocatorRef allocatorRef,
                                     EFFileRef file)
{
    if(file == NULL || !EFFileOpen(file))
    {
        return NULL;
    }

    return EFFileHandleCreateCopy(allocatorRef, file->fileHandle);
}

EFBitWalkerRef EFFileCopyBitWalker(EFAllocatorRef allocator,
                                   EFFileRef file,
                                   EFEndian endian)
{
    if(file == NULL || !EFFileOpen(file))
    {
        return NULL;
    }

    return EFBitWalkerCreateWithHandle(allocator, file->fileHandle, endian);
}

EFDataRef EFFileCopyData(EFAllocatorRef allocator,
                         EFFileRef file)
{
    if(file == NULL || !EFFileOpen(file))
    {
        return NULL;
    }

    EFIndex length = EFFileHandleGetLength(file->fileHandle);
    UInt8 *buffer = EFAllocatorAllocate(allocator, length, 0);
    if(buffer == NULL)
    {
        return NULL;
    }

    EFFileHandleSeek(file->fileHandle, 0, kEFFileHandleSeekTypeSet);
    EFIndex readLength = EFFileHandleRead(file->fileHandle, buffer, length);
    if(length > readLength)
    {
        EFAllocatorDeallocate(allocator, buffer);
        return NULL;
    }

    EFDataRef data = EFDataCreateWithBuffer(allocator, buffer, length);
    EFAllocatorDeallocate(allocator, buffer);
    return data;
}

EFFileType EFFileGetType(EFFileRef file)
{
    if(file == NULL)
    {
        return kEFFileTypeUnknown;
    }

    return file->type;
}

EFFileType EFFileTypeForPath(EFStringRef path,
                             Boolean mustExist)
{
    if(path == NULL)
    {
        /* don't know?! */
        return kEFFileTypeUnknown;
    }

    struct stat st;
    if(stat(EFStringGetCStringPtr(path, kEFStringEncodingUTF8), &st) != 0)
    {
        if(!mustExist)
        {
            goto extension_validation;
        }

        return kEFFileTypeUnknown;
    }

    if(S_ISDIR(st.st_mode))
    {
        return kEFFileTypeDirectory;
    }
    else if(S_ISREG(st.st_mode))
extension_validation:
    {
        if(EFStringHasSuffix(path, EFSTR(".e64")))
        {
            return kEFFileTypeAssembly;
        }
        else if(EFStringHasSuffix(path, EFSTR(".e64inc")))
        {
            return kEFFileTypeAssemblyIncludations;
        }
        else if(EFStringHasSuffix(path, EFSTR(".c")))
        {
            return kEFFileTypeC;
        }
        else if(EFStringHasSuffix(path, EFSTR(".h")))
        {
            return kEFFileTypeCHeader;
        }
        else if(EFStringHasSuffix(path, EFSTR(".cpp")) ||
                EFStringHasSuffix(path, EFSTR(".cxx")) ||
                EFStringHasSuffix(path, EFSTR(".cc")))
        {
            return kEFFileTypeCXX;
        }
        else if(EFStringHasSuffix(path, EFSTR(".hpp")))
        {
            return kEFFileTypeCXXHeader;
        }
        else if(EFStringHasSuffix(path, EFSTR(".m")))
        {
            return kEFFileTypeObjC;
        }
        else if(EFStringHasSuffix(path, EFSTR(".mm")))
        {
            return kEFFileTypeObjCXX;
        }
        else if(EFStringHasSuffix(path, EFSTR(".o")))
        {
            return kEFFileTypeObject;
        }
    }

    /* couldn't resolve file type lol */
    return kEFFileTypeUnknown;
}

void EFFileUnlink(EFFileRef file)
{
    if(file == NULL)
    {
        return;
    }

    unlink(EFStringGetCStringPtr(EFURLGetPath(file->url), kEFStringEncodingUTF8));
}

EFURLRef EFFileGetURL(EFFileRef file)
{
    if(file == NULL)
    {
        return NULL;
    }

    return file->url;
}
