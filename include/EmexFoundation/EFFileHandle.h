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

#ifndef EFFILEHANDLE_H
#define EFFILEHANDLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fcntl.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/runtime/EFBase.h>

typedef enum: UInt8 {
    kEFFileHandleSeekTypeSet,
    kEFFileHandleSeekTypeCur,
    kEFFileHandleSeekTypeEnd,
    /* fuck darwin no SEEK_HOLE */
} kEFFileHandleSeekType;

typedef struct __EFFileHandle *EFFileHandleRef;

EFTypeID EFFileHandleGetTypeID(void);

EFFileHandleRef EFFileHandleCreate(EFAllocatorRef allocatorRef);
EFFileHandleRef EFFileHandleCreateWithFileDescriptor(EFAllocatorRef allocatorRef, int fd);
EFFileHandleRef EFFileHandleCreateWithOptions(EFAllocatorRef allocatorRef, EFStringRef pathStringRef, int flg, ...);

EFIndex EFFileHandleRead(EFFileHandleRef fileHandleRef, UInt8 *buffer, EFIndex length);
EFIndex EFFileHandleWrite(EFFileHandleRef fileHandleRef, const UInt8 *buffer, EFIndex length);
EFIndex EFFileHandleTruncate(EFFileHandleRef fileHandleRef, EFIndex length);

EFIndex EFFileHandleSeek(EFFileHandleRef fileHandleRef, EFIndex offset, kEFFileHandleSeekType seekType);
void EFFileHandleSync(EFFileHandleRef fileHandleRef);

EFIndex EFFileHandleGetLength(EFFileHandleRef fileHandleRef);

EFDataRef EFFileHandleCopyDataForRange(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef, EFRange range);

#endif /* EFFILEHANDLE_H */
