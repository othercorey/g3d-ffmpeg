/**
  \file G3D-base.lib/include/G3D-base/PathDirection.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#ifndef G3D_PathDirection_h
#define G3D_PathDirection_h

#include "G3D-base/platform.h"
#include "G3D-base/enumclass.h"

namespace G3D {
/** 
    \brief Direction of light transport along a ray or path.
 */
class PathDirection {
public:
    enum Value {
        /** A path originating at a light source that travels in the
            direction of real photon propagation. Used in bidirectional
            path tracing and photon mapping.*/
        SOURCE_TO_EYE, 

        /** A path originating in the scene (often, at the "eye" or
            camera) and propagating backwards towards a light
            source. Used in path tracing and Whitted ray tracing.*/
        EYE_TO_SOURCE
    } value;


    static const char* toString(int i, Value& v) {
        static const char* str[] = {"SOURCE_TO_EYE", "EYE_TO_SOURCE", nullptr};
        static const Value val[] = {SOURCE_TO_EYE, EYE_TO_SOURCE};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }
    G3D_DECLARE_ENUM_CLASS_METHODS(PathDirection);
};


} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::PathDirection);

#endif
