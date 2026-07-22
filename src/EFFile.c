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

static inline int __EFFilePolicyToORW(EFFilePolicyPermission p)
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

/*
static inline int __EFFilePolicyToProt(EFFilePolicyPermission p)
{
    int prot = PROT_NONE;
    prot |= ((p & kEFFilePolicyPermissionRead) ? PROT_READ : PROT_NONE);
    prot |= ((p & kEFFilePolicyPermissionWrite) ? PROT_WRITE : PROT_NONE);
    prot |= ((p & kEFFilePolicyPermissionExecute) ? PROT_EXEC : PROT_NONE);
    return prot;
}
*/

typedef struct __EFFile {
    EFObject header;
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

EFClass EFFileClass = {
    .name = "EFFile",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFBitWalkerDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
};

static void EFFileRegisterClass(void)
{
    EFClassRegister(&EFFileClass);
}

EFTypeID EFFileGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFFileRegisterClass);
    return EFFileClass.typeID;
}

EFFileRef __EFFileCreate(EFAllocatorRef allocatorRef,
                         EFFilePolicy policy,
                         EFURLRef urlRef,
                         Boolean care_about_file_exist_policy)
{
    if(urlRef == NULL)
    {
        return NULL;
    }

    EFAUTOREL __EFFile file = (__EFFile)EFObjectCreate(allocatorRef, EFFileGetTypeID(), (EFIndex)sizeof(struct __EFFile));
    if(file == NULL)
    {
        return NULL;
    }

    file->url = EFRetain(urlRef);
    if(file->url == NULL)
    {
        return NULL;
    }

    file->policy = policy;

    EFURLType urlType = EFURLGetType(urlRef);
    EFAUTOREL EFStringRef path = EFURLCopyPath(allocatorRef, file->url);
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

    return (EFFileRef)EFAUTOTRANSFER(file);
}

EFFileRef EFFileCreateWithPath(EFAllocatorRef allocatorRef,
                               EFFilePolicy policy,
                               EFStringRef stringRef)
{
    EFAUTOREL EFURLRef urlRef = EFURLCreateWithString(allocatorRef, stringRef);
    return __EFFileCreate(allocatorRef, policy, urlRef, true);
}

EFFileRef EFFileCreateWithURL(EFAllocatorRef allocatorRef,
                              EFFilePolicy policy,
                              EFURLRef urlRef)
{
    return __EFFileCreate(allocatorRef, policy, urlRef, true);
}

EFFileRef EFFileCreateWithString(EFAllocatorRef allocatorRef,
                                 EFFilePolicy policy,
                                 EFURLRef urlRef,
                                 EFStringRef stringRef)
{
    EFAUTOREL __EFFile file = (__EFFile)__EFFileCreate(allocatorRef, policy, urlRef, false);
    if(file == NULL)
    {
        return NULL;
    }

    /* setting unsaved values */
    file->fileHandle = EFFileHandleCreate(allocatorRef);
    if(file->fileHandle == NULL)
    {
        return NULL;
    }

    /* TODO: shall be validated (wasn't even validated in original C version of emex_file_t) */
    EFFileHandleWrite(file->fileHandle, (const UInt8*)EFStringGetCStringPtr(stringRef, kEFStringEncodingUTF8), EFStringGetLength(stringRef));
    EFFileHandleSeek(file->fileHandle, 0, kEFFileHandleSeekTypeSet);

    return (EFFileRef)EFAUTOTRANSFER(file);
}

Boolean EFFileOpen(EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
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
    file->fileHandle = EFFileHandleCreateWithURLAndOptions(EFGetAllocator(fileRef), file->url, __EFFilePolicyToORW(file->policy.neededPermission) | (file->policy.createOnOpen ? (O_CREAT | O_TRUNC) : 0), 0755);
    if(file->fileHandle == NULL)
    {
        return false;
    }

    return true;
}

void EFFileClose(EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL)
    {
        return;
    }

    EFReleaseTry(file->fileHandle);
    file->fileHandle = NULL;
}

EFFileHandleRef EFFileCopyFileHandle(EFAllocatorRef allocatorRef,
                                     EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL || !EFFileOpen(fileRef))
    {
        return NULL;
    }
    return EFFileHandleCreateCopy(allocatorRef, file->fileHandle);
}

EFBitWalkerRef EFFileCopyBitWalker(EFAllocatorRef allocatorRef,
                                   EFFileRef fileRef,
                                   EFEndian endian)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL || !EFFileOpen(fileRef))
    {
        return NULL;
    }
    return EFBitWalkerCreateWithHandle(allocatorRef, file->fileHandle, endian);
}

EFDataRef EFFileCopyData(EFAllocatorRef allocatorRef,
                         EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL || !EFFileOpen(fileRef))
    {
        return NULL;
    }

    EFIndex length = EFFileHandleGetLength(file->fileHandle);
    UInt8 *buffer = EFAllocatorAllocate(allocatorRef, length, 0);
    if(buffer == NULL)
    {
        return NULL;
    }

    EFFileHandleSeek(file->fileHandle, 0, kEFFileHandleSeekTypeSet);
    EFIndex readLength = EFFileHandleRead(file->fileHandle, buffer, length);
    if(length > readLength)
    {
        EFAllocatorDeallocate(allocatorRef, buffer);
        return NULL;
    }

    EFDataRef data = EFDataCreateWithBuffer(allocatorRef, buffer, length);
    EFAllocatorDeallocate(allocatorRef, buffer);
    return data;
}

EFFileType EFFileGetType(EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL)
    {
        return kEFFileTypeUnknown;
    }
    return file->type;
}

static inline const char *get_extension(const char *path)
{
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;

    const char *dot = strrchr(base, '.');
    if(!dot || dot == base)
    {
        return "";
    }
    return dot + 1;
}

EFFileType EFFileTypeForPath(EFStringRef pathRef,
                             Boolean mustExist)
{
    const char *path = EFStringGetCStringPtr(pathRef, kEFStringEncodingUTF8);
    if(path == NULL)
    {
        /* don't know?! */
        return kEFFileTypeUnknown;
    }

    struct stat st;
    if(stat(path, &st) != 0)
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
        /* needs SERIOUS OPTIMIZATION */
        const char *extension = get_extension(path);
        if(strcmp("e64", extension) == 0)
        {
            return kEFFileTypeAssembly;
        }
        else if(strcmp("e64inc", extension) == 0)
        {
            return kEFFileTypeAssemblyIncludations;
        }
        else if(strcmp("c", extension) == 0)
        {
            return kEFFileTypeC;
        }
        else if(strcmp("h", extension) == 0)
        {
            return kEFFileTypeCHeader;
        }
        else if(strcmp("cpp", extension) == 0 ||
                strcmp("cxx", extension) == 0 ||
                strcmp("cc", extension) == 0)
        {
            return kEFFileTypeCXX;
        }
        else if(strcmp("hpp", extension) == 0)
        {
            return kEFFileTypeCXXHeader;
        }
        else if(strcmp("m", extension) == 0)
        {
            return kEFFileTypeObjC;
        }
        else if(strcmp("mm", extension) == 0)
        {
            return kEFFileTypeObjCXX;
        }
        else if(strcmp("o", extension) == 0)
        {
            return kEFFileTypeObject;
        }
    }

    /* couldn't resolve file type lol */
    return kEFFileTypeUnknown;
}

void EFFileUnlink(EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL)
    {
        return;
    }

    EFAUTOREL EFStringRef path = EFURLCopyPath(EFGetAllocator(fileRef), file->url);
    unlink(EFStringGetCStringPtr(path, kEFStringEncodingUTF8));
}

EFURLRef EFFileGetURL(EFFileRef fileRef)
{
    __EFFile file = (__EFFile)fileRef;
    if(file == NULL)
    {
        return NULL;
    }
    return file->url;
}
