/**
  \file G3D-base.lib/source/Capsule.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/Capsule.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/BinaryOutput.h"
#include "G3D-base/LineSegment.h"
#include "G3D-base/Sphere.h"
#include "G3D-base/CoordinateFrame.h"
#include "G3D-base/Line.h"
#include "G3D-base/AABox.h"

namespace G3D {

Capsule::Capsule(class BinaryInput& b) {
    deserialize(b);
}


Capsule::Capsule() {
}


Capsule::Capsule(const Vector3& _p1, const Vector3& _p2, float _r) 
    : p1(_p1), p2(_p2), _radius(_r) {
}


void Capsule::serialize(class BinaryOutput& b) const {
    p1.serialize(b);
    p2.serialize(b);
    b.writeFloat64(_radius);
}


void Capsule::deserialize(class BinaryInput& b) {
    p1.deserialize(b);
    p2.deserialize(b);
    _radius = (float)b.readFloat64();
}


Line Capsule::axis() const {
    return Line::fromTwoPoints(p1, p2);
}


float Capsule::volume() const {
    return 
        // Sphere volume
        (_radius * _radius * _radius) * pif() * 4.0f / 3.0f +

        // Cylinder volume
        (_radius * _radius) * (p1 - p2).magnitude();
}


float Capsule::area() const {
    return
        // Sphere area
       (_radius * _radius) * 4.0f * pif() +

        // Cylinder area
        2.0f * pif() * _radius * (p1 - p2).magnitude();
}


void Capsule::getBounds(AABox& out) const {
    const Vector3& min = p1.min(p2) - (Vector3(1, 1, 1) * _radius);
    const Vector3& max = p1.max(p2) + (Vector3(1, 1, 1) * _radius);

    out = AABox(min, max);
}


bool Capsule::contains(const Vector3& p) const { 
    return LineSegment::fromTwoPoints(p1, p2).distanceSquared(p) <= square(radius());
}


void Capsule::getRandomSurfacePoint(Vector3& p, Vector3& N, Random& rnd) const {
    float h = height();
    float r = radius();

    // Create a random point on a standard capsule and then rotate to the global frame.

    // Relative areas
    float capRelArea  = sqrt(r) / 2.0f;
    float sideRelArea = r * h;

    float r1 = rnd.uniform(0, capRelArea * 2 + sideRelArea);

    if (r1 < capRelArea * 2) {
        // Select a point uniformly at random on a sphere
        N = Sphere(Vector3::zero(), 1).randomSurfacePoint(rnd);
        p = N * r;
        p.y += sign(p.y) * h / 2.0f;
    } else {
        // Side
        float a = rnd.uniform(0, (float)twoPi());
        N.x = cos(a);
        N.y = 0;
        N.z = sin(a);
        p.x = N.x * r;
        p.z = N.y * r;
        p.y = rnd.uniform(-h / 2.0f, h / 2.0f);
    }

    // Transform to world space
    CoordinateFrame cframe;
    getReferenceFrame(cframe);
    
    p = cframe.pointToWorldSpace(p);
    N = cframe.normalToWorldSpace(N);
}


void Capsule::getReferenceFrame(CoordinateFrame& cframe) const {
    cframe.translation = center();

    Vector3 Y = (p1 - p2).direction();
    Vector3 X = (abs(Y.dot(Vector3::unitX())) > 0.9) ? Vector3::unitY() : Vector3::unitX();
    Vector3 Z = X.cross(Y).direction();
    X = Y.cross(Z);        
    cframe.rotation.setColumn(0, X);
    cframe.rotation.setColumn(1, Y);
    cframe.rotation.setColumn(2, Z);
}


Vector3 Capsule::randomInteriorPoint(Random& rnd) const {
    float h = height();
    float r = radius();

    // Create a random point in a standard capsule and then rotate to the global frame.

    Vector3 p;

    float hemiVolume = (float)pi() * (r*r*r) * 4 / 6.0f;
    float cylVolume = (float)pi() * square(r) * h;
    
    float r1 = rnd.uniform(0, 2.0f * hemiVolume + cylVolume);

    if (r1 < 2.0 * hemiVolume) {

        p = Sphere(Vector3::zero(), r).randomInteriorPoint(rnd);

        p.y += sign(p.y) * h / 2.0f;

    } else {

        // Select a point uniformly at random on a disk
        float a = rnd.uniform(0, (float)twoPi());
        float r2 = sqrt(rnd.uniform(0, 1)) * r;

        p = Vector3(cos(a) * r2,
                    rnd.uniform(-h / 2.0f, h / 2.0f),
                    sin(a) * r2);
    }

    // Transform to world space
    CoordinateFrame cframe;
    getReferenceFrame(cframe);
    
    return cframe.pointToWorldSpace(p);
}

} // namespace
