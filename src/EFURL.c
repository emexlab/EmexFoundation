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
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFURL.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFArray.h>

typedef struct __EFURL {
    EFObject super;
    EFURLType type;
    EFArrayRef pathComponents;
    EFStringRef pathString;
} *__EFURL;

static void __EFURLDeinit(EFObjectRef urlRef)
{
    __EFURL url = (__EFURL)urlRef;
    EFReleaseTry(url->pathComponents);
    EFReleaseTry(url->pathString);
}

static EFStringRef __EFURLCopyDebugDescription(EFObjectRef urlRef)
{
    return EFRetainTry(EFURLGetPath(urlRef));
}

EF_HIDDEN EFClassDefinitionNewest EFURLClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDURL,
        .name = EFSTR_FILESCOPE("EFURL"),
    },
    .init = NULL,
    .deinit = __EFURLDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = __EFURLCopyDebugDescription,
};

EFTypeID EFURLGetTypeID(void)
{
    return kEFTypeIDURL;
}

EFURLRef EFURLCreateWithString(EFAllocatorRef allocator,
                               EFStringRef string)
{
    if(string == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFStringRef ownedString = EFRetainTry(string);
    if(ownedString == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFURLRef url = (EFURLRef)EFObjectCreate(allocator, EFURLGetTypeID(), (EFIndex)sizeof(struct __EFURL));
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFStringRef pathString = NULL;

    url->type = kEFURLTypePOSIX;
    if(EFStringEqualRange(ownedString, EFSTR("http://"), EFRangeMake(0, 7)))
    {
        url->type = kEFURLTypeHTTP;
        pathString = EFStringCreateCopyWithRange(allocator, ownedString, EFRangeMake(7, EFStringGetLength(ownedString) - 7));
    }
    else if(EFStringEqualRange(ownedString, EFSTR("https://"), EFRangeMake(0, 8)))
    {
        url->type = kEFURLTypeHTTPS;
        pathString = EFStringCreateCopyWithRange(allocator, ownedString, EFRangeMake(8, EFStringGetLength(ownedString) - 8));
    }
    else
    {
        url->type = kEFURLTypePOSIX;
        pathString = EFRetain(ownedString);

        char *tmpPath = EFAllocatorAllocate(allocator, PATH_MAX, 0);
        if(realpath(EFStringGetCStringPtr(ownedString, kEFStringEncodingUTF8), tmpPath) == NULL)
        {
            if(EFStringHasPrefix(ownedString, EFSTR("/")))
            {
                pathString = EFRetain(ownedString);
                goto six_feet_under;
            }

            /* need to take cwd env */
            if(getcwd(tmpPath, PATH_MAX) == NULL)
            {
                EFAllocatorDeallocate(allocator, tmpPath);
                return NULL;
            }

            EFAUTOREL EFStringRef cwd = EFStringCreateWithCString(allocator, tmpPath, kEFStringEncodingUTF8);
            EFAllocatorDeallocate(allocator, tmpPath);
            if(cwd == NULL)
            {
                return NULL;
            }

            EFAUTOREL EFMutableArrayRef pathComponents = EFArrayCreateMutable(allocator, kEFArrayCallbacksObjectCallbacks, 0);
            EFAUTOREL EFArrayRef pathComponentsCwdBase = EFStringComponentsSplitBySeparator(cwd, EFSTR("/"));
            EFAUTOREL EFArrayRef pathComponentsEnd = EFStringComponentsSplitBySeparator(pathString, EFSTR("/"));
            if(pathComponents == NULL || pathComponentsCwdBase == NULL || pathComponentsEnd == NULL ||
               !EFArrayAppendValuesOfArray(pathComponents, pathComponentsCwdBase) ||
               !EFArrayAppendValuesOfArray(pathComponents, pathComponentsEnd))
            {
                return NULL;
            }

            url->pathComponents = EFAUTOTRANSFER(pathComponents);
            return EFAUTOTRANSFER(url);
        }
        else
        {
            EFStringRef newStringRef = EFStringCreateWithCString(allocator, tmpPath, kEFStringEncodingUTF8);
            if(newStringRef != NULL)
            {
                EFReleaseTry(pathString);
                pathString = newStringRef;
            }
        }
    }

six_feet_under:
    if(EFStringEqual(pathString, EFSTR("/")))
    {
        url->pathComponents = EFArrayCreate(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, NULL, 0);
        return EFAUTOTRANSFER(url);
    }

    url->pathComponents = EFStringComponentsSplitBySeparator(pathString, EFSTR("/"));
    if(url->pathComponents == NULL)
    {
        return NULL;
    }

    return EFAUTOTRANSFER(url);
}

EFURLRef EFURLCreateByAppendingPathComponent(EFAllocatorRef allocator,
                                             EFURLRef url,
                                             EFStringRef pathComponent)
{
    if(url == NULL || pathComponent == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef mutablePathComponents = EFArrayCreateMutableCopy(allocator, url->pathComponents);
    if(!EFArrayAppendValue(mutablePathComponents, pathComponent))
    {
        return NULL;
    }

    EFAUTOREL EFURLRef newUrl = (EFURLRef)EFObjectCreate(allocator, EFURLGetTypeID(), (EFIndex)sizeof(struct __EFURL));
    if(newUrl == NULL)
    {
        return NULL;
    }

    newUrl->type = url->type;
    newUrl->pathComponents = EFAUTOTRANSFER(mutablePathComponents);

    return EFAUTOTRANSFER(newUrl);
}

EFURLRef EFURLCreateByDeletingLastPathComponent(EFAllocatorRef allocator,
                                                EFURLRef url)
{
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef mutablePathComponents = EFArrayCreateMutableCopy(allocator, url->pathComponents);
    if(mutablePathComponents == NULL)
    {
        return NULL;
    }
    EFArrayRemoveValueAtIndex(mutablePathComponents, EFArrayGetCount(mutablePathComponents) - 1);

    EFAUTOREL EFURLRef newUrl = (EFURLRef)EFObjectCreate(allocator, EFURLGetTypeID(), (EFIndex)sizeof(struct __EFURL));
    if(newUrl == NULL)
    {
        return NULL;
    }

    newUrl->type = url->type;
    newUrl->pathComponents = EFAUTOTRANSFER(mutablePathComponents);

    return EFAUTOTRANSFER(newUrl);
}

EFURLRef EFURLCreateByReplacingLastPathComponent(EFAllocatorRef allocator,
                                                 EFURLRef url,
                                                 EFStringRef pathComponent)
{
    EFAUTOREL EFURLRef secondURL = EFURLCreateByDeletingLastPathComponent(allocator, url);
    return EFURLCreateByAppendingPathComponent(allocator, secondURL, pathComponent);
}

EFURLType EFURLGetType(EFURLRef url)
{
    if(url == NULL)
    {
        return kEFURLTypePOSIX;
    }
    return url->type;
}

EFArrayRef EFURLGetPathComponents(EFURLRef url)
{
    if(url == NULL)
    {
        return NULL;
    }
    return url->pathComponents;
}

EFStringRef EFURLCopyPath(EFAllocatorRef allocator,
                          EFURLRef url)
{
    if(url == NULL)
    {
        return NULL;
    }

    EFStringRef prefix = EFSTR("/");

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

    EFAUTOREL EFMutableStringRef mutableString = EFStringCreateMutableCopy(allocator, prefix);

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

EFStringRef EFURLCopyPathWithoutPrefix(EFAllocatorRef allocator,
                                       EFURLRef url)
{
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableStringRef mutableString = EFStringCreateMutableCopy(allocator, EFSTR(""));

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

EFStringRef EFURLCopyPathWithoutHostname(EFAllocatorRef allocator,
                                         EFURLRef url)
{
    if(url == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableStringRef mutableString = EFStringCreateMutableCopy(allocator, EFSTR(""));

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

EFStringRef EFURLGetPath(EFURLRef url)
{
    if(url == NULL)
    {
        return NULL;
    }

    if(url->pathString == NULL)
    {
EFSUPPRESS_DEPRECATED_START
        url->pathString = EFURLCopyPath(EFGetAllocator(url), url);
EFSUPPRESS_DEPRECATED_END
    }

    return url->pathString;
}
