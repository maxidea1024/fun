#pragma once

namespace fun {

namespace ConvexHull2D {
  /** Returns < 0 if c is left of a-b */
  inline float ComputeDeterminant(const Vector& a, const Vector& b, const Vector& c)
  {
    const float u1 = b.x - a.x;
    const float v1 = b.y - a.y;
    const float u2 = c.x - a.x;
    const float v2 = c.y - a.y;

    return u1 * v2 - v1 * u2;
  }

  /** Returns true if 'a' is more lower-left than 'b'. */
  inline bool ComparePoints(const Vector& a, const Vector& b)
  {
    if (a.x < b.x) {
      return true;
    }

    if (a.x > b.x) {
      return false;
    }

    if (a.y < b.y) {
      return true;
    }

    if (a.y > b.y) {
      return false;
    }

    return false;
  }

  /**
  Calculates convex hull on xy-plane of points on 'points' and stores the indices of the resulting hull in 'out_indices'.
  This code was fixed to work with duplicated vertices and precision issues.
  */
  template <typename Allocator>
  void ComputeConvexHull(const Array<Vector, Allocator>& points, Array<int32, Allocator>& out_indices)
  {
    // Find lower-leftmost point.
    int32 hull_start = 0;
    int32 hull_end = 0;

    for (int32 i = 1; i < points.Count(); ++i) {
      if (ComparePoints(points[i], points[hull_start])) {
        hull_start = i;
      }
      if (ComparePoints(points[hull_end], points[i])) {
        hull_end = i;
      }
    }

    out_indices.Add(hull_start);

    if (hull_start == hull_end) {
      // convex hull degenerated to a single point
      return;
    }

    // Gift wrap hull.
    int32 hull = hull_start;
    int32 local_end = hull_end;
    bool go_right = true;
    bool finished = false;

    // sometimes it hangs on infinite loop, repeating sequence of indices (e.g. 4, 9, 8, 9, 8, ...)
    while (out_indices.Count() <= points.Count()) {
      int32 next_point = local_end;

      for (int32 j = 0; j < points.Count(); ++j) {
        if (j == next_point || j == hull) {
          continue;
        }

        Vector a = points[hull];
        Vector b = points[next_point];
        Vector c = points[j];
        float deter = ComputeDeterminant(a, b, c);

        // 0.001 bias is to stop floating point errors, when comparing points on a straight line; KINDA_SMALL_NUMBER was slightly too small to use.
        if (deter < -0.001) {
          // C is left of AB, take it
          next_point = j;
        }
        else if (deter < 0.001) {
          if (go_right) {
            if (ComparePoints(b, c)) {
              // we go right, take it
              next_point = j;
            }
          }
          else {
            if (ComparePoints(c, b)) {
              // we go left, take it
              next_point = j;
            }
          }
        }
        else {
          // c is right of ab, don't take it
        }
      }

      if (next_point == hull_end) {
        // turn around
        go_right = false;
        local_end = hull_start;
      }

      if (next_point == hull_start) {
        // finish
        finished = true;
        break;
      }

      out_indices.Add(next_point);

      hull = next_point;
    }

    // clear all indices if main loop was left without finishing shape
    if (!finished) {
      out_indices.Reset();
    }
  }

  /**
   * Returns < 0 if c is left of a-b
   */
  inline float ComputeDeterminant2D(const Vector2& a, const Vector2& b, const Vector2& c)
  {
    const float u1 = b.x - a.x;
    const float v1 = b.y - a.y;
    const float u2 = c.x - a.x;
    const float v2 = c.y - a.y;

    return u1 * v2 - v1 * u2;
  }

  /**
   * Alternate simple implementation that was found to work correctly for points that are very close together (inside the 0-1 range).
   */
  template <typename Allocator>
  void ComputeConvexHull2(const Array<Vector2, Allocator>& points, Array<int32, Allocator>& out_indices)
  {
    // FUN march implementation
    fun_check(points.Count() > 0);

    int32 left_most_index = -1;
    Vector2 left_most(float_MAX, float_MAX);

    for (int32 i = 0; i < points.Count(); i++) {
      if (points[i].x < left_most.x
        || (points[i].x == left_most.x && points[i].y < left_most.y)) {
        left_most_index = i;
        left_most = points[i];
      }
    }

    int32 point_on_hull_index = left_most_index;
    int32 end_point_index;

    do {
      out_indices.Add(point_on_hull_index);
      end_point_index = 0;

      // Find the 'leftmost' point to the line from the last hull vertex to a candidate
      for (int32 i = 1; i < points.Count(); i++) {
        if (end_point_index == point_on_hull_index
          || ComputeDeterminant2D(points[end_point_index], points[out_indices.Last()], points[i]) < 0) {
          end_point_index = i;
        }
      }

      point_on_hull_index = end_point_index;
    }
    while (end_point_index != left_most_index);
  }

/*
  static Test()
  { {
      Array<Vector, InlineAllocator<8> > in;
      in.Clear(8);

      in.Add(Vector(2, 0, 0));
      in.Add(Vector(0, 0, 0));
      in.Add(Vector(1, 0, 0));
      in.Add(Vector(3, 0, 0));

      Array<int32, InlineAllocator<8>> out;
      out.Clear(8);

      // Compute the 2d convex hull of the frustum vertices in light space
      ConvexHull2D::ComputeConvexHull(in, out);
      fun_check(out.Count() == 2);
      fun_check(out[0] == 1);
      fun_check(out[1] == 3);
    }
 {
      Array<Vector, InlineAllocator<8> > in;
      in.Clear(8);

      in.Add(Vector(2, 1, 0));

      Array<int32, InlineAllocator<8>> out;
      out.Clear(8);

      // Compute the 2d convex hull of the frustum vertices in light space
      ConvexHull2D::ComputeConvexHull(in, out);
      fun_check(out.Count() == 1);
      fun_check(out[0] == 0);
    }
 {
      Array<Vector, InlineAllocator<8> > in;
      in.Clear(8);

      in.Add(Vector(0, 0, 0));
      in.Add(Vector(1, 0, 0));
      in.Add(Vector(0, 1, 0));
      in.Add(Vector(1, 1, 0));

      Array<int32, InlineAllocator<8>> out;
      out.Clear(8);

      // Compute the 2d convex hull of the frustum vertices in light space
      ConvexHull2D::ComputeConvexHull(in, out);
      fun_check(out.Count() == 4);
      fun_check(out[0] == 0);
      fun_check(out[1] == 1);
      fun_check(out[2] == 3);
      fun_check(out[3] == 2);
    }
 {
      Array<Vector, InlineAllocator<8> > in;
      in.Clear(8);

      in.Add(Vector(0, 0, 0));
      in.Add(Vector(1, 0, 0));
      in.Add(Vector(2, 0, 0));
      in.Add(Vector(0, 1, 0));
      in.Add(Vector(1, 1, 0));
      in.Add(Vector(0, 2, 0));
      in.Add(Vector(2, 2, 0));
      in.Add(Vector(2, 2, 0));

      Array<int32, InlineAllocator<8>> out;
      out.Clear(8);

      // Compute the 2d convex hull of the frustum vertices in light space
      ConvexHull2D::ComputeConvexHull(in, out);
      fun_check(out.Count() == 4);
      fun_check(out[0] == 0);
      fun_check(out[1] == 2);
      fun_check(out[2] == 6);
      fun_check(out[3] == 5);
    }
 {
      Array<Vector, InlineAllocator<8> > in;
      in.Clear(8);

      in.Add(Vector(2, 0, 0));
      in.Add(Vector(3, 1, 0));
      in.Add(Vector(4, 2, 0));
      in.Add(Vector(0, 2, 0));
      in.Add(Vector(1, 3, 0));
      in.Add(Vector(2, 4, 0));
      in.Add(Vector(1, 1, 0));
      in.Add(Vector(3, 3, 0));

      Array<int32, InlineAllocator<8>> out;
      out.Clear(8);

      // Compute the 2d convex hull of the frustum vertices in light space
      ConvexHull2D::ComputeConvexHull(in, out);
      fun_check(out.Count() == 4);
      fun_check(out[0] == 3);
      fun_check(out[1] == 0);
      fun_check(out[2] == 2);
      fun_check(out[3] == 5);
    }
  }
*/
}

} // namespace fun
