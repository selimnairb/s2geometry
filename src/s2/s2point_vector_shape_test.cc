// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Author: ericv@google.com (Eric Veach)

#include "s2/s2point_vector_shape.h"

#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/flags/flag.h"
#include "s2/s2point.h"
#include "s2/s2shape.h"
#include "s2/s2shapeutil_testing.h"
#include "s2/s2testing.h"
#include "s2/s2text_format.h"

using std::vector;

TEST(S2PointVectorShape, Empty) {
  vector<S2Point> points;
  S2PointVectorShape shape(std::move(points));
  EXPECT_EQ(0, shape.num_edges());
  EXPECT_EQ(0, shape.num_chains());
  EXPECT_EQ(0, shape.dimension());
  EXPECT_TRUE(shape.is_empty());
  EXPECT_FALSE(shape.is_full());
  EXPECT_FALSE(shape.GetReferencePoint().contained);
}

TEST(S2PointVectorShape, ConstructionAndAccess) {
  vector<S2Point> points;
  S2Testing::rnd.Reset(absl::GetFlag(FLAGS_s2_random_seed));
  const int kNumPoints = 100;
  for (int i = 0; i < kNumPoints; ++i) {
    points.push_back(S2Testing::RandomPoint());
  }
  S2PointVectorShape shape(points);

  EXPECT_EQ(kNumPoints, shape.num_edges());
  EXPECT_EQ(kNumPoints, shape.num_chains());
  EXPECT_EQ(0, shape.dimension());
  EXPECT_FALSE(shape.is_empty());
  EXPECT_FALSE(shape.is_full());
  for (int i = 0; i < kNumPoints; ++i) {
    EXPECT_EQ(i, shape.chain(i).start);
    EXPECT_EQ(1, shape.chain(i).length);
    auto edge = shape.edge(i);
    const S2Point& pt = points.at(i);
    EXPECT_EQ(pt, edge.v0);
    EXPECT_EQ(pt, edge.v1);
    EXPECT_EQ(shape.point(i), pt);
  }
}

TEST(S2PointVectorShape, Move) {
  // Construct a shape to use as the correct answer and a second identical shape
  // to be moved.
  vector<S2Point> points;
  const int kNumPoints = 100;
  for (int i = 0; i < kNumPoints; ++i) {
    points.push_back(S2Testing::RandomPoint());
  }
  const S2PointVectorShape correct{points};
  S2PointVectorShape to_move{points};

  // Test the move constructor.
  S2PointVectorShape move1(std::move(to_move));
  s2testing::ExpectEqual(correct, move1);

  // Test the move-assignment operator.
  S2PointVectorShape move2;
  move2 = std::move(move1);
  s2testing::ExpectEqual(correct, move2);
}

TEST(S2PointVectorShape, ChainIteratorWorks) {
  S2PointVectorShape empty;
  vector<S2Point> points = s2textformat::ParsePointsOrDie("0:0, 0:1, 1:1");
  S2PointVectorShape shape(points);

  S2Shape::ChainIterator empty_begin = empty.chains().begin();
  S2Shape::ChainIterator empty_end = empty.chains().end();
  S2Shape::ChainIterator it = shape.chains().begin();
  S2Shape::ChainIterator end = shape.chains().end();

  int chain_counter = 0;
  for (auto chain : shape.chains()) {
    EXPECT_EQ(chain.start, chain_counter);
    EXPECT_EQ(chain.length, 1);
    ++chain_counter;
  }

  EXPECT_EQ(chain_counter, 3);
  EXPECT_EQ(empty_begin, empty_end);
  EXPECT_NE(it, end);
  EXPECT_EQ((*it).start, 0);
  EXPECT_EQ((*it).length, 1);
  EXPECT_NE(++it, end);
  EXPECT_NE(it++, end);
  EXPECT_NE(it++, end);
  EXPECT_EQ(it, end);
}

TEST(S2PointVectorShape, ChainVertexIteratorWorks) {
  S2PointVectorShape empty;
  vector<S2Point> points = s2textformat::ParsePointsOrDie("0:0, 0:1, 1:1");
  S2PointVectorShape shape(points);

  int chain_counter = 0;
  for (auto chain : shape.chains()) {
    S2Shape::ChainVertexRange vertices(&shape, chain);
    EXPECT_EQ(vertices.num_vertices(), 1);

    auto it1 = vertices.begin();
    auto it2 = it1;

    for (S2Point p : vertices) {
      EXPECT_EQ(p, points[chain_counter]);
      EXPECT_EQ(p, *vertices.begin());
      EXPECT_EQ(p, *S2Shape::ChainVertexIterator(&shape, chain, 0));

      EXPECT_NE(it1, vertices.end());
      EXPECT_NE(it2, vertices.end());
      EXPECT_NE(it1++, vertices.end());
      ++it2;
    }

    EXPECT_EQ(it1, vertices.end());
    EXPECT_EQ(it2, vertices.end());
    ++chain_counter;
  }
}
