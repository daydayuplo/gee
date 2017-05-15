/* libs/graphics/images/SkBitmapRef.cpp
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

#include "SkBitmapRefPriv.h"
#include "SkTemplates.h"
#include "SkThread.h"
#include "SkString.h"
#include "SkGlobals.h"
#include "SkThread.h"

SkGlobals::Rec* SkBitmapRef_Globals::Create()
{
    SkBitmapRef_Globals* rec = SkNEW(SkBitmapRef_Globals);
    rec->fCache = NULL;
    return rec;
}

///////////////////////////////////////////////////////////////////////////////////////////

SkBitmapRef::SkBitmapRef(Rec* rec) : fRec(rec)
{
    SkASSERT(rec);
    rec->fRefCnt += 1;
}

SkBitmapRef::SkBitmapRef(const SkBitmap& src, bool transferOwnsPixels)
{
    fRec = SkNEW_ARGS(SkBitmapRef::Rec, (src));
    fRec->fIsCache = false;
    if (transferOwnsPixels)
    {
        fRec->fBM.setOwnsPixels(src.getOwnsPixels());
        ((SkBitmap*)&src)->setOwnsPixels(false);
    }
}
  
SkBitmapRef::~SkBitmapRef()
{
    if (fRec->fIsCache)
    {
        SkBitmapRef_Globals& globals = *(SkBitmapRef_Globals*)SkGlobals::Find(kBitmapRef_GlobalsTag,
                                                                              SkBitmapRef_Globals::Create);
        SkAutoMutexAcquire  ac(globals.fMutex);

        SkASSERT(fRec->fRefCnt > 0);
        fRec->fRefCnt -= 1;
    }
    else
    {
        SkDELETE(fRec);
    }
}

const SkBitmap& SkBitmapRef::bitmap()
{
    return fRec->fBM;
}

///////////////////////////////////////////////////////////////////////////////////////////

SkBitmapRef* SkBitmapRef::create(const SkBitmap& src, bool transferOwnsPixels)
{
    return SkNEW_ARGS(SkBitmapRef, (src, transferOwnsPixels));
}

void SkBitmapRef::PurgeCacheAll()
{
    SkBitmapRef_Globals* globals = (SkBitmapRef_Globals*)SkGlobals::Find(kBitmapRef_GlobalsTag, NULL);
    if (globals == NULL)
        return;

    SkAutoMutexAcquire  ac(globals->fMutex);
    SkBitmapRef::Rec*   rec = globals->fCache;
    SkDEBUGCODE(int     count = 0;)

    while (rec)
    {
        SkDEBUGCODE(count += 1;)
        SkASSERT(rec->fRefCnt == 0);
        Rec* next = rec->fNext;
        SkDELETE(rec);
        rec = next;
    }
    globals->fCache = NULL;
    
    SkDEBUGF(("PurgeCacheAll freed %d bitmaps\n", count));
}

bool SkBitmapRef::PurgeCacheOne()
{
    SkBitmapRef_Globals* globals = (SkBitmapRef_Globals*)SkGlobals::Find(kBitmapRef_GlobalsTag, NULL);
    if (globals == NULL)
        return false;

    SkAutoMutexAcquire  ac(globals->fMutex);
    SkBitmapRef::Rec*   rec = globals->fCache;
    SkBitmapRef::Rec*   prev = NULL;
    SkDEBUGCODE(int     count = 0;)

    while (rec)
    {
        SkDEBUGCODE(count += 1;)

        SkBitmapRef::Rec* next = rec->fNext;
        if (rec->fRefCnt == 0)
        {
            if (prev)
                prev = next;
            else
                globals->fCache = next;

            SkDEBUGF(("PurgeCacheOne for bitmap[%d %d]\n", rec->fBM.width(), rec->fBM.height()));
            SkDELETE(rec);
            return true;
        }
        prev = rec;
        rec = next;
    }

    SkDEBUGF(("PurgeCacheOne: nothing to purge from %d bitmaps\n", count));
    return false;
}

