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

#ifndef EFSTRING_H
#define EFSTRING_H

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdarg.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/EFNumber.h>

typedef enum: UInt8 {
    kEFStringEncodingUTF7,
    kEFStringEncodingASCII,
    kEFStringEncodingUTF8,
} EFStringEncoding;

typedef struct __EFString *EFStringRef;
typedef struct __EFString *EFMutableStringRef;

typedef struct __EFString {
    EFObject header;
    EFStringEncoding encoding;
    Boolean isMutable;
    Boolean isInlined; /* meaning the buffer pointer points to after the string object */
    char *buffer;          /* it is neither inlined nor undeallocatable if mutable */
    EFIndex length;
} *__EFString;

#define EFSTR(cStr) EFSTR_ENC(cStr, kEFStringEncodingUTF8)

#define EFSTR_ENC(cStr, enc) (__extension__ ({ \
    static struct __EFString _evk = { \
        .encoding = (enc), \
        .isMutable = false, \
        .isInlined = false, \
        .buffer = (char *)("" cStr ""), \
        .length = (EFIndex)(sizeof("" cStr "") - 1), \
        .header.isStatic = true, \
    }; \
    _evk.header.typeID = EFStringGetTypeID(), \
    (EFStringRef)&_evk; \
}))

EFTypeID EFStringGetTypeID(void);

EFStringRef EFStringCreateWithBuffer(EFAllocatorRef allocatorRef, const UInt8 *buffer, EFIndex length, EFStringEncoding encoding);
EFStringRef EFStringCreateWithBufferNoCopy(EFAllocatorRef allocatorRef, const UInt8 *buffer, EFIndex length, EFStringEncoding encoding);
EFStringRef EFStringCreateWithCString(EFAllocatorRef allocatorRef, const char *str, EFStringEncoding encoding);
EFStringRef EFStringCreateWithCStringNoCopy(EFAllocatorRef allocatorRef, const char *str, EFStringEncoding encoding);
EFStringRef EFStringCreateWithFormatAndArguments(EFAllocatorRef allocatorRef, EFStringRef format, va_list arguments);
EFStringRef EFStringCreateWithFormat(EFAllocatorRef allocatorRef, EFStringRef format, ...);
EFStringRef EFStringCreateCopy(EFAllocatorRef allocatorRef, EFStringRef stringRef);
EFStringRef EFStringCreateCopyWithRange(EFAllocatorRef allocatorRef, EFStringRef stringRef, EFRange range);
EFMutableStringRef EFStringCreateMutableCopy(EFAllocatorRef allocatorRef, EFStringRef stringRef);
EFMutableStringRef EFStringCreateMutableCopyWithRange(EFAllocatorRef allocatorRef, EFStringRef stringRef, EFRange range);

EFDataRef EFStringCreateExternalRepresentation(EFAllocatorRef allocatorRef, EFStringRef stringRef, EFStringEncoding encoding);
EFStringRef EFStringCreateFromExternalRepresentation(EFAllocatorRef allocatorRef, EFDataRef dataRef, EFStringEncoding encoding);

const char *EFStringGetCStringPtr(EFStringRef stringRef, EFStringEncoding encoding);
EFIndex EFStringGetLength(EFStringRef stringRef);
Boolean EFStringGetCString(EFStringRef stringRef, char *str, EFIndex length, EFStringEncoding encoding);

Boolean EFStringHasPrefix(EFStringRef stringRef, EFStringRef prefixRef);
Boolean EFStringHasSuffix(EFStringRef stringRef, EFStringRef suffixRef);

Boolean EFStringEqual(EFStringRef stringRef1, EFStringRef stringRef2);
Boolean EFStringEqualRange(EFStringRef stringRef1, EFStringRef stringRef2, EFRange range);  /* range applies to the first string */

EFArrayRef EFStringComponentsSplitBySeparator(EFStringRef stringRef, EFStringRef separatorStringRef);

Boolean EFStringTrimWhitespace(EFMutableStringRef mutableStringRef);
Boolean EFStringAppendString(EFMutableStringRef mutableStringRef, EFStringRef stringRef);
Boolean EFStringAppendFormat(EFMutableStringRef mutableStringRef, EFStringRef format, ...);

Boolean EFStringIsNumber(EFStringRef stringRef);
EFNumberRef EFStringCopyNumber(EFAllocatorRef allocator, EFStringRef stringRef);

#endif /* EFSTRING_H */
