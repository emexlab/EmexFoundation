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

#ifndef EVPAGE_H
#define EVPAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <evObj/runtime/EVBase.h>

typedef EVObjectRef EVPageRef;

size_t __EVPageGetPageSize(void);

EVTypeID EVPageGetTypeID(void);

EVPageRef EVPageCreate(EVAllocatorRef allocatorRef);
EVPageRef EVPageCreateWithOptions(EVAllocatorRef allocatorRef, void *addr, size_t len, int prot, int flags, int fd, off_t offset);

size_t EVPageGetSize(EVPageRef pageRef);
void *EVPageGetPtr(EVPageRef pageRef);

#endif /* EVPAGE_H */
