/* libs/graphics/images/SkImageDecoder_libgif.cpp
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

#include "SkImageDecoder.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkStream.h"
#include "SkTemplates.h"

class SkGIFImageDecoder : public SkImageDecoder {
protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, SkBitmap::Config pref);
};


extern "C" {
#include "gif_lib.h"
}

#define GIF_STAMP       "GIF"    /* First chars in file - GIF stamp. */
#define GIF_STAMP_LEN   (sizeof(GIF_STAMP) - 1)

SkImageDecoder* SkImageDecoder_GIF_Factory(SkStream* stream)
{
    char buf[GIF_STAMP_LEN];
    if (stream->read(buf, GIF_STAMP_LEN) == GIF_STAMP_LEN &&
        memcmp(GIF_STAMP, buf, GIF_STAMP_LEN) == 0)
    {
        stream->rewind();
        return SkNEW(SkGIFImageDecoder);
    }
    return NULL;
}

static int Decode(GifFileType* fileType, GifByteType* out, int size)
{
    SkStream* stream = (SkStream*) fileType->UserData;
    return (int) stream->read(out, size);
}

class AutoGifClose {
public:
    AutoGifClose(GifFileType* gif) : fGIF(gif) {}
    ~AutoGifClose()
    {
        DGifCloseFile(fGIF);
    }
private:
    GifFileType*    fGIF;
};

bool SkGIFImageDecoder::onDecode(SkStream* sk_stream, SkBitmap* bm, SkBitmap::Config prefConfig)
{   
    GifFileType* gif = DGifOpen( sk_stream, Decode );
    if (gif == NULL)
        return false;

    AutoGifClose    ac(gif);

    if (DGifSlurp(gif) != GIF_OK)
        return false;

    // should we check for the Image cmap or the global (SColorMap) first? <reed>
    ColorMapObject* cmap = gif->SColorMap;
    if (cmap == NULL)
        cmap = gif->Image.ColorMap;

    if (cmap == NULL || gif->ImageCount < 1 || cmap->ColorCount != (1 << cmap->BitsPerPixel))
        return false;

    SavedImage* gif_image = gif->SavedImages + 0;
    const int width = gif->SWidth;
    const int height = gif->SHeight;
    SkBitmap::Config    config = SkBitmap::kIndex8_Config;
    if (!this->chooseFromOneChoice(config, width, height))
        return false;

    bm->setConfig(config, width, height, 0);
    if (!this->allocPixels(bm))
        return false;

    SkColorTable* colorTable = SkNEW(SkColorTable);
    colorTable->setColors(cmap->ColorCount);
    bm->setColorTable(colorTable)->unref();

    int transparent = -1;
    for (int i = 0; i < gif_image->ExtensionBlockCount; ++i) {
      ExtensionBlock* eb = gif_image->ExtensionBlocks + i;
      if (eb->Function == 0xF9 && 
          eb->ByteCount == 4) {
        bool has_transparency = ((eb->Bytes[0] & 1) == 1);
        if (has_transparency) {
          transparent = eb->Bytes[3];
        }
      }
    }

    SkPMColor* colorPtr = colorTable->lockColors();

    if (transparent >= 0)
        memset(colorPtr, 0, cmap->ColorCount * 4);
    else
        colorTable->setFlags(colorTable->getFlags() | SkColorTable::kColorsAreOpaque_Flag);

    for (int index = 0; index < cmap->ColorCount; index++)
    {
        if (transparent != index)
            colorPtr[index] = SkColorSetRGB(cmap->Colors[index].Red, 
                cmap->Colors[index].Green, cmap->Colors[index].Blue);
    }
    colorTable->unlockColors(true);

    unsigned char* in = (unsigned char*)gif_image->RasterBits;
    unsigned char* out = bm->getAddr8(0, 0);
    if (gif->Image.Interlace) {

      // deinterlace
        int row;
      // group 1 - every 8th row, starting with row 0
      for (row = 0; row < height; row += 8) {
        memcpy(out + width * row, in, width);
        in += width;
      }

      // group 2 - every 8th row, starting with row 4
      for (row = 4; row < height; row += 8) {
        memcpy(out + width * row, in, width);
        in += width;
      }

      // group 3 - every 4th row, starting with row 2
      for (row = 2; row < height; row += 4) {
        memcpy(out + width * row, in, width);
        in += width;
      }

      for (row = 1; row < height; row += 2) {
        memcpy(out + width * row, in, width);
        in += width;
      }

    } else {
      memcpy(out, in, width * height);
    }
    return true;
}
