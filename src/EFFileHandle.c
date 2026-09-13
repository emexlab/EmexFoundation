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
#include <EmexFoundation/CrossSupport/MFD.h>
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFUUID.h>
#include <EmexFoundation/EFURL.h>
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/EFMapping.h>

struct __EFFileHandle {
    EFObject super;
    SInt32 flg;
    Boolean readable;
    Boolean writable;
    SInt32 fileDescriptor;
};

static void __EVFileHandleDeinit(EFObjectRef fileHandleRef)
{
    EFFileHandleRef fileHandle = (EFFileHandleRef)fileHandleRef;
    close(fileHandle->fileDescriptor);
}

EF_HIDDEN EFClassDefinitionNewest EFFileHandleClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDFileHandle,
        .name = EFSTR_FILESCOPE("EFFileHandle"),
    },
    .init = NULL,
    .deinit = __EVFileHandleDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

EFTypeID EFFileHandleGetTypeID(void)
{
    return kEFTypeIDFileHandle;
}

EFFileHandleRef EFFileHandleCreate(EFAllocatorRef allocator)
{
    EFAUTOREL EFFileHandleRef fileHandle = (EFFileHandleRef)EFObjectCreate(allocator, EFFileHandleGetTypeID(), (EFIndex)sizeof(struct __EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    fileHandle->fileDescriptor = MFDCreate();   /* likely read-write */
    if(fileHandle->fileDescriptor < 0)
    {
        return NULL;
    }

    fileHandle->flg = O_RDWR | O_CREAT | O_TRUNC;   /* some bs flags */
    fileHandle->readable = true;
    fileHandle->writable = true;

    return EFAUTOTRANSFER(fileHandle);
}

EFFileHandleRef EFFileHandleCreateWithFileDescriptor(EFAllocatorRef allocator,
                                                     SInt32 fd)
{
    fd = dup(fd);
    if(fd < 0)
    {
        return NULL;
    }

    EFAUTOREL EFFileHandleRef fileHandle = (EFFileHandleRef)EFObjectCreate(allocator, EFFileHandleGetTypeID(), (EFIndex)sizeof(struct __EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    fileHandle->fileDescriptor = fd;

    fileHandle->flg = fcntl(fileHandle->fileDescriptor, F_GETFL);
    if(fileHandle->flg == -1)
    {
        return NULL;
    }

    UInt8 access_mode = fileHandle->flg & O_ACCMODE;
    fileHandle->readable = access_mode == O_RDONLY || access_mode == O_RDWR;
    fileHandle->writable = access_mode == O_WRONLY || access_mode == O_RDWR;

    return EFAUTOTRANSFER(fileHandle);
}

EFFileHandleRef EFFileHandleCreateWithPathAndOptions(EFAllocatorRef allocator,
                                                     EFStringRef pathString,
                                                     SInt32 flg,
                                                     ...)
{
    EFAUTOREL EFURLRef urlRef = EFURLCreateWithString(allocator, pathString);
    if(urlRef == NULL)
    {
        return NULL;
    }

    /* potentially getting mode */
    mode_t mode = 0;
    if(flg & O_CREAT)
    {
        va_list ap;
        va_start(ap, flg);
        mode = va_arg(ap, SInt32);
        va_end(ap);
    }

    return EFFileHandleCreateWithURLAndOptions(allocator, urlRef, flg, mode);
}

static EFFileHandleRef __EFFileHandleCreateNetDesc(EFAllocatorRef allocator,
                                                   EFURLRef url)
{
    EFArrayRef pathComponents = EFURLGetPathComponents(url);
    if(pathComponents == NULL || EFArrayGetCount(pathComponents) <= 0)
    {
        return NULL;
    }

    EFAUTOREL EFStringRef path = EFURLCopyPathWithoutHostname(allocator, url);
    const char *pathCPtr = EFStringGetCStringPtr(path, kEFStringEncodingUTF8);
    const char *hostNameCPtr = EFStringGetCStringPtr(EFArrayGetValueAtIndex(pathComponents, 0), kEFStringEncodingUTF8);
    if(pathCPtr == NULL || hostNameCPtr == NULL)
    {
        return NULL;
    }

    FILE *sslPipe = NULL;
    SInt32 sockfd = -1;
    EFURLType urlType = EFURLGetType(url);
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
    SInt32 state = 0;
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

    EFFileHandleRef virtualHandle = EFFileHandleCreate(allocator);
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
    EFIndex networkBytesRead;
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

EFFileHandleRef EFFileHandleCreateWithURLAndOptions(EFAllocatorRef allocator,
                                                    EFURLRef url,
                                                    SInt32 flg,
                                                    ...)
{
    if(url == NULL)
    {
        return NULL;
    }

    EFURLType type = EFURLGetType(url);
    if(type == kEFURLTypeHTTP || type == kEFURLTypeHTTPS)
    {
        return __EFFileHandleCreateNetDesc(allocator, url);
    }

    const char *str = EFStringGetCStringPtr(EFURLGetPath(url), kEFStringEncodingASCII);
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
        mode = va_arg(ap, SInt32);
        va_end(ap);
    }

    /* really opening the file */
    SInt32 fd = open(str, flg, mode);
    if(fd < 0)
    {
        return NULL;
    }

    EFFileHandleRef fileHandleRef = EFFileHandleCreateWithFileDescriptor(allocator, fd);
    close(fd);
    return fileHandleRef;
}

EFFileHandleRef EFFileHandleCreateCopy(EFAllocatorRef allocator,
                                       EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFFileHandleRef newFileHandle = (EFFileHandleRef)EFObjectCreate(allocator, EFFileHandleGetTypeID(), (EFIndex)sizeof(struct __EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    newFileHandle->flg = fileHandle->flg;
    newFileHandle->readable = fileHandle->readable;
    newFileHandle->writable = fileHandle->writable;

    newFileHandle->fileDescriptor = dup(fileHandle->fileDescriptor);
    if(newFileHandle->fileDescriptor < 0)
    {
        return NULL;
    }

    return (EFFileHandleRef)EFAUTOTRANSFER(newFileHandle);
}

EFDataRef EFFileHandleReadData(EFFileHandleRef fileHandle,
                               EFIndex length)
{
    if(fileHandle == NULL || !fileHandle->readable || (EFFileHandleGetLength(fileHandle) + EFFileHandleSeek(fileHandle, 0, kEFFileHandleSeekTypeCur)) < length)
    {
        return NULL;
    }

    EFAUTOREL EFMutableDataRef mutableData = EFDataCreateMutable(EFGetAllocator(fileHandle), length);
    if(mutableData == NULL)
    {
        return NULL;
    }

    UInt8 *buffer = EFDataGetMutablePtr(mutableData);
    if(buffer == NULL)
    {
        return NULL;
    }

    if((EFIndex)read(fileHandle->fileDescriptor, buffer, (EFSize)length) < length)
    {
        return NULL;
    }
    return EFAUTOTRANSFER(mutableData);
}

Boolean EFFileHandleWriteData(EFFileHandleRef fileHandle,
                              EFDataRef data)
{
    if(fileHandle == NULL || data == NULL || !fileHandle->writable)
    {
        return false;
    }

    const EFIndex length = EFDataGetLength(data);
    const UInt8 *buffer = EFDataGetPtr(data);
    if(buffer == NULL)
    {
        return false;
    }

    return (EFIndex)write(fileHandle->fileDescriptor, buffer, length);
}

EFIndex EFFileHandleRead(EFFileHandleRef fileHandle,
                         UInt8 *buffer,
                         EFIndex length)
{
    if(fileHandle == NULL)
    {
        return -1;
    }

    return (EFIndex)read(fileHandle->fileDescriptor, buffer, (EFSize)length);
}

EFIndex EFFileHandleWrite(EFFileHandleRef fileHandle,
                          const UInt8 *buffer,
                          EFIndex length)
{
    if(fileHandle == NULL)
    {
        return -1;
    }

    return (EFIndex)write(fileHandle->fileDescriptor, buffer, (EFSize)length);
}

EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandle,
                             EFIndex length)
{
    if(fileHandle == NULL)
    {
        return -1;
    }
    
    return (EFIndex)ftruncate(fileHandle->fileDescriptor, length);
}

EFIndex EFFileHandleSeek(EFFileHandleRef fileHandle,
                         EFIndex offset,
                         EFFileHandleSeekType seekType)
{
    if(fileHandle == NULL)
    {
        return -1;
    }

    SInt32 a = 0;
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

    return (EFIndex)lseek(fileHandle->fileDescriptor, offset, a);
}

void EFFileHandleSync(EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return;
    }

    fsync(fileHandle->fileDescriptor);
}

EFIndex EFFileHandleGetLength(EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return -1;
    }

    struct stat fdstat;
    if(fstat(fileHandle->fileDescriptor, &fdstat) != 0)
    {
        return -1;
    }
    return (EFIndex)fdstat.st_size;
}

Boolean EFFileHandleIsReadable(EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return false;
    }

    return fileHandle->readable;
}

Boolean EFFileHandleIsWritable(EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return false;
    }

    return fileHandle->writable;
}

EFDataRef EFFileHandleCopyDataForRange(EFAllocatorRef allocator,
                                       EFFileHandleRef fileHandle,
                                       EFRange range)
{
    if(fileHandle == NULL)
    {
        return NULL;
    }

    if(allocator == NULL)
    {
        allocator = EFGetAllocator(fileHandle);
    }

    EFIndex backupPosition = EFFileHandleSeek(fileHandle, 0, kEFFileHandleSeekTypeCur); /* to be restored */
    EFIndex position = EFFileHandleSeek(fileHandle, range.location, kEFFileHandleSeekTypeSet);
    if(position != range.location)
    {
        goto out_failed_restore_position;
    }

    EFMutableDataRef mutableDataRef = EFDataCreateMutable(allocator, range.length);
    if(mutableDataRef == NULL)
    {
        goto out_failed_restore_position;
    }

    UInt8 *dataBuffer = EFDataGetMutablePtr(mutableDataRef);
    EFIndex read = EFFileHandleRead(fileHandle, dataBuffer, range.length);
    if(read < range.length)
    {
        EFRelease(mutableDataRef);
        goto out_failed_restore_position;
    }

    EFFileHandleSeek(fileHandle, backupPosition, kEFFileHandleSeekTypeSet);

    EFDataRef dataRef = EFDataCreateCopy(allocator, mutableDataRef);
    EFRelease(mutableDataRef);
    return dataRef;

out_failed_restore_position:
    EFFileHandleSeek(fileHandle, backupPosition, kEFFileHandleSeekTypeSet);
    return NULL;
}

char *EFFileHandleGets(EFFileHandleRef fileHandle,
                       char *s,
                       SInt32 n)
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

    SInt32 i = 0;
    while(i < n - 1)
    {
        char c;
        EFIndex r = EFFileHandleRead(fileHandle, (UInt8*)&c, (EFIndex)1);

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

void EFFileHandlePutc(EFFileHandleRef fileHandle,
                      char c)
{
    EFFileHandleWrite(fileHandle, (const UInt8*)&c, (EFIndex)sizeof(c));
}

void EFFileHandlePuts(EFFileHandleRef fileHandle,
                      const char *s)
{
    EFFileHandleWrite(fileHandle, (const UInt8*)s, strlen(s));
}

void EFFileHandlePrintf(EFFileHandleRef fileHandle,
                        const char *format,
                        ...)
{
    if(fileHandle == NULL || format == NULL)
    {
        return;
    }

    EFStringRef formatStr = EFStringCreateWithCString(EFGetAllocator(fileHandle), format, kEFStringEncodingUTF8);
    if(formatStr == NULL)
    {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    EFAUTOREL EFStringRef resultRef = EFStringCreateWithFormatAndArguments(NULL, formatStr, arguments);
    EFRelease(formatStr);
    va_end(arguments);

    EFFileHandlePuts(fileHandle, EFStringGetCStringPtr(resultRef, kEFStringEncodingUTF8));
}

SInt32 EFFileHandleGetFileDescriptor(EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return -1;
    }

    return fileHandle->fileDescriptor;
}

EFMappingRef EFFileHandleCopyMapping(EFAllocatorRef allocator,
                                     EFFileHandleRef fileHandle)
{
    if(fileHandle == NULL)
    {
        return NULL;
    }

    SInt32 protFlags = 0;
    protFlags |= fileHandle->readable ? PROT_READ : 0;
    protFlags |= fileHandle->writable ? PROT_WRITE : 0;

    return EFMappingCreate(allocator, NULL, (EFSize)EFFileHandleGetLength(fileHandle), protFlags, MAP_SHARED, fileHandle->fileDescriptor, 0);
}
