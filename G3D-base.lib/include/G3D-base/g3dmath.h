/**
  \file G3D-base.lib/include/G3D-base/g3dmath.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_g3dmath_h
#define G3D_g3dmath_h

#ifdef _MSC_VER
// Disable conditional expression is constant, which occurs incorrectly on inlined functions
#   pragma warning (push)
#   pragma warning (disable : 4127)
// disable: "C++ exception handler used"
#   pragma warning (disable : 4530)
#endif

#include "G3D-base/platform.h"
#include <ctype.h>
#include <float.h>
#include <limits>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>


/*These defines enable functionality introduced with the 1999 ISO C
**standard. They must be defined before the inclusion of math.h to
**engage them. If optimisation is enabled, these functions will be 
**inlined. With optimisation switched off, you have to link in the
**maths library using -lm.
*/

#define _ISOC9X_SOURCE1
#define _ISOC99_SOURCE1
#define __USE_ISOC9X1
#define __USE_ISOC991

#include <math.h>

#include "G3D-base/debug.h"

#undef min
#undef max

/**
\def G3D_DECLARE_SYMBOL(s)
Defines SYMBOL_s as a static const String with the value s.
Useful for avoiding heap allocation from C-string constants being
converted at runtime.
*/
#define G3D_DECLARE_SYMBOL(s) \
    static const String SYMBOL_##s = #s

namespace G3D {

    /** For use with default output arguments. The value is always undefined. */
    extern float ignoreFloat;

    /** For use with default output arguments. The value is always undefined. */
    extern double ignoreDouble;

    /** For use with default output arguments. The value is always undefined. */
    extern int ignoreInt;


    typedef float Radiance;
    typedef float Power;
    typedef float Radiosity;
    typedef float Irradiance;
    typedef float Biradiance;
    typedef float Power;

#ifdef _MSC_VER
    inline double drand48() {
        return ::rand() / double(RAND_MAX);
    }
#endif


#define fuzzyEpsilon64 (0.0000005)
#define fuzzyEpsilon32 (0.00002f)

    /**
        This value should not be tested against directly, instead
        G3D::isNan() and G3D::isFinite() will return reliable results. */
    double inf();

    /** This value should not be tested against directly, instead
        G3D::isNan() and G3D::isFinite() will return reliable results. */
    double nan();

    float finf();

    float fnan();

    inline double pi() {
        return 3.1415926535898;
    }

    inline float pif() {
        return 3.1415926535898f;
    }

    inline double halfPi() {
        return 1.57079633;
    }

    inline double twoPi() {
        return 6.28318531;
    }

    typedef int8_t      int8;
    typedef uint8_t     uint8;
    typedef int16_t     int16;
    typedef uint16_t    uint16;
    typedef int32_t     int32;
    typedef uint32_t    uint32;
    typedef int64_t     int64;
    typedef uint64_t    uint64;

    typedef float           float32;
    typedef double          float64;

    /** Rounds so that the mean of a set of rounded numbers is close to the mean of the original numbers. */
    int roundStochastically(float f);

    int iAbs(int iValue);
    int iCeil(double fValue);

    /**
     Clamps the value to the range [low, hi] (inclusive)
     */
    int iClamp(int val, int low, int hi);
    inline int clamp(int val, int low, int hi) {
        return iClamp(val, low, hi);
    }

    int16 iClamp(int16 val, int16 low, int16 hi);
    double clamp(double val, double low, double hi);
    float clamp(float val, float low, float hi);

    /**
     Returns a + (b - a) * f;
     */
    inline double lerp(double a, double b, double f) {
        return a + (b - a) * f;
    }

    inline float lerp(float a, float b, float f) {
        return a + (b - a) * f;
    }


    inline float fract(float x) {
        return fmodf(x, 1.0f);
    }

    inline double fract(double x) {
        return fmod(x, 1.0);
    }
    /**
     Wraps the value to the range [0, hi) (exclusive
     on the high end).  This is like the clock arithmetic
     produced by % (modulo) except the result is guaranteed
     to be positive.
     */
    int iWrap(int val, int hi);

    int iFloor(double fValue);

    int iSign(int iValue);
    int iSign(double fValue);

    inline int iSign(float f) {
        return iSign((double)f);
    }

    inline double round(double f) {
        return floor(f + 0.5f);
    }

    inline float round(float f) {
        return floor(f + 0.5f);
    }

    /**
        Fast round to integer using the lrint routine.
        Typically 6x faster than casting to integer.
     */
    inline int iRound(double fValue) {
        return lrint(fValue);
    }

    /**
        Fast round to integer using the lrint routine.
        Typically 6x faster than casting to integer.
     */
    inline int iRound(float f) {
        return lrintf(f);
    }


    double abs(double fValue);
    double aCos(double fValue);
    double aSin(double fValue);
    double aTan(double fValue);
    double aTan2(double fY, double fX);
    double sign(double fValue);
    double square(double fValue);

    /**
     Returns true if the argument is a finite real number.
     */
    bool isFinite(double x);

    /**
     Returns true if the argument is NaN (not a number).
     You can't use x == nan to test this because all
     comparisons against nan return false.
     */
    bool isNaN(double x);
    bool isNaN(float x);
    inline bool isNaN(int x) {
        (void)x;
        return false;
    }

    inline bool isNaN(uint64 x) {
        (void)x;
        return false;
    }

    /**
     Computes x % 3.
     */
    int iMod3(int x);

    /**
     Normally distributed random number.

     @deprecated
     @sa Random::gaussian
     */
    float gaussRandom(float mean = 0.0f, float stdev = 1.0f);


    /** Returns x<sup>5</sup> */
    template <class T>
    inline T pow5(T x) {
        const T y = x * x;
        return y * y * x;
    }


    template <class T>
    inline T min(const T& x, const T& y) {
        return x < y ? x : y;
        //    return std::min<T>(x, y);
    }

    template <class T>
    inline T min(const T& x, const T& y, const T& z) {
        return min(min(x, y), z);
        //    return std::min<T>(std::min<T>(x, y), z);
    }

    template <class T>
    inline T min(const T& x, const T& y, const T& z, const T& w) {
        return min(min(x, y), min(z, w));
        //return std::min<T>(std::min<T>(x, y), std::min<T>(z, w));
    }

    template <class T>
    inline T max(const T& x, const T& y) {
        return x > y ? x : y;
        //    return std::max<T>(x, y);
    }

    template <class T>
    inline T max(const T& x, const T& y, const T& z) {
        return max(max(x, y), z);
        //    return std::max<T>(std::max<T>(x, y), z);
    }

    template <class T>
    inline T max(const T& x, const T& y, const T& z, const T& w) {
        return max(max(x, y), max(z, w));
        //    return std::max<T>(std::max<T>(x, y), std::max<T>(z, w));
    }

    [[deprecated]] int iMin(int x, int y);
    [[deprecated]] int iMax(int x, int y);

    double square(double x);
    double sumSquares(double x, double y);
    double sumSquares(double x, double y, double z);
    double distance(double x, double y);
    double distance(double x, double y, double z);

    /**
      Returnes the 0-based index of the highest 1 bit from
      the left.  -1 means the number was 0.

      @cite Based on code by jukka@liimatta.org
     */
    int highestBit(uint32 x);

    /**
     Note that fuzzyEq(a, b) && fuzzyEq(b, c) does not imply
     fuzzyEq(a, c), although that will be the case on some
     occasions.
     */
    bool fuzzyEq(double a, double b);


    /** True if a is definitely not equal to b.
        Guaranteed false if a == b.
        Possibly false when a != b.*/
    bool fuzzyNe(double a, double b);

    /** Is a strictly greater than b? (Guaranteed false if a <= b).
        (Possibly false if a > b) */
    bool fuzzyGt(double a, double b);

    /** Is a near or greater than b? */
    bool fuzzyGe(double a, double b);

    /** Is a strictly less than b? (Guaranteed false if a >= b)*/
    bool fuzzyLt(double a, double b);

    /** Is a near or less than b? */
    bool fuzzyLe(double a, double b);


    inline float sign(float x) {
        return (x == 0.0f) ? 0.0f : copysign(1.0f, x);
    }


    /**
     Computes 1 / sqrt(x).
     */
    inline float rsq(float x) {
        return 1.0f / sqrtf(x);
    }

    /**
     Return the next power of 2 higher than the input
     If the input is already a power of 2, the output will be the same
     as the input.
     */
    int ceilPow2(unsigned int in);

    inline int ceilPow2(int in) {
        return ceilPow2(unsigned(in));
    }

    /** Returns 2^x */
    inline int pow2(unsigned int x) {
        return 1 << x;
    }

    inline double log2(double x) {
        return ::log(x) * 1.442695;
    }

    inline float log2(float x) {
        return ::logf(x) * 1.442695f;
    }

    inline double log2(int x) {
        return log2((double)x);
    }


    /**
     * True if num is a power of two.
     */
    bool isPow2(int num);
    bool isPow2(uint64 num);

    bool isOdd(int num);
    bool isEven(int num);

    double toRadians(double deg);
    double toDegrees(double rad);

    /**
     Returns true if x is not exactly equal to 0.0f.
     */
    inline bool any(float x) {
        return x != 0;
    }

    /**
     Returns true if x is not exactly equal to 0.0f.
     */
    inline bool all(float x) {
        return x != 0;
    }

    /**
     v / v (for DirectX/Cg support)
     */
    inline float normalize(float v) {
        return v / v;
    }

    /**
     a * b (for DirectX/Cg support)
     */
    inline float dot(float a, float b) {
        return a * b;
    }


    /**
     a * b (for DirectX/Cg support)
     */
    inline float mul(float a, float b) {
        return a * b;
    }


    /** @deprecated Use rsq */
    inline double rsqrt(double x) {
        return 1.0 / sqrt(x);
    }

    /** @deprecated Use rsq */
    inline float rsqrt(float x) {
        // TODO: default this to using the SSE2 instruction
        return 1.0f / sqrtf(x);
    }

    /**
     sin(x)/x
     */
    inline double sinc(double x) {
        double r = sin(x) / x;

        if (isNaN(r)) {
            return 1.0;
        }
        else {
            return r;
        }
    }


    inline float mod1(float t) {
        return t - floor(t);
    }

    inline double mod1(double t) {
        return t - floor(t);
    }


    /**
     Computes a floating point modulo; the result is t wrapped to the range [lo, hi).
     */
    inline float wrap(float t, float lo, float hi) {
        if ((t >= lo) && (t < hi)) {
            return t;
        }

        debugAssert(hi > lo);

        float interval = hi - lo;

        return t - interval * floor((t - lo) / interval);
    }

    inline float wrap(float t, float hi) {
        return wrap(t, 0.0f, hi);
    }


    inline double wrap(double t, double lo, double hi) {
        if ((t >= lo) && (t < hi)) {
            return t;
        }

        debugAssert(hi > lo);

        double interval = hi - lo;

        return t - interval * floor((t - lo) / interval);
    }

    inline double wrap(double t, double hi) {
        return wrap(t, 0.0, hi);
    }

    /**
      Returns the ith Hammersley point from a N-point
      sequence on the unit square, [0, 1].
    */
    void hammersleySequence2D(int i, const int N, float& x, float& y);

    inline bool isFinite(double x) {
        return !isNaN(x) && (x < G3D::inf()) && (x > -G3D::inf());
    }

    inline bool isFinite(float x) {
        return !isNaN(x) && (x < G3D::finf()) && (x > -G3D::finf());
    }

    //----------------------------------------------------------------------------
    inline int iAbs(int iValue) {
        return (iValue >= 0 ? iValue : -iValue);
    }

    //----------------------------------------------------------------------------
    inline int iCeil(double fValue) {
        return int(::ceil(fValue));
    }

    //----------------------------------------------------------------------------

    inline int iClamp(int val, int low, int hi) {
        debugAssert(low <= hi);
        if (val <= low) {
            return low;
        }
        else if (val >= hi) {
            return hi;
        }
        else {
            return val;
        }
    }


    inline unsigned int uiClamp(unsigned int val, unsigned int low, unsigned int hi) {
        debugAssert(low <= hi);
        if (val <= low) {
            return low;
        }
        else if (val >= hi) {
            return hi;
        }
        else {
            return val;
        }
    }
    //----------------------------------------------------------------------------

    inline int16 iClamp(int16 val, int16 low, int16 hi) {
        debugAssert(low <= hi);
        if (val <= low) {
            return low;
        }
        else if (val >= hi) {
            return hi;
        }
        else {
            return val;
        }
    }

    //----------------------------------------------------------------------------

    inline double clamp(double val, double low, double hi) {
        debugAssert(low <= hi);
        if (val <= low) {
            return low;
        }
        else if (val >= hi) {
            return hi;
        }
        else {
            return val;
        }
    }

    inline float clamp(float val, float low, float hi) {
        debugAssert(low <= hi);
        if (val <= low) {
            return low;
        }
        else if (val >= hi) {
            return hi;
        }
        else {
            return val;
        }
    }
    //----------------------------------------------------------------------------

    inline int iWrap(int val, int hi) {
        if (val < 0) {
            return ((val % hi) + hi) % hi;
        }
        else {
            return val % hi;
        }
    }

    //----------------------------------------------------------------------------
    inline int iFloor(double fValue) {
        return int(::floor(fValue));
    }

    //----------------------------------------------------------------------------
    inline int iSign(int iValue) {
        return (iValue > 0 ? +1 : (iValue < 0 ? -1 : 0));
    }

    inline int iSign(double fValue) {
        return (fValue > 0.0 ? +1 : (fValue < 0.0 ? -1 : 0));
    }

    //----------------------------------------------------------------------------
    inline double abs(double fValue) {
        return double(::fabs(fValue));
    }

    //----------------------------------------------------------------------------
    inline double aCos(double fValue) {
        if (-1.0 < fValue) {
            if (fValue < 1.0)
                return double(::acos(fValue));
            else
                return 0.0;
        }
        else {
            return pi();
        }
    }

    inline float acos(float fValue) {
        if (-1.0f < fValue) {
            if (fValue < 1.0f) {
                return ::acos(fValue);
            }
            else {
                return 0.0f;
            }
        }
        else {
            return pif();
        }
    }

    //----------------------------------------------------------------------------
    inline double aSin(double fValue) {
        if (-1.0 < fValue) {
            if (fValue < 1.0) {
                return double(::asin(fValue));
            }
            else {
                return -halfPi();
            }
        }
        else {
            return halfPi();
        }
    }

    //----------------------------------------------------------------------------
    inline double aTan(double fValue) {
        return double(::atan(fValue));
    }

    //----------------------------------------------------------------------------
    inline double aTan2(double fY, double fX) {
        return double(::atan2(fY, fX));
    }

    //----------------------------------------------------------------------------
    inline double sign(double x) {
        return (x == 0.0) ? 0.0 : copysign(1.0, x);
    }


    inline float uniformRandom(float low, float hi) {
        return (hi - low) * float(::rand()) / float(RAND_MAX) + low;
    }

    inline double uniformRandomD(double low, double hi) {
        return (hi - low) * ::rand() / RAND_MAX + low;
    }

    inline double square(double x) {
        return x * x;
    }

    inline float square(float x) {
        return x * x;
    }

    inline int square(int x) {
        return x * x;
    }

    //----------------------------------------------------------------------------
    inline double sumSquares(double x, double y) {
        return x * x + y * y;
    }

    //----------------------------------------------------------------------------
    inline float sumSquares(float x, float y) {
        return x * x + y * y;
    }

    //----------------------------------------------------------------------------
    inline double sumSquares(double x, double y, double z) {
        return x * x + y * y + z * z;
    }

    //----------------------------------------------------------------------------
    inline float sumSquares(float x, float y, float z) {
        return x * x + y * y + z * z;
    }

    //----------------------------------------------------------------------------
    inline double distance(double x, double y) {
        return sqrt(sumSquares(x, y));
    }

    //----------------------------------------------------------------------------
    inline float distance(float x, float y) {
        return sqrt(sumSquares(x, y));
    }

    //----------------------------------------------------------------------------
    inline double distance(double x, double y, double z) {
        return sqrt(sumSquares(x, y, z));
    }

    //----------------------------------------------------------------------------
    inline float distance(float x, float y, float z) {
        return sqrt(sumSquares(x, y, z));
    }

    //----------------------------------------------------------------------------

    /** \deprecated use G3D::min */
    inline int iMin(int x, int y) {
        return (x >= y) ? y : x;
    }

    //----------------------------------------------------------------------------
    /** \deprecated use G3D::max */
    inline int iMax(int x, int y) {
        return (x >= y) ? x : y;
    }

    //----------------------------------------------------------------------------
    inline int ceilPow2(unsigned int in) {
        in -= 1;

        in |= in >> 16;
        in |= in >> 8;
        in |= in >> 4;
        in |= in >> 2;
        in |= in >> 1;

        return in + 1;
    }


    inline int ceilPow2(float in) {
        return ceilPow2((unsigned int)(ceil(in)));
    }


    inline bool isPow2(int num) {
        return ((num & -num) == num);
    }

    inline bool isPow2(uint64 x) {
        // See http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/, method #9
        return ((x != 0) && !(x & (x - 1)));
    }

    inline bool isPow2(uint32 x) {
        // See http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/, method #9
        return ((x != 0) && !(x & (x - 1)));
    }

    inline bool isOdd(int num) {
        return (num & 1) == 1;
    }

    inline bool isEven(int num) {
        return (num & 1) == 0;
    }

    inline double toRadians(double deg) {
        return deg * pi() / 180.0;
    }

    inline double toDegrees(double rad) {
        return rad * 180.0 / pi();
    }

    inline float toRadians(float deg) {
        return deg * (float)pi() / 180.0f;
    }

    inline float toDegrees(float rad) {
        return rad * 180.0f / (float)pi();
    }

    inline float toRadians(int deg) {
        return deg * (float)pi() / 180.0f;
    }

    inline float toDegrees(int rad) {
        return rad * 180.0f / (float)pi();
    }
    /**
     Computes an appropriate epsilon for comparing a and b.
     */
    inline double eps(double a, double b) {
        // For a and b to be nearly equal, they must have nearly
        // the same magnitude.  This means that we can ignore b
        // since it either has the same magnitude or the comparison
        // will fail anyway.
        (void)b;
        const double aa = abs(a) + 1.0;
        if (aa == inf()) {
            return fuzzyEpsilon64;
        }
        else {
            return fuzzyEpsilon64 * aa;
        }
    }

    inline float eps(float a, float b) {
        // For a and b to be nearly equal, they must have nearly
        // the same magnitude.  This means that we can ignore b
        // since it either has the same magnitude or the comparison
        // will fail anyway.
        (void)b;
        const float aa = (float)abs(a) + 1.0f;
        if (aa == inf()) {
            return fuzzyEpsilon32;
        }
        else {
            return fuzzyEpsilon32 * aa;
        }
    }


    inline bool fuzzyEq(float a, float b) {
        return (a == b) || (abs(a - b) <= eps(a, b));
    }

    inline bool fuzzyEq(double a, double b) {
        return (a == b) || (abs(a - b) <= eps(a, b));
    }

    inline bool fuzzyNe(double a, double b) {
        return !fuzzyEq(a, b);
    }

    inline bool fuzzyGt(double a, double b) {
        return a > b + eps(a, b);
    }

    inline bool fuzzyGe(double a, double b) {
        return a > b - eps(a, b);
    }

    inline bool fuzzyLt(double a, double b) {
        return a < b - eps(a, b);
    }

    inline bool fuzzyLe(double a, double b) {
        return a < b + eps(a, b);
    }

    inline int iMod3(int x) {
        return x % 3;
    }

    inline int nextMod3(int i) {
        return (1 << i) & 3;
    }


/**
 Given a 32-bit integer, returns the integer with the bytes in the opposite order.
 */
inline uint32 flipEndian32(const uint32 x) {
    return (x << 24) | ((x & 0xFF00) << 8) | 
           ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24);
}

/**
 Given a 16-bit integer, returns the integer with the bytes in the opposite order.
 */
inline uint16 flipEndian16(const uint16 x) {
    return (x << 8) | ((x & 0xFF00) >> 8);
}

/** The GLSL smoothstep function */
inline float smoothstep(float edge0, float edge1, float x) {
    // Scale, bias and saturate x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}


/** Perlin's C2 continous variation on smoothstep() */
inline float smootherstep(float edge0, float edge1, float x) {

    // Scale, and saturate x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    // Evaluate polynomial
    return x * x * x * (x * (x * 6 - 15) + 10);
}


/** Computes |b|^e * sign(b) */
inline float signedPow(float b, float e) {
    return sign(b) * powf(fabsf(b), e);
}


/** Computes |b|^e * sign(b) */
inline double signedPow(double b, double e) {
    return sign(b) * pow(abs(b), e);
}

/** Preserves sign while squaring magnitude */
inline float squareMagnitude(float x) {
    return square(x) * sign(x);
}

/** Preserves sign while squaring magnitude */
inline double squareMagnitude(double x) {
    return square(x) * sign(x);
}

/** Preserves sign while squaring magnitude */
inline int squareMagnitude(int x) {
    return square(x) * iSign(x);
}

/** A lerp() for angles in radians that guarantees moving the shortest way around the circle. */
float lerpAngle(float a, float b, float t);

} // namespace

namespace std {
inline int pow(int a, int b) {
    return (int)::pow(double(a), double(b));
}

}
#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#endif

