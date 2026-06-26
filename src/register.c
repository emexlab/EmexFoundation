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

#include <stdatomic.h>
#include <evObj/register.h>

static _Atomic(EVClass *) ev_class_table[EV_MAX_CLASSES];
static _Atomic(uint64_t) ev_class_next = 1;

EVTypeID EVClassRegister(EVClass *cls)
{
    uint64_t id = atomic_fetch_add_explicit(&ev_class_next, 1, memory_order_relaxed);
    if(id >= EV_MAX_CLASSES)
    {
        return kEVNotATypeID;
    }

    cls->typeID = id;
    atomic_store_explicit(&ev_class_table[id], cls, memory_order_release);
    return id;
}

EVClass *EVClassGetByID(EVTypeID id)
{
    if(id == kEVNotATypeID || id >= EV_MAX_CLASSES)
    {
        return NULL;
    }
    return atomic_load_explicit(&ev_class_table[id], memory_order_acquire);
}
