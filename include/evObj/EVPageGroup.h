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

#ifndef EVPAGE_GROUP_H
#define EVPAGE_GROUP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <evObj/EVPage.h>
#include <evObj/EVArray.h>
#include <evObj/runtime/EVBase.h>

typedef EVObjectRef EVPageGroupRef;

EVTypeID EVPageGroupGetTypeID(void);

EVPageGroupRef EVPageGroupCreate(EVAllocatorRef allocatorRef);
EVPageGroupRef EVPageGroupCreateWithPages(EVAllocatorRef allocatorRef, EVArrayRef pagesArrayRef);

EVArrayRef EVPageGroupCopyPages(EVAllocatorRef allocatorRef, EVPageGroupRef groupRef);

size_t EVPageGroupGetSize(EVPageGroupRef groupRef);
bool EVPageGroupExtend(EVPageGroupRef groupRef);
bool EVPageGroupMerge(EVPageGroupRef groupRef);

size_t EVPageGroupWrite(EVPageGroupRef groupRef, size_t off, const uint8_t *b, size_t len);
size_t EVPageGroupRead(EVPageGroupRef groupRef, size_t off, uint8_t *b, size_t len);

#endif /* EVPAGE_GROUP_H */
