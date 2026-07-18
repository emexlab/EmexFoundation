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

#ifndef EFBITWALKER_H
#define EFBITWALKER_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFFileHandle.h>

typedef struct __EFBitWalker *EFBitWalkerRef;

extern EFTypeID EFBitWalkerGetTypeID(void);

extern EFBitWalkerRef EFBitWalkerCreateWithHandle(EFAllocatorRef allocatorRef, EFFileHandleRef fileHandleRef, EFEndian endian);

extern void EFBitWalkerReset(EFBitWalkerRef walkerRef);

extern Boolean EFBitWalkerWrite(EFBitWalkerRef walkerRef, UInt64 value, UInt8 numBits);
extern UInt64 EFBitWalkerRead(EFBitWalkerRef walkerRef, UInt8 numBits);
extern EFIndex EFBitWalkerWriteBuffer(EFBitWalkerRef walkerRef, const char *buffer, EFIndex length);
extern EFIndex EFBitWalkerReadBuffer(EFBitWalkerRef walkerRef, char *buffer, EFIndex length);

extern void EFBitWalkerSeek(EFBitWalkerRef walkerRef, EFIndex bytePos, UInt8 bitIndex);
extern void EFBitWalkerSkip(EFBitWalkerRef walkerRef, EFIndex numBits);

extern EFIndex EFBitWalkerBytesUsed(EFBitWalkerRef walkerRef);
extern void EFBitWalkerAlignByte(EFBitWalkerRef walkerRef);
extern void EFBitWalkerSync(EFBitWalkerRef walkerRef);

#endif /* EFBITWALKER_H */
