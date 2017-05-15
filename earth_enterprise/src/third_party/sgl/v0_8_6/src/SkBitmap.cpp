/* libs/graphics/sgl/SkBitmap.cpp
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

#include "SkBitmap.h"
#include "SkColorPriv.h"
#include "SkMask.h"
#include "SkUtils.h"

//#define TRACE_BITMAP_ALLOCATIONS

SkBitmap::SkBitmap()
{
    memset(this, 0, sizeof(*this));
}

SkBitmap::SkBitmap(const SkBitmap& src)
{
    src.fColorTable->safeRef();

    memcpy(this, &src, sizeof(src));
    fFlags &= ~(kWeOwnThePixels_Flag | kWeOwnTheMipMap_Flag);
}

SkBitmap::~SkBitmap()
{
    fColorTable->safeUnref();
    this->freePixels();
}

SkBitmap& SkBitmap::operator=(const SkBitmap& src)
{
    src.fColorTable->safeRef();
    fColorTable->safeUnref();

    this->freePixels();
    memcpy(this, &src, sizeof(src));
    fFlags &= ~(kWeOwnThePixels_Flag | kWeOwnTheMipMap_Flag);

    return *this;
}

void SkBitmap::swap(SkBitmap& other)
{
    SkTSwap<SkColorTable*>(fColorTable, other.fColorTable);

#ifdef SK_SUPPORT_MIPMAP
    SkTSwap<MipMap*>(fMipMap, other.fMipMap);
#endif

    SkTSwap<void*>(fPixels, other.fPixels);
    SkTSwap<uint16_t>(fWidth, other.fWidth);
    SkTSwap<uint16_t>(fHeight, other.fHeight);
    SkTSwap<uint16_t>(fRowBytes, other.fRowBytes);
    SkTSwap<uint8_t>(fConfig, other.fConfig);
    SkTSwap<uint8_t>(fFlags, other.fFlags);
}

void SkBitmap::reset()
{
    fColorTable->safeUnref();
    this->freePixels();
    memset(this, 0, sizeof(*this));
}

U16CPU SkBitmap::ComputeRowBytes(Config c, U16CPU width)
{
    U16CPU rowBytes = 0;

    switch (c) {
    case kA1_Config:
        rowBytes = (width + 7) >> 3;
        break;
    case kA8_Config:
    case kIndex8_Config:
        rowBytes = width;
        break;
    case kRGB_565_Config:
        rowBytes = SkAlign4(width << 1);
        break;
    case kARGB_8888_Config:
        rowBytes = width << 2;
        break;
    default:
        SkASSERT(!"unknown config");
        break;
    }
    return rowBytes;
}

void SkBitmap::setConfig(Config c, U16CPU width, U16CPU height, U16CPU rowBytes)
{
    this->freePixels();

    if (rowBytes == 0)
        rowBytes = SkBitmap::ComputeRowBytes(c, width);

    fConfig     = SkToU8(c);
    fWidth      = SkToU16(width);
    fHeight     = SkToU16(height);
    fRowBytes   = SkToU16(rowBytes);
}

void SkBitmap::setPixels(void* p)
{
    this->freePixels();

    fPixels = p;
    fFlags &= ~(kWeOwnThePixels_Flag | kWeOwnTheMipMap_Flag);
}

#ifdef TRACE_BITMAP_ALLOCATIONS
    static size_t gTotalPixelMemory;
    static int gTotalBitmapCount;
#endif

/** We explicitly use the same allocator for our pixels that SkMask does, so that we can
    freely assign memory allocated by one class to the other.
*/
void SkBitmap::allocPixels()
{
    this->freePixels();
    
#ifdef TRACE_BITMAP_ALLOCATIONS
    gTotalBitmapCount += 1;
    gTotalPixelMemory += fHeight * fRowBytes;
#endif

    fPixels = (uint32_t*)SkMask::AllocImage(fHeight * fRowBytes);
    fFlags |= kWeOwnThePixels_Flag;
    
#ifdef TRACE_BITMAP_ALLOCATIONS
    printf("--------- alloc [%d] %p total [%d, %dK]\n", fHeight*fRowBytes, fPixels, gTotalBitmapCount, (int)gTotalPixelMemory >> 10);
#endif
}

/** We explicitly use the same allocator for our pixels that SkMask does, so that we can
    freely assign memory allocated by one class to the other.
*/
void SkBitmap::freePixels()
{
    if (fFlags & kWeOwnThePixels_Flag)
    {
#ifdef TRACE_BITMAP_ALLOCATIONS
        gTotalPixelMemory -= fHeight * fRowBytes;
        gTotalBitmapCount -= 1;
        printf("--------- free  [%d] %p total [%d, %dK]\n", fHeight*fRowBytes, fPixels, gTotalBitmapCount, (int)gTotalPixelMemory >> 10);
#endif

        SkASSERT(fPixels);
        SkMask::FreeImage(fPixels);
        fPixels = NULL;
        fFlags &= ~kWeOwnThePixels_Flag;
    }
#ifdef SK_SUPPORT_MIPMAP
    if (fFlags & kWeOwnTheMipMap_Flag)
    {
        sk_free(fMipMap);
        fMipMap = NULL;
    }
#endif
}

bool SkBitmap::getOwnsPixels() const
{
    return SkToBool(fFlags & kWeOwnThePixels_Flag);
}

void SkBitmap::setOwnsPixels(bool ownsPixels)
{
    if (ownsPixels)
        fFlags |= kWeOwnThePixels_Flag;
    else
        fFlags &= ~kWeOwnThePixels_Flag;
}

SkColorTable* SkBitmap::setColorTable(SkColorTable* ct)
{
    SkRefCnt_SafeAssign(fColorTable, ct);
    return ct;
}

bool SkBitmap::isOpaque() const
{
    switch (fConfig) {
    case kNo_Config:
        return true;

    case kA1_Config:
    case kA8_Config:
    case kARGB_8888_Config:
        return (fFlags & kImageIsOpaque_Flag) != 0;

    case kIndex8_Config:
        return (fColorTable->getFlags() & SkColorTable::kColorsAreOpaque_Flag) != 0;

    case kRGB_565_Config:
        return true;

    default:
        SkASSERT(!"unknown bitmap config");
        return false;
    }
}

void SkBitmap::setIsOpaque(bool isOpaque)
{
    /*  we record this regardless of fConfig, though it is ignored in isOpaque() for
        configs that can't support per-pixel alpha.
    */
    if (isOpaque)
        fFlags |= kImageIsOpaque_Flag;
    else
        fFlags &= ~kImageIsOpaque_Flag;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void SkBitmap::eraseARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    if (fPixels == NULL || fConfig == kNo_Config)
        return;

    int height = fHeight;

    this->setIsOpaque(a == 255);

    // make rgb premultiplied
    if (a != 255)
    {
        r = SkAlphaMul(r, a);
        g = SkAlphaMul(g, a);
        b = SkAlphaMul(b, a);
    }

    switch (fConfig) {
    case kA1_Config:
        {
            uint8_t* p = (uint8_t*)fPixels;
            size_t count = (fWidth + 7) >> 3;
            a = (a >> 7) ? 0xFF : 0;
            SkASSERT(count <= fRowBytes);
            while (--height >= 0)
            {
                memset(p, a, count);
                p += fRowBytes;
            }
        }
        break;
    case kA8_Config:
        memset(fPixels, a, fRowBytes * fWidth);
        break;
    case kIndex8_Config:
        SkASSERT(!"Don't support writing to Index8 bitmaps");
        break;
    case kRGB_565_Config:
        // now erase the color-plane
        {
            uint16_t* p = (uint16_t*)fPixels;
            uint16_t  v = SkPackRGB16(r >> (8 - SK_R16_BITS),
                                g >> (8 - SK_G16_BITS),
                                b >> (8 - SK_B16_BITS));

            while (--height >= 0)
            {
                sk_memset16(p, v, fWidth);
                p = (uint16_t*)((char*)p + fRowBytes);
            }
        }
        break;
    case kARGB_8888_Config:
        {
            uint32_t* p = (uint32_t*)fPixels;
            uint32_t  v = SkPackARGB32(a, r, g, b);

            while (--height >= 0)
            {
                sk_memset32(p, v, fWidth);
                p = (uint32_t*)((char*)p + fRowBytes);
            }
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

static void downsampleby2_proc32(SkBitmap* dst, int x, int y, const SkBitmap& src)
{
    x <<= 1;
    y <<= 1;
    const SkPMColor* p = src.getAddr32(x, y);
    SkPMColor c, ag, rb;

    c = *p; ag = (c >> 8) & 0xFF00FF; rb = c & 0xFF00FF;
    if (x < (int)src.width() - 1)
        p += 1;
    c = *p; ag += (c >> 8) & 0xFF00FF; rb += c & 0xFF00FF;

    if (y < (int)src.height() - 1)
        p = src.getAddr32(x, y + 1);
    c = *p; ag += (c >> 8) & 0xFF00FF; rb += c & 0xFF00FF;
    if (x < (int)src.width() - 1)
        p += 1;
    c = *p; ag += (c >> 8) & 0xFF00FF; rb += c & 0xFF00FF;

    *dst->getAddr32(x >> 1, y >> 1) = ((rb >> 2) & 0xFF00FF) | ((ag << 6) & 0xFF00FF00);
}

static inline uint32_t expand16(U16CPU c)
{
    return (c & ~SK_G16_MASK_IN_PLACE) | ((c & SK_G16_MASK_IN_PLACE) << 16);
}

// returns dirt in the top 16bits, but we don't care, since we only
// store the low 16bits.
static inline U16CPU pack16(uint32_t c)
{
    return (c & ~SK_G16_MASK_IN_PLACE) | ((c >> 16) & SK_G16_MASK_IN_PLACE);
}

static void downsampleby2_proc16(SkBitmap* dst, int x, int y, const SkBitmap& src)
{
    x <<= 1;
    y <<= 1;
    const uint16_t* p = src.getAddr16(x, y);
    SkPMColor       c;

    c = expand16(*p);
    if (x < (int)src.width() - 1)
        p += 1;
    c += expand16(*p);

    if (y < (int)src.height() - 1)
        p = src.getAddr16(x, y + 1);
    c += expand16(*p);
    if (x < (int)src.width() - 1)
        p += 1;
    c += expand16(*p);

    *dst->getAddr16(x >> 1, y >> 1) = SkToU16(pack16(c >> 2));
}

bool SkBitmap::quarterSizeFiltered(SkBitmap* dst) const
{
    int shift;
    void (*proc)(SkBitmap* dst, int x, int y, const SkBitmap& src);
    
    switch (getConfig()) {
        case SkBitmap::kARGB_8888_Config:
            shift = 2;
            proc = downsampleby2_proc32;
            break;
        case SkBitmap::kRGB_565_Config:
            shift = 1;
            proc = downsampleby2_proc16;
            break;
        default:
            //other formats not supported
            return false;
    }
    
    unsigned width = (this->width() + 1) >> 1;
    unsigned height = (this->height() + 1) >> 1;
    unsigned rowBytes = width << shift;
    
    dst->setConfig(getConfig(), width, height, rowBytes);
    dst->allocPixels();
    
    for (unsigned j = 0; j < height; j++)
        for (unsigned i = 0; i < width; i++)
            proc(dst, i, j, *this);

    return true;
}


void SkBitmap::buildMipMap(bool forceRebuild)
{
#ifdef SK_SUPPORT_MIPMAP
    if (!forceRebuild && fMipMap)
        return;

    if (fFlags & kWeOwnTheMipMap_Flag)
    {
        SkASSERT(fMipMap);
        sk_free(fMipMap);
        fMipMap = NULL;
        fFlags &= ~kWeOwnTheMipMap_Flag;
    }

    int shift;
    void (*proc)(SkBitmap* dst, int x, int y, const SkBitmap& src);

    switch (this->getConfig()) {
    case kARGB_8888_Config:
        shift = 2;
        proc = downsampleby2_proc32;
        break;
    case kRGB_565_Config:
        shift = 1;
        proc = downsampleby2_proc16;
        break;
    case kIndex8_Config:
    case kA8_Config:
//      shift = 0; break;
    default:
        return; // don't build mipmaps for these configs
    }

    // whip through our loop to compute the exact size needed
    size_t  size;
    {
        unsigned    width = this->width();
        unsigned    height = this->height();
        size = 0;
        for (int i = 1; i < kMaxMipLevels; i++)
        {
            width = (width + 1) >> 1;
            height = (height + 1) >> 1;
            size += width * height << shift;
        }
    }

    MipMap* mm = (MipMap*)sk_malloc_throw(sizeof(MipMap) + size);
    uint8_t*     addr = (uint8_t*)(mm + 1);

    unsigned    width = this->width();
    unsigned    height = this->height();
    unsigned    rowBytes = this->rowBytes();
    SkBitmap    srcBM(*this), dstBM;

    mm->fLevel[0].fPixels   = this->getPixels();
    mm->fLevel[0].fWidth    = SkToU16(width);
    mm->fLevel[0].fHeight   = SkToU16(height);
    mm->fLevel[0].fRowBytes = SkToU16(rowBytes);
    mm->fLevel[0].fConfig   = SkToU8(this->getConfig());
    mm->fLevel[0].fShift    = SkToU8(shift);

    for (int i = 1; i < kMaxMipLevels; i++)
    {
        width = (width + 1) >> 1;
        height = (height + 1) >> 1;
        rowBytes = width << shift;

        mm->fLevel[i].fPixels   = addr;
        mm->fLevel[i].fWidth    = SkToU16(width);
        mm->fLevel[i].fHeight   = SkToU16(height);
        mm->fLevel[i].fRowBytes = SkToU16(rowBytes);
        mm->fLevel[i].fConfig   = SkToU8(this->getConfig());
        mm->fLevel[i].fShift    = SkToU8(shift);

        dstBM.setConfig(this->getConfig(), width, height, rowBytes);
        dstBM.setPixels(addr);
    
        for (unsigned y = 0; y < height; y++)
            for (unsigned x = 0; x < width; x++)
                proc(&dstBM, x, y, srcBM);

        srcBM = dstBM;
        addr += height * rowBytes;
    }
    SkASSERT(addr == (uint8_t*)mm->fLevel[1].fPixels + size);

    fMipMap = mm;
    fFlags |= kWeOwnTheMipMap_Flag;
#endif
}

unsigned SkBitmap::countMipLevels() const
{
#ifdef SK_SUPPORT_MIPMAP
    return fMipMap ? kMaxMipLevels : 0;
#else
    return 0;
#endif
}

#ifdef SK_SUPPORT_MIPMAP
const SkBitmap::MipLevel* SkBitmap::getMipLevel(unsigned level) const
{
    SkASSERT(level < this->countMipLevels());

    return &fMipMap->fLevel[level];
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////

static void GetBitmapAlpha(const SkBitmap& src, uint8_t alpha[], int alphaRowBytes)
{
    SkASSERT(alpha != NULL);
    SkASSERT(alphaRowBytes >= src.width());

    SkBitmap::Config config = src.getConfig();
    int              w = src.width();
    int              h = src.height();
    int              rb = src.rowBytes();

    if (SkBitmap::kA8_Config == config && !src.isOpaque())
    {
        const uint8_t* s = src.getAddr8(0, 0);
        while (--h >= 0)
        {
            memcpy(alpha, s, w);
            s += rb;
            alpha += alphaRowBytes;
        }
    }
    else if (SkBitmap::kARGB_8888_Config == config && !src.isOpaque())
    {
        const SkPMColor* s = src.getAddr32(0, 0);
        while (--h >= 0)
        {
            for (int x = 0; x < w; x++)
            {
                alpha[x] = SkGetPackedA32(s[x]);
            }
            s = (const SkPMColor*)((const char*)s + rb);
            alpha += alphaRowBytes;
        }
    }
    else    // src is opaque, so just fill alpha[] with 0xFF
    {
        memset(alpha, 0xFF, h * alphaRowBytes);
    }
}

#include "SkPaint.h"
#include "SkMaskFilter.h"
#include "SkMatrix.h"

void SkBitmap::extractAlpha(SkBitmap* dst, const SkPaint* paint, SkPoint16* offset) const
{
    SkMatrix    identity;
    SkMask      srcM, dstM;

    srcM.fBounds.set(0, 0, this->width(), this->height());
    srcM.fRowBytes = SkAlign4(this->width());
    srcM.fFormat = SkMask::kA8_Format;

    SkMaskFilter* filter = paint ? paint->getMaskFilter() : NULL;

    // compute our (larger?) dst bounds if we have a filter
    if (NULL != filter)
    {
        identity.reset();
        srcM.fImage = NULL;
        if (!filter->filterMask(&dstM, srcM, identity, NULL))
            goto NO_FILTER_CASE;
        dstM.fRowBytes = SkAlign4(dstM.fBounds.width());
    }
    else
    {
    NO_FILTER_CASE:
        dst->setConfig(SkBitmap::kA8_Config, this->width(), this->height(), srcM.fRowBytes);
        dst->allocPixels();        
        GetBitmapAlpha(*this, dst->getAddr8(0, 0), srcM.fRowBytes);
        if (offset)
            offset->set(0, 0);
        return;
    }

    SkAutoMaskImage srcCleanup(&srcM, true);

    GetBitmapAlpha(*this, srcM.fImage, srcM.fRowBytes);
    if (!filter->filterMask(&dstM, srcM, identity, NULL))
        goto NO_FILTER_CASE;
    
    SkAutoMaskImage dstCleanup(&dstM, false);

    dst->setConfig(SkBitmap::kA8_Config, dstM.fBounds.width(), dstM.fBounds.height(), dstM.fRowBytes);
    dst->allocPixels();
    memcpy(dst->getPixels(), dstM.fImage, dstM.computeImageSize());
    if (offset)
        offset->set(dstM.fBounds.fLeft, dstM.fBounds.fTop);
}


