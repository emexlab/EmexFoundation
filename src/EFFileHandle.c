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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFPageGroup.h>

typedef struct __EFFileHandle {
    EFObject header;
    int flg;
    Boolean readable;
    Boolean writable;
    Boolean isBackedByFileDescriptor;

    union {
        int fileDescriptor;

        struct {
            EFIndex offset;
            EFIndex endOffset;  /* last offset written to */
            EFPageGroupRef pageGroupRef;
        } virtualFileDescriptor;
    };
} *__EFFileHandle;

static void __EVFileHandleDeinit(EFObjectRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle->isBackedByFileDescriptor)
    {
        close(fileHandle->fileDescriptor);
    }
    else if(fileHandle->virtualFileDescriptor.pageGroupRef != NULL)
    {
        EFRelease(fileHandle->virtualFileDescriptor.pageGroupRef);
    }
}

static EFClass EFFileHandleClass = {
    .name = "EFFileHandle",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EVFileHandleDeinit,
    .equal = NULL,
    .copyDescription = NULL,
    .hash = NULL,
};

static void EFFileHandleRegisterClass(void)
{
    EFClassRegister(&EFFileHandleClass);
}

EFTypeID EFFileHandleGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFFileHandleRegisterClass);
    return EFFileHandleClass.typeID;
}

EFFileHandleRef EFFileHandleCreate(EFAllocatorRef allocatorRef)
{
    EFAUTOREL __EFFileHandle fileHandle = (__EFFileHandle)EFObjectCreate(allocatorRef, EFFileHandleGetTypeID(), (EFIndex)sizeof(struct __EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    fileHandle->fileDescriptor = -1;
    fileHandle->isBackedByFileDescriptor = false;
    fileHandle->virtualFileDescriptor.offset = 0;
    fileHandle->virtualFileDescriptor.endOffset = 0;
    fileHandle->virtualFileDescriptor.pageGroupRef = EFPageGroupCreate(allocatorRef);
    if(fileHandle->virtualFileDescriptor.pageGroupRef == NULL)
    {
        return NULL;
    }

    fileHandle->flg = O_RDWR | O_CREAT | O_TRUNC;   /* some bs flags */
    fileHandle->readable = true;
    fileHandle->writable = true;

    return (EFFileHandleRef)EFAUTOTRANSFER(fileHandle);
}

EFFileHandleRef EFFileHandleCreateWithFileDescriptor(EFAllocatorRef allocatorRef,
                                                     int fd)
{
    fd = dup(fd);
    if(fd < 0)
    {
        return NULL;
    }

    EFAUTOREL __EFFileHandle fileHandle = (__EFFileHandle)EFObjectCreate(allocatorRef, EFFileHandleGetTypeID(), (EFIndex)sizeof(struct __EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    fileHandle->fileDescriptor = fd;
    fileHandle->isBackedByFileDescriptor = true;

    fileHandle->flg = fcntl(fileHandle->fileDescriptor, F_GETFL);
    if(fileHandle->flg == -1)
    {
        return NULL;
    }

    UInt8 access_mode = fileHandle->flg & O_ACCMODE;
    fileHandle->readable = access_mode == O_RDONLY || access_mode == O_RDWR;
    fileHandle->writable = access_mode == O_WRONLY || access_mode == O_RDWR;

    return (EFFileHandleRef)EFAUTOTRANSFER(fileHandle);
}

EFFileHandleRef EFFileHandleCreateWithOptions(EFAllocatorRef allocatorRef,
                                              EFStringRef pathStringRef,
                                              int flg,
                                              ...)
{
    if(pathStringRef == NULL)
    {
        return NULL;
    }

    const char *str = EFStringGetCStringPtr(pathStringRef, kEFStringEncodingASCII);
    if(str == NULL)
    {
        return NULL;
    }

    /* potentially getting mode */
    mode_t mode = 0;
    if(flg & O_CREAT)
    {
        va_list ap;
        va_start(ap, flg);
        mode = va_arg(ap, int);
        va_end(ap);
    }

    /* really opening the file */
    int fd = open(str, flg, mode);
    if(fd < 0)
    {
        return NULL;
    }

    EFFileHandleRef fileHandleRef = EFFileHandleCreateWithFileDescriptor(allocatorRef, fd);
    close(fd);
    return fileHandleRef;
}

EFIndex EFFileHandleRead(EFFileHandleRef fileHandleRef,
                         UInt8 *buffer,
                         EFIndex length)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        return (EFIndex)read(fileHandle->fileDescriptor, buffer, (size_t)length);
    }
    else
    {
        EFIndex vret = EFPageGroupRead(fileHandle->virtualFileDescriptor.pageGroupRef, (size_t)fileHandle->virtualFileDescriptor.offset, buffer, length);
        if(vret > 0)
        {
            fileHandle->virtualFileDescriptor.offset += vret;
        }
        return vret;
    }
    return -1;
}

EFIndex EFFileHandleWrite(EFFileHandleRef fileHandleRef,
                          const UInt8 *buffer,
                          EFIndex length)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        return (EFIndex)write(fileHandle->fileDescriptor, buffer, (size_t)length);
    }
    else
    {
        EFIndex start = fileHandle->virtualFileDescriptor.offset;
        EFIndex endOffset = start + length;

        while(endOffset > EFPageGroupGetLength(fileHandle->virtualFileDescriptor.pageGroupRef))
        {
            if(!EFPageGroupExtend(fileHandle->virtualFileDescriptor.pageGroupRef))
            {
                return -1;
            }
        }

        EFIndex vret = EFPageGroupWrite(fileHandle->virtualFileDescriptor.pageGroupRef, start, buffer, length);
        if(vret < 0)
        {
            return vret;
        }

        fileHandle->virtualFileDescriptor.offset = start + vret;
        if(fileHandle->virtualFileDescriptor.offset > fileHandle->virtualFileDescriptor.endOffset)
        {
            fileHandle->virtualFileDescriptor.endOffset = fileHandle->virtualFileDescriptor.offset;
        }
        return vret;
    }
    return -1;
}

EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandleRef,
                             EFIndex length)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        return (EFIndex)ftruncate(fileHandle->fileDescriptor, length);
    }
    else
    {
        if(length < 0)
        {

            return -1;
        }

        EFIndex newlen = length;
        EFIndex oldlen = fileHandle->virtualFileDescriptor.endOffset;

        /* make sure the backing store can hold the new lenght */
        while(EFPageGroupGetLength(fileHandle->virtualFileDescriptor.pageGroupRef) < newlen)
        {
            if(!EFPageGroupExtend(fileHandle->virtualFileDescriptor.pageGroupRef))
            {
                return -1;
            }
        }

        if(newlen < oldlen)
        {
            EFIndex pageSize = __EFPageGetPageLength();
            uint8_t zeros[pageSize];
            memset(zeros, 0, pageSize);
            EFIndex pos = newlen;
            while(pos < oldlen)
            {
                EFIndex chunk = oldlen - pos;
                if(chunk > (EFIndex)sizeof(zeros))
                {
                    chunk = (EFIndex)sizeof(zeros);
                }
                EFPageGroupWrite(fileHandle->virtualFileDescriptor.pageGroupRef, pos, zeros, chunk);
                pos += chunk;
            }
        }
    }
    return -1;
}

EFIndex EFFileHandleSeek(EFFileHandleRef fileHandleRef,
                         EFIndex offset,
                         EFFileHandleSeekType seekType)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    int a = 0;
    switch(seekType)
    {
        case kEFFileHandleSeekTypeSet:
            a = SEEK_SET;
            break;
        case kEFFileHandleSeekTypeCur:
            a = SEEK_CUR;
            break;
        case kEFFileHandleSeekTypeEnd:
            a = SEEK_END;
            break;
        default:
            return -1;
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        return (EFIndex)lseek(fileHandle->fileDescriptor, offset, a);
    }
    else
    {
        EFIndex base;
        switch(a)
        {
            case SEEK_SET:
                base = 0;
                break;
            case SEEK_CUR:
                base = fileHandle->virtualFileDescriptor.offset;
                break;
            case SEEK_END:
                base = fileHandle->virtualFileDescriptor.endOffset;
                break;
            default:
                return -1;
        }

        EFIndex newOffset;
        if(__builtin_add_overflow(base, offset, &newOffset))
        {
            return -1;
        }
        if(newOffset < 0)
        {
            return -1;
        }

        fileHandle->virtualFileDescriptor.offset = newOffset;
        return newOffset;
    }

    return -1;
}

void EFFileHandleSync(EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return;
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        fsync(fileHandle->fileDescriptor);
    }
}

EFIndex EFFileHandleGetLength(EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        struct stat fdstat;
        if(fstat(fileHandle->fileDescriptor, &fdstat) != 0)
        {
            return -1;
        }
        return fdstat.st_size;
    }
    else
    {
        return fileHandle->virtualFileDescriptor.endOffset;
    }
}

Boolean EFFileHandleIsReadable(EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return false;
    }

    return fileHandle->readable;
}

Boolean EFFileHandleIsWritable(EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return false;
    }

    return fileHandle->writable;
}

EFDataRef EFFileHandleCopyDataForRange(EFAllocatorRef allocatorRef,
                                       EFFileHandleRef fileHandleRef,
                                       EFRange range)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EFGetAllocator(fileHandleRef);
    }

    EFIndex backupPosition = EFFileHandleSeek(fileHandleRef, 0, kEFFileHandleSeekTypeCur);  /* to be restored */
    EFIndex position = EFFileHandleSeek(fileHandleRef, range.location, kEFFileHandleSeekTypeSet);
    if(position != range.location)
    {
        goto out_failed_restore_position;
    }

    EFMutableDataRef mutableDataRef = EFDataCreateMutable(allocatorRef, range.length);
    if(mutableDataRef == NULL)
    {
        goto out_failed_restore_position;
    }

    UInt8 *dataBuffer = EFDataGetMutablePtr(mutableDataRef);
    EFIndex read = EFFileHandleRead(fileHandleRef, dataBuffer, range.length);
    if(read < range.length)
    {
        EFRelease(mutableDataRef);
        goto out_failed_restore_position;
    }

    EFFileHandleSeek(fileHandleRef, backupPosition, kEFFileHandleSeekTypeSet);

    EFDataRef dataRef = EFDataCreateCopy(allocatorRef, mutableDataRef);
    EFRelease(mutableDataRef);
    return dataRef;

out_failed_restore_position:
    EFFileHandleSeek(fileHandleRef, backupPosition, kEFFileHandleSeekTypeSet);
    return NULL;
}

EFPageGroupRef EFFIleHandleCopyPageGroup(EFAllocatorRef allocatorRef,
                                         EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EFGetAllocator(fileHandleRef);
    }

    if(fileHandle->isBackedByFileDescriptor)
    {
        int prot_flg = PROT_NONE;
        prot_flg |= (fileHandle->readable ? PROT_READ : PROT_NONE);
        prot_flg |= (fileHandle->writable ? PROT_WRITE : PROT_NONE);

        EFPageRef pageRef = EFPageCreateWithOptions(allocatorRef, NULL, (size_t)EFFileHandleGetLength(fileHandleRef), prot_flg, MAP_SHARED, fileHandle->fileDescriptor, 0);
        if(pageRef == NULL)
        {
            return NULL;
        }

        EFPageGroupRef groupRef = EFPageGroupCreateWithPage(allocatorRef, pageRef);
        EFRelease(pageRef);
        if(groupRef == NULL)
        {
            return NULL;
        }
        return groupRef;
    }
    else
    {
        return EFPageGroupCreateCopy(allocatorRef, fileHandle->virtualFileDescriptor.pageGroupRef);
    }
}

char *EFFileHandleGets(EFFileHandleRef fileHandle, char *s, int n)
{
    if(s == NULL || n <= 0)
    {
        return NULL;
    }

    if(n == 1)
    {
        s[0] = '\0';
        return s;
    }

    int i = 0;
    while(i < n - 1)
    {
        char c;
        ssize_t r = EFFileHandleRead(fileHandle, (UInt8*)&c, (EFIndex)1);

        if(r < 0)
        {

            return NULL;
        }
        if(r == 0)
        {
            if(i == 0)
            {
                return NULL;
            }
            break;
        }

        s[i++] = c;
        if(c == '\n')
        {
            break;
        }
    }

    s[i] = '\0';
    return s;
}
