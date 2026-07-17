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
#include <sys/mman.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFPage.h>
#include <EmexFoundation/EFString.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__unix)
    #include <unistd.h>
#else
    #error "EFGetPageSize: unsupported platform"
#endif

static EFIndex __EFPageLength = 0;

static void __EFPageFindOutPageLength(void)
{
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    __EFPageLength = (EFIndex)si.dwPageSize;
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__unix)
    long ps = sysconf(_SC_PAGESIZE);
    __EFPageLength = (EFIndex)((ps > 0) ? (EFIndex)ps : 4096); /* fallback, sysconf rarely fails but be defensive */
#endif
}

EFIndex __EFPageGetPageLength(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, __EFPageFindOutPageLength);
    return __EFPageLength;
}

typedef struct __EFPage {
    EFObject header;
    UInt8 *mem;
    EFIndex length;
} *__EFPage;

static void __EFPageDeinit(EFObjectRef pageRef)
{
    __EFPage page = (__EFPage)pageRef;
    if(page->mem != MAP_FAILED)
    {
        munmap(page->mem, page->length);
    }
}

static EFStringRef __EFPageCopyDescription(EFObjectRef pageRef)
{
    __EFPage page = (__EFPage)pageRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(pageRef);
    EFClass *cls = EFClassGetByID(page->header.typeID);
    return EFStringCreateWithFormat(allocatorRef, EFSTR("<%s %p>{mem = %p, length = %ld}"), cls->name, pageRef, page->mem, page->length);
}

static EFClass EFPageClass = {
    .name = "EFPage",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFPageDeinit,
    .equal = NULL,
    .copyDescription = __EFPageCopyDescription,
    .hash = NULL,
};

static void EFPageRegisterClass(void)
{
    EFClassRegister(&EFPageClass);
}

EFTypeID EFPageGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFPageRegisterClass);
    return EFPageClass.typeID;
}

EFPageRef EFPageCreate(EFAllocatorRef allocatorRef)
{
    return EFPageCreateWithOptions(allocatorRef, NULL, __EFPageGetPageLength(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

EFPageRef EFPageCreateWithOptions(EFAllocatorRef allocatorRef,
                                  void *addr,
                                  size_t length,
                                  int prot,
                                  int flags,
                                  int fd,
                                  off_t offset)
{
    EFAUTOREL __EFPage page = (__EFPage)EFObjectCreate(allocatorRef, EFPageGetTypeID(), (EFIndex)sizeof(struct __EFPage));
    if(page == NULL)
    {
        return NULL;
    }

    page->length = length;
    page->mem = mmap(addr, length, prot, flags, fd, offset);
    if(page->mem == MAP_FAILED)
    {
        return NULL;
    }

    return (EFPageRef)EFAUTOTRANSFER(page);
}

EFIndex EFPageGetLength(EFPageRef pageRef)
{
    __EFPage page = (__EFPage)pageRef;
    if(page == NULL)
    {
        return 0;
    }
    return page->length;
}

void *EFPageGetPtr(EFPageRef pageRef)
{
    __EFPage page = (__EFPage)pageRef;
    if(page == NULL)
    {
        return NULL;
    }
    return page->mem;
}
