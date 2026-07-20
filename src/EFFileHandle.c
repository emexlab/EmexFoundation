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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netdb.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFPageGroup.h>

typedef struct __EFFileHandle {
    EFObject header;
    int flg;
    Boolean readable;
    Boolean writable;
    EFFileHandleType type;

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
    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            close(fileHandle->fileDescriptor);
            break;
        case kEFFileHandleTypeVirtual:
            EFRelease(fileHandle->virtualFileDescriptor.pageGroupRef);
            [[fallthrough]];
        default:
            break;
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
    fileHandle->type = kEFFileHandleTypeVirtual;
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
    fileHandle->type = kEFFileHandleTypeBSD;

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

EFFileHandleRef EFFileHandleCreateWithPathAndOptions(EFAllocatorRef allocatorRef,
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

static EFFileHandleRef __EFFileHandleCreateNetDesc(EFAllocatorRef allocatorRef,
                                                   EFURLRef urlRef)
{
    EFArrayRef pathComponents = EFURLGetPathComponents(urlRef);
    if(pathComponents == NULL || EFArrayGetCount(pathComponents) <= 0)
    {
        return NULL;
    }

    EFAUTOREL EFStringRef path = EFURLCopyPathWithoutHostname(allocatorRef, urlRef);
    const char *pathCPtr = EFStringGetCStringPtr(path, kEFStringEncodingUTF8);
    const char *hostNameCPtr = EFStringGetCStringPtr(EFArrayGetValueAtIndex(pathComponents, 0), kEFStringEncodingUTF8);
    if(pathCPtr == NULL || hostNameCPtr == NULL)
    {
        return NULL;
    }

    FILE *sslPipe = NULL;
    int sockfd = -1;
    EFURLType urlType = EFURLGetType(urlRef);
    switch(urlType)
    {
        case kEFURLTypeHTTP:
            struct addrinfo hints, *res;

            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            if(getaddrinfo(hostNameCPtr, "80", &hints, &res) != 0)
            {
                return NULL;
            }

            sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if(sockfd != -1)
            {
                if(connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
                {
                    close(sockfd);
                    sockfd = -1;
                }
            }

            freeaddrinfo(res);
            if(sockfd == -1)
            {
                return NULL;
            }

            char request[512];
            snprintf(request, sizeof(request),
                "GET /%s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: close\r\n\r\n",
                pathCPtr, hostNameCPtr);

            if(write(sockfd, request, strlen(request)) < 0)
            {
                close(sockfd);
                return NULL;
            }
            break;
        case kEFURLTypeHTTPS:
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "curl -s -i \"https://%s/%s\"", hostNameCPtr, pathCPtr);

            sslPipe = popen(cmd, "r");
            if(sslPipe == NULL)
            {
                return NULL;
            }

            sockfd = fileno(sslPipe);
            break;
        case kEFURLTypePOSIX:
        default:
            return NULL;
    }

    char c;
    int state = 0;
    while(read(sockfd, &c, 1) == 1)
    {
        if(c == '\r' && state == 0)
        {
            state = 1;
        }
        else if(c == '\n' && state == 1)
        {
            state = 2;
        }
        else if(c == '\r' && state == 2)
        {
            state = 3;
        }
        else if(c == '\n' && state == 3)
        {
            break;
        }
        else
        {
            state = 0;
        }
    }

    EFFileHandleRef virtualHandle = EFFileHandleCreate(allocatorRef);
    if(!virtualHandle)
    {
        if(sslPipe)
        {
            pclose(sslPipe);
        }
        else
        {
            close(sockfd);
        }
        return NULL;
    }

    /* need a run-loop later! */
    UInt8 downloadBuffer[4096];
    ssize_t networkBytesRead;
    while((networkBytesRead = read(sockfd, downloadBuffer, sizeof(downloadBuffer))) > 0)
    {
        EFFileHandleWrite(virtualHandle, downloadBuffer, (EFIndex)networkBytesRead);
    }

    if(sslPipe)
    {
        pclose(sslPipe);
    }
    else
    {
        close(sockfd);
    }
    EFFileHandleSeek(virtualHandle, 0, kEFFileHandleSeekTypeSet);

    return virtualHandle;
}

EFFileHandleRef EFFileHandleCreateWithURLAndOptions(EFAllocatorRef allocatorRef,
                                                    EFURLRef urlRef,
                                                    int flg,
                                                    ...)
{
    if(urlRef == NULL)
    {
        return NULL;
    }

    if(EFURLGetType(urlRef) == kEFURLTypeHTTP || EFURLGetType(urlRef) == kEFURLTypeHTTPS)
    {
        return __EFFileHandleCreateNetDesc(allocatorRef, urlRef);
    }

    EFAUTOREL EFStringRef string = EFURLCopyPath(allocatorRef, urlRef);
    const char *str = EFStringGetCStringPtr(string, kEFStringEncodingASCII);
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

EFFileHandleRef EFFileHandleCreateCopy(EFAllocatorRef allocatorRef,
                                       EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return NULL;
    }

    EFAUTOREL __EFFileHandle newFileHandle = (__EFFileHandle)EFObjectCreate(allocatorRef, EFFileHandleGetTypeID(), (EFIndex)sizeof(struct __EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    newFileHandle->flg = fileHandle->flg;
    newFileHandle->readable = fileHandle->readable;
    newFileHandle->writable = fileHandle->writable;
    newFileHandle->type = fileHandle->type;

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            newFileHandle->fileDescriptor = dup(fileHandle->fileDescriptor);
            if(newFileHandle->fileDescriptor < 0)
            {
                return NULL;
            }
            break;
        case kEFFileHandleTypeVirtual:
            newFileHandle->virtualFileDescriptor = fileHandle->virtualFileDescriptor;
            if(EFRetain(newFileHandle->virtualFileDescriptor.pageGroupRef) == NULL)
            {
                return NULL;
            }
            break;
        default:
            return NULL;
    }

    return (EFFileHandleRef)EFAUTOTRANSFER(newFileHandle);
}

EFDataRef EFFileHandleReadData(EFFileHandleRef fileHandleRef,
                               EFIndex length)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL || !fileHandle->readable || (EFFileHandleGetLength(fileHandleRef) + EFFileHandleSeek(fileHandleRef, 0, kEFFileHandleSeekTypeCur)) < length)
    {
        return NULL;
    }

    EFAUTOREL EFMutableDataRef mutableData = EFDataCreateMutable(EFGetAllocator(fileHandleRef), length);
    if(mutableData == NULL)
    {
        return NULL;
    }

    UInt8 *buffer = EFDataGetMutablePtr(mutableData);
    if(buffer == NULL)
    {
        return NULL;
    }

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            if((EFIndex)read(fileHandle->fileDescriptor, buffer, (size_t)length) < length)
            {
                return NULL;
            }
            return EFAUTOTRANSFER(mutableData);
        case kEFFileHandleTypeVirtual:
            EFIndex vret = EFPageGroupRead(fileHandle->virtualFileDescriptor.pageGroupRef, (size_t)fileHandle->virtualFileDescriptor.offset, buffer, length);
            if(vret > 0)
            {
                fileHandle->virtualFileDescriptor.offset += vret;
            }
            if(vret < length)
            {
                return NULL;
            }
            return EFAUTOTRANSFER(mutableData);
        default:
            return NULL;
    }
}

Boolean EFFileHandleWriteData(EFFileHandleRef fileHandleRef,
                              EFDataRef dataRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL || dataRef == NULL || !fileHandle->writable)
    {
        return false;
    }

    const EFIndex length = EFDataGetLength(dataRef);
    const UInt8 *buffer = EFDataGetPtr(dataRef);
    if(buffer == NULL)
    {
        return false;
    }

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            return (EFIndex)write(fileHandle->fileDescriptor, buffer, length);
        case kEFFileHandleTypeVirtual:
            EFIndex start = fileHandle->virtualFileDescriptor.offset;
            EFIndex endOffset = start + length;

            while(endOffset > EFPageGroupGetLength(fileHandle->virtualFileDescriptor.pageGroupRef))
            {
                if(!EFPageGroupExtend(fileHandle->virtualFileDescriptor.pageGroupRef))
                {
                    return false;
                }
            }

            EFIndex vret = EFPageGroupWrite(fileHandle->virtualFileDescriptor.pageGroupRef, start, buffer, length);
            if(vret < 0)
            {
                return false;
            }

            fileHandle->virtualFileDescriptor.offset = start + vret;
            if(fileHandle->virtualFileDescriptor.offset > fileHandle->virtualFileDescriptor.endOffset)
            {
                fileHandle->virtualFileDescriptor.endOffset = fileHandle->virtualFileDescriptor.offset;
            }
            return true;
        default:
            return false;
    }
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

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            return (EFIndex)read(fileHandle->fileDescriptor, buffer, (size_t)length);
        case kEFFileHandleTypeVirtual:
            EFIndex vret = EFPageGroupRead(fileHandle->virtualFileDescriptor.pageGroupRef, (size_t)fileHandle->virtualFileDescriptor.offset, buffer, length);
            if(vret > 0)
            {
                fileHandle->virtualFileDescriptor.offset += vret;
            }
            return vret;
        default:
            return -1;
    }
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

     switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            return (EFIndex)write(fileHandle->fileDescriptor, buffer, (size_t)length);
        case kEFFileHandleTypeVirtual:
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
        default:
            return -1;
    }
}

EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandleRef,
                             EFIndex length)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            return (EFIndex)ftruncate(fileHandle->fileDescriptor, length);
        case kEFFileHandleTypeVirtual:
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

            fileHandle->virtualFileDescriptor.endOffset = newlen;
            return 0;
        default:
            return -1;
    }
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

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            return (EFIndex)lseek(fileHandle->fileDescriptor, offset, a);
        case kEFFileHandleTypeVirtual:
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
        default:
            return -1;
    }
}

void EFFileHandleSync(EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return;
    }

    if(fileHandle->type == kEFFileHandleTypeBSD)
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

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
            struct stat fdstat;
            if(fstat(fileHandle->fileDescriptor, &fdstat) != 0)
            {
                return -1;
            }
            return fdstat.st_size;
        case kEFFileHandleTypeVirtual:
            return fileHandle->virtualFileDescriptor.endOffset;
        default:
            return -1;
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

    switch(fileHandle->type)
    {
        case kEFFileHandleTypeBSD:
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
        case kEFFileHandleTypeVirtual:
            return EFPageGroupCreateCopy(allocatorRef, fileHandle->virtualFileDescriptor.pageGroupRef);
        default:
            return NULL;
    }
}

char *EFFileHandleGets(EFFileHandleRef fileHandleRef, char *s, int n)
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
        ssize_t r = EFFileHandleRead(fileHandleRef, (UInt8*)&c, (EFIndex)1);

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

void EFFileHandlePutc(EFFileHandleRef fileHandleRef,
                      char c)
{
    EFFileHandleWrite(fileHandleRef, (const UInt8*)&c, (EFIndex)sizeof(c));
}

void EFFileHandlePuts(EFFileHandleRef fileHandleRef,
                      const char *s)
{
    EFFileHandleWrite(fileHandleRef, (const UInt8*)s, strlen(s));
}

void EFFileHandlePrintf(EFFileHandleRef fileHandleRef,
                        const char *format,
                        ...)
{
    if(fileHandleRef == NULL || format == NULL)
    {
        return;
    }

    EFStringRef formatStr = EFStringCreateWithCString(EFGetAllocator(fileHandleRef), format, kEFStringEncodingUTF8);
    if(formatStr == NULL)
    {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    EFAUTOREL EFStringRef resultRef = EFStringCreateWithFormatAndArguments(NULL, formatStr, arguments);
    EFRelease(formatStr);
    va_end(arguments);

    EFFileHandlePuts(fileHandleRef, EFStringGetCStringPtr(resultRef, kEFStringEncodingUTF8));
}

EFFileHandleType EFFileHandleGetType(EFFileHandleRef fileHandleRef)
{
    __EFFileHandle fileHandle = (__EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return kEFFileHandleTypeBSD;
    }
    return fileHandle->type;
}
