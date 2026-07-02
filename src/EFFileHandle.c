/*
 * MIT License
 *
 * Copyright (c) 2026 emexLabs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EFENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFPageGroup.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>

typedef struct EFFileHandle {
    EFObject header;
    Boolean isBackedByRealFile;

    union {
        int fileDescriptor;

        struct {
            EFIndex offset;
            EFIndex endOffset;  /* last offset written to */
            EFPageGroupRef pageGroupRef;
        } vfd;
    };
} *EFFileHandle;

static void __EVFileHandleDeinit(EFFileHandleRef fileHandleRef)
{
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
    if(fileHandle->isBackedByRealFile)
    {
        close(fileHandle->fileDescriptor);
    }
    else if(fileHandle->vfd.pageGroupRef != NULL)
    {
        EFRelease(fileHandle->vfd.pageGroupRef);
    }
}

static EFClass EFFileHandleClass = {
    .name = "EFFileHandle",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EVFileHandleDeinit,
    .equal = NULL,
    .copyDescription = NULL,
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
    EFFileHandle fileHandle = (EFFileHandle)EFObjectAlloc(allocatorRef, EFFileHandleGetTypeID(), sizeof(struct EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    fileHandle->fileDescriptor = -1;
    fileHandle->isBackedByRealFile = false;
    fileHandle->vfd.offset = 0;
    fileHandle->vfd.endOffset = 0;
    fileHandle->vfd.pageGroupRef = EFPageGroupCreate(allocatorRef);
    if(fileHandle->vfd.pageGroupRef == NULL)
    {
        EFRelease(fileHandle);
        return NULL;
    }

    return fileHandle;
}

EFFileHandleRef EFFileHandleCreateWithFileDescriptor(EFAllocatorRef allocatorRef,
                                                     int fd)
{
    fd = dup(fd);
    if(fd < 0)
    {
        return NULL;
    }

    EFFileHandle fileHandle = (EFFileHandle)EFObjectAlloc(allocatorRef, EFFileHandleGetTypeID(), sizeof(struct EFFileHandle));
    if(fileHandle == NULL)
    {
        return NULL;
    }

    fileHandle->fileDescriptor = fd;
    fileHandle->isBackedByRealFile = true;

    return fileHandle;
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
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByRealFile)
    {
        return (EFIndex)read(fileHandle->fileDescriptor, buffer, (size_t)length);
    }
    else
    {
        EFIndex vret = EFPageGroupRead(fileHandle->vfd.pageGroupRef, (size_t)fileHandle->vfd.offset, buffer, length);
        if(vret > 0)
        {
            fileHandle->vfd.offset += vret;
        }
        return vret;
    }
    return -1;
}

EFIndex EFFileHandleWrite(EFFileHandleRef fileHandleRef,
                          const UInt8 *buffer,
                          EFIndex length)
{
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByRealFile)
    {
        return (EFIndex)write(fileHandle->fileDescriptor, buffer, (size_t)length);
    }
    else
    {
        EFIndex start = fileHandle->vfd.offset;
        EFIndex endOffset = start + length;

        while(endOffset > EFPageGroupGetLength(fileHandle->vfd.pageGroupRef))
        {
            if(!EFPageGroupExtend(fileHandle->vfd.pageGroupRef))
            {
                return -1;
            }
        }

        EFIndex vret = EFPageGroupWrite(fileHandle->vfd.pageGroupRef, start, buffer, length);
        if(vret < 0)
        {
            return vret;
        
        }

        fileHandle->vfd.offset = start + vret;
        if(fileHandle->vfd.offset > fileHandle->vfd.endOffset)
        {
            fileHandle->vfd.endOffset = fileHandle->vfd.offset;
        }
        return vret;
    }
    return -1;
}

EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandleRef,
                             EFIndex length)
{
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByRealFile)
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
        EFIndex oldlen = fileHandle->vfd.endOffset;

        /* make sure the backing store can hold the new lenght */
        while(EFPageGroupGetLength(fileHandle->vfd.pageGroupRef) < newlen)
        {
            if(!EFPageGroupExtend(fileHandle->vfd.pageGroupRef))
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
                EFPageGroupWrite(fileHandle->vfd.pageGroupRef, pos, zeros, chunk);
                pos += chunk;
            }
        }
    }
    return -1;
}

EFIndex EFFileHandleSeek(EFFileHandleRef fileHandleRef,
                         EFIndex offset,
                         kEFFileHandleSeekType seekType)
{
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
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

    if(fileHandle->isBackedByRealFile)
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
                base = fileHandle->vfd.offset;
                break;
            case SEEK_END:
                base = fileHandle->vfd.endOffset;
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

        fileHandle->vfd.offset = newOffset;
        return newOffset;
    }

    return -1;
}

void EFFileHandleSync(EFFileHandleRef fileHandleRef)
{
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return;
    }

    if(fileHandle->isBackedByRealFile)
    {
        fsync(fileHandle->fileDescriptor);
    }
}

EFIndex EFFileHandleGetLength(EFFileHandleRef fileHandleRef)
{
    EFFileHandle fileHandle = (EFFileHandle)fileHandleRef;
    if(fileHandle == NULL)
    {
        return -1;
    }

    if(fileHandle->isBackedByRealFile)
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
        return fileHandle->vfd.endOffset;
    }
}
