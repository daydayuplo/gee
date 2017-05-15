/* libs/graphics/sgl/SkGlyphCache.h
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

#ifndef SkGlyphCache_DEFINED
#define SkGlyphCache_DEFINED

#include "SkBitmap.h"
#include "SkChunkAlloc.h"
#include "SkDescriptor.h"
#include "SkScalerContext.h"
#include "SkTemplates.h"

class SkPaint;

class SkGlyphCache_Globals;

class SkGlyphCache {
public:
    const SkGlyph& getUnicharMetrics(SkUnichar);
    const SkGlyph& getGlyphIDMetrics(uint16_t);
    
    const void*     findImage(const SkGlyph&);
    const SkPath*   findPath(const SkGlyph&);
    void            getLineHeight(SkPoint* above, SkPoint* below);

    static SkGlyphCache* DetachCache(const SkPaint&, const SkMatrix* matrix);
    static SkGlyphCache* DetachCache(const SkDescriptor*);
    static void          AttachCache(SkGlyphCache*);
    
    /** This can be called to purge old font data, in an attempt to free
        the specified number of bytes. It returns true if it freed something
        (though it may have freed more or less than the request).
        It is thread-safe, and may be called at any time.
    */
    static bool          FreeCache(size_t bytesNeeded);

private:
    SkGlyphCache(const SkDescriptor*);
    ~SkGlyphCache();

    SkGlyph* lookupMetrics(uint16_t);

    SkGlyphCache*       fNext;
    SkDescriptor*       fDesc;
    SkScalerContext*    fScalerContext;

    enum {
        kHashBits   = 6,
        kHashCount  = 1 << kHashBits,
        kHashMask   = kHashCount - 1
    };
    SkGlyph*            fGlyphHash[kHashCount];
    SkTDArray<SkGlyph*> fGlyphArray;
    SkChunkAlloc        fGlyphAlloc;
    SkChunkAlloc        fImageAlloc;
    
    struct CharGlyphRec {
        SkUnichar   fCharCode;
        SkGlyph*    fGlyph;
    };
    // no reason to use the same kHashCount as fGlyphHash, but we do for now
    CharGlyphRec    fCharToGlyphHash[kHashCount];

    SkPoint fAbove, fBelow;
    
    // used to track (approx) how much ram is tied-up in this cache
    size_t  fMemoryUsed;

    // This relies on the caller to have already acquired the mutex to access the global cache
    static size_t InternalFreeCache(SkGlyphCache_Globals*, size_t bytesNeeded);

    inline static SkGlyphCache* ReverseLinkList(SkGlyphCache* head);
#ifdef SK_DEBUG
    static size_t ComputeMemoryUsed(const SkGlyphCache* head);
#endif
};

class SkAutoGlyphCache {
public:
    SkAutoGlyphCache(SkGlyphCache* cache) : fCache(cache) {}
    SkAutoGlyphCache(const SkDescriptor* desc)
    {
        fCache = SkGlyphCache::DetachCache(desc);
    }
    SkAutoGlyphCache(const SkPaint& paint, const SkMatrix* matrix)
    {
        fCache = SkGlyphCache::DetachCache(paint, matrix);
    }
    ~SkAutoGlyphCache()
    {
        if (fCache)
            SkGlyphCache::AttachCache(fCache);
    }

    SkGlyphCache*   getCache() const { return fCache; }

    void release()
    {
        if (fCache)
        {
            SkGlyphCache::AttachCache(fCache);
            fCache = NULL;
        }
    }
private:
    SkGlyphCache*   fCache;
};

#endif

