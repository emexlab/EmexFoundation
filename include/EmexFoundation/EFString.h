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
#include <EmexFoundation/EFRuntime/EFRuntime.h>

typedef enum: UInt8 {
    kEFStringEncodingUTF7,
    kEFStringEncodingASCII,
    kEFStringEncodingUTF8,
} EFStringEncoding;

typedef enum: UInt8 {
    kEFStringCompareCaseInsensitive     = 1 << 0,
    kEFStringCompareBackwards           = 1 << 1,
    kEFStringCompareAnchored            = 1 << 2,
    kEFStringCompareNonliteral          = 1 << 3,
    kEFStringCompareLocalized           = 1 << 4,
    kEFStringCompareNumerically         = 1 << 5,
    kEFStringCompareWidthInsensitive    = 1 << 6,
} EFStringCompareFlags;

struct __EFString {
    EFObject super;
    EFStringEncoding encoding;
    Boolean isMutable;
    Boolean isInlined;          /* meaning the buffer pointer points to after the string object */
    char *buffer;               /* it is neither inlined nor undeallocatable if mutable */
    EFIndex length;
};

#define EFSTR(cStr) EFSTR_ENC(cStr, kEFStringEncodingUTF8)

/* FIXME: doesn't work on file scope (clang and apple screw your self, support C23 properly) */
#define EFSTR_ENC(cStr, enc) (__extension__ ({ \
    static struct __EFString _efk = { \
        .encoding = (enc), \
        .isMutable = false, \
        .isInlined = false, \
        .buffer = (char *)("" cStr ""), \
        .length = (EFIndex)(sizeof("" cStr "") - 1), \
        .super = { \
            ._rt = kEFRootTypeStaticObject,  \
            .typeID = kEFTypeIDString, \
        } \
    }; \
    (EFStringRef)&_efk; \
}))

#define EFSTR_FILESCOPE(cStr) \
    EFSTR_FILESCOPE_ENC(cStr, kEFStringEncodingUTF8)

#define EFSTR_FILESCOPE_ENC(cStr, enc) \
    ((EFStringRef)&(struct __EFString){ \
        .encoding = (enc), \
        .isMutable = false, \
        .isInlined = false, \
        .buffer = (char *)("" cStr ""), \
        .length = (EFIndex)(sizeof("" cStr "") - 1), \
        .super = { \
            ._rt = kEFRootTypeStaticObject, \
            .typeID = kEFTypeIDString, \
        }, \
    })

EF_EXTERN EFTypeID EFStringGetTypeID(void);

EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithBuffer(EFAllocatorRef allocator, const UInt8 *buffer, EFIndex length, EFStringEncoding encoding);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithBufferNoCopy(EFAllocatorRef allocator, const UInt8 *buffer, EFIndex length, EFStringEncoding encoding);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithCString(EFAllocatorRef allocator, const char *str, EFStringEncoding encoding);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithCStringNoCopy(EFAllocatorRef allocator, const char *str, EFStringEncoding encoding);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithFormatAndArguments(EFAllocatorRef allocator, EFStringRef format, va_list arguments);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithFormat(EFAllocatorRef allocator, EFStringRef format, ...);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateCopy(EFAllocatorRef allocator, EFStringRef string);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateCopyWithRange(EFAllocatorRef allocator, EFStringRef string, EFRange range);
EF_EXTERN EF_RETURNS_RETAINED EFMutableStringRef EFStringCreateMutableCopy(EFAllocatorRef allocator, EFStringRef string);
EF_EXTERN EF_RETURNS_RETAINED EFMutableStringRef EFStringCreateMutableCopyWithRange(EFAllocatorRef allocator, EFStringRef string, EFRange range);

EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateWithContentsOfURL(EFAllocatorRef allocator, EFURLRef url, EFStringEncoding encoding);
EF_EXTERN Boolean EFStringSaveTofURL(EFStringRef string, EFURLRef url);

EF_EXTERN EF_RETURNS_RETAINED EFDataRef EFStringCreateExternalRepresentation(EFAllocatorRef allocator, EFStringRef string, EFStringEncoding encoding);
EF_EXTERN EF_RETURNS_RETAINED EFStringRef EFStringCreateFromExternalRepresentation(EFAllocatorRef allocator, EFDataRef data, EFStringEncoding encoding);

EF_EXTERN const char *EFStringGetCStringPtr(EFStringRef string, EFStringEncoding encoding);
EF_EXTERN EFIndex EFStringGetLength(EFStringRef string);
EF_EXTERN Boolean EFStringGetCString(EFStringRef string, char *str, EFIndex length, EFStringEncoding encoding);

EF_EXTERN Boolean EFStringHasPrefix(EFStringRef string, EFStringRef prefixRef);
EF_EXTERN Boolean EFStringHasSuffix(EFStringRef string, EFStringRef suffixRef);

EF_EXTERN Boolean EFStringEqual(EFStringRef string1, EFStringRef string2);
EF_EXTERN Boolean EFStringEqualRange(EFStringRef string, EFStringRef rangeString, EFRange range);   /* range applies to the first string */

EF_EXTERN EF_RETURNS_RETAINED EFArrayRef EFStringComponentsSplitBySeparator(EFStringRef string, EFStringRef separatorString);

EF_EXTERN Boolean EFStringTrimWhitespace(EFMutableStringRef mutableString);
EF_EXTERN Boolean EFStringAppendString(EFMutableStringRef mutableString, EFStringRef appendString);
EF_EXTERN Boolean EFStringAppendFormat(EFMutableStringRef mutableString, EFStringRef formatString, ...);
EF_EXTERN Boolean EFStringDeleteRange(EFMutableStringRef mutableString, EFRange range);

EF_EXTERN Boolean EFStringIsNumber(EFStringRef string);
EF_EXTERN EF_RETURNS_RETAINED EFNumberRef EFStringCreateNumber(EFAllocatorRef allocator, EFStringRef string);

EF_EXTERN EFRange EFStringFind(EFStringRef string,  EFStringRef findString,  EFStringCompareFlags compareOptions);  /* unimplemented (later for torvalds dick ass linux proc fs to find fields like UID/GID and so on) */

#endif /* EFSTRING_H */
