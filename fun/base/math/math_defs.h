#pragma once

#include "fun/base/base.h"
#include <math.h>

namespace fun {

#undef  PI
#define PI                  (3.1415926535897932f)
#define SMALL_NUMBER        (1.e-8f)
#define KINDA_SMALL_NUMBER  (1.e-4f)
#define BIG_NUMBER          (3.4e+38f)
#define EULERS_NUMBER       (2.71828182845904523536f)

// Copied from float.h
#define MAX_FLT             (3.402823466e+38F)

// Aux constants.
#define INV_PI              (0.31830988618f)
#define HALF_PI             (1.57079632679f)
#define TWO_PI              (6.28318530717f)

// Magic numbers for numerical precision.
#define DELTA               (0.00001f)

#define RADIANS_TO_DEGREES  (180.f / PI)
#define DEGREES_TO_RADIANS  (PI / 180.f)

} // namespace fun
