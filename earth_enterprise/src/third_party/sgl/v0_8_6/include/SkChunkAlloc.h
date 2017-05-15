/* include/corecg/SkChunkAlloc.h
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

#ifndef SkChunkAlloc_DEFINED
#define SkChunkAlloc_DEFINED

#include "SkNoncopyable.h"

class SkChunkAlloc : SkNoncopyable {
public:
    SkChunkAlloc(size_t minSize) : fBlock(NULL), fMinSize(SkAlign4(minSize)) {}
    ~SkChunkAlloc();

    void    reset();

    enum AllocFailType {
        kReturnNil_AllocFailType,
        kThrow_AllocFailType
    };
    void*   alloc(size_t bytes, AllocFailType);
    
private:
    struct Block {
        Block*  fNext;
        size_t  fFreeSize;
        char*   fFreePtr;
        // data[] follows
    };
    Block*  fBlock;
    size_t  fMinSize;
};

#endif
