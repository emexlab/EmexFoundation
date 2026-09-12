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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <strings.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/EFRuntime/EFBase.h>
#include <EmexFoundation/EFRuntime/EFClass.h>
#include <EmexFoundation/EFRuntime/EFAllocator.h>
#include <EmexFoundation/EFString.h>

extern EFClassDefinitionNewest EFStringClass;
extern EFClassDefinitionNewest EFNumberClass;
extern EFClassDefinitionNewest EFURLClass;
extern EFClassDefinitionNewest EFUUIDClass;
extern EFClassDefinitionNewest EFDataClass;
extern EFClassDefinitionNewest EFFileHandleClass;
extern EFClassDefinitionNewest EFFileClass;
extern EFClassDefinitionNewest EFBitWalkerClass;
extern EFClassDefinitionNewest EFMappingClass;
extern EFClassDefinitionNewest EFProcessClass;
extern EFClassDefinitionNewest EFMallocBlockClass;
extern EFClassDefinitionNewest EFArrayClass;
extern EFClassDefinitionNewest EFDictionaryClass;
extern EFClassDefinitionNewest EFFileManagerClass;

static pthread_mutex_t efClassLock = PTHREAD_MUTEX_INITIALIZER;
static EFClass *efClassTable = NULL;
static EFTypeID efClassNext = kEFTypeIDFileManager;
static EFSize efClassCapacity = 0;

static Boolean __EFClassTableExtendIfNeeded(void)
{
    if(pthread_mutex_trylock(&efClassLock) == 0)
    {
        pthread_mutex_unlock(&efClassLock);
        return false;
    }

    if(efClassTable == NULL)
    {
        efClassTable = EFAllocatorAllocate(kEFAllocatorDefault, sizeof(EFClass) * 1024, 0);
        if(efClassTable == NULL)
        {
            return false;
        }

        efClassTable[kEFTypeIDNone]                     = NULL;
        efClassTable[kEFTypeIDString]                   = &EFStringClass;
        efClassTable[kEFTypeIDNumber]                   = &EFNumberClass;
        efClassTable[kEFTypeIDURL]                      = &EFURLClass;
        efClassTable[kEFTypeIDUUID]                     = &EFUUIDClass;
        efClassTable[kEFTypeIDData]                     = &EFDataClass;
        efClassTable[kEFTypeIDFileHandle]               = &EFFileHandleClass;
        efClassTable[kEFTypeIDFile]                     = &EFFileClass;
        efClassTable[kEFTypeIDBitWalker]                = &EFBitWalkerClass;
        efClassTable[kEFTypeIDMapping]                  = &EFMappingClass;
        efClassTable[kEFTypeIDProcess]                  = &EFProcessClass;
        efClassTable[kEFTypeIDMallocBlock]              = &EFMallocBlockClass;
        efClassTable[kEFTypeIDArray]                    = &EFArrayClass;
        efClassTable[kEFTypeIDDictionary]               = &EFDictionaryClass;
        efClassTable[kEFTypeIDFileManager]              = &EFFileManagerClass;
        efClassCapacity = 1024;
    }
    else if(efClassCapacity < efClassNext)
    {
        void *newp = EFAllocatorReallocate(kEFAllocatorDefault, efClassTable, sizeof(EFClass) * (efClassCapacity + 1024), 0);
        if(newp == NULL)
        {
            return false;
        }
        efClassTable = newp;
        efClassCapacity += 1024;
    }

    return true;
}

EF_HIDDEN EFClass __EFClassGetByID(EFTypeID id)
{
    pthread_mutex_lock(&efClassLock);
    if((efClassTable == NULL && !__EFClassTableExtendIfNeeded()) || id > efClassNext)
    {
        pthread_mutex_unlock(&efClassLock);
        return NULL;
    }
    EFClass class = efClassTable[id];
    pthread_mutex_unlock(&efClassLock);
    return class;
}

static EFClassDefinitionNewest *EFClassCopySafely(void *classDefinition)
{
    EFClassStableHeader *header = (EFClassStableHeader*)classDefinition;
    switch(header->version)
    {
        case 2:
        {
            EFClassDefinitionV2 *classDefinitionV2 = (EFClassDefinitionV2*)classDefinition;
            EFClassDefinitionNewest *newestClassDefinition = EFAllocatorAllocate(kEFAllocatorDefault, (EFSize)sizeof(EFClassDefinitionNewest), 0);
            if(newestClassDefinition == NULL)
            {
                return NULL;
            }

            newestClassDefinition->header.name = EFStringCreateWithCString(kEFAllocatorDefault, classDefinitionV2->name, kEFStringEncodingUTF8);
            newestClassDefinition->header.version = EFCLASS_NEWEST_VERSION;
            newestClassDefinition->init = classDefinitionV2->init;
            newestClassDefinition->deinit = classDefinitionV2->deinit;
            newestClassDefinition->equal = classDefinitionV2->equal;
            newestClassDefinition->hash = classDefinitionV2->hash;
            newestClassDefinition->copyDescription = NULL;
            newestClassDefinition->copyDebugDescription = classDefinitionV2->copyDescription;

            return newestClassDefinition;
        }
        case 3:
        {
            EFClassDefinitionV3 *classDefinitionV3 = (EFClassDefinitionV3*)classDefinition;
            EFClassDefinitionNewest *newestClassDefinition = EFAllocatorAllocate(kEFAllocatorDefault, (EFSize)sizeof(EFClassDefinitionNewest), 0);
            if(newestClassDefinition == NULL)
            {
                return NULL;
            }

            newestClassDefinition->header.name = EFStringCreateCopy(kEFAllocatorDefault, classDefinitionV3->header.name);
            newestClassDefinition->header.version = EFCLASS_NEWEST_VERSION;
            newestClassDefinition->init = classDefinitionV3->init;
            newestClassDefinition->deinit = classDefinitionV3->deinit;
            newestClassDefinition->equal = classDefinitionV3->equal;
            newestClassDefinition->hash = classDefinitionV3->hash;
            newestClassDefinition->copyDescription = NULL;
            newestClassDefinition->copyDebugDescription = classDefinitionV3->copyDescription;

            return newestClassDefinition;
        }
        case 4:
        {
            EFClassDefinitionV4 *classDefinitionV4 = (EFClassDefinitionV4*)classDefinition;
            EFClassDefinitionNewest *newestClassDefinition = EFAllocatorAllocate(kEFAllocatorDefault, (EFSize)sizeof(EFClassDefinitionNewest), 0);
            if(newestClassDefinition == NULL)
            {
                return NULL;
            }

            bcopy(classDefinitionV4, newestClassDefinition, sizeof(EFClassDefinitionNewest));

            return newestClassDefinition;
        }
        default:
            break;
    }
    return NULL;
}

EFTypeID EFClassRegister(void *classDefinition)
{
    assert(classDefinition != NULL);
    EFClassDefinitionNewest *classDefinitionCopy = (EFClassDefinitionNewest*)EFClassCopySafely(classDefinition);
    assert(classDefinitionCopy != NULL);

    pthread_mutex_lock(&efClassLock);

    EFTypeID id = ++efClassNext;
    assert(id != 0 && __EFClassTableExtendIfNeeded());
    ((EFClassStableHeader*)classDefinition)->typeID = id;
    classDefinitionCopy->header.typeID = id;
    efClassTable[id] = classDefinitionCopy;

    pthread_mutex_unlock(&efClassLock);

    return id;
}
