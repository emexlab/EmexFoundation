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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <pthread.h>
#include <sys/mman.h>
#include <evObj/EVPage.h>
#include <evObj/EVString.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__unix)
    #include <unistd.h>
#else
    #error "EVGetPageSize: unsupported platform"
#endif

static size_t __EVPageSize = 0;

static void __EVPageFindOutPageSize(void)
{
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    __EVPageSize = (size_t)si.dwPageSize;
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__unix)
    long ps = sysconf(_SC_PAGESIZE);
    __EVPageSize = (ps > 0) ? (size_t)ps : 4096; /* fallback, sysconf rarely fails but be defensive */
#endif
}

size_t __EVPageGetPageSize(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, __EVPageFindOutPageSize);
    return __EVPageSize;
}

typedef struct EVPage {
    EVObject header;
    uint8_t *mem;
    size_t len;
} *EVPage;

static void __EVPageDeinit(EVPageRef pageRef)
{
    EVPage page = (EVPage)pageRef;
    if(page->mem != MAP_FAILED)
    {
        munmap(page->mem, page->len);
    }
}

static EVStringRef __EVPageCopyDescription(EVPageRef pageRef)
{
    EVPage page = (EVPage)pageRef;
    EVAllocatorRef allocatorRef = EVGetAllocator(pageRef);
    EVClass *cls = EVClassGetByID(page->header.typeID);
    return EVStringCreateWithFormat(allocatorRef, EV_STR("<%s %p>{mem = %p, len = %zu}"), cls->name, pageRef, page->mem, page->len);
}

static EVClass EVPageClass = {
    .name = "EVPage",
    .typeID = kEVNotATypeID,
    .init = NULL,
    .deinit = __EVPageDeinit,
    .equal = NULL,
    .copyDescription = __EVPageCopyDescription,
};

static void EVPageRegisterClass(void)
{
    EVClassRegister(&EVPageClass);
}

EVTypeID EVPageGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EVPageRegisterClass);
    return EVPageClass.typeID;
}

EVPageRef EVPageCreate(EVAllocatorRef allocatorRef)
{
    return EVPageCreateWithOptions(allocatorRef, NULL, __EVPageGetPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

EVPageRef EVPageCreateWithOptions(EVAllocatorRef allocatorRef,
                                  void *addr,
                                  size_t len,
                                  int prot,
                                  int flags,
                                  int fd,
                                  off_t offset)
{
    EVPage page = (EVPage)EVObjectAlloc(allocatorRef, EVPageGetTypeID(), sizeof(struct EVPage));
    if(page == NULL)
    {
        return NULL;
    }
    
    page->len = len;
    page->mem = mmap(addr, len, prot, flags, fd, offset);
    if(page->mem == MAP_FAILED)
    {
        EVRelease(page);
        return NULL;
    }

    return (EVPageRef)page;
}

size_t EVPageGetSize(EVPageRef pageRef)
{
    EVPage page = (EVPage)pageRef;
    if(page == NULL)
    {
        return 0;
    }
    return page->len;
}

void *EVPageGetPtr(EVPageRef pageRef)
{
    EVPage page = (EVPage)pageRef;
    if(page == NULL)
    {
        return NULL;
    }
    return page->mem;
}
