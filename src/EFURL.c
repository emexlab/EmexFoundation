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
#include <pthread.h>
#include <limits.h>
#include <stdlib.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFURL.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFArray.h>

typedef struct __EFURL {
    EFObject header;
    EFURLType type;
    EFArrayRef pathComponents;
} *__EFURL;

static void __EFURLDeinit(EFObjectRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    EFReleaseTry(url->pathComponents);
}

static EFStringRef __EFURLCopyDescription(EFObjectRef urlRef)
{
    return EFURLCopyPath(EFGetAllocator(urlRef), urlRef);
}

EFClass EFURLClass = {
    .name = "EFURL",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFURLDeinit,
    .equal = NULL,
    .copyDescription = __EFURLCopyDescription,
    .hash = NULL,
};

void EFURLRegisterClass(void)
{
    EFClassRegister(&EFURLClass);
}

EFTypeID EFURLGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFURLRegisterClass);
    return EFURLClass.typeID;
}

EFURLRef EFURLCreateWithString(EFAllocatorRef allocatorRef,
                               EFStringRef stringRefRaw)
{
    if(stringRefRaw == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFStringRef stringRef = EFRetainTry(stringRefRaw);
    if(stringRef == NULL)
    {
        return NULL;
    }

    EFAUTOREL __EFURL url = (__EFURL)EFObjectCreate(allocatorRef, EFURLGetTypeID(), (EFIndex)sizeof(struct __EFURL));
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFStringRef pathString = NULL;

    url->type = kEFURLTypePOSIX;
    if(EFStringEqualRange(stringRef, EFSTR("http://"), EFRangeMake(0, 7)))
    {
        url->type = kEFURLTypeHTTP;
        pathString = EFStringCreateCopyWithRange(allocatorRef, stringRef, EFRangeMake(7, EFStringGetLength(stringRef) - 7));
    }
    else if(EFStringEqualRange(stringRef, EFSTR("https://"), EFRangeMake(0, 8)))
    {
        url->type = kEFURLTypeHTTPS;
        pathString = EFStringCreateCopyWithRange(allocatorRef, stringRef, EFRangeMake(8, EFStringGetLength(stringRef) - 8));
    }
    else
    {
        url->type = kEFURLTypePOSIX;
        pathString = EFRetain(stringRef);

        char *tmpPath = malloc(PATH_MAX);
        if(realpath(EFStringGetCStringPtr(stringRef, kEFStringEncodingUTF8), tmpPath) == NULL)
        {
            free(tmpPath);
        }
        else
        {
            EFStringRef newStringRef = EFStringCreateWithCString(kEFAllocatorDefault, tmpPath, kEFStringEncodingUTF8);
            if(newStringRef != NULL)
            {
                stringRef = newStringRef;
            }
        }
    }

    url->pathComponents = EFStringComponentsSplitBySeparator(pathString, EFSTR("/"));
    if(url->pathComponents == NULL)
    {
        return NULL;
    }

    return (EFURLRef)EFAUTOTRANSFER(url);
}

EFURLType EFURLGetType(EFURLRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    if(url == NULL)
    {
        return kEFURLTypePOSIX;
    }
    return url->type;
}

EFArrayRef EFURLGetPathComponents(EFURLRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    if(url == NULL)
    {
        return NULL;
    }
    return url->pathComponents;
}

EFStringRef EFURLCopyPath(EFAllocatorRef allocatorRef,
                          EFURLRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    if(url == NULL)
    {
        return NULL;
    }

    EFStringRef prefix = EFSTR("");

    switch(url->type)
    {
        case kEFURLTypeHTTPS:
            prefix = EFSTR("https://");
            break;
        case kEFURLTypeHTTP:
            prefix = EFSTR("http://");
            break;
        case kEFURLTypePOSIX:
        default:
            break;
    }

    EFAUTOREL EFMutableStringRef mutableString = EFStringCreateMutableCopy(kEFAllocatorDefault, prefix);

    EFIndex pathComponentCount = EFArrayGetCount(url->pathComponents);
    for(EFIndex index = 0; index < pathComponentCount; index++)
    {
        if(index > 0 && !EFStringAppendString(mutableString, EFSTR("/")))
        {
            return NULL;
        }

        if(!EFStringAppendString(mutableString, EFArrayGetValueAtIndex(url->pathComponents, index)))
        {
            return NULL;
        }
    }

    return EFAUTOTRANSFER(mutableString);
}

EFStringRef EFURLCopyPathWithoutPrefix(EFAllocatorRef allocatorRef,
                                       EFURLRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableStringRef mutableString = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR(""));

    EFIndex pathComponentCount = EFArrayGetCount(url->pathComponents);
    for(EFIndex index = 0; index < pathComponentCount; index++)
    {
        if(index > 0 && !EFStringAppendString(mutableString, EFSTR("/")))
        {
            return NULL;
        }

        if(!EFStringAppendString(mutableString, EFArrayGetValueAtIndex(url->pathComponents, index)))
        {
            return NULL;
        }
    }

    return EFAUTOTRANSFER(mutableString);
}

EFStringRef EFURLCopyPathWithoutHostname(EFAllocatorRef allocatorRef,
                                         EFURLRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableStringRef mutableString = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR(""));

    EFIndex pathComponentCount = EFArrayGetCount(url->pathComponents);
    for(EFIndex index = 1; index < pathComponentCount; index++)
    {
        if(index > 1 && !EFStringAppendString(mutableString, EFSTR("/")))
        {
            return NULL;
        }

        if(!EFStringAppendString(mutableString, EFArrayGetValueAtIndex(url->pathComponents, index)))
        {
            return NULL;
        }
    }

    return EFAUTOTRANSFER(mutableString);
}
