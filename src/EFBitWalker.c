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
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFBitWalker.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFString.h>

struct __EFBitWalker {
    EFObject super;
    EFFileHandleRef fileHandle;
    EFIndex bytePos;
    UInt8 bitIndex;
    EFEndian endian;
};

static void __EFBitWalkerDeinit(EFObjectRef walkerRef)
{
    EFBitWalkerRef walker = (EFBitWalkerRef)walkerRef;
    EFRelease(walker->fileHandle);
}

EF_HIDDEN EFClassDefinitionNewest EFBitWalkerClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDBitWalker,
        .name = EFSTR_FILESCOPE("EFBitWalker"),
    },
    .init = NULL,
    .deinit = __EFBitWalkerDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

EFTypeID EFBitWalkerGetTypeID(void)
{
    return kEFTypeIDBitWalker;
}

EFBitWalkerRef EFBitWalkerCreateWithHandle(EFAllocatorRef allocator,
                                           EFFileHandleRef fileHandle,
                                           EFEndian endian)
{
    EFAUTOREL EFBitWalkerRef walker = (EFBitWalkerRef)EFObjectCreate(allocator, EFBitWalkerGetTypeID(), (EFIndex)sizeof(struct __EFBitWalker));
    if(walker == NULL)
    {
        return NULL;
    }

    walker->fileHandle = EFFileHandleCreateCopy(allocator, fileHandle);
    if(walker->fileHandle == NULL)
    {
        return NULL;
    }
    walker->bytePos = 0;
    walker->bitIndex = 0;
    walker->endian = endian;

    return EFAUTOTRANSFER(walker);
}

void EFBitWalkerReset(EFBitWalkerRef walker)
{
    if(walker == NULL)
    {
        return;
    }

    walker->bytePos = 0;
    walker->bitIndex = 0;
}

Boolean EFBitWalkerWrite(EFBitWalkerRef walker,
                         UInt64 value,
                         UInt8 numBits)
{
    if(walker == NULL || numBits == 0 || numBits > 64)
    {
        return false;
    }

    UInt64 mask = (numBits == 64) ? ~0ULL : ((1ULL << numBits) - 1);
    value &= mask;

    /*
     * for multi-byte values we gonna have to convert host endian to target endian
     * in case they aint the same
     */
    if(numBits > 8)
    {
        UInt8 num_bytes = (numBits + 7) / 8;
        if(walker->endian == kEFEndianBig)
        {
            value = EFEndianBSwapN(value, num_bytes);
        }
    }

    UInt8 win[9] = {0};
    EFFileHandleSeek(walker->fileHandle, walker->bytePos, kEFFileHandleSeekTypeSet);
    if(EFFileHandleRead(walker->fileHandle, win, sizeof(win)) < 0)
    {
        return false;
    }

    __uint128_t chunk = EFEndianLoadWindowLE(win, sizeof win);
    chunk |= (__uint128_t)value << walker->bitIndex;
    EFEndianStoreWindowLE(win, chunk, sizeof win);

    EFFileHandleSeek(walker->fileHandle, walker->bytePos, kEFFileHandleSeekTypeSet);
    if(EFFileHandleWrite(walker->fileHandle, win, sizeof(win)) != (EFIndex)sizeof(win))
    {
        return false;
    }

    /* advance cursor */
    EFSize tmp = walker->bitIndex + numBits;
    walker->bytePos += tmp >> 3;
    walker->bitIndex = tmp & 7;

    return true;
}

UInt64 EFBitWalkerRead(EFBitWalkerRef walker,
                       UInt8 numBits)
{
    if(walker == NULL || numBits == 0 || numBits > 64)
    {
        return false;
    }

    UInt8 win[9] = {0};
    EFFileHandleSeek(walker->fileHandle, walker->bytePos, kEFFileHandleSeekTypeSet);
    if(EFFileHandleRead(walker->fileHandle, win, sizeof(win)) < 0)
    {
        return false;
    }

    __uint128_t chunk = EFEndianLoadWindowLE(win, sizeof win);
    chunk >>= walker->bitIndex;

    UInt64 mask = (numBits == 64) ? UINT64_MAX : ((1ULL << numBits) - 1);
    UInt64 value = chunk & mask;

    /* endian fix */
    if(numBits > 8)
    {
        UInt8 numBytes = (numBits + 7) / 8;
        if(walker->endian == kEFEndianBig)
        {
            value = EFEndianBSwapN(value, numBytes);
        }
    }

    return value;
}

EFIndex EFBitWalkerWriteBuffer(EFBitWalkerRef walker,
                               const char *buffer,
                               EFIndex length)
{
    if(walker == NULL)
    {
        return -1;
    }

    EFBitWalkerAlignByte(walker);
    EFFileHandleSeek(walker->fileHandle, walker->bytePos, kEFFileHandleSeekTypeSet);
    EFIndex written = EFFileHandleWrite(walker->fileHandle, (const UInt8*)buffer, length);
    walker->bytePos += written;
    return written;
}

EFIndex EFBitWalkerReadBuffer(EFBitWalkerRef walker,
                              char *buffer,
                              EFIndex length)
{
    if(walker == NULL)
    {
        return -1;
    }

    EFBitWalkerAlignByte(walker);
    EFFileHandleSeek(walker->fileHandle, walker->bytePos, kEFFileHandleSeekTypeSet);
    EFIndex reddit = EFFileHandleRead(walker->fileHandle, (UInt8*)buffer, length);
    walker->bytePos += reddit;
    return reddit;  /* obviously it is a joke lol */
}

void EFBitWalkerSeek(EFBitWalkerRef walker,
                     EFIndex bytePos,
                     UInt8 bitIndex)
{
    if(walker == NULL)
    {
        return;
    }

    walker->bytePos = bytePos;
    walker->bitIndex = bitIndex;
}

void EFBitWalkerSkip(EFBitWalkerRef walker,
                     EFIndex numBits)
{
    if(walker == NULL)
    {
        return;
    }

    EFIndex tmp = walker->bitIndex + numBits;
    walker->bytePos += tmp >> 3;
    walker->bitIndex = tmp & 7;
}

EFIndex EFBitWalkerBytesUsed(EFBitWalkerRef walker)
{
    if(walker == NULL)
    {
        return -1;
    }

    return walker->bytePos + ((walker->bitIndex == 0) ? 0 : 1);
}

void EFBitWalkerAlignByte(EFBitWalkerRef walker)
{
    if(walker == NULL)
    {
        return;
    }

    if(walker->bitIndex != 0)
    {
        walker->bitIndex = 0;
        walker->bytePos += 1;
    }
}

void EFBitWalkerSync(EFBitWalkerRef walker)
{
    if(walker == NULL)
    {
        return;
    }

    EFFileHandleSync(walker->fileHandle);
}

EFBitWalkerPosition EFBitWalkerGetPosition(EFBitWalkerRef walker)
{
    if(walker == NULL)
    {
        return (EFBitWalkerPosition){
            .bytePos = 0,
            .bitIndex = 0,
        };
    }

    return (EFBitWalkerPosition){
        .bytePos = walker->bytePos,
        .bitIndex = walker->bitIndex,
    };
}

void EFBitWalkerSetPosition(EFBitWalkerRef walker,
                            EFBitWalkerPosition position)
{
    if(walker == NULL)
    {
        return;
    }

    walker->bytePos = position.bytePos;
    walker->bitIndex = position.bitIndex;
}

EFFileHandleRef EFBitWalkerGetHandle(EFBitWalkerRef walker)
{
    if(walker == NULL)
    {
        return NULL;
    }

    return walker->fileHandle;
}
