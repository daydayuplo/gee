/* include/graphics/SkUtils.h
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

#ifndef SkUtils_DEFINED
#define SkUtils_DEFINED

#include "SkTypes.h"

///////////////////////////////////////////////////////////////////////////

/** Similar to memset(), but this function assigns a 16bit value into the buffer.
    @param buffer   The memory to have value copied into it
    @param value    The 16bit value to be copied into buffer
    @param count    The number of times value should be copied into the buffer.
*/
void sk_memset16_portable(uint16_t dst[], uint16_t value, int count);

/** Similar to memset(), but this function assigns a 32bit value into the buffer.
    @param buffer   The memory to have value copied into it
    @param value    The 32bit value to be copied into buffer
    @param count    The number of times value should be copied into the buffer.
*/
void sk_memset32_portable(uint32_t dst[], uint32_t value, int count);

#ifdef ANDROID
    #include "utils/memory.h"
    
    #define sk_memset16(dst, value, count)    android_memset16(dst, value, (count) << 1)
    #define sk_memset32(dst, value, count)    android_memset32(dst, value, (count) << 2)
#endif

#ifndef sk_memset16
    #define sk_memset16(dst, value, count)  sk_memset16_portable(dst, value, count)
#endif

#ifndef sk_memset32
    #define sk_memset32(dst, value, count)  sk_memset32_portable(dst, value, count)
#endif


///////////////////////////////////////////////////////////////////////////

#define kMaxBytesInUTF8Sequence     4

#ifdef SK_DEBUG
    int SkUTF8_LeadByteToCount(unsigned c);
#else
    #define SkUTF8_LeadByteToCount(c)   ((((0xE5 << 24) >> ((unsigned)c >> 4 << 1)) & 3) + 1)
#endif

inline int SkUTF8_CountUTF8Bytes(const char utf8[])
{
    SkASSERT(utf8);
    return SkUTF8_LeadByteToCount(*(const uint8_t*)utf8);
}

int         SkUTF8_CountUnichars(const char utf8[]);
int         SkUTF8_CountUnichars(const char utf8[], size_t byteLength);
SkUnichar   SkUTF8_ToUnichar(const char utf8[]);
SkUnichar   SkUTF8_NextUnichar(const char**);

/** Return the number of bytes need to convert a unichar
    into a utf8 sequence. Will be 1..kMaxBytesInUTF8Sequence,
    or 0 if uni is illegal.
*/
size_t      SkUTF8_FromUnichar(SkUnichar uni, char utf8[] = NULL);

/////////////////////////////////////////////////////////////////////////////////

#define SkUTF16_IsHighSurrogate(c)  (((c) & 0xFC00) == 0xD800)
#define SkUTF16_IsLowSurrogate(c)   (((c) & 0xFC00) == 0xDC00)

int         SkUTF16_CountUnichars(const uint16_t utf16[]);
int         SkUTF16_CountUnichars(const uint16_t utf16[], int numberOf16BitValues);
SkUnichar   SkUTF16_NextUnichar(const uint16_t**);
size_t      SkUTF16_FromUnichar(SkUnichar uni, uint16_t utf16[] = NULL);

size_t      SkUTF16_ToUTF8(const uint16_t utf16[], int numberOf16BitValues, char utf8[] = NULL);

class SkUtils {
public:
#ifdef SK_DEBUG
    static void UnitTest();
#endif
};

#endif

