// Copyright Google Inc. All Rights Reserved.
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

#include "s2/s2text_format.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

#include "s2/mutable_s2shape_index.h"
#include "s2/s1angle.h"
#include "s2/s2cell_id.h"
#include "s2/s2cell_union.h"
#include "s2/s2debug.h"
#include "s2/s2latlng.h"
#include "s2/s2latlng_rect.h"
#include "s2/s2lax_polygon_shape.h"
#include "s2/s2lax_polyline_shape.h"
#include "s2/s2loop.h"
#include "s2/s2point.h"
#include "s2/s2point_vector_shape.h"
#include "s2/s2polygon.h"
#include "s2/s2polyline.h"
#include "s2/s2shape.h"
#include "s2/s2shape_index.h"

using absl::Span;
using absl::string_view;
using std::make_unique;
using std::string;
using std::unique_ptr;
using std::vector;

namespace s2textformat {

static vector<string_view> SplitString(string_view str, char separator) {
  vector<string_view> result =
      absl::StrSplit(str, separator, absl::SkipWhitespace());
  for (auto& e : result) {
    e = absl::StripAsciiWhitespace(e);
  }
  return result;
}

vector<S2LatLng> ParseLatLngsOrDie(string_view str) {
  vector<S2LatLng> latlngs;
  ABSL_CHECK(ParseLatLngs(str, &latlngs)) << ": str == \"" << str << "\"";
  return latlngs;
}

bool ParseLatLngs(string_view str, vector<S2LatLng>* latlngs) {
  for (const string_view lat_lng_str :
       absl::StrSplit(str, ',', absl::SkipEmpty())) {
    const vector<string_view> lat_lng = absl::StrSplit(lat_lng_str, ':');
    if (lat_lng.size() != 2) return false;
    double lat, lng;
    if (!absl::SimpleAtod(lat_lng[0], &lat)) return false;
    if (!absl::SimpleAtod(lat_lng[1], &lng)) return false;
    latlngs->push_back(S2LatLng::FromDegrees(lat, lng));
  }
  return true;
}

vector<S2Point> ParsePointsOrDie(string_view str) {
  vector<S2Point> vertices;
  ABSL_CHECK(ParsePoints(str, &vertices)) << ": str == \"" << str << "\"";
  return vertices;
}

bool ParsePoints(string_view str, vector<S2Point>* vertices) {
  vector<S2LatLng> latlngs;
  if (!ParseLatLngs(str, &latlngs)) return false;
  for (const auto& latlng : latlngs) {
    vertices->push_back(latlng.ToPoint());
  }
  return true;
}

S2Point MakePointOrDie(string_view str) {
  S2Point point;
  ABSL_CHECK(MakePoint(str, &point)) << ": str == \"" << str << "\"";
  return point;
}

bool MakePoint(string_view str, S2Point* point) {
  vector<S2Point> vertices;
  if (!ParsePoints(str, &vertices) || vertices.size() != 1) return false;
  *point = vertices[0];
  return true;
}

bool MakeLatLng(string_view str, S2LatLng* latlng) {
  vector<S2LatLng> latlngs;
  if (!ParseLatLngs(str, &latlngs) || latlngs.size() != 1) return false;
  *latlng = latlngs[0];
  return true;
}

S2LatLng MakeLatLngOrDie(string_view str) {
  S2LatLng latlng;
  ABSL_CHECK(MakeLatLng(str, &latlng)) << ": str == \"" << str << "\"";
  return latlng;
}

S2LatLngRect MakeLatLngRectOrDie(string_view str) {
  S2LatLngRect rect;
  ABSL_CHECK(MakeLatLngRect(str, &rect)) << ": str == \"" << str << "\"";
  return rect;
}

bool MakeLatLngRect(string_view str, S2LatLngRect* rect) {
  vector<S2LatLng> latlngs;
  if (!ParseLatLngs(str, &latlngs) || latlngs.empty()) return false;
  *rect = S2LatLngRect::FromPoint(latlngs[0]);
  for (size_t i = 1; i < latlngs.size(); ++i) {
    rect->AddPoint(latlngs[i]);
  }
  return rect;
}

bool MakeCellId(string_view str, S2CellId* cell_id) {
  *cell_id = S2CellId::FromDebugString(str);
  return *cell_id != S2CellId::None();
}

S2CellId MakeCellIdOrDie(string_view str) {
  S2CellId cell_id;
  ABSL_CHECK(MakeCellId(str, &cell_id)) << ": str == \"" << str << "\"";
  return cell_id;
}

bool MakeCellUnion(string_view str, S2CellUnion* cell_union) {
  vector<S2CellId> cell_ids;
  for (const auto& cell_str : SplitString(str, ',')) {
    S2CellId cell_id;
    if (!MakeCellId(cell_str, &cell_id)) return false;
    cell_ids.push_back(cell_id);
  }
  *cell_union = S2CellUnion(std::move(cell_ids));
  return true;
}

S2CellUnion MakeCellUnionOrDie(string_view str) {
  S2CellUnion cell_union;
  ABSL_CHECK(MakeCellUnion(str, &cell_union)) << ": str == \"" << str << "\"";
  return cell_union;
}

unique_ptr<S2Loop> MakeLoopOrDie(string_view str, S2Debug debug_override) {
  unique_ptr<S2Loop> loop;
  ABSL_CHECK(MakeLoop(str, &loop, debug_override))
      << ": str == \"" << str << "\"";
  return loop;
}

bool MakeLoop(string_view str, unique_ptr<S2Loop>* loop,
              S2Debug debug_override) {
  if (str == "empty") {
    *loop = make_unique<S2Loop>(S2Loop::kEmpty());
    return true;
  }
  if (str == "full") {
    *loop = make_unique<S2Loop>(S2Loop::kFull());
    return true;
  }
  vector<S2Point> vertices;
  if (!ParsePoints(str, &vertices)) return false;
  *loop = make_unique<S2Loop>(vertices, debug_override);
  return true;
}

unique_ptr<S2Polyline> MakePolylineOrDie(string_view str,
                                         S2Debug debug_override) {
  unique_ptr<S2Polyline> polyline;
  ABSL_CHECK(MakePolyline(str, &polyline, debug_override))
      << ": str == \"" << str << "\"";
  return polyline;
}

bool MakePolyline(string_view str, unique_ptr<S2Polyline>* polyline,
                  S2Debug debug_override) {
  vector<S2Point> vertices;
  if (!ParsePoints(str, &vertices)) return false;
  *polyline = make_unique<S2Polyline>(vertices, debug_override);
  return true;
}

unique_ptr<S2LaxPolylineShape> MakeLaxPolylineOrDie(string_view str) {
  unique_ptr<S2LaxPolylineShape> lax_polyline;
  ABSL_CHECK(MakeLaxPolyline(str, &lax_polyline))
      << ": str == \"" << str << "\"";
  return lax_polyline;
}

bool MakeLaxPolyline(string_view str,
                     unique_ptr<S2LaxPolylineShape>* lax_polyline) {
  vector<S2Point> vertices;
  if (!ParsePoints(str, &vertices)) return false;
  *lax_polyline = make_unique<S2LaxPolylineShape>(vertices);
  return true;
}

static bool InternalMakePolygon(string_view str,
                                S2Debug debug_override,
                                bool normalize_loops,
                                unique_ptr<S2Polygon>* polygon) {
  if (str == "empty") str = "";
  vector<string_view> loop_strs = SplitString(str, ';');
  vector<unique_ptr<S2Loop>> loops;
  for (const auto& loop_str : loop_strs) {
    unique_ptr<S2Loop> loop;
    if (!MakeLoop(loop_str, &loop, debug_override)) return false;
    // Don't normalize loops that were explicitly specified as "full".
    if (normalize_loops && !loop->is_full()) loop->Normalize();
    loops.push_back(std::move(loop));
  }
  *polygon = make_unique<S2Polygon>(std::move(loops), debug_override);
  return true;
}

unique_ptr<S2Polygon> MakePolygonOrDie(string_view str,
                                       S2Debug debug_override) {
  unique_ptr<S2Polygon> polygon;
  ABSL_CHECK(MakePolygon(str, &polygon, debug_override))
      << ": str == \"" << str << "\"";
  return polygon;
}

bool MakePolygon(string_view str, unique_ptr<S2Polygon>* polygon,
                 S2Debug debug_override) {
  return InternalMakePolygon(str, debug_override, true, polygon);
}

unique_ptr<S2Polygon> MakeVerbatimPolygonOrDie(string_view str) {
  unique_ptr<S2Polygon> polygon;
  ABSL_CHECK(MakeVerbatimPolygon(str, &polygon))
      << ": str == \"" << str << "\"";
  return polygon;
}

bool MakeVerbatimPolygon(string_view str, unique_ptr<S2Polygon>* polygon) {
  return InternalMakePolygon(str, S2Debug::ALLOW, false, polygon);
}

unique_ptr<S2LaxPolygonShape> MakeLaxPolygonOrDie(string_view str) {
  unique_ptr<S2LaxPolygonShape> lax_polygon;
  ABSL_CHECK(MakeLaxPolygon(str, &lax_polygon)) << ": str == \"" << str << "\"";
  return lax_polygon;
}

bool MakeLaxPolygon(string_view str,
                    unique_ptr<S2LaxPolygonShape>* lax_polygon) {
  vector<string_view> loop_strs = SplitString(str, ';');
  vector<vector<S2Point>> loops;
  for (const auto& loop_str : loop_strs) {
    if (loop_str == "full") {
      loops.push_back(vector<S2Point>());
    } else if (loop_str != "empty") {
      vector<S2Point> points;
      if (!ParsePoints(loop_str, &points)) return false;
      loops.push_back(points);
    }
  }
  *lax_polygon = make_unique<S2LaxPolygonShape>(loops);
  return true;
}

unique_ptr<MutableS2ShapeIndex> MakeIndexOrDie(string_view str) {
  auto index = make_unique<MutableS2ShapeIndex>();
  ABSL_CHECK(MakeIndex(str, &index)) << ": str == \"" << str << "\"";
  return index;
}

bool MakeIndex(string_view str, unique_ptr<MutableS2ShapeIndex>* index) {
  vector<string_view> strs = absl::StrSplit(str, '#');
  ABSL_DCHECK_EQ(3, strs.size()) << "Must contain two # characters: " << str;

  vector<S2Point> points;
  for (const auto& point_str : SplitString(strs[0], '|')) {
    S2Point point;
    if (!MakePoint(point_str, &point)) return false;
    points.push_back(point);
  }
  if (!points.empty()) {
    (*index)->Add(make_unique<S2PointVectorShape>(std::move(points)));
  }
  for (const auto& line_str : SplitString(strs[1], '|')) {
    unique_ptr<S2LaxPolylineShape> lax_polyline;
    if (!MakeLaxPolyline(line_str, &lax_polyline)) return false;
    (*index)->Add(std::move(lax_polyline));
  }
  for (const auto& polygon_str : SplitString(strs[2], '|')) {
    unique_ptr<S2LaxPolygonShape> lax_polygon;
    if (!MakeLaxPolygon(polygon_str, &lax_polygon)) return false;
    (*index)->Add(std::move(lax_polygon));
  }
  return true;
}

static void AppendVertex(const S2LatLng& ll, string* out,
                         bool roundtrip_precision = false) {
  if (roundtrip_precision) {
    absl::StrAppendFormat(out, "%.17g:%.17g", ll.lat().degrees(),
                          ll.lng().degrees());
  } else {
    absl::StrAppendFormat(out, "%.15g:%.15g", ll.lat().degrees(),
                          ll.lng().degrees());
  }
}

static void AppendVertex(const S2Point& p, string* out,
                         bool roundtrip_precision = false) {
  S2LatLng ll(p);
  return AppendVertex(ll, out, roundtrip_precision);
}

static void AppendVertices(const S2Point* v, int n, string* out) {
  for (int i = 0; i < n; ++i) {
    if (i > 0) *out += ", ";
    AppendVertex(v[i], out);
  }
}

string ToString(const S2Point& point) {
  string out;
  AppendVertex(point, &out);
  return out;
}

string ToString(const S2LatLng& latlng) {
  string out;
  AppendVertex(latlng, &out);
  return out;
}

string ToString(const S2LatLngRect& rect) {
  string out;
  AppendVertex(rect.lo(), &out);
  out += ", ";
  AppendVertex(rect.hi(), &out);
  return out;
}

string ToString(const S2CellId cell_id) {
  return cell_id.ToString();
}

string ToString(const S2CellUnion& cell_union) {
  string out;
  for (S2CellId cell_id : cell_union) {
    if (!out.empty()) out += ", ";
    out += cell_id.ToString();
  }
  return out;
}

string ToString(const S2Loop& loop) {
  if (loop.is_empty()) {
    return "empty";
  } else if (loop.is_full()) {
    return "full";
  }
  string out;
  if (loop.num_vertices() > 0) {
    AppendVertices(&loop.vertex(0), loop.num_vertices(), &out);
  }
  return out;
}

string ToString(const S2Polyline& polyline) {
  string out;
  if (polyline.num_vertices() > 0) {
    AppendVertices(&polyline.vertex(0), polyline.num_vertices(), &out);
  }
  return out;
}

string ToString(const S2Polygon& polygon, string_view loop_separator) {
  if (polygon.is_empty()) {
    return "empty";
  } else if (polygon.is_full()) {
    return "full";
  }
  string out;
  for (int i = 0; i < polygon.num_loops(); ++i) {
    if (i > 0) absl::StrAppend(&out, loop_separator);
    const S2Loop& loop = *polygon.loop(i);
    AppendVertices(&loop.vertex(0), loop.num_vertices(), &out);
  }
  return out;
}

string ToString(Span<const S2Point> points) {
  string out;
  AppendVertices(points.data(), points.size(), &out);
  return out;
}

string ToString(Span<const S2LatLng> latlngs) {
  string out;
  for (size_t i = 0; i < latlngs.size(); ++i) {
    if (i > 0) out += ", ";
    AppendVertex(latlngs[i], &out);
  }
  return out;
}

string ToString(const S2Shape& shape) {
  // Polygon chains are separated by a ; instead of |.
  const char* separator = shape.dimension() == 2 ? "; " : " | ";

  string out;
  if (shape.dimension() == 1) out += "# ";
  if (shape.dimension() == 2) out += "## ";

  int nchain = 0;
  for (const auto& chain : shape.chains()) {
    if (nchain++ > 0) {
      out += separator;
    }

    int nvertex = 0;
    for (const S2Point& vertex : shape.vertices(chain)) {
      if (nvertex++ > 0) {
        out += ", ";
      }
      AppendVertex(vertex, &out);
    }
  }

  if (shape.dimension() == 1) out += " #";
  if (shape.dimension() == 0) out += " ##";
  return out;
}

string ToString(const S2LaxPolylineShape& polyline) {
  string out;
  if (polyline.num_vertices() > 0) {
    AppendVertices(&polyline.vertex(0), polyline.num_vertices(), &out);
  }
  return out;
}

string ToString(const S2LaxPolygonShape& polygon, string_view loop_separator) {
  string out;
  for (int i = 0; i < polygon.num_loops(); ++i) {
    if (i > 0) absl::StrAppend(&out, loop_separator);
    int n = polygon.num_loop_vertices(i);
    if (n == 0) {
      out += "full";
    } else {
      AppendVertices(&polygon.loop_vertex(i, 0), n, &out);
    }
  }
  return out;
}

string ToString(const S2ShapeIndex& index, bool roundtrip_precision) {
  string out;
  for (int dim = 0; dim < 3; ++dim) {
    if (dim > 0) out += "#";
    int count = 0;
    for (const S2Shape* shape : index) {
      if (shape == nullptr || shape->dimension() != dim) continue;
      out += (count > 0) ? " | " : (dim > 0) ? " " : "";
      for (int i = 0; i < shape->num_chains(); ++i, ++count) {
        if (i > 0) out += (dim == 2) ? "; " : " | ";
        S2Shape::Chain chain = shape->chain(i);
        if (chain.length == 0) {
          ABSL_DCHECK_EQ(dim, 2);
          out += "full";
        } else {
          AppendVertex(shape->edge(chain.start).v0, &out, roundtrip_precision);
        }
        int limit = chain.start + chain.length;
        if (dim != 1) --limit;
        for (int e = chain.start; e < limit; ++e) {
          out += ", ";
          AppendVertex(shape->edge(e).v1, &out, roundtrip_precision);
        }
      }
    }
    // Example output: "# #", "0:0 # #", "# # 0:0, 0:1, 1:0"
    if (dim == 1 || (dim == 0 && count > 0)) out += " ";
  }
  return out;
}

}  // namespace s2textformat
