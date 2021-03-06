/**
  \file G3D-app.lib/include/G3D-app/Tri.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#pragma once

#include "G3D-base/platform.h"
#include "G3D-base/HashTrait.h"
#include "G3D-base/BoundsTrait.h"
#include "G3D-base/Vector2.h"
#include "G3D-base/Vector3.h"
#include "G3D-base/AABox.h"
#include "G3D-base/Array.h"
#include "G3D-base/CoordinateFrame.h"
#include "G3D-base/ReferenceCount.h"
#include "G3D-base/Triangle.h"
#include "G3D-app/Component.h"
#include "G3D-gfx/CPUVertexArray.h"

namespace G3D {
class Material;
class Surfel;
class Surface;


/**
 \brief Triangle implementation optimized for ray-triangle intersection.  

 Single sided and immutable once created.
 
 The size of this class is carefully controlled so that large scenes can
 be stored efficiently and that cache coherence is maintained during processing.
 The implementation is currently 32 bytes in a 64-bit build.

 \sa G3D::Triangle, G3D::MeshShape, G3D::ArticulatedModel, G3D::Surface, G3D::MeshAlg
 */
class Tri {
private:
    friend class NativeTriTree;
    friend class UniversalSurfel;

    // Flags:
    static const uint64 TWO_SIDED            = 1;
    static const uint64 HAS_PARTIAL_COVERAGE = 2;


    /** Usually a Material or Surface, but can be an arbitrary hook. */
    shared_ptr<ReferenceCountedObject> m_data;

    /** 
      The area of the triangle: (e0 x e1).length() * 0.5 
      
      Since the area is always positive, we encode a twoSided
      flag into the sign bit. If the sign bit is 1, the triangle
      should be treated as double sided.
      */
    float                   m_area;

public:

    /** Indices into the CPU Vertex array */
    uint32                  index[3];

private:

    uint64                  m_flags = 0;

public:

    /** Assumes that normals are perpendicular to tangents, or that the tangents are zero.
        \param data Usually a Surface or Material, but can be an arbitrary hook
    */
    Tri(const int i0, const int i1, const int i2,
        const CPUVertexArray& vertexArray,
        const shared_ptr<ReferenceCountedObject>& data = shared_ptr<ReferenceCountedObject>(),
        bool twoSided = false);

    Tri(const int i0, const int i1, const int i2,
        const CPUVertexArray& vertexArray,
        const shared_ptr<ReferenceCountedObject>& data,
        bool twoSided,
        bool partialCoverage);

    Tri() {}

    /** Edge vector v1 - v0 */
    Vector3 e1(const CPUVertexArray& vertexArray) const{
        return position(vertexArray, 1) - position(vertexArray, 0);
    }

    /** Edge vector v2 - v0 */
    Vector3 e2(const CPUVertexArray& vertexArray) const{
        return position(vertexArray, 2) - position(vertexArray, 0);
    }

    /* Override the current data with the parameter */
    void setData(const shared_ptr<ReferenceCountedObject>& newData){
        m_data = newData;
    }

    /** Returns a bounding box */
    void getBounds(const CPUVertexArray& vertexArray, AABox& box) const {
        const Vector3& v0 = position(vertexArray, 0);
        const Vector3& v1 = position(vertexArray, 1);
        const Vector3& v2 = position(vertexArray, 2);

        box = AABox(v0.min(v1).min(v2), v0.max(v1).max(v2));
    }

    /** Surface area. */
    float area() const {
        return m_area;
    }

    /** True if this triangle should be treated as double-sided. */
    bool twoSided() const {
        return (m_flags & TWO_SIDED) != 0;
    }

    /** True if this triangle has a material with any alpha < 1 */
    bool hasPartialCoverage() const {
        return (m_flags & HAS_PARTIAL_COVERAGE) != 0;
    }

    /** Vertex position (must be computed) */
    Point3 position(const CPUVertexArray& vertexArray, int i) const {
        return vertexArray.vertex[index[i]].position;
    }

    /** Useful for accessing several vertex properties at once (for less pointer indirection) */
    const CPUVertexArray::Vertex& vertex(const CPUVertexArray& vertexArray, int i) const {
        debugAssert(i >= 0 && i <= 2);
        return vertexArray.vertex[index[i]];
    }

     /** Face normal.  For degenerate triangles, this is zero.  For all other triangles
        it has arbitrary length and is defined by counter-clockwise winding. Calculated every call.*/
    Vector3 nonUnitNormal(const CPUVertexArray& vertexArray) const {
        return e1(vertexArray).cross(e2(vertexArray));
    }

    /** Face normal.  For degenerate triangles, this is zero.  For all other triangles
        it has unit length and is defined by counter-clockwise winding. Calculated every call.*/
    Vector3 normal(const CPUVertexArray& vertexArray) const {
        return nonUnitNormal(vertexArray).directionOrZero();
    }

    /** Vertex normal */
    const Vector3& normal(const CPUVertexArray& vertexArray, int i) const {
        debugAssert(i >= 0 && i <= 2);
        return vertex(vertexArray, i).normal;
    }

    const Vector2& texCoord(const CPUVertexArray& vertexArray, int i) const {
        debugAssert(i >= 0 && i <= 2);
        return vertex(vertexArray, i).texCoord0;
    }

    const Vector4& packedTangent(const CPUVertexArray& vertexArray, int i) const {
        debugAssert(i >= 0 && i <= 2);
        return vertex(vertexArray, i).tangent;
    }

    uint32 getIndex(int i) const {
        debugAssert(i >= 0 && i <= 2);
        return index[i];
    }

    /** Per-vertex unit tangent, for bump mapping. Tangents are perpendicular to 
        the corresponding vertex normals.*/
    Vector3 tangent(const CPUVertexArray& vertexArray, int i) const {
        debugAssert(i >= 0 && i <= 2);
        return vertex(vertexArray, i).tangent.xyz();
    }

    /** Per-vertex unit tangent = normal x tangent, for bump mapping.
        (Erroneously called the "binormal" in some literature) */
    Vector3 tangent2(const CPUVertexArray& vertexArray, int i) const {
        debugAssert(i >= 0 && i <= 2);
        const CPUVertexArray::Vertex& vertex = this->vertex(vertexArray, i);
        return vertex.normal.cross(vertex.tangent.xyz()) * vertex.tangent.w;
    }
    
    /** \brief Resolve and return the material for this Tri.
      */
    shared_ptr<Material> material() const;

    shared_ptr<Surface> surface() const;

    /** 
     Extract the data field.  Mostly useful when using data that is not a Material or Surface.
     \see surface(), material()
    */
    template<class T>
    shared_ptr<T> data() const {
        return dynamic_pointer_cast<T>(m_data);
    }

    /** 
        \brief Returns a (relatively) unique integer for this object
        
        NOTE: Hashes only on the indices! Think of Tri simply as
        a set of indices and not an actual triangle.
      */
    uint32 hashCode() const {
        return (uint32)((index[0] << 20) + (index[1] << 10) + index[2]);
    }

    /** Returns true if the alpha value at intersection coordinates (u, v) is less than or equal to the threshold */
    bool intersectionAlphaTest(const CPUVertexArray& vertexArray, float u, float v, float threshold) const;

    bool operator==(const Tri& t) const {
        return 
            (index[0] == t.index[0]) &&
            (index[1] == t.index[1]) &&
            (index[2] == t.index[2]) &&
            (m_data == t.m_data);
    }
    
    Triangle toTriangle(const CPUVertexArray& vertexArray) const;

    void sample(float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backface, shared_ptr<Surfel>& surfel, float du = 0, float dv = 0, bool twoSided = true) const;

    /** Set the storage on all Materials in the array */
    static void setStorage(const Array<Tri>& triArray, ImageStorage newStorage);

};// G3D_END_PACKED_CLASS(4)
} // namespace G3D

