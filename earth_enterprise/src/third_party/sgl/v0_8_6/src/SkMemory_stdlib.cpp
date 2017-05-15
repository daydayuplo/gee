/* libs/corecg/SkMemory_stdlib.cpp
**
** Copyright 2006, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include "SkTypes.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef SK_DEBUG
    #define SK_TAG_BLOCKS
#endif

void sk_throw()
{
#ifdef ANDROID
    fprintf(stderr, "throwing...\n");
#endif
    abort();
}

void sk_out_of_memory(void)
{
#ifdef ANDROID
    fprintf(stderr,"- out of memory in SGL -\n");
#endif
    abort();
}

void* sk_malloc_throw(size_t size)
{
    return sk_malloc_flags(size, SK_MALLOC_THROW);
}

void* sk_realloc_throw(void* addr, size_t size)
{
#ifdef SK_TAG_BLOCKS
    if (size) size += 4;
    if (addr) addr = (char*)addr - 4;
#endif

    void* p = realloc(addr, size);
    if (size == 0)
        return p;

    if (p == NULL)
        sk_throw();
#ifdef SK_TAG_BLOCKS
    else
    {
        memcpy(p, "skia", 4);
        p = (char*)p + 4;
    }
#endif
    return p;
}

void sk_free(void* p)
{
    if (p)
    {
#ifdef SK_TAG_BLOCKS
        p = (char*)p - 4;
        SkASSERT(memcmp(p, "skia", 4) == 0);
#endif
        free(p);
    }
}

void* sk_malloc_flags(size_t size, unsigned flags)
{
#ifdef SK_TAG_BLOCKS
    size += 4;
#endif
    
    void* p = malloc(size);
    if (p == NULL)
    {
        if (flags & SK_MALLOC_THROW)
            sk_throw();
    }
#ifdef SK_TAG_BLOCKS
    else
    {
        memcpy(p, "skia", 4);
        p = (char*)p + 4;
        memset(p, 0xCD, size - 4);
    }
#endif
    return p;
}

