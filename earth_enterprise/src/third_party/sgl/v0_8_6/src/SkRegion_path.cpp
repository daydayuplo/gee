/* libs/graphics/sgl/SkRegion_path.cpp
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

#include "SkRegionPriv.h"
#include "SkBlitter.h"
#include "SkScan.h"
#include "SkTDArray.h"
#include "SkPath.h"

class SkRgnBuilder : public SkBlitter {
public:
    virtual ~SkRgnBuilder();
    
    void    init(int maxHeight, int maxTransitions);
    void    done()
    {
        if (fCurrScanline != NULL)
        {
            fCurrScanline->fXCount = SkToS16((int)(fCurrXPtr - fCurrScanline->firstX()));
            if (!this->collapsWithPrev()) // flush the last line
                fCurrScanline = fCurrScanline->nextScanline();
        }
    }

    int     computeRunCount() const;
    void    copyToRect(SkRect16*) const;
    void    copyToRgn(int16_t runs[]) const;

    virtual void blitH(int x, int y, int width);

#ifdef SK_DEBUG
    void dump() const
    {
        SkDebugf("SkRgnBuilder: Top = %d\n", fTop);
        const Scanline* line = (Scanline*)fStorage;
        while (line < fCurrScanline)
        {
            SkDebugf("SkRgnBuilder::Scanline: LastY=%d, fXCount=%d", line->fLastY, line->fXCount);
            for (int i = 0; i < line->fXCount; i++)
                SkDebugf(" %d", line->firstX()[i]);
            SkDebugf("\n");

            line = line->nextScanline();
        }
    }
#endif
private:
    struct Scanline {
        int16_t fLastY;
        int16_t fXCount;

        int16_t*    firstX() const { return (int16_t*)(this + 1); }
        Scanline*   nextScanline() const { return (Scanline*)((int16_t*)(this + 1) + fXCount); }
    };
    int16_t*    fStorage;
    Scanline*   fCurrScanline;
    Scanline*   fPrevScanline;
    int16_t*    fCurrXPtr;      //  points at next avialable x[] in fCurrScanline
    int16_t     fTop;           // first Y value
    
    int fStorageCount;

    bool collapsWithPrev()
    {
        if (fPrevScanline != NULL &&
            fPrevScanline->fLastY + 1 == fCurrScanline->fLastY &&
            fPrevScanline->fXCount == fCurrScanline->fXCount &&
            !memcmp(fPrevScanline->firstX(),
                    fCurrScanline->firstX(),
                    fCurrScanline->fXCount * sizeof(int16_t)))
        {
            // update the height of fPrevScanline
            fPrevScanline->fLastY = fCurrScanline->fLastY;
            return true;
        }
        return false;
    }
};

SkRgnBuilder::~SkRgnBuilder()
{
    sk_free(fStorage);
}

void SkRgnBuilder::init(int maxHeight, int maxTransitions)
{
    int count = maxHeight * (3 + maxTransitions);

    // add maxTransitions to have slop for working buffer
    fStorageCount = count + 3 + maxTransitions;
    fStorage = (int16_t*)sk_malloc_throw(fStorageCount * sizeof(int16_t));

    fCurrScanline = NULL;    // signal empty collection
    fPrevScanline = NULL;    // signal first scanline
}

void SkRgnBuilder::blitH(int x, int y, int width)
{
    if (fCurrScanline == NULL)   // first time
    {
        fTop = SkToS16(y);
        fCurrScanline = (Scanline*)fStorage;
        fCurrScanline->fLastY = SkToS16(y);
        fCurrXPtr = fCurrScanline->firstX();
    }
    else
    {
        SkASSERT(y >= fCurrScanline->fLastY);

        if (y > fCurrScanline->fLastY)
        {
            // if we get here, we're done with fCurrScanline
            fCurrScanline->fXCount = SkToS16((int)(fCurrXPtr - fCurrScanline->firstX()));

            int prevLastY = fCurrScanline->fLastY;
            if (!this->collapsWithPrev())
            {
                fPrevScanline = fCurrScanline;
                fCurrScanline = fCurrScanline->nextScanline();

            }
            if (y - 1 > prevLastY)  // insert empty run
            {
                fCurrScanline->fLastY = SkToS16(y - 1);
                fCurrScanline->fXCount = 0;
                fCurrScanline = fCurrScanline->nextScanline();
            }
            // setup for the new curr line
            fCurrScanline->fLastY = SkToS16(y);
            fCurrXPtr = fCurrScanline->firstX();
        }
    }
    //  check if we should extend the current run, or add a new one
    if (fCurrXPtr > fCurrScanline->firstX() && fCurrXPtr[-1] == x)
        fCurrXPtr[-1] = SkToS16(x + width);
    else
    {
        fCurrXPtr[0] = SkToS16(x);
        fCurrXPtr[1] = SkToS16(x + width);
        fCurrXPtr += 2;
    }
    SkASSERT(fCurrXPtr - fStorage < fStorageCount);
}

int SkRgnBuilder::computeRunCount() const
{
    if (fCurrScanline == NULL)
        return 0;

    const int16_t*  line = fStorage;
    const int16_t*  stop = (const int16_t*)fCurrScanline;

    return 2 + (int)(stop - line);
}

void SkRgnBuilder::copyToRect(SkRect16* r) const
{
    SkASSERT(fCurrScanline != NULL);
    SkASSERT((const int16_t*)fCurrScanline - fStorage == 4);

    const Scanline* line = (const Scanline*)fStorage;
    SkASSERT(line->fXCount == 2);

    r->set(line->firstX()[0], fTop, line->firstX()[1], line->fLastY + 1);
}

void SkRgnBuilder::copyToRgn(int16_t runs[]) const
{
    SkASSERT(fCurrScanline != NULL);
    SkASSERT((const int16_t*)fCurrScanline - fStorage > 4);

    const Scanline* line = (const Scanline*)fStorage;
    const Scanline* stop = fCurrScanline;

    *runs++ = fTop;
    do {
        *runs++ = SkToS16(line->fLastY + 1);
        int count = line->fXCount;
        if (count)
        {
            memcpy(runs, line->firstX(), count * sizeof(int16_t));
            runs += count;
        }
        *runs++ = kRunTypeSentinel;
        line = line->nextScanline();
    } while (line < stop);
    SkASSERT(line == stop);
    *runs = kRunTypeSentinel;
}

static int count_path_runtype_values(const SkPath& path, int* itop, int* ibot)
{
    static const uint8_t gPathVerbToInitialLastIndex[] = {
        0,  //  kMove_Verb
        1,  //  kLine_Verb
        2,  //  kQuad_Verb
        3,  //  kCubic_Verb
        0,  //  kClose_Verb
        0   //  kDone_Verb
    };

    static const uint8_t gPathVerbToMaxEdges[] = {
        0,  //  kMove_Verb
        1,  //  kLine_Verb
        2,  //  kQuad_VerbB
        3,  //  kCubic_Verb
        0,  //  kClose_Verb
        0   //  kDone_Verb
    };

    SkPath::Iter    iter(path, true);
    SkPoint         pts[4];
    SkPath::Verb    verb;

    int maxEdges = 0;
    SkScalar    top = SkIntToScalar(SK_MaxS16);
    SkScalar    bot = SkIntToScalar(SK_MinS16);

    while ((verb = iter.next(pts)) != SkPath::kDone_Verb)
    {
        maxEdges += gPathVerbToMaxEdges[verb];

        int lastIndex = gPathVerbToInitialLastIndex[verb];
        if (lastIndex > 0)
        {
            for (int i = 1; i <= lastIndex; i++)
            {
                if (top > pts[i].fY)
                    top = pts[i].fY;
                else if (bot < pts[i].fY)
                    bot = pts[i].fY;
            }
        }
        else if (SkPath::kMove_Verb == verb)
        {
            if (top > pts[0].fY)
                top = pts[0].fY;
            else if (bot < pts[0].fY)
                bot = pts[0].fY;
        }
    }
    SkASSERT(top <= bot);

    *itop = SkScalarRound(top);
    *ibot = SkScalarRound(bot);
    return maxEdges;
}

bool SkRegion::setPath(const SkPath& path, const SkRegion* clip)
{
    SkDEBUGCODE(this->validate();)

    if (path.isEmpty() || clip && clip->isEmpty())
        return this->setEmpty();

    //  compute worst-case rgn-size for the path
    int pathTop, pathBot;
    int pathTransitions = count_path_runtype_values(path, &pathTop, &pathBot);
    int clipTop, clipBot;
    int clipTransitions;
    
     if (clip)
        clipTransitions = clip->count_runtype_values(&clipTop, &clipBot);
    else
    {
        clipTransitions = 0;
        clipTop = pathTop;
        clipBot = pathBot;
    }

    int top = SkMax32(pathTop, clipTop);
    int bot = SkMin32(pathBot, clipBot);

    if (top >= bot)
        return this->setEmpty();

    SkRgnBuilder builder;
    
    builder.init(bot - top, SkMax32(pathTransitions, clipTransitions));
    SkScan::FillPath(path, clip, &builder);
    builder.done();

    int count = builder.computeRunCount();
    if (count == 0)
    {
        return this->setEmpty();
    }
    else if (count == kRectRegionRuns)
    {
        builder.copyToRect(&fBounds);
        this->setRect(fBounds);
    }
    else
    {
        SkRegion    tmp;

        tmp.fRunHead = RunHead::Alloc(count);
        builder.copyToRgn(tmp.fRunHead->runs());
        compute_run_bounds(tmp.fRunHead->runs(), count, &tmp.fBounds);
        this->swap(tmp);
    }
    SkDEBUGCODE(this->validate();)
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

struct SkPrivPoint {
    int fX, fY;
    
    friend int operator==(const SkPrivPoint& a, const SkPrivPoint& b)
    {
        return a.fX == b.fX && a.fY == b.fY;
    }
    friend int operator!=(const SkPrivPoint& a, const SkPrivPoint& b)
    {
        return a.fX != b.fX || a.fY != b.fY;
    }
};

struct SkPrivLine {
    SkPrivPoint   fP0, fP1;
    int     fWinding;
    SkPrivLine*   fNext, *fPrev;
    
    void set(int x, int y0, int y1)
    {
        fP0.fX = x;
        fP0.fY = y0;
        fP1.fX = x;
        fP1.fY = y1;
        fWinding = y1 > y0;     // 1: up, 0: down
        fNext = fPrev = NULL;
    }
    
    void detach()
    {
        fNext->fPrev = fPrev;
        fPrev->fNext = fNext;
    }
    
    void attachNext(SkPrivLine* next)
    {
        next->fNext = fNext;
        next->fPrev = this;
        fNext->fPrev = next;
        fNext = next;
    }
};

static SkPrivLine* find_match(SkPrivLine* ctr, SkPrivLine* skip)
{
    const SkPrivPoint pt = skip->fP1;
    int         winding = skip->fWinding;

    SkPrivLine* start = ctr;
    
    SkPrivLine*   closest_pos = NULL;
    int     dist_pos = 0x7FFFFFFF;
    SkPrivLine*   closest_neg = NULL;
    int     dist_neg = -0x7FFFFFFF;

    do {
        // find a Line one the same scan as pt
        if (ctr != skip && (ctr->fP0.fY == pt.fY || ctr->fP1.fY == pt.fY))
        {
            int dist = ctr->fP0.fX - pt.fX; // keep the sign

            if (dist == 0)
            {
                if (winding == ctr->fWinding)   // quick accept
                    goto FOUND;
                else
                    goto NEXT;                  // quick reject
            }
            
            if (dist < 0)
            {
                if (dist > dist_neg)
                {
                    dist_neg = dist;
                    closest_neg = ctr;
                }
            }
            else
            {
                if (dist < dist_pos)
                {
                    dist_pos = dist;
                    closest_pos = ctr;
                }
            }
        }
    NEXT:
        ctr = ctr->fNext;
    } while (ctr != start);

    SkASSERT(closest_pos != NULL || closest_neg != NULL);

    if (closest_pos == NULL)
        ctr = closest_neg;
    else if (closest_neg == NULL)
        ctr = closest_pos;
    else
    {
        if (closest_neg->fP0.fY != pt.fY)
            ctr = closest_pos;
        else if (closest_pos->fP0.fY != pt.fY)
            ctr = closest_neg;
        else
        {
            if (closest_pos->fWinding != closest_neg->fWinding)
            {
                if (closest_pos->fWinding == winding)
                    ctr = closest_pos;
                else
                    ctr = closest_neg;
            }
            else
            {
                if (winding == 0)
                    ctr = closest_pos;
                else
                    ctr = closest_neg;
            }
        }
    }

FOUND:
    SkASSERT(ctr && ctr->fP0.fY == pt.fY);
    return ctr;
}

static void LinesToPath(SkPrivLine lines[], int count, SkPath* path)
{
    SkASSERT(count > 1);

    // turn the array into a linked list
    lines[0].fNext = &lines[1];
    lines[0].fPrev = &lines[count - 1];
    for (int i = 1; i < count - 1; i++)
    {
        lines[i].fNext = &lines[i+1];
        lines[i].fPrev = &lines[i-1];
    }
    lines[count - 1].fNext = &lines[0];
    lines[count - 1].fPrev = &lines[count - 2];
    
    SkPrivLine* head = lines;

    // loop through looking for contours
    while (count > 0)
    {
        SkPrivLine* ctr = head;
        SkPrivLine* first = ctr;
        head = head->fNext;
        
        path->moveTo(SkIntToScalar(ctr->fP0.fX), SkIntToScalar(ctr->fP0.fY));
        do {
            SkPrivLine* next = find_match(head, ctr);

            if (ctr->fP1 != next->fP0)
            {
                path->lineTo(SkIntToScalar(ctr->fP1.fX), SkIntToScalar(ctr->fP1.fY));      // Vertical
                path->lineTo(SkIntToScalar(next->fP0.fX), SkIntToScalar(next->fP0.fY));    // Horzontal
            }
            if (head == next)
                head = head->fNext;
            next->detach();
            count -= 1;
            ctr = next;
        } while (ctr != first);
        path->close();
        ctr->detach();
        count -= 1;
    }
//    SkASSERT(count == 0);
}

bool SkRegion::getBoundaryPath(SkPath* path) const
{
    if (this->isEmpty())
        return false;

    const SkRect16& bounds = this->getBounds();

    if (this->isRect())
    {
        SkRect  r;        
        r.set(bounds);      // this converts the ints to scalars
        path->addRect(r);
        return true;
    }

    SkRegion::Iterator      iter(*this);
    SkTDArray<SkPrivLine>     lines;
    
    for (const SkRect16& r = iter.rect(); !iter.done(); iter.next())
    {
        SkPrivLine* line = lines.append(2);
        line[0].set(r.fLeft, r.fBottom, r.fTop);
        line[1].set(r.fRight, r.fTop, r.fBottom);
    }
    
    LinesToPath(lines.begin(), lines.count(), path);
    return true;
}


