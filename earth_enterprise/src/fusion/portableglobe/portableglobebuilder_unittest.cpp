// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// Unit Tests for portable packet bundles.

#include <limits.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/portableglobe/portableglobebuilder.h"
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"

namespace fusion_portableglobe {

// Test class for portable globe builder.
class PortableGlobeBuilderTest : public testing::Test {
 protected:
  std::string test_directory;
  HiresTree* hires_tree;
  std::string qtnodes;

  virtual void SetUp() {
    qtnodes.append("30132020333322002\n");
    qtnodes.append("30132020333322003\n");
    qtnodes.append("30132020333322012\n");
    qtnodes.append("30132020333322013\n");
    qtnodes.append("301320203333221022\n");
    qtnodes.append("301320203333221023\n");
    qtnodes.append("301320203333221032\n");
    qtnodes.append("301320203333221033\n");
    qtnodes.append("301320203333221132\n");
    qtnodes.append("101323\n");
    qtnodes.append("03212\n");
    qtnodes.append("02\n");

    hires_tree = NULL;
  }

  /**
   * Helper routine to set up hi-res quadtree nodes.
   */
  void SetUpTree() {
    if (hires_tree != NULL) {
      delete hires_tree;
    }

    hires_tree = new HiresTree();
    std::stringstream nodes_stream(qtnodes);
    hires_tree->LoadHiResQtNodes(&nodes_stream);
  }
};

// Tests reading in a set of nodes and determining whether
// other strings are encompassed by the tree or not.
TEST_F(PortableGlobeBuilderTest, TestHiResTree) {
  SetUpTree();

  // Try ancestors of a specified node.
  EXPECT_TRUE(hires_tree->IsTreePath("301320203"));
  EXPECT_TRUE(hires_tree->IsTreePath("3013202033"));
  EXPECT_TRUE(hires_tree->IsTreePath("301320203333"));
  EXPECT_TRUE(hires_tree->IsTreePath("301320203333221"));
  EXPECT_TRUE(hires_tree->IsTreePath("30132020333322103"));

  // Try a specified node.
  EXPECT_TRUE(hires_tree->IsTreePath("301320203333221032"));

  // Try any child of a specified node.
  EXPECT_TRUE(hires_tree->IsTreePath("3013202033332210320123"));

  // Try any children of a specified node that is shorter than the
  // default level.
  EXPECT_TRUE(hires_tree->IsTreePath("02"));
  EXPECT_TRUE(hires_tree->IsTreePath("020"));
  EXPECT_TRUE(hires_tree->IsTreePath("021"));
  EXPECT_TRUE(hires_tree->IsTreePath("022"));
  EXPECT_TRUE(hires_tree->IsTreePath("023"));
  EXPECT_TRUE(hires_tree->IsTreePath("020120121021021"));
  EXPECT_TRUE(hires_tree->IsTreePath("02012012102102102210"));

  // Try off by one at same levels.
  // One off ancestors of a specified node.
  EXPECT_FALSE(hires_tree->IsTreePath("301320201"));
  EXPECT_FALSE(hires_tree->IsTreePath("3013202032"));
  EXPECT_FALSE(hires_tree->IsTreePath("301320203330"));
  EXPECT_FALSE(hires_tree->IsTreePath("301320203333222"));
  EXPECT_FALSE(hires_tree->IsTreePath("30132020333322100"));

  // One off a specified node.
  EXPECT_FALSE(hires_tree->IsTreePath("301320203333221031"));

  // Try all bad node.
  EXPECT_FALSE(hires_tree->IsTreePath("22222"));
}

// Tests reading in a set of nodes from file and determining whether
// other strings are encompassed by the trees or not. Should be
// almost the same as the  hires trees test except for the node
// below the default level and the one above the max level.
TEST_F(PortableGlobeBuilderTest, TestGlobeBuilderKeepNode) {
  std::ofstream fout("/tmp/qtnodes.txt", std::ios::out);
  fout.write(qtnodes.c_str(), qtnodes.size());
  fout.close();

  PortableGlobeBuilder builder(4, 18, "/tmp/qtnodes.txt");

  // Try ancestors of a specified node.
  EXPECT_TRUE(builder.KeepNode("301320203"));
  EXPECT_TRUE(builder.KeepNode("3013202033"));
  EXPECT_TRUE(builder.KeepNode("301320203333"));
  EXPECT_TRUE(builder.KeepNode("301320203333221"));
  EXPECT_TRUE(builder.KeepNode("30132020333322103"));

  // Try a specified node.
  EXPECT_TRUE(builder.KeepNode("301320203333221032"));

  // Even though hires tree says "yes", this fails because
  // it is beyond the max level.
  EXPECT_FALSE(builder.KeepNode("3013202033332210320123"));

  // Try node below default level.
  // This should pass because of the default level, even though
  // the hires tree would say "no."
  EXPECT_TRUE(builder.KeepNode("301"));

  // Try any children of a specified node that is shorter than the
  // default level.
  EXPECT_TRUE(builder.KeepNode("02"));
  EXPECT_TRUE(builder.KeepNode("020120121021021"));
  EXPECT_FALSE(builder.KeepNode("02012012102102102210"));

  // Try off by one at same levels.
  // One off ancestors of a specified node.
  EXPECT_FALSE(builder.KeepNode("301320201"));
  EXPECT_FALSE(builder.KeepNode("3013202032"));
  EXPECT_FALSE(builder.KeepNode("301320203330"));
  EXPECT_FALSE(builder.KeepNode("301320203333222"));
  EXPECT_FALSE(builder.KeepNode("30132020333322100"));

  // One off a specified node.
  EXPECT_FALSE(builder.KeepNode("301320203333221031"));

  // Try all bad node.
  EXPECT_FALSE(builder.KeepNode("22222"));
}

// TODO: Mock the gst server and do a test of whole system.

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
