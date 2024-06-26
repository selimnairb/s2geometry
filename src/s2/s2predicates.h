// Copyright 2016 Google Inc. All Rights Reserved.
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
//
// This class contains various predicates that are guaranteed to produce
// correct, consistent results.  They are also relatively efficient.  This is
// achieved by computing conservative error bounds and falling back to high
// precision or even exact arithmetic when the result is uncertain.  Such
// predicates are useful in implementing robust algorithms.
//
// s2edge_crossings.h contains the following exact predicates that test for
// edge crossings.  (Note that usually you should use S2EdgeCrosser, which
// implements them in a much more efficient way.)
//
//   int CrossingSign(const S2Point& a0, const S2Point& a1,
//                    const S2Point& b0, const S2Point& b1);
//
//   bool EdgeOrVertexCrossing(const S2Point& a0, const S2Point& a1,
//                             const S2Point& b0, const S2Point& b1);
//
// It also contains the following functions, which compute their result to
// within a guaranteed tolerance and are consistent with the predicates
// defined here (including using symbolic perturbations when necessary):
//
//   S2Point RobustCrossProd(const S2Point& a, const S2Point& b);
//
//   S2Point GetIntersection(const S2Point& a, const S2Point& b,
//                           const S2Point& c, const S2Point& d);
//
// TODO(ericv): Add InCircleSign() (the Voronoi/Delaunay predicate).
// (This is trickier than the usual textbook implementations because we want
// to model S2Points as lying exactly on the mathematical unit sphere.)

#ifndef S2_S2PREDICATES_H_
#define S2_S2PREDICATES_H_

#include <cfloat>
#include <cmath>
#include <iosfwd>
#include <ostream>

#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "s2/_fp_contract_off.h"
#include "s2/s1chord_angle.h"
#include "s2/s2debug.h"
#include "s2/s2point.h"
#include "s2/s2pointutil.h"

namespace s2pred {

// Returns +1 if the points A, B, C are counterclockwise, -1 if the points
// are clockwise, and 0 if any two points are the same.  This function is
// essentially like taking the sign of the determinant of ABC, except that
// it has additional logic to make sure that the above properties hold even
// when the three points are coplanar, and to deal with the limitations of
// floating-point arithmetic.
//
// Sign satisfies the following conditions:
//
//  (1) Sign(a,b,c) == 0 if and only if a == b, b == c, or c == a
//  (2) Sign(b,c,a) == Sign(a,b,c) for all a,b,c
//  (3) Sign(c,b,a) == -Sign(a,b,c) for all a,b,c
//
// In other words:
//
//  (1) The result is zero if and only if two points are the same.
//  (2) Rotating the order of the arguments does not affect the result.
//  (3) Exchanging any two arguments inverts the result.
//
// On the other hand, note that it is not true in general that
// Sign(-a,b,c) == -Sign(a,b,c), or any similar identities
// involving antipodal points.
int Sign(const S2Point& a, const S2Point& b, const S2Point& c);

// Given 4 points on the unit sphere, return true if the edges OA, OB, and
// OC are encountered in that order while sweeping CCW around the point O.
// You can think of this as testing whether A <= B <= C with respect to the
// CCW ordering around O that starts at A, or equivalently, whether B is
// contained in the range of angles (inclusive) that starts at A and extends
// CCW to C.  Properties:
//
//  (1) If OrderedCCW(a,b,c,o) && OrderedCCW(b,a,c,o), then a == b
//  (2) If OrderedCCW(a,b,c,o) && OrderedCCW(a,c,b,o), then b == c
//  (3) If OrderedCCW(a,b,c,o) && OrderedCCW(c,b,a,o), then a == b == c
//  (4) If a == b or b == c, then OrderedCCW(a,b,c,o) is true
//  (5) Otherwise if a == c, then OrderedCCW(a,b,c,o) is false
//
// REQUIRES: a != o && b != o && c != o
bool OrderedCCW(const S2Point& a, const S2Point& b, const S2Point& c,
                const S2Point& o);

// Returns -1, 0, or +1 according to whether AX < BX, A == B, or AX > BX
// respectively.  Distances are measured with respect to the positions of X,
// A, and B as though they were reprojected to lie exactly on the surface of
// the unit sphere.  Furthermore, this method uses symbolic perturbations to
// ensure that the result is non-zero whenever A != B, even when AX == BX
// exactly, or even when A and B project to the same point on the sphere.
// Such results are guaranteed to be self-consistent, i.e. if AB < BC and
// BC < AC, then AB < AC.
int CompareDistances(const S2Point& x, const S2Point& a, const S2Point& b);

// Returns -1, 0, or +1 according to whether the distance XY is less than,
// equal to, or greater than "r" respectively.  Distances are measured with
// respect the positions of all points as though they are projected to lie
// exactly on the surface of the unit sphere.
int CompareDistance(const S2Point& x, const S2Point& y, S1ChordAngle r);

// Returns -1, 0, or +1 according to whether the distance from the point X to
// the edge A is less than, equal to, or greater than "r" respectively.
// Distances are measured with respect the positions of all points as though
// they were projected to lie exactly on the surface of the unit sphere.
//
// REQUIRES: A0 and A1 do not project to antipodal points (e.g., A0 == -A1).
//           This requires that (A0 != C * A1) for any constant C < 0.
//
// NOTE(ericv): All of the predicates defined here could be extended to handle
// edges consisting of antipodal points by implementing additional symbolic
// perturbation logic (similar to Sign) in order to rigorously define the
// direction of such edges.
int CompareEdgeDistance(const S2Point& x, const S2Point& a0, const S2Point& a1,
                        S1ChordAngle r);

// Returns -1, 0, or +1 according to whether the distance from edge A edge B
// is less than, equal to, or greater than "r" respectively.  Distances are
// measured with respect the positions of all points as though they were
// projected to lie exactly on the surface of the unit sphere.
//
// REQUIRES: A0 and A1 do not project to antipodal points (e.g., A0 == -A1).
// REQUIRES: B0 and B1 do not project to antipodal points (e.g., B0 == -B1).
int CompareEdgePairDistance(const S2Point& a0, const S2Point& a1,
                            const S2Point& b0, const S2Point& b1,
                            S1ChordAngle r);

// Returns -1, 0, or +1 according to whether the normal of edge A has
// negative, zero, or positive dot product with the normal of edge B.  This
// essentially measures whether the edges A and B are closer to proceeding in
// the same direction or in opposite directions around the sphere.
//
// This method returns an exact result, i.e. the result is zero if and only if
// the two edges are exactly perpendicular or at least one edge is degenerate.
// (i.e., both edge endpoints project to the same point on the sphere).
//
// CAVEAT: This method does not use symbolic perturbations.  Therefore it can
// return zero even when A0 != A1 and B0 != B1, e.g. if (A0 == C * A1) exactly
// for some constant C > 0 (which is possible even when both points are
// considered "normalized").
//
// REQUIRES: Neither edge can consist of antipodal points (e.g., A0 == -A1)
//           (see comments in CompareEdgeDistance).
int CompareEdgeDirections(const S2Point& a0, const S2Point& a1,
                          const S2Point& b0, const S2Point& b1);

// Computes the exact sign of the dot product between A and B.
//
// REQUIRES: |a|^2 <= 2 and |b|^2 <= 2
int SignDotProd(const S2Point& a, const S2Point& b);

// Forms the intersection of edge AB with the great circle specified by normal N
// as (A×B)×N and computes the sign of that point dotted with Y.
//
// When you have an edge you know crosses a cell boundary corresponding to N,
// then this function can tell you whether the intersection point is to the
// positive, negative, or exactly on an adjacent side X.  Two such tests can
// determine if the intersection is in range of the S2Cell along the crossed
// boundary.
//
//                            |     A  |
//                            | N   •  |
//                      ------┌────╱───┐------
//                            │ ↓ •   ←│X
//                                B
//
// The intersection of AxB and N results in two (antipodal) points.  This method
// allows either of those points to test as contained in the lune, so the
// ambiguity must be resolved externally.
//
// Fortunately, if we have an edge on a face, and it crosses some great circle
// we take from that face, then we know it can't cross on the antipodal side
// too, because the edge would be > 180 degrees in length.  So checking manually
// for an edge crossing before calling is sufficient to avoid any issues.
//
// REQUIRES: A and B are not equal or antipodal.
// REQUIRES: A and B are not coplanar with the plane specified by N
// REQUIRES: AB crosses N (vertices have opposite dot product signs with N)
//
// Returns:
//   -1 - Intersection was on the negative side of X
//    0 - Intersection was exactly on X
//   +1 - Intersection was on the positive side of X
//
int CircleEdgeIntersectionSign(const S2Point& a, const S2Point& b,
                               const S2Point& n, const S2Point& x);

// Given two edges AB and CD that cross a great circle defined by a normal
// vector M, orders the crossings of AB and CD relative to another great circle
// N representing a zero point.
//
// This predicate can be used in any circumstance where we have an exact normal
// vector to order edge crossings relative to some zero point.
//
// As an example, if we have edges AB and CD that cross boundary 2 of a cell:
//
//                             B     D
//                             •  2  •
//                            ┌─\───/─┐
//                          3 │  • •  │ 1
//                               A C
//
// We could order them by using the normal of boundary 2 as M, and the normal of
// either boundary 1 or 3 as N.  If we use boundary 1 as N, then:
//
//   CircleEdgeIntersectionOrdering(A, B, C, D, M, N) == +1
//
// Indicating that CD is closer to boundary 1 than AB is.
//
// But, if we use boundary 3 as N, then:
//
//   CircleEdgeIntersectionOrdering(A, B, C, D, M, N) == -1
//
// Indicating that AB is closer to boundary 3 than CD is.
//
// These results are consistent but one needs to bear in mind what boundary is
// being used as the reference.
//
// The edges AB and CD should be specified such that A and C are on the positive
// side of M and B and D are on the negative side, as illustrated above.  This
// will make the sign of their cross products with M consistent.
//
// Because we use a dot product to check the distance from N, this predicate can
// only unambiguously order along edges within [0,90] degrees if N (both
// vertices must be in quadrant one of the unit circle).
//
// REQUIRES: A and B are not equal or antipodal.
// REQUIRES: C and D are not equal or antipodal.
// REQUIRES: M and N are not equal or antipodal.
// REQUIRES: AB crosses M (vertices have opposite dot product signs with M)
// REQUIRES: CD crosses M (vertices have opposite dot product signs with M)
// REQUIRES: A and C are on the positive side of M
// REQUIRES: B and D are on the negative side of M
// REQUIRES: Intersection of AB and N is on the positive side of N
// REQUIRES: Intersection of CD and N is on the positive side of N
//
// Returns:
//   -1 if crossing AB is closer to N than crossing CD
//    0 if the two edges cross at exactly the same position
//   +1 if crossing AB is further from N than crossing CD
//
int CircleEdgeIntersectionOrdering(const S2Point& a, const S2Point& b,
                                   const S2Point& c, const S2Point& d,
                                   const S2Point& m, const S2Point& n);

// Returns Sign(X0, X1, Z) where Z is the circumcenter of triangle ABC.
// The return value is +1 if Z is to the left of edge X, and -1 if Z is to the
// right of edge X.  The return value is zero if A == B, B == C, or C == A
// (exactly), and also if X0 and X1 project to identical points on the sphere
// (e.g., X0 == X1).
//
// The result is determined with respect to the positions of all points as
// though they were projected to lie exactly on the surface of the unit
// sphere.  Furthermore this method uses symbolic perturbations to compute a
// consistent non-zero result even when Z lies exactly on edge X.
//
// REQUIRES: X0 and X1 do not project to antipodal points (e.g., X0 == -X1)
//           (see comments in CompareEdgeDistance).
int EdgeCircumcenterSign(const S2Point& x0, const S2Point& x1,
                         const S2Point& a, const S2Point& b, const S2Point& c);

// This is a specialized method that is used to compute the intersection of an
// edge X with the Voronoi diagram of a set of points, where each Voronoi
// region is intersected with a disc of fixed radius "r".
//
// Given two sites A and B and an edge (X0, X1) such that d(A,X0) < d(B,X0)
// and both sites are within the given distance "r" of edge X, this method
// intersects the Voronoi region of each site with a disc of radius r and
// determines whether either region has an empty intersection with edge X.  It
// returns FIRST if site A has an empty intersection, SECOND if site B has an
// empty intersection, NEITHER if neither site has an empty intersection, or
// UNCERTAIN if A == B exactly.  Note that it is not possible for both
// intersections to be empty because of the requirement that both sites are
// within distance r of edge X.  (For example, the only reason that Voronoi
// region A can have an empty intersection with X is that site B is closer to
// all points on X that are within radius r of site A.)
//
// The result is determined with respect to the positions of all points as
// though they were projected to lie exactly on the surface of the unit
// sphere.  Furthermore this method uses symbolic perturbations to compute a
// consistent non-zero result even when A and B lie on opposite sides of X
// such that the Voronoi edge between them exactly coincides with edge X, or
// when A and B are distinct but project to the same point on the sphere
// (i.e., they are linearly dependent).
//
// REQUIRES: r < S1ChordAngle::Right() (90 degrees)
// REQUIRES: s2pred::CompareDistances(x0, a, b) < 0
// REQUIRES: s2pred::CompareEdgeDistance(a, x0, x1, r) <= 0
// REQUIRES: s2pred::CompareEdgeDistance(b, x0, x1, r) <= 0
// REQUIRES: X0 and X1 do not project to antipodal points (e.g., X0 == -X1)
//           (see comments in CompareEdgeDistance).
enum class Excluded { FIRST, SECOND, NEITHER, UNCERTAIN };
std::ostream& operator<<(std::ostream& os, Excluded excluded);
Excluded GetVoronoiSiteExclusion(const S2Point& a, const S2Point& b,
                                 const S2Point& x0, const S2Point& x1,
                                 S1ChordAngle r);

/////////////////////////// Low-Level Methods ////////////////////////////
//
// Most clients will not need the following methods.  They can be slightly
// more efficient but are harder to use, since they require the client to do
// all the actual crossing tests.

// A more efficient version of Sign that allows the precomputed
// cross-product of A and B to be specified.  (Unlike the 3 argument
// version this method is also inlined.)  Note that "a_cross_b" must be
// computed using CrossProd rather than S2::RobustCrossProd.
//
// REQUIRES: a_cross_b == a.CrossProd(b)
inline int Sign(const S2Point& a, const S2Point& b, const S2Point& c,
                const Vector3_d& a_cross_b);

// This version of Sign returns +1 if the points are definitely CCW, -1 if
// they are definitely CW, and 0 if two points are identical or the result
// is uncertain.  Uncertain cases can be resolved, if desired, by calling
// ExpensiveSign.
//
// The purpose of this method is to allow additional cheap tests to be done,
// where possible, in order to avoid calling ExpensiveSign unnecessarily.
//
// REQUIRES: a_cross_b == a.CrossProd(b)
inline int TriageSign(const S2Point& a, const S2Point& b,
                      const S2Point& c, const Vector3_d& a_cross_b);

// This function is invoked by Sign() if the sign of the determinant is
// uncertain.  It always returns a non-zero result unless two of the input
// points are the same.  It uses a combination of multiple-precision
// arithmetic and symbolic perturbations to ensure that its results are
// always self-consistent (cf. Simulation of Simplicity, Edelsbrunner and
// Muecke).  The basic idea is to assign an infinitesimal symbolic
// perturbation to every possible S2Point such that no three S2Points are
// collinear and no four S2Points are coplanar.  These perturbations are so
// small that they do not affect the sign of any determinant that was
// non-zero before the perturbations.  If "perturb" is false, then instead
// the exact sign of the unperturbed input points is returned, which can be
// zero even when all three points are distinct.
//
// Unlike Sign(), this method does not require the input points to be
// normalized.
int ExpensiveSign(const S2Point& a, const S2Point& b,
                  const S2Point& c, bool perturb = true);

//////////////////   Implementation details follow   ////////////////////

inline int Sign(const S2Point& a, const S2Point& b, const S2Point& c,
                const Vector3_d& a_cross_b) {
  int sign = TriageSign(a, b, c, a_cross_b);
  if (sign == 0) sign = ExpensiveSign(a, b, c);
  return sign;
}

inline int TriageSign(const S2Point& a, const S2Point& b,
                      const S2Point& c, const Vector3_d& a_cross_b) {
  // kMaxDetError is the maximum error in computing (AxB).C where all vectors
  // are unit length.  Using standard inequalities, it can be shown that
  //
  //  fl(AxB) = AxB + D where |D| <= (|AxB| + (2/sqrt(3))*|A|*|B|) * e
  //
  // where "fl()" denotes a calculation done in floating-point arithmetic,
  // |x| denotes either absolute value or the L2-norm as appropriate, and
  // e = 0.5*DBL_EPSILON.  Similarly,
  //
  //  fl(B.C) = B.C + d where |d| <= (1.5*|B.C| + 1.5*|B|*|C|) * e .
  //
  // Applying these bounds to the unit-length vectors A,B,C and neglecting
  // relative error (which does not affect the sign of the result), we get
  //
  //  fl((AxB).C) = (AxB).C + d where |d| <= (2.5 + 2/sqrt(3)) * e
  //
  // which is about 3.6548 * e, or 1.8274 * DBL_EPSILON.
  //
  // In order to support vectors of magnitude <= sqrt(2), we double this value.
  const double kMaxDetError = 3.6548 * DBL_EPSILON;
  ABSL_DCHECK_LE(a.Norm2(), 2);
  ABSL_DCHECK_LE(b.Norm2(), 2);
  ABSL_DCHECK_LE(c.Norm2(), 2);
  ABSL_DCHECK_EQ(a_cross_b, a.CrossProd(b));
  double det = a_cross_b.DotProd(c);

  // Double-check borderline cases in debug mode.
  ABSL_DCHECK(!absl::GetFlag(FLAGS_s2debug) || std::fabs(det) <= kMaxDetError ||
              std::fabs(det) >= 100 * kMaxDetError ||
              det * ExpensiveSign(a, b, c) > 0);

  if (det > kMaxDetError) return 1;
  if (det < -kMaxDetError) return -1;
  return 0;
}

// Like Sign, except this method does not use symbolic perturbations when the
// input points are exactly coplanar with the origin (i.e., linearly dependent).
// Clients should never use this method, but it is useful here in order to
// implement the combined pedestal/axis-aligned perturbation scheme used by some
// methods (such as EdgeCircumcenterSign).
inline int UnperturbedSign(const S2Point& a, const S2Point& b,
                           const S2Point& c) {
  int sign = TriageSign(a, b, c, a.CrossProd(b));
  if (sign == 0) sign = ExpensiveSign(a, b, c, false /*perturb*/);
  return sign;
}

}  // namespace s2pred

#endif  // S2_S2PREDICATES_H_
