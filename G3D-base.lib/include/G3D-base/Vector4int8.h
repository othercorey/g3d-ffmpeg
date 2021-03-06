/**
  \file G3D-base.lib/include/G3D-base/Vector4int8.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_VECTOR4INT8_H
#define G3D_VECTOR4INT8_H

#include "G3D-base/platform.h"
#include "G3D-base/g3dmath.h"
#include "G3D-base/unorm8.h"

namespace G3D {

class Vector3;
class Vector4;

class Vector4uint8 {
public:
    G3D::uint8 x, y, z, w;

    /** Clamps to 255 when converting, it is assumed you've already multiplied by 255. */
    explicit Vector4uint8(const Vector4& source);

    Vector4uint8() : x(0), y(0), z(0), w(0) {}

    inline G3D::uint8& operator[] (int i) {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::uint8*)this)[i];
    }

    inline const G3D::uint8& operator[] (int i) const {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::uint8*)this)[i];
    }
};


class Vector4unorm8 {
public:
    G3D::unorm8 x, y, z, w;

    Vector4unorm8() : x(unorm8::zero()), y(unorm8::zero()), z(unorm8::zero()), w(unorm8::zero()) {}

    inline G3D::unorm8& operator[] (int i) {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::unorm8*)this)[i];
    }

    inline const G3D::unorm8& operator[] (int i) const {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::unorm8*)this)[i];
    }
};


/**
 Homogeneous vector stored efficiently in four signed int8s.

 */
class Vector4int8 {
private:
    // Hidden operators
    bool operator<(const Vector4int8&) const;
    bool operator>(const Vector4int8&) const;
    bool operator<=(const Vector4int8&) const;
    bool operator>=(const Vector4int8&) const;

  
    /** For fast operations, treat this packed data structure as 
      an int32 */
    inline uint32& asInt32() {
        return *reinterpret_cast<uint32*>(this);
    }

    inline const uint32& asInt32() const {
        return *reinterpret_cast<const uint32*>(this);
    }

public:
    // construction
    inline Vector4int8() : x(0), y(0), z(0), w(0) {}
    
    /** Multiplies the source by 127 and clamps to (-128, 127) when converting */
    explicit Vector4int8(const Vector4& source);

    /** Multiplies the source by 127 and clamps to (-128, 127) when converting */
    explicit Vector4int8(const Vector3& source, int8 w);

    inline Vector4int8(int8 x, int8 y, int8 z, int8 w) : x(x), y(y), z(z), w(w) {}

    explicit Vector4int8(class BinaryInput& b);
    void serialize(class BinaryOutput& b) const;
    void deserialize(class BinaryInput& b);

    // coordinates
    int8 x, y, z, w;

    inline operator int8* () {
        return reinterpret_cast<int8*>(this);
    }

    inline operator const int8* () const {
        return reinterpret_cast<const int8*>(this);
    }

    // access vector V as V[0] = V.x, V[1] = V.y, V[2] = V.z, etc.
    //
    // WARNING.  These member functions rely on
    // (1) Vector4int8 not having virtual functions
    // (2) the data packed in a 4*sizeof(int8) memory block
    inline int8& operator[] (int i) {
        debugAssert(i >= 0 && i <= 4);
        return ((int8*)this)[i];
    }

    const int8& operator[] (int i) const {
        debugAssert(i >= 0 && i <= 4);
        return ((const int8*)this)[i];
    }

    // assignment and comparison
    Vector4int8& operator= (const Vector4int8& other) {
        asInt32() = other.asInt32();
        return *this;
    }

    inline bool operator== (const Vector4int8& other) const {
        return asInt32() == other.asInt32();
    }

    inline bool operator!= (const Vector4int8& other) const {
        return ! (*this == other);
    }

    inline unsigned int hashCode() const {
        return asInt32();
    }
};

} // namespace G3D


#endif
