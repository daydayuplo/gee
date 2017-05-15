/* include/graphics/SkColorPriv.h
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

#ifndef SkColorPriv_DEFINED
#define SkColorPriv_DEFINED

#include "SkColor.h"

inline unsigned SkAlpha255To256(U8CPU alpha)
{
    SkASSERT(SkToU8(alpha) == alpha);
    return alpha + (alpha >> 7);
}

#define SkAlphaMul(value, alpha256)     ((value) * (alpha256) >> 8)

//  The caller may want negative values, so keep all params signed (int)
//  so we don't accidentally slip into unsigned math and lose the sign
//  extension when we shift (in SkAlphaMul)
inline int SkAlphaBlend(int src, int dst, int scale256)
{
    SkASSERT((unsigned)scale256 <= 256);
    return dst + SkAlphaMul(src - dst, scale256);
}

#define SK_R16_BITS     5
#define SK_G16_BITS     6
#define SK_B16_BITS     5

#define SK_R16_SHIFT    (SK_B16_BITS + SK_G16_BITS)
#define SK_G16_SHIFT    (SK_B16_BITS)
#define SK_B16_SHIFT    0

#define SK_R16_MASK     ((1 << SK_R16_BITS) - 1)
#define SK_G16_MASK     ((1 << SK_G16_BITS) - 1)
#define SK_B16_MASK     ((1 << SK_B16_BITS) - 1)

#define SkGetPackedR16(color)   (((unsigned)(color) >> SK_R16_SHIFT) & SK_R16_MASK)
#define SkGetPackedG16(color)   (((unsigned)(color) >> SK_G16_SHIFT) & SK_G16_MASK)
#define SkGetPackedB16(color)   (((unsigned)(color) >> SK_B16_SHIFT) & SK_B16_MASK)

inline uint16_t SkPackRGB16(unsigned r, unsigned g, unsigned b)
{
    SkASSERT(r <= SK_R16_MASK);
    SkASSERT(g <= SK_G16_MASK);
    SkASSERT(b <= SK_B16_MASK);

    return SkToU16((r << SK_R16_SHIFT) | (g << SK_G16_SHIFT) | (b << SK_B16_SHIFT));
}

#define SK_R16_MASK_IN_PLACE        (SK_R16_MASK << SK_R16_SHIFT)
#define SK_G16_MASK_IN_PLACE        (SK_G16_MASK << SK_G16_SHIFT)
#define SK_B16_MASK_IN_PLACE        (SK_B16_MASK << SK_B16_SHIFT)

/** Expand the 16bit color into a 32bit value that can be scaled all at once
    by a value up to 32. Used in conjunction with SkCompact_rgb_16.
*/
inline uint32_t SkExpand_rgb_16(U16CPU c)
{
    SkASSERT(c == (uint16_t)c);

    return ((c & SK_G16_MASK_IN_PLACE) << 16) | (c & ~SK_G16_MASK_IN_PLACE);
}

/** Compress an expanded value (from SkExpand_rgb_16) back down to a 16bit
    color value.
    NOTE: this explicitly does not clean the top 16 bits (which may contain garbage).
    It does this for speed, since if it is being written directly to 16bits of memory,
    the top 16bits will be ignored. Casting the result to int16_t here would add 2
    more instructions, slow us down. It is up to the caller to perform the cast if needed.
*/
static inline U16CPU SkCompact_rgb_16(uint32_t c)
{
    return ((c >> 16) & SK_G16_MASK_IN_PLACE) | (c & ~SK_G16_MASK_IN_PLACE);
}

/** Scale the 16bit color value by the 0..256 scale parameter.
    NOTE: this explicitly does not clean the top 16 bits (which may contain garbage).
    It does this for speed, since if it is being written directly to 16bits of memory,
    the top 16bits will be ignored. Casting the result to int16_t here would add 2
    more instructions, slow us down. It is up to the caller to perform the cast if needed.
*/
inline U16CPU SkAlphaMulRGB16(U16CPU c, unsigned scale)
{
#if SK_G16_MASK_IN_PLACE != 0x07E0
    return SkPackRGB16( SkAlphaMul(SkGetPackedR16(c), scale),
                        SkAlphaMul(SkGetPackedG16(c), scale),
                        SkAlphaMul(SkGetPackedB16(c), scale));
#else
    scale >>= (8 - SK_G16_BITS);
    uint32_t rb = (c & ~SK_G16_MASK_IN_PLACE) * scale >> SK_G16_BITS;
    uint32_t  g = (c & SK_G16_MASK_IN_PLACE) * scale >> SK_G16_BITS;
    return (g & SK_G16_MASK_IN_PLACE) | (rb & ~SK_G16_MASK_IN_PLACE);
#endif
}

/** Blend src and dst 16bit colors by the 0..256 scale parameter.
    NOTE: this explicitly does not clean the top 16 bits (which may contain garbage).
    It does this for speed, since if it is being written directly to 16bits of memory,
    the top 16bits will be ignored. Casting the result to int16_t here would add 2
    more instructions, slow us down. It is up to the caller to perform the cast if needed.
*/
inline U16CPU SkBlendRGB16(U16CPU src, U16CPU dst, int srcScale)
{
    SkASSERT((unsigned)srcScale <= 256);

    srcScale >>= 3;

    uint32_t es = SkExpand_rgb_16(src) * srcScale;
    uint32_t ed = SkExpand_rgb_16(dst) * (32 - srcScale);
    return SkCompact_rgb_16((es + ed) >> 5);
}

inline void SkBlendRGB16(const uint16_t src[], uint16_t dst[], int srcScale, int count)
{
    SkASSERT(count > 0);
    SkASSERT((unsigned)srcScale <= 256);
 
    srcScale >>= 3;
    int dstScale = 32 - srcScale;

    do {
        uint32_t es = SkExpand_rgb_16(*src++) * srcScale;
        uint32_t ed = SkExpand_rgb_16(*dst) * dstScale;
        *dst++ = SkCompact_rgb_16((es + ed) >> 5);
    } while (--count > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

#define SK_A32_BITS     8
#define SK_R32_BITS     8
#define SK_G32_BITS     8
#define SK_B32_BITS     8

/* we check to see if the SHIFT value has already been defined (SkUserConfig.h)
    if not, we define it ourself to some default values
*/
#ifndef SK_A32_SHIFT
    #define SK_A32_SHIFT    24
    #define SK_R32_SHIFT    16
    #define SK_G32_SHIFT    8
    #define SK_B32_SHIFT    0
#endif

#define SK_A32_MASK     ((1 << SK_A32_BITS) - 1)
#define SK_R32_MASK     ((1 << SK_R32_BITS) - 1)
#define SK_G32_MASK     ((1 << SK_G32_BITS) - 1)
#define SK_B32_MASK     ((1 << SK_B32_BITS) - 1)

#define SkGetPackedA32(packed)      ((uint32_t)((packed) << (24 - SK_A32_SHIFT)) >> 24)
#define SkGetPackedR32(packed)      ((uint32_t)((packed) << (24 - SK_R32_SHIFT)) >> 24)
#define SkGetPackedG32(packed)      ((uint32_t)((packed) << (24 - SK_G32_SHIFT)) >> 24)
#define SkGetPackedB32(packed)      ((uint32_t)((packed) << (24 - SK_B32_SHIFT)) >> 24)

inline SkPMColor SkPackARGB32(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    SkASSERT(a <= SK_A32_MASK);
    SkASSERT(r <= a);
    SkASSERT(g <= a);
    SkASSERT(b <= a);

    return (a << SK_A32_SHIFT) | (r << SK_R32_SHIFT) | (g << SK_G32_SHIFT) | (b << SK_B32_SHIFT);
}

extern const uint32_t gMask_00FF00FF;

inline uint32_t SkAlphaMulQ(uint32_t c, unsigned scale)
{
    uint32_t mask = gMask_00FF00FF;
//    uint32_t mask = 0xFF00FF;

    uint32_t rb = ((c & mask) * scale) >> 8;
    uint32_t ag = ((c >> 8) & mask) * scale;
    return (rb & mask) | (ag & ~mask);
}

inline SkPMColor SkPMSrcOver(SkPMColor src, SkPMColor dst)
{
    return src + SkAlphaMulQ(dst, SkAlpha255To256(255 - SkGetPackedA32(src)));
}

inline SkPMColor SkBlendARGB32(SkPMColor src, SkPMColor dst, U8CPU aa)
{
    SkASSERT((unsigned)aa <= 255);

    unsigned src_scale = SkAlpha255To256(aa);
    unsigned dst_scale = SkAlpha255To256(255 - SkAlphaMul(SkGetPackedA32(src), src_scale));

    return SkAlphaMulQ(src, src_scale) + SkAlphaMulQ(dst, dst_scale);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Convert a 32bit pixel to a 16bit pixel (no dither)

#define SkR32ToR16(r)   ((unsigned)(r) >> (SK_R32_BITS - SK_R16_BITS))
#define SkG32ToG16(g)   ((unsigned)(g) >> (SK_G32_BITS - SK_G16_BITS))
#define SkB32ToB16(b)   ((unsigned)(b) >> (SK_B32_BITS - SK_B16_BITS))

#define SkPacked32ToR16(c)  (((unsigned)(c) >> (SK_R32_SHIFT + SK_R32_BITS - SK_R16_BITS)) & SK_R16_MASK)
#define SkPacked32ToG16(c)  (((unsigned)(c) >> (SK_G32_SHIFT + SK_G32_BITS - SK_G16_BITS)) & SK_G16_MASK)
#define SkPacked32ToB16(c)  (((unsigned)(c) >> (SK_B32_SHIFT + SK_B32_BITS - SK_B16_BITS)) & SK_B16_MASK)

#define SK_R_32To16_DIFF   (SK_R32_SHIFT + SK_R32_BITS - SK_R16_BITS - SK_R16_SHIFT)
#define SK_G_32To16_DIFF   (SK_G32_SHIFT + SK_G32_BITS - SK_G16_BITS - SK_G16_SHIFT)
#define SK_B_32To16_DIFF   (SK_B32_SHIFT + SK_B32_BITS - SK_B16_BITS - SK_B16_SHIFT)

inline U16CPU SkPixel32ToPixel16(SkPMColor src)
{
    /*  We want to shift and mask each component "in place", and then OR the answers together. This is
        faster than shft+mask each down to the low bits, and then shifting backup.
        However, we need to check for each component if the "shift" is down or up, hence the #if test
        for each component (too bad >> can't take a negative value).
    */
    return
#if SK_R_32To16_DIFF >= 0
    ((src >> SK_R_32To16_DIFF) & SK_R16_MASK_IN_PLACE)
#else
    ((src << -SK_R_32To16_DIFF) & SK_R16_MASK_IN_PLACE)
#endif    
    |
#if SK_G_32To16_DIFF >= 0
    ((src >> SK_G_32To16_DIFF) & SK_G16_MASK_IN_PLACE)
#else
    ((src << -SK_G_32To16_DIFF) & SK_G16_MASK_IN_PLACE)
#endif    
    |
#if SK_B_32To16_DIFF >= 0
    ((src >> SK_B_32To16_DIFF) & SK_B16_MASK_IN_PLACE)
#else
    ((src << -SK_B_32To16_DIFF) & SK_B16_MASK_IN_PLACE)
#endif    
    ;
}

inline U16CPU SkPack888ToRGB16(U8CPU r, U8CPU g, U8CPU b)
{
    return  (SkR32ToR16(r) << SK_R16_SHIFT) |
            (SkG32ToG16(g) << SK_G16_SHIFT) |
            (SkB32ToB16(b) << SK_B16_SHIFT);
}

#define SkPixel32ToPixel16_ToU16(src)   SkToU16(SkPixel32ToPixel16(src))

/////////////////////////////////////////////////////////////////////////////////////////
// Fast dither from 32->16

#define SkShouldDitherXY(x, y)  (((x) ^ (y)) & 1)

inline uint16_t SkDitherPack888ToRGB16(U8CPU r, U8CPU g, U8CPU b)
{
    r = ((r << 1) - ((r >> (8 - SK_R16_BITS) << (8 - SK_R16_BITS)) | (r >> SK_R16_BITS))) >> (8 - SK_R16_BITS);
    g = ((g << 1) - ((g >> (8 - SK_G16_BITS) << (8 - SK_G16_BITS)) | (g >> SK_G16_BITS))) >> (8 - SK_G16_BITS);
    b = ((b << 1) - ((b >> (8 - SK_B16_BITS) << (8 - SK_B16_BITS)) | (b >> SK_B16_BITS))) >> (8 - SK_B16_BITS);

   return SkPackRGB16(r, g, b);
}

inline uint16_t SkDitherPixel32ToPixel16(SkPMColor c)
{
    return SkDitherPack888ToRGB16(SkGetPackedR32(c), SkGetPackedG32(c), SkGetPackedB32(c));
}

////////////////////////////////////////////////////////////////////////////////////////////
// Convert a 16bit pixel to a 32bit pixel

inline unsigned SkR16ToR32(unsigned r)
{
    return (r << (8 - SK_R16_BITS)) | (r >> (2 * SK_R16_BITS - 8));
}
inline unsigned SkG16ToG32(unsigned g)
{
    return (g << (8 - SK_G16_BITS)) | (g >> (2 * SK_G16_BITS - 8));
}
inline unsigned SkB16ToB32(unsigned b)
{
    return (b << (8 - SK_B16_BITS)) | (b >> (2 * SK_B16_BITS - 8));
}

#define SkPacked16ToR32(c)      SkR16ToR32(SkGetPackedR16(c))
#define SkPacked16ToG32(c)      SkG16ToG32(SkGetPackedG16(c))
#define SkPacked16ToB32(c)      SkB16ToB32(SkGetPackedB16(c))

inline SkPMColor SkPixel16ToPixel32(U16CPU src)
{
    SkASSERT(src == SkToU16(src));

    unsigned    r = SkPacked16ToR32(src);
    unsigned    g = SkPacked16ToG32(src);
    unsigned    b = SkPacked16ToB32(src);

    SkASSERT((r >> (8 - SK_R16_BITS)) == SkGetPackedR16(src));
    SkASSERT((g >> (8 - SK_G16_BITS)) == SkGetPackedG16(src));
    SkASSERT((b >> (8 - SK_B16_BITS)) == SkGetPackedB16(src));

    return SkPackARGB32(0xFF, r, g, b);
}

#endif

