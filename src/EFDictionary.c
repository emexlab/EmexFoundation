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

#include <string.h>
#include <EmexFoundation/EFDictionary.h>
#include <EmexFoundation/EFString.h>

static const EFIndex __EFDictionaryBucketSizes[] = {
    3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723, 2803,
    4523, 7351, 11959, 19447, 31391, 50839, 82261, 133099, 215353,
    348457, 563791, 912249, 1476041, 2388277, 3864551, 6252751
};

#define __EFDictionaryBucketSizesCount \
    (EFIndex)(sizeof(__EFDictionaryBucketSizes) / sizeof(EFIndex))

static inline EFIndex __EFDictionaryCapacityForIndex(EFIndex index)
{
    return (__EFDictionaryBucketSizes[index] * 3) / 4;
}

struct __EFDictionary {
    const void **keys;
    const void **values;
    UInt8 *metadata;
    EFIndex bucketsIndex;
    EFIndex count;
    EFIndex deletedCount;
    UInt32 mutations;
    Boolean isMutable;
    EFDictionaryKeyCallbacks keyCallbacks;
    EFDictionaryValueCallbacks valueCallbacks;
};

EF_HIDDEN EFClassDefinitionNewest EFDictionaryClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDDictionary,
        .name = EFSTR_FILESCOPE("EFDictionary"),
    },
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

EFTypeID EFDictionaryGetTypeID(void)
{
    return kEFTypeIDDictionary;
}

static EFMutableDictionaryRef __EFDictionaryCreate(EFAllocatorRef allocatorRef,
                                                   const EFDictionaryKeyCallbacks *keyCallbacks,
                                                   const EFDictionaryValueCallbacks *valueCallbacks,
                                                   EFIndex capacity,
                                                   Boolean isMutable)
{
    if(capacity < 0)
    {
        return NULL;
    }

    EFIndex bucketsIndex = 0;
    while(bucketsIndex < (__EFDictionaryBucketSizesCount - 1) && __EFDictionaryCapacityForIndex(bucketsIndex) < capacity)
    {
        bucketsIndex++;
    }

    EFMutableDictionaryRef dictionary = (EFMutableDictionaryRef)EFObjectCreate(allocatorRef, EFDictionaryGetTypeID(), (EFIndex)sizeof(struct __EFDictionary));
    if(dictionary == NULL)
    {
        return NULL;
    }

    EFIndex buckets = __EFDictionaryBucketSizes[bucketsIndex];
    dictionary->keys = EFAllocatorAllocate(allocatorRef, (EFIndex)(buckets * sizeof(const void *)), 0);
    dictionary->values = EFAllocatorAllocate(allocatorRef, (EFIndex)(buckets * sizeof(const void *)), 0);
    dictionary->metadata = EFAllocatorAllocate(allocatorRef, buckets, 0);
    if(dictionary->keys == NULL || dictionary->values == NULL || dictionary->metadata == NULL)
    {
        EFRelease(dictionary);
        return NULL;
    }

    memset(dictionary->metadata, kEFDictionaryMetadataEmpty, (EFSize)buckets);

    dictionary->bucketsIndex = bucketsIndex;
    dictionary->count = 0;
    dictionary->deletedCount = 0;
    dictionary->mutations = 0;
    dictionary->isMutable = isMutable;
    dictionary->keyCallbacks = keyCallbacks   ? *keyCallbacks   : (EFDictionaryKeyCallbacks){0};
    dictionary->valueCallbacks = valueCallbacks ? *valueCallbacks : (EFDictionaryValueCallbacks){0};

    return dictionary;
}


