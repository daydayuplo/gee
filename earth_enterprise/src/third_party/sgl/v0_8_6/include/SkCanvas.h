/* include/graphics/SkCanvas.h
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

#ifndef SkCanvas_DEFINED
#define SkCanvas_DEFINED

#include "SkBitmap.h"
#include "SkDeque.h"
#include "SkDevice.h"
#include "SkNoncopyable.h"
#include "SkPaint.h"
#include "SkPorterDuff.h"
#include "SkPath.h"
#include "SkRegion.h"

class SkBounder;

/** \class SkCanvas

    The SkCanvas class holds the "draw" calls. To draw something, you need
    4 basic components: A SkBitmap to hold the pixels, a SkCanvas to host
    the draw calls (writing into the bitmap), a drawing primitive (e.g. SkRect, SkPath,
    text, SkBitmap), and a paint (to describe the colors and styles for the drawing).
*/
class SkCanvas : SkNoncopyable {
public:
    /** Construct a canvas with the specified bitmap to draw into.
        @param bitmap   Specifies a bitmap for the canvas to draw into. Its contents are copied to the canvas.
    */
    explicit SkCanvas(const SkBitmap& bitmap);
    /** Construct a canvas with the specified device to draw into.
        @param device   Specifies a device for the canvas to draw into.
    */
    explicit SkCanvas(SkDevice* device = NULL);
    virtual ~SkCanvas();

    /** Return the canvas' device. Note that this may not be the current device, if saveLayer()
        has been called. To retrieve the current active device, call getCurrDevice().
    */
    SkDevice* getDevice() const { return fDevice; }
    /** Assign a device to the canvas. If a previous device is install, decrement its
        reference count. If the new device is not null, increment its reference
        count.
        @param device device for the canvas to draw into. May be null.
        @return the specified device
    */
    virtual SkDevice* setDevice(SkDevice* device);

    /** Helper method that creates a device to encapsulate the bitmap.
    */
    void setBitmapDevice(const SkBitmap& bitmap, bool transferPixelOwnershp = false);
    
    /** This call saves the current matrix and clip information, and pushes a copy onto a
        private stack. Subsequent calls to translate,scale,rotate,skew,concat or clipRect,clipPath
        all operate on this copy. When the balancing call to restore() is made, this copy is deleted
        and the previous matrix/clip state is restored.
        @return The value to pass to restoreToCount() to balance this save()
    */
    virtual int save();
    
    enum LayerFlags {
        kHasAlpha_LayerFlag     = 0x01, //!< the layer needs to support per-pixel alpha
        kFullColor_LayerFlag    = 0x02  //!< the layer needs to support 8-bits per color component
    };

    /** This behaves the same as save(), but in addition it allocates an offscreen bitmap.
        All drawing calls are directed there, and only when the balancing call to restore() is made
        is that offscreen transfered to the canvas (or the previous layer).
        Subsequent calls to translate,scale,rotate,skew,concat or clipRect,clipPath
        all operate on this copy. When the balancing call to restore() is made, this copy is deleted
        and the previous matrix/clip state is restored.
        @param bounds Optional: the maximum size the offscreen bitmap needs to be (in local coordinates)
        @param paint This is copied, and is applied to the offscreen when restore() is called.
        @param flags  LayerFlags
        @return The value to pass to restoreToCount() to balance this save()
    */
    virtual int saveLayer(const SkRect* bounds, const SkPaint& paint, LayerFlags flags);        
    /** This behaves the same as save(), but in addition it allocates an offscreen bitmap.
        All drawing calls are directed there, and only when the balancing call to restore() is made
        is that offscreen transfered to the canvas (or the previous layer).
        Subsequent calls to translate,scale,rotate,skew,concat or clipRect,clipPath
        all operate on this copy. When the balancing call to restore() is made, this copy is deleted
        and the previous matrix/clip state is restored.
        @param bounds Optional: the maximum size the offscreen bitmap needs to be (in local coordinates)
        @param alpha  This is applied to the offscreen when restore() is called.
        @param flags  LayerFlags
        @return The value to pass to restoreToCount() to balance this save()
    */
    int saveLayerAlpha(const SkRect* bounds, U8CPU alpha, LayerFlags flags);
    /** This call balances a previous call to save(), and is used to remove all modifications to
        the matrix/clip state since the last save call. It is an error to call restore() more times
        than save() was called.
    */
    virtual void restore();
    /** Returns the number of matrix/clip states on the SkCanvas' private stack. This will equal
        # save() calls - # restore() calls.
    */
    int getSaveCount() const;
    /** Efficient way to pop any calls to save() that happened after the save count reached saveCount.
        It is an error for saveCount to be less than getSaveCount()
        @param saveCount    The number of save() levels to restore from
    */
    void restoreToCount(int saveCount);

    /** Preconcat the current matrix with the specified translation
        @param dx   The distance to translate in X
        @param dy   The distance to translate in Y
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool translate(SkScalar dx, SkScalar dy);
    /** Preconcat the current matrix with the specified scale and pivot point.
        The pivot is the point that will remain unchanged after the scale is applied.
        @param sx   The amount to scale in X, about the pivot point (px,py)
        @param sy   The amount to scale in Y, about the pivot point (px,py)
        @param px   The pivot's X coordinate
        @param py   The pivot's Y coordinate
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool scale(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py);
    /** Preconcat the current matrix with the specified scale.
        @param sx   The amount to scale in X
        @param sy   The amount to scale in Y
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool scale(SkScalar sx, SkScalar sy);
    /** Preconcat the current matrix with the specified rotation and pivot point.
        The pivot is the point that will remain unchanged after the rotation is applied.
        @param degrees  The amount to rotate, in degrees
        @param px   The pivot's X coordinate
        @param py   The pivot's Y coordinate
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool rotate(SkScalar degrees, SkScalar px, SkScalar py);
    /** Preconcat the current matrix with the specified rotation.
        @param degrees  The amount to rotate, in degrees
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool rotate(SkScalar degrees);
    /** Preconcat the current matrix with the specified skew and pivot point.
        The pivot is the point that will remain unchanged after the skew is applied.
        @param sx   The amount to skew in X, about the pivot point (px,py)
        @param sy   The amount to skew in Y, about the pivot point (px,py)
        @param px   The pivot's X coordinate
        @param py   The pivot's Y coordinate
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool skew(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py);
    /** Preconcat the current matrix with the specified skew.
        @param sx   The amount to skew in X
        @param sy   The amount to skew in Y
        returns true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool skew(SkScalar sx, SkScalar sy);
    /** Preconcat the current matrix with the specified matrix.
        @param matrix   The matrix to preconcatenate with the current matrix
        @return true if the operation succeeded (e.g. did not overflow)
    */
    virtual bool concat(const SkMatrix& matrix);

    /** Intersect the current clip with the specified rectangle.
        @param rect The rect to intersect with the current clip
        @return true if the canvas' clip is non-empty
    */
    virtual bool clipRect(const SkRect& rect);
    /** Intersect the current clip with the specified rectangle.
        @param left     The left side of the rectangle to intersect with the current clip
        @param top      The top side of the rectangle to intersect with the current clip
        @param right    The right side of the rectangle to intersect with the current clip
        @param bottom   The bottom side of the rectangle to intersect with the current clip
        @return true if the canvas' clip is non-empty
    */
    bool clipRect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom);
    /** Intersect the current clip with the specified path.
        @param path The path to intersect with the current clip
        @return true if the canvas' clip is non-empty
    */
    virtual bool clipPath(const SkPath& path);
    /** Intersect the current clip with the specified region. Note that unlike clipRect()
        and clipPath() which transform their arguments by the current matrix, clipDeviceRgn()
        assumes its argument is already in the coordinate system of the current layer's bitmap,
        and so not transformation is performed.
        @param deviceRgn    The region to intersect with the current clip
        @return true if the canvas' clip is non-empty
    */
    virtual bool clipDeviceRgn(const SkRegion& deviceRgn);

    /** Return true if the specified rectangle, after being transformed by the current
        matrix, would lie completely outside of the current clip. Call this to check
        if an area you intend to draw into is clipped out (and therefore you can skip
        making the draw calls).
        @param rect the rect to compare with the current clip
        @param antialiased  true if the rect should be considered antialiased, since that means it may
                            affect a larger area (more pixels) than non-antialiased.
        @return true if the rect (transformed by the canvas' matrix) does not intersect with the canvas' clip
    */
    bool quickReject(const SkRect& rect, bool antialiased = false) const;
    /** Return true if the specified path, after being transformed by the current
        matrix, would lie completely outside of the current clip. Call this to check
        if an area you intend to draw into is clipped out (and therefore you can skip
        making the draw calls).
        Note, for speed it may return false even if the path itself might not intersect
        the clip (i.e. the bounds of the path intersects, but the path doesnot).
        @param path The path to compare with the current clip
        @param antialiased  true if the path should be considered antialiased, since that means it may
                            affect a larger area (more pixels) than non-antialiased.
        @return true if the path (transformed by the canvas' matrix) does not intersect with the canvas' clip
    */
    bool quickReject(const SkPath& path, bool antialiased = false) const;
    /** Return true if the specified rectangle, after being transformed by the current
        matrix, would lie completely outside of the current clip. Call this to check
        if an area you intend to draw into is clipped out (and therefore you can skip
        making the draw calls).
        @param left     The left side of the rectangle to compare with the current clip
        @param top      The top side of the rectangle to compare with the current clip
        @param right    The right side of the rectangle to compare with the current clip
        @param bottom   The bottom side of the rectangle to compare with the current clip
        @param antialiased  true if the rect should be considered antialiased, since that means it may
                            affect a larger area (more pixels) than non-antialiased.
        @return true if the rect (transformed by the canvas' matrix) does not intersect with the canvas' clip
    */
    bool quickReject(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom, bool antialiased = false) const
    {
        SkRect r;
        r.set(left, top, right, bottom);
        return this->quickReject(r, antialiased);
    }

    /** Return the bounds of the current clip (in local coordinates) in the bounds
        parameter, and return true if it is non-empty. This can be useful in a way similar to
        quickReject, in that it tells you that drawing outside of these bounds will be clipped
        out.
    */
    bool getClipBounds(SkRect* bounds) const;

    /** Fill the entire canvas' bitmap (restricted to the current clip) with the
        specified RGB color, using srcover porterduff mode.
        @param r    the red component (0..255) of the color used to draw onto the canvas
        @param g    the green component (0..255) of the color used to draw onto the canvas
        @param b    the blue component (0..255) of the color used to draw onto the canvas
    */
    void drawRGB(U8CPU r, U8CPU g, U8CPU b);
    /** Fill the entire canvas' bitmap (restricted to the current clip) with the
        specified ARGB color, using srcover porterduff mode.
        @param a    the alpha component (0..255) of the color used to draw onto the canvas
        @param r    the red component (0..255) of the color used to draw onto the canvas
        @param g    the green component (0..255) of the color used to draw onto the canvas
        @param b    the blue component (0..255) of the color used to draw onto the canvas
    */
    void drawARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b);
    /** Fill the entire canvas' bitmap (restricted to the current clip) with the
        specified color, using srcover porterduff mode.
        @param color    the color to draw onto the canvas
    */
    void drawColor(SkColor color)
    {
        this->drawColor(color, SkPorterDuff::kSrcOver_Mode);
    }
    /** Fill the entire canvas' bitmap (restricted to the current clip) with the
        specified color and porter-duff xfermode.
        @param color    the color to draw with
        @param mode     the porter-duff mode to apply to the color
    */
    void drawColor(SkColor color, SkPorterDuff::Mode mode);

    /** Fill the entire canvas' bitmap (restricted to the current clip) with the
        specified paint. This is equivalent (but faster) to drawing an infinitely
        large rectangle with the specified paint.
        @param paint    The paint used to draw onto the canvas
    */
    virtual void drawPaint(const SkPaint& paint);
    /** Draw a line segment with the specified start and stop points, using the specified
        paint. NOTE: since a line is always "framed", the Style is ignored in
        the paint.
        @param start    The start point of the line
        @param stop     The stop point of the line
        @param paint    The paint used to draw the line
    */
    virtual void drawLine(const SkPoint& start, const SkPoint& stop, const SkPaint& paint);
    /** Draw a line segment with the specified start and stop x,y coordinates, using the specified
        paint. NOTE: since a line is always "framed", the Style is ignored in
        the paint.
        @param startX   The x-coordinate of the start point of the line
        @param startY   The y-coordinate of the start point of the line
        @param endX     The x-coordinate of the end point of the line
        @param endY     The y-coordinate of the end point of the line
        @param paint    The paint used to draw the line
    */
    void drawLine(SkScalar startX, SkScalar startY, SkScalar stopX, SkScalar stopY, const SkPaint& paint);
    /** Draw the specified SkRect using the specified paint. The rectangle will be filled
        or framed based on the Style in the paint.
        @param rect     The rect to be drawn
        @param paint    The paint used to draw the rect
    */
    virtual void drawRect(const SkRect& rect, const SkPaint& paint);
    /** Draw the specified SkRect using the specified paint. The rectangle will be filled
        or framed based on the Style in the paint.
        @param left     The left side of the rectangle to be drawn
        @param top      The top side of the rectangle to be drawn
        @param right    The right side of the rectangle to be drawn
        @param bottom   The bottom side of the rectangle to be drawn
        @param paint    The paint used to draw the rect
    */
    void drawRect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom, const SkPaint& paint);
    /** Draw the specified oval using the specified paint. The oval will be filled
        or framed based on the Style in the paint.
        @param oval     The rectangle bounds of the oval to be drawn
        @param paint    The paint used to draw the oval
    */
    virtual void drawOval(const SkRect& oval, const SkPaint&);
    /** Draw the specified circle using the specified paint. If radius is <= 0, then
        nothing will be drawn. The circle will be filled
        or framed based on the Style in the paint.
        @param cx       The x-coordinate of the center of the cirle to be drawn
        @param cy       The y-coordinate of the center of the cirle to be drawn
        @param radius   The radius of the cirle to be drawn
        @param paint    The paint used to draw the circle
    */
    void drawCircle(SkScalar cx, SkScalar cy, SkScalar radius, const SkPaint& paint);
    /** Draw the specified arc, which will be scaled to fit inside the specified oval.
        @param oval The bounds of oval used to define the shape and size of the arc
        @param startAngle Starting angle (in degrees) where the arc begins
        @param sweepAngle Sweep angle (in degrees) measured clockwise
        @param paint    The paint used to draw the arc
    */
    virtual void drawArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle, const SkPaint& paint);
    /** Draw the specified round-rect using the specified paint. The round-rect will be filled
        or framed based on the Style in the paint.
        @param rect     The rectangular bounds of the roundRect to be drawn
        @param rx       The x-radius of the oval used to round the corners
        @param ry       The y-radius of the oval used to round the corners
        @param paint    The paint used to draw the roundRect
    */
    virtual void drawRoundRect(const SkRect& rect, SkScalar rx, SkScalar ry, const SkPaint& paint);
    /** Draw the specified path using the specified paint. The path will be filled
        or framed based on the Style in the paint.
        @param path     The path to be drawn
        @param paint    The paint used to draw the path
    */
    virtual void drawPath(const SkPath& path, const SkPaint& paint);
    /** Draw the specified bitmap, with its top/left corner at (x,y), using the specified paint,
        transformed by the current matrix.
        @param bitmap   The bitmap to be drawn
        @param left     The position of the left side of the bitmap being drawn
        @param top      The position of the top side of the bitmap being drawn
        @param paint    The paint used to draw the bitmap
    */
    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top, const SkPaint& paint);
    /** Draw the specified bitmap, with its top/left corner at (x,y), transformed
        by the current matrix. Since no paint is specified, the bitmap is drawn with no overriding
        alpha or colorfilter, and in srcover porterduff mode.
        @param bitmap   The bitmap to be drawn
        @param left     The position of the left side of the bitmap being drawn
        @param top      The position of the top side of the bitmap being drawn
    */
    void drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top);
    /** Draw the specified bitmap, scaled to fit into the destination rectangle.
        If the source rectangle is not null, then only the subset of the bitmap that
        intersects that rectangle will be drawn.
        @param bitmap   The bitmap to be drawn
        @param src      Optional: specify the subset of the bitmap to be drawn
        @param dst      The destination rectangle where the scaled/translated image will be drawn
        @param paint    The paint used to draw the bitmap
    */
    void drawBitmap(const SkBitmap& bitmap, const SkRect16* src, const SkRect& dst, const SkPaint& paint);
    /** Draw the specified bitmap, with its top/left corner at (x,y), NOT transformed
        by the current matrix. This method is not exported to java.
        @param bitmap   The bitmap to be drawn
        @param left     The position of the left side of the bitmap being drawn
        @param top      The position of the top side of the bitmap being drawn
        @param paint    The paint used to draw the bitmap
    */
    virtual void drawSprite(const SkBitmap& bitmap, int left, int top, const SkPaint& paint);
    /** Draw the text, with origin at (x,y), using the specified paint. The origin is interpreted
        based on the Align setting in the paint.
        @param text The text to be drawn
        @param byteLength   The number of bytes to read from the text parameter
        @param x        The x-coordinate of the origin of the text being drawn
        @param y        The y-coordinate of the origin of the text being drawn
        @param paint    The paint used for the text (e.g. color, size, style)
    */
    virtual void drawText(const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint);
    /** Draw the text, with origin at (x,y), using the specified paint. The origin is interpreted
        based on the Align setting in the paint.
        @param text The text to be drawn
        @param byteLength   The number of bytes to read from the text parameter
        @param pos      Array of positions, used to position each character
        @param paint    The paint used for the text (e.g. color, size, style)
    */
    virtual void drawPosText(const void* text, size_t byteLength, const SkPoint pos[], const SkPaint& paint);
    /** Draw the text, with origin at (x,y), using the specified paint, along the specified path.
        The paint's Align setting determins where along the path to start the text.
        @param text The text to be drawn
        @param byteLength   The number of bytes to read from the text parameter
        @param path         The path the text should follow for its baseline
        @param hOffset      The distance along the path to add to the text's starting position
        @param vOffset      The distance above(-) or below(+) the path to position the text
        @param paint        The paint used for the text (e.g. color, size, style)
    */
    void drawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                SkScalar hOffset, SkScalar vOffset, const SkPaint& paint);

    /** Draw the text, with origin at (x,y), using the specified paint, along the specified path.
        The paint's Align setting determins where along the path to start the text.
        @param text The text to be drawn
        @param byteLength   The number of bytes to read from the text parameter
        @param path         The path the text should follow for its baseline
        @param matrix       (may be null) Applied to the text before it is mapped onto the path
        @param paint        The paint used for the text (e.g. color, size, style)
        */
    virtual void drawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                const SkMatrix* matrix, const SkPaint& paint);

    /** Return the current set mask, used to temporarily modify the paint's flags
        when something is being drawin.
    */
    uint32_t getPaintSetBits() const;
    /** Return the current clear mask, used to temporarily modify the paint's flags
        when something is being drawin.
    */
    uint32_t getPaintClearBits() const;
    /** Set the current set and clear masks, used to temporarily modify the paint's flags
        when something is being drawin. The setBits are applied before the clrBits.
        These values are subject to save/restore, just like the matrix and clip.
        @param setBits  A mask of bits to be OR'd into the paint's flag bits
        @param clrBits  A mask of bits to be cleared from the paint's flag bits
    */
    virtual void setPaintSetClearBits(uint32_t setBits, uint32_t clrBits);
    /** Helper for getPaintSetClearBits/setPaintSetClearBits. The parameters are OR'd into
        the current values, rather than replacing them as with setPaintSetClearBits.
        @param setBits  A mask of bits to be OR'd with the existing setBits on the canvas
        @param clearBits  A mask of bits to be OR'd with the existing clearBits on the canvas
    */
    void orPaintSetClearBits(uint32_t setBits, uint32_t clearBits);

    /** Get the current bounder object. The bounder's reference count is not affected.
        This method is not exported to java.
        @return the canva's bounder (or NULL).
    */
    SkBounder*  getBounder() const { return fBounder; }
    /** Set a new bounder (or NULL).
        Pass NULL to clear any previous bounder.
        As a convenience, the parameter passed is also returned.
        If a previous bounder exists, its reference count is decremented.
        If bounder is not NULL, its reference count is incremented.
        This method is not exported to java.
        @param bounder the new bounder (or NULL) to be installed in the canvas
        @return the set bounder object
    */
    virtual SkBounder* setBounder(SkBounder*);

    /** Returns the currently active device, which may not be the same as calling getDevice(),
        depending if 1) setDevice() was called with null, and 2) if saveLayer() has been called.
    */
    SkDevice& getCurrDevice() const;

    /** Return the MapPtProc for the current matrix on the canvas.
        This method is not exported to java.
        @return the MapPtProc for the current matrix on the canvas.
    */
    SkMatrix::MapPtProc getCurrMapPtProc() const;
    /** Return the current matrix on the canvas.
        This method is not exported to java.
        @return The current matrix on the canvas.
    */
    const SkMatrix& getTotalMatrix() const;
    /** Return the current device clip (concatenation of all clip calls).
        This method is not exported to java.
        @return the current device clip (concatenation of all clip calls).
    */
    const SkRegion& getTotalClip() const;

protected:
    virtual SkDevice* createDevice(SkBitmap::Config, int width, int height, bool isOpaque);

private:
    struct MCRec;

    SkDeque     fMCStack;
    MCRec*      fMCRec;             // points to top of stack
    uint32_t    fMCRecStorage[32];  // the first N recs that can fit here mean we won't call malloc

    SkDevice*   fDevice;
    SkBounder*  fBounder;
    SkDevice    fEmptyDevice;   // fMCRec points at this if fDevice is NULL

    friend class SkDraw;

    void init(SkDevice*);
};

/** Stack helper class to automatically call restoreToCount() on the canvas
    when this object goes out of scope. Use this to guarantee that the canvas
    is restored to a known state.
*/
class SkAutoCanvasRestore : SkNoncopyable {
public:
    SkAutoCanvasRestore(SkCanvas* canvas, bool doSave) : fCanvas(canvas)
    {
        SkASSERT(canvas);
        fSaveCount = canvas->getSaveCount();
        if (doSave)
            canvas->save();
    }
    ~SkAutoCanvasRestore()
    {
        fCanvas->restoreToCount(fSaveCount);
    }

private:
    SkCanvas*   fCanvas;
    int         fSaveCount;
};

#endif

