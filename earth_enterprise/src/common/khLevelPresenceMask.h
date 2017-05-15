/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHLEVELPRESENCEMASK_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHLEVELPRESENCEMASK_H_

#include <limits>

#include "common/khTileAddr.h"
#include "common/khGuard.h"
#include "common/khTypes.h"


// khLevelPresenceMask
class khLevelPresenceMask : public khLevelCoverage {
  // unimplemented - illegal to assign after the fact
  khLevelPresenceMask& operator=(const khLevelPresenceMask&);

 public:
  // Used to construct from another khLevelPresenceMask.
  khLevelPresenceMask(const khLevelPresenceMask &o);

  // Used to build a level from a stored buffer.
  khLevelPresenceMask(uint lev, const khExtents<uint32> &extents, uchar *src);

  // Used to build an empty level (all not-present) or a filled
  // level (all present) in preparation to fill it in with real values.
  // set_present - whether build level with all "not present" or all "present".
  khLevelPresenceMask(uint lev, const khExtents<uint32> &extents,
                      bool set_present = false);

  khDeleteGuard<uchar, ArrayDeleter> buf;

  // Whether presence is set for specific (row, col).
  // If asked for (row, col) outside the extents it will always return false.
  bool GetPresence(uint32 row, uint32 col) const;

  // Set presence for all elements.
  void SetPresence(bool set_present);

  // Set presence for element (row, col).
  template<int IsCoverageT>
  void SetPresence(uint32 row, uint32 col, bool present,
                   const Int2Type<IsCoverageT>&);

  static uint32 CalcBufferSize(uint32 numRows, uint32 numCols) {
    // We can store 8 bools per byte (1 bit each)
    // so calc the number of tiles, and divide by 8 (rounding up)

    // Check for overflow.
    assert(((static_cast<uint64>(numRows) *
             static_cast<uint64>(numCols) + 7) >> 3) <=
           std::numeric_limits<uint32>::max());
    return static_cast<uint32>((static_cast<uint64>(numRows) *
                                static_cast<uint64>(numCols) + 7) >> 3);
  }

  inline uint32 BufferSize(void) const {
    return CalcBufferSize(extents.height(),
                          extents.width());
  }
};

// Specialization of SetPresence() that is used from khPresenceMask.
template<>
void khLevelPresenceMask::SetPresence(
    uint32 row, uint32 col, bool present, const Int2Type<false>&);

// Specialization of SetPresence() that is used from khCoverageMask.
template<>
void khLevelPresenceMask::SetPresence(
    uint32 row, uint32 col, bool present, const Int2Type<true>&);

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHLEVELPRESENCEMASK_H_
