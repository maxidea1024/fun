#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements an edge consisting of two vertices.
 */
class Edge {
 public:
  Vector vertex[2];
  int32 count;

  Edge() {}

  Edge(const Vector& v1, const Vector& v2) {
    vertex[0] = v1;
    vertex[1] = v2;
    count = 0;
  }

  bool operator == (const Edge& other) const {
    return (((other.vertex[0] == vertex[0]) && (other.vertex[1] == vertex[1])) || ((other.vertex[0] == vertex[1]) && (other.vertex[1] == vertex[0])));
  }
};

} // namespace fun
