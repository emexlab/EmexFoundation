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

#ifndef EFDATA_H
#define EFDATA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <EmexFoundation/runtime/EFBase.h>

typedef struct __EFData *EFDataRef;
typedef struct __EFData *EFMutableDataRef;

EFTypeID EFDataGetTypeID(void);

EFDataRef EFDataCreateWithBuffer(EFAllocatorRef allocatorRef, const UInt8 *buffer, EFIndex length);
EFDataRef EFDataCreateWithBufferNoCopy(EFAllocatorRef allocatorRef, const UInt8 *buffer, EFIndex length);
EFDataRef EFDataCreateCopy(EFAllocatorRef allocatorRef, EFDataRef dataRef);
EFMutableDataRef EFDataCreateMutable(EFAllocatorRef allocatorRef, EFIndex capacity);
EFMutableDataRef EFDataCreateMutableCopy(EFAllocatorRef allocatorRef, EFDataRef dataRef);

EFIndex EFDataGetLength(EFDataRef dataRef);
const UInt8 *EFDataGetPtr(EFDataRef dataRef);
UInt8 *EFDataGetMutablePtr(EFMutableDataRef mutableDataRef);
Boolean EFDataCopyRangeToBuffer(EFDataRef dataRef, EFRange range, UInt8 *buffer);

Boolean EFDataSetLength(EFMutableDataRef mutableDataRef, EFIndex length);
Boolean EFDataIncreaseLength(EFMutableDataRef mutableDataRef, EFIndex extraLength);
Boolean EFDataAppendBuffer(EFMutableDataRef mutableDataRef, const UInt8 *buffer, EFIndex length);
Boolean EFDataReplaceBufferInRange(EFMutableDataRef mutableDataRef, EFRange range, const UInt8 *newBytes, EFIndex newLength);
Boolean EFDataDeleteBufferInRange(EFMutableDataRef mutableDataRef, EFRange range);

#endif /* EFDATA_H */
