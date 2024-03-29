/**
  \file G3D-gfx.lib/include/G3D-gfx/Texture.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2021, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#pragma once
#define G3D_gfx_Texture_h

#include "G3D-base/platform.h"
#include "G3D-base/ReferenceCount.h"
#include "G3D-base/Array.h"
#include "G3D-base/Table.h"
#include "G3D-base/constants.h"
#include "G3D-base/CubeFace.h"
#include "G3D-base/Vector2.h"
#include "G3D-base/WrapMode.h"
#include "G3D-base/InterpolateMode.h"
#include "G3D-base/DepthReadMode.h"
#include "G3D-base/Queue.h"
#include "G3D-base/Image.h"
#include "G3D-base/Image1.h"
#include "G3D-base/Image1unorm8.h"
#include "G3D-base/Image3.h"
#include "G3D-base/Image3unorm8.h"
#include "G3D-base/Image4.h"
#include "G3D-base/Image4unorm8.h"
#include "G3D-base/PixelTransferBuffer.h"
#include "G3D-base/BumpMapPreprocess.h"
#include "G3D-base/WeakCache.h"
#include "G3D-base/FrameName.h"
#include "G3D-gfx/glheaders.h"
#include "G3D-gfx/Sampler.h"

#ifdef G3D_ENABLE_CUDA
#include <cuda.h>
#include <cudaGL.h>
#include <builtin_types.h>
#include <cuda_runtime.h>
#endif


namespace G3D {

class Rect2D;
class Matrix3;
class Texture;
class RenderDevice;
class GLPixelTransferBuffer;
class Args;
class UniformTable;


/**
 \brief A 1D, 2D, or 3D array (e.g., an image) stored on the GPU, commonly used for
  mapping reflectance values (colors) over meshes.

 Abstraction of OpenGL textures.  This class can be used with raw OpenGL, 
 without RenderDevice.  G3D::Texture supports all of the image formats
 that G3D::Image can load, and DDS (DirectX textures), and Quake-style cube 
 maps.

 If you enable texture compression, textures will be compressed on the fly.
 This can be slow (up to a second).

 The special filename "<white>" generates an all-white Color4 texture (this works for both
 2D and cube map texures; "<whiteCube>" can also be used explicitly for cube maps).  You can use Preprocess::modulate
 to create other colors from this.
 */
class Texture : public ReferenceCountedObject {
friend class GLSamplerObject;
friend class Shader;
public:

    /** Textures are normally cached by Texture::Specification to speed loading and reduce
        memory consumption unless explicitly flagged not to enter the cache. This method
        flushes the G3D-wide Texture cache to allow it to reload from disk. */
    static void clearCache();

    /** 
      Attaches semantics for reading and writing this texture beyond the OpenGL bitwise
      description.  This allows G3D to automatically bind texture variables more usefully
      in shaders, visualize 

      Read the texture in GLSL as:
      \code
      vec4 v = texture(t, x) * readMultiplyFirst + readAddSecond;
      \endcode

      Note that these encoding values are applied in addition to whatever the underlying ImageFormat
      specifies.  So, sRGB data could be stored in an RGB ImageFormat using an explicit exponent
      here.

      It is common for shaders to assume a limited encoding and their corresponding host code
      to assert that the encoding is obeyed rather than pass all parameters to the shader.

      Any file examples:

      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      t = Texture::Encoding {
        frame = CAMERA;
        readMultiplyFirst = Color4(1,1,1,0.5);
        readAddSecond = Color3(0.5);
        format = RGBA8;
      };

      t = Color4(0.5, 1, 0, 1);

      t = 0.5;
      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      
    */
    class Encoding {
    public:

        /** 
          This is primarily for debugging and visualization. It is not automatically bound
          with the texture for shaders.  However, programs can explicitly pass information
          based on it to a shader to allow generalizing over coordinate transformations, for
          example.
         */
        FrameName           frame;

        /** This is automatically bound as an optional argument when the texture is passed
            to a shader.
         */
        Color4              readMultiplyFirst;

        /** This is automatically bound as an optional argument when the texture is passed
            to a shader.
         */
        Color4              readAddSecond;

        const ImageFormat*  format;

        Encoding(const ImageFormat* fmt = nullptr, FrameName n = FrameName::NONE, const Color4& readMultiplyFirst = Color4::one(), const Color4& readAddSecond = Color4::zero()) : frame(n), readMultiplyFirst(readMultiplyFirst), readAddSecond(readAddSecond), format(fmt) {}
        Encoding(const ImageFormat* fmt, FrameName n, const float readMultiplyFirst = 1.0f, const float readAddSecond = 0.0f) : frame(n), readMultiplyFirst(Color4::one() * readMultiplyFirst), readAddSecond(Color4::one() * readAddSecond), format(fmt) {}

        explicit Encoding(const Color4& readMultiplyFirst) : frame(FrameName::NONE), readMultiplyFirst(readMultiplyFirst), format(nullptr) {}
        Encoding(const Any& a);

        /** Computes a suitable representation for low-precision 2D signed motion vectors 
            on this machine, with an exactly representable zero and range of approximately
            +/- 64. */
        static Encoding lowPrecisionScreenSpaceMotionVector();

        Color4 writeMultiplyFirst() const {
            // y = x * a + b
            //
            // x = (y - b) / a
            //   = y/a - b/a
            return Color4::one() / readMultiplyFirst;
        }

        Color4 writeAddSecond() const {
            return -readAddSecond / readMultiplyFirst;
        }

        bool operator==(const Encoding& e) const;

        bool operator!=(const Encoding& e) const {
            return ! (*this == e);
        }

        Any toAny() const;

        size_t hashCode() const {
            return (format ? format->code : 0xFFFFFFFF) ^ readMultiplyFirst.hashCode() + 
                (readAddSecond.hashCode() << 2) + 
                (frame.value << 10);
        }
    };

    /**
     Returns the rotation matrix that should be used for rendering the
     given cube map face.  The orientations will seem to have the
     camera "upside down" compared to what you might expect because
     OpenGL's cube map convention and texture convention are both
     inverted from how we usually visualize the data.
     
     The resulting cube maps can be saved to disk by:

     <pre>
     const Texture::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];
     Image temp;
     renderTarget->getImage(temp, ImageFormat::RGB8());
     temp.flipVertical();
     temp.rotate90CW(-faceInfo.rotations);
     if (faceInfo.flipY) temp.flipVertical();
     if (faceInfo.flipX) temp.flipHorizontal();
     temp.save(format("out-%s.png", faceInfo.suffix.c_str()));
     </pre>

    */
    static void getCubeMapRotation(CubeFace face, Matrix3& outMatrix);

    /** \a filename should contain a * wildcard */
    static CubeMapConvention determineCubeConvention(const String& filename);
    
    /** Returns the mapping from [0, 5] to cube map faces and filename
        suffixes. There are multiple filename conventions, so the
        suffixes specify each of the options. */
    static const CubeMapConvention::CubeMapInfo& cubeMapInfo(CubeMapConvention convention);

    /** These values are guaranteed to correspond to the equivalent OpenGL constant, so they can
        be cast directly for convenience when porting.*/
    enum Dimension {
        DIM_2D                  = GL_TEXTURE_2D, 
        DIM_2D_ARRAY            = GL_TEXTURE_2D_ARRAY,
        DIM_3D                  = GL_TEXTURE_3D, 
        DIM_2D_RECT             = GL_TEXTURE_RECTANGLE, 
        DIM_CUBE_MAP            = GL_TEXTURE_CUBE_MAP,
        DIM_CUBE_MAP_ARRAY      = GL_TEXTURE_CUBE_MAP_ARRAY
    };

    /** 
      Returns true if this is a legal wrap mode for a G3D::Texture.
     */
    static bool supportsWrapMode(WrapMode m) {
        return (m == WrapMode::TILE) || (m == WrapMode::CLAMP) || (m == WrapMode::ZERO);
    }

    class Visualization {
    public:

        /** Which channels to display. */
        enum Channels {
            /** RGB as a color*/
            RGB,

            /** Red only */
            R, 

            /** Green only */
            G, 

            /** Blue only */
            B, 

            /** Red as grayscale */
            RasL,

            /** Green as grayscale */
            GasL,

            /** Blue as grayscale */
            BasL,
        
            /** Alpha as grayscale */
            AasL, 

            /** RGB mean as luminance: (R + G + B) / 3; visualizes the net 
               reflectance or energy of a texture */
            MeanRGBasL,
    
            /** (Perceptual) Luminance; visualizes the brightness
                people perceive of an image. */
            Luminance
        };

        Channels         channels;

        /** Texture's gamma. Texels will be converted to pixels by p = t^(g/2.2)*/
        float            documentGamma;

        /** Lowest value to visualize */
        float            min;

        /** Highest expected value */
        float            max;

        /** If true, show as 1 - (adjusted value) */
        bool             invertIntensity;

        /** For a texture array, the coordinate of the layer to display. Otherwise just 0. */
        int              layer;

        /** The mipLevel to display */
        int              mipLevel;

        /** Wrap values at 1 for visualization, intended for texture coordinates */
        bool             mod1 = false;

        /** Visualize motion vectors/flow */
        bool             showMotionVectors = false; 
        
        /** Tile size for arrow visualization */
        float            motionVectorSpacing = 24.0f;

        /** 1.0 = stretch a unit vector to maxMotionVectorLength in the visualization. */
        float            motionVectorScale = 1.0f;

        /** nan = none. Show this texcoord in the texture inspector, including any projection
            and border math. The z component is not a MIP level. It is used for cube maps
            and 3D textures.*/
        Vector3          highlightTexCoord = Vector3::nan();

        /** Defaults to linear data on [0, 1]: packed normal maps,
            reflectance maps, etc. */
        Visualization(Channels c = RGB, float g = 1.0f, float mn = 0.0f, float mx = 1.0f, bool mod1 = false, bool motVec = false);

        /** Accepts the name of any static factory method as an Any::ARRAY, e.g.,
            "v = sRGB()" or a table, e.g., "v = Texture::Visualization { documentGamma = 2.2, ... }"
        */
        Visualization(const Any& a);

        Any toAny() const;

        bool operator==(const Visualization& v) const {
            return
                (channels == v.channels) &&
                (documentGamma == v.documentGamma) &&
                (min == v.min) &&
                (max == v.max) &&
                (mod1 == v.mod1) &&
                (invertIntensity == v.invertIntensity) &&
                (layer == v.layer) &&
                (mipLevel == v.mipLevel) &&
                (showMotionVectors == v.showMotionVectors) &&
                (motionVectorSpacing == v.motionVectorSpacing) && 
                (motionVectorScale == v.motionVectorScale);
        }

        /** Returns the matrix corresponding to the color shift implied by channels */
        Matrix4 colorShiftMatrix();

        /**
            Sets the following arguments:

            float mipLevel
            float adjustGamma
            float bias
            float scale
            float invertIntensity
            mat4  colorShift

            macro LAYER
        */
        void setShaderArgs(UniformTable& args);

        /** For photographs and other images with document gamma of about 2.2.  Note that this does not 
          actually match true sRGB values, which have a non-linear gamma. */
        static const Visualization& sRGB();

        /** For signed unit vectors, like a GBuffer's normals, on the
            range [-1, 1] for RGB channels */
        static const Visualization& unitVector();

        static const Visualization& textureCoordinates();

        /** For bump map packed in an alpha channel. */
        static const Visualization& bumpInAlpha();

        /** For a hyperbolic depth map in the red channel (e.g., a shadow map). */
        static const Visualization& depthBuffer();

        /** For motion vectors */
        static const Visualization& motionVectors();

        static const Visualization& defaults();

        /** Unit vectors packed into RGB channels, e.g. a normal map.  Same as defaults() */
        static const Visualization& packedUnitVector() {
            return defaults();
        }

        /** Reflectivity map.  Same as defaults() */
        static const Visualization& reflectivity() {
            return defaults();
        }

        /** Radiance map.  Same as defaults() */
        static const Visualization& radiance() {
            return defaults();
        }

        /** Linear RGB map.  Same as defaults() */
        static const Visualization& linearRGB() {
            return defaults();
        }
    };

    static const char* toString(DepthReadMode m);
    static DepthReadMode toDepthReadMode(const String& s);
    
    static const char* toString(Dimension m);
    static Dimension toDimension(const String& s);

    /**
     Splits a filename around the '*' character-- used by cube maps to generate all filenames.
     */
    static void splitFilenameAtWildCard
       (const String&  filename,
        String&        filenameBeforeWildCard,
        String&        filenameAfterWildCard);

    /**
       Returns true if the specified filename exists and is an image that can be loaded as a Texture.
    */
    static bool isSupportedImage(const String& filename);
    
    /** \brief Returns a small all-white (1,1,1,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static const shared_ptr<Texture>& white();

    /** \brief Returns a small all-white (1,1,1,1) cube map texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. 
        
        The underlying OpenGL texture is guaranteed not to be deallocated
        before RenderDevice shuts down.*/
    static const shared_ptr<Texture>& whiteCube();

    static shared_ptr<Texture> createColorCube(const Color4& color);

    /** \brief Returns a small opaque all-black (0,0,0,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. 
        \param d must be DIM_2D, DIM_3D, or DIM_2D_ARRAY
      */
    static const shared_ptr<Texture>& opaqueBlack(Dimension d = DIM_2D);

    /** \brief Returns a small opaque all-black (0,0,0,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static const shared_ptr<Texture>& opaqueBlackCube();

    /** \brief Returns a small, all zero Color4(0,0,0,0) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. 
        \param d must be DIM_2D, DIM_3D, or DIM_2D_ARRAY
     */
    static const shared_ptr<Texture>& zero(Dimension d = DIM_2D);

    /** \brief Returns a small all-gray (0.5,0.5,0.5,1) texture.  
    
        The result is memoized and shared. Do not mutate this texture
        or future calls will return the mutated texture as well. */
    static const shared_ptr<Texture>& opaqueGray();


    /** \copydoc white(). */
    static const shared_ptr<Texture>& one() {
        return white();
    }

    /** Returns \a t if it is non-nullptr, or white() if \a t is nullptr */
    static const shared_ptr<Texture>& whiteIfNull(const shared_ptr<Texture>& t) {
        if (isNull(t)) {
            return white();
        } else {
            return t;
        }
    }

    /** Returns \a t if it is non-nullptr, or whiteCube() if \a t is nullptr */
    static const shared_ptr<Texture>& whiteCubeIfNull(const shared_ptr<Texture>& t) {
        if (isNull(t)) {
            return whiteCube();
        } else {
            return t;
        }
    }


    /** Returns \a t if it is non-nullptr, or opaqueBlack() if \a t is nullptr */
    inline static const shared_ptr<Texture>& opaqueBlackIfNull(const shared_ptr<Texture>& t) {
        if (isNull(t)) {
            return opaqueBlack();
        } else {
            return t;
        }
    }

    /** Returns \a t if it is non-nullptr, or zero() if \a t is nullptr */
    inline static const shared_ptr<Texture>& zeroIfNull(const shared_ptr<Texture>& t) {
        if (isNull(t)) {
            return zero();
        } else {
            return t;
        }
    }

    /** Returns \a t if it is non-nullptr, or gray() if \a t is nullptr */
    inline static const shared_ptr<Texture>& opaqueGrayIfNull(const shared_ptr<Texture>& t) {
        if (isNull(t)) {
            return opaqueGray();
        } else {
            return t;
        }
    }

    /** 
        Returns an RG32F difference texture of the same dimensions as \a t0 and
        \a t1 of (t0 - t1) of the specified \a channel, 
        with positive values in the red channel and negative values in the green.

         Expect changes to this API as we use it more and make it more manageable
    */
    static shared_ptr<Texture> singleChannelDifference(RenderDevice* rd, const shared_ptr<Texture>& t0, const shared_ptr<Texture>& t1, int channel = 0);
    
    class Preprocess {
    public:

        /** Multiplies color channels.  Useful for rescaling to make
            textures brighter or to tint textures as they are loaded.  Modulation
            happens first of all preprocessing.
         */
        Color4                      modulate;

        /** Offset values by this amount. Happens second in preprocessing. */
        Color4                      offset;

        /**
           After brightening, each (unit-scale) pixel is raised to
           this power. Many textures are drawn to look good when
           displayed on the screen in PhotoShop, which means that they
           are drawn with a document gamma of about 2.2.

           If the document gamma is 2.2, set \a gammaAdjust to:
           <ul>
             <li> 2.2 for reflectivity, emissive, and environment maps (e.g., lambertian, glossy, etc.)
             <li> 1.0 for 2D elements, like fonts and full-screen images
             <li> 1.0 for computed data (e.g., normal maps, bump maps, GPGPU data)
           </ul>

           To maintain maximum precision, author and store the
           original image files in a 1.0 gamma space, at which point
           no gamma correction is necessary.
         */
        float                       gammaAdjust;

        /** If true (default), constructors automatically compute the min, max, and mean
            value of the texture. This is necessary, for example, for use with UniversalBSDF. */
        bool                        computeMinMaxMean;
           
        BumpMapPreprocess           bumpMapPreprocess;

        /** After loading and before computing any MIP maps or uploading to the GPU, but after gamma adjustment
            and modulation,
            multiply the color
            values by the alpha value. */
        bool                        convertToPremultipliedAlpha;

        Preprocess() : modulate(Color4::one()), gammaAdjust(1.0f), 
            computeMinMaxMean(true),
            convertToPremultipliedAlpha(false) {}

        /** \param a Must be in the form of a table of the fields or appear as
            a call to a static factory method, e.g.,:

            - Texture::Preprocess{  modulate = Color4(...), ... }
            - Texture::Preprocess::gamma(2.2)
            - Texture::Preprocess::none()
        */
        Preprocess(const Any& a);

        Any toAny() const;

        /** Defaults + gamma adjust set to g*/
        static Preprocess gamma(float g);

        static const Preprocess& defaults();

        /** Default settings + computeMinMaxMean = false */
        static const Preprocess& none();

        /** Brighten by 2 and adjust gamma by 1.6, the default values
            expected for Quake versions 1 - 3 textures.*/
        static const Preprocess& quake();

        static const Preprocess& normalMap();

        bool operator==(const Preprocess& other) const;
        
        bool operator!=(const Preprocess& other) const {
            return !(*this == other);
        }

    private:
        friend class Texture;

        /**
         Scales the intensity up or down of an entire image and gamma corrects.
         \deprecated
         */
        [[deprecated]] void modulateOffsetAndGammaAdjustImage(ImageFormat::Code fmt, void* src, void* dst, int n) const;
    };


    class Specification {
    public:
        String                      filename;
        String                      alphaFilename;

        /** If non-empty, overwrites the filename as the Texture::name */
        String                      name;

        /** Defaults to ImageFormat::AUTO() */
        Encoding                    encoding;

        /** Defaults to Texture::DIM_2D */
        Dimension                   dimension;

        /** Defaults to true. */
        bool                        generateMipMaps;

        Preprocess                  preprocess;

        Visualization               visualization;

        /** If true and desiredFormat is auto, prefer SRGB formats to RGB ones. */
        bool                        assumeSRGBSpaceForAuto;

        /** If false, this texture may not be loaded from or stored in the global texture cache. */
        bool                        cachable;
        
        Specification() : 
            dimension(DIM_2D),
            generateMipMaps(true),
            assumeSRGBSpaceForAuto(false),
            cachable(true) {}

        /** If the any is a number or color, creates a colored texture of the default dimension. 
            If it is a string, loads that as a filename. Otherwise, parses all fields. */
        Specification(const Any& any, bool assumesRGBForAuto = false, Dimension defaultDimension = DIM_2D);

        explicit Specification(const String& filename, bool assumesRGBForAuto = false, Dimension defaultDimension = DIM_2D) {
            *this = Specification(Any(filename), assumesRGBForAuto, defaultDimension);
        }

        explicit Specification(const Color4& c) {
            *this = Specification();
            filename = "<white>";
            encoding = Encoding(c);
        }

        void deserialize(BinaryInput& b);

        Specification(BinaryInput& b) {
            deserialize(b);
        }

        bool operator==(const Specification& s) const;

        bool operator!=(const Specification& s) const {
            return !(*this == s);
        }

        size_t hashCode() const;

        Any toAny() const;

        void serialize(BinaryOutput& b) const;
    };

    G3D_DECLARE_ENUM_CLASS(TexelType, FLOAT, INTEGER, UNSIGNED_INTEGER);

protected:

    typedef Array<Array<shared_ptr<PixelTransferBuffer>>> MIPCubeFacePTBArray;
    typedef Array<Array<const void*>> MIPCubeFacePointerArray;
        
    /** OpenGL texture ID. Always invoke force() before accessing this. */
    GLuint                            m_textureID;

    /** True if the GL texture is owned by the instance and should be cleaned up with the class instance. */
    bool                              m_destroyGLTextureInDestructor;

    Sampler                           m_cachedSamplerSettings;

    String                            m_name;
    String                            m_caption;
    Dimension                         m_dimension;
    bool                              m_opaque;

    Encoding                          m_encoding;
    int                               m_width;
    int                               m_height;
    int                               m_depth;

    /** Invoke force() before accessing. Does not factor in the encoding's scaling. */
    Color4                            m_min;

    /** Invoke force() before accessing. Does not factor in the encoding's scaling. */
    Color4                            m_max;

    /** Invoke force() before accessing. Does not factor in the encoding's scaling. */
    Color4                            m_mean;

    /** What AlphaFilter::DETECT should resolve to for this texture. Left as DETECT if not computed. */
    AlphaFilter                       m_detectedHint;

    /** Multi-sampled texture parameters */
    int                               m_numSamples;

    bool                              m_hasMipMaps;

    /** If true, this texture can be registered in the TextureBrowser for run-time
     debugging. */
    bool                              m_appearsInTextureBrowserWindow;

    bool                              m_conservativelyHasUnitAlpha = false;
    bool                              m_conservativelyHasNonUnitAlpha = false;


    //// Lambda expression type erasure
    //// This allows lambda expressions to be captured as their compiler deduced type
    //// avoiding the overhead of std::function
    //// https://stackoverflow.com/questions/2562176/storing-a-type-in-c
    //// also interesting: https://stackoverflow.com/questions/28746744/passing-lambda-as-function-pointer
    //class LambdaErasureBase {
    //public:
    //    virtual ~LambdaErasureBase() {}
    //    virtual void operator()(const GLuint) = 0;
    //};

    //template <typename T>
    //class LambdaErase : public LambdaErasureBase {
    //private:
    //    T erasedLambda;
    //public:
    //    LambdaErase(T erasedLambda) : erasedLambda(erasedLambda) {}
    //    virtual void operator()(const GLuint glID) override {
    //        return erasedLambda(glID);
    //    }
    //};

    mutable shared_ptr<std::function<void(GLuint)>>    m_reallocateHook;

    /** All transient information used by image loading and preprocessing of
        Textures, collected here so that the process can efficiently run on separate threads
        in a multithreaded environment.

        Using a class for this information works around some MSVC internal
        compiler errors with lambdas and simplifies the data ownership and
        collection. 
        
        \see fromFile(), fromMemory(), initFromPixelTransferBuffer        
      */
    class LoadingInfo {
    public:
        enum NextStep {
            UNINITIALIZED,

            // CPU
            LOAD_FROM_DISK, PREPROCESS, 
            
            // GPU
            TRANSFER_TO_GPU, SET_SAMPLER_PARAMETERS,
        
            DONE
        };
        
        /** Loading can be resumed from discrete steps because different factory methods begin
            at different points in the pipeline, some already having done preliminary preprocessing
            steps externally. */
        NextStep                        nextStep = LOAD_FROM_DISK;

        /** ptbArray[mip][face] = the buffer with image data.
            numFaces specifies the number of faces and ptbArray.size()
            is the number of MIPs (frequently, only one).
          
            Valid after LOAD_FROM_DISK. May be mutated by 
            PREPROCESS.
          */
        MIPCubeFacePTBArray             ptbArray;

        /** Filled out in Texture::fromFile */
        CubeMapConvention::CubeMapInfo  cubeMapInfo;

        /** Valid from construction. May be mutated if the format is ImageFormat::AUTO()
            after LOAD_FROM_DISK. */
        Encoding                        desiredEncoding;

        /** Set based on m_dimension on construction. */
        int                             numFaces = 1;

        /** The loaded data for the first image, used in LOAD_FROM_DISK.
            Set on construction. This must be loaded before a thread
            is launched so that the metadata (width, height, and format)
            can be parsed early. */
        BinaryInput*                    binaryInput = nullptr;

        /** Filenames on disk. Valid on construction, used by LOAD_FROM_DISK. */
        String                          filename[6];

        /** If true, the ptbArray owns its data and the calls can be threaded or delayed. */
        bool                            lazyLoadable = false;

        /** Used during TRANSFER_TO_GPU, set on construction */
        bool                            generateMipMaps = false;

        /** Used during PREPROCESS */
        Preprocess                      preprocess;
        
        /** Used during PREPROCESS when autodetecting formats */
        bool                            preferSRGBForAuto = true;

        LoadingInfo(NextStep s) : nextStep(s) {}
    };

    /** If true, this Texture is waiting for loading and/or upload to the GPU. Set by certain
        lazy initialization paths of Texture::fromFile. This is protected by m_loadingMutex,
        but can be conservatively checked for the false case without a mutex for efficiency. 
        Mutable so that it can be written during the const force() method. 
        
        \sa force() */
    mutable volatile bool             m_needsForce    = false;

    mutable LoadingInfo*              m_loadingInfo = nullptr;

    /** Protects m_needsForce, m_loadingThread, and m_loadingGLCallback. \sa force()  */
    mutable std::mutex                m_loadingMutex;

    /** The thread that lazy loads into memory by invoking the m_loadingCPUCallbackQueue
        callbacks. Not permitted to make any OpenGL calls.
        Mutable so that it can be deallocated during the const force() method.

        The loading thread advances through m_loadingInfo.nextStep processing
        until it reaches the UPLOAD_TO_GPU stage. That must be run during force
        on the GL thread.

        \sa force() */
    mutable std::thread*              m_loadingThread = nullptr;
    
    static int64                      m_sizeOfAllTexturesInMemory;
    
    /** Used for the Texture browser.  nullptr elements are flushed during reloadAll().
        The key is the raw pointer. */
    static WeakCache<uintptr_t, shared_ptr<Texture> >     s_allTextures;

    /** Used to avoid re-loading textures */
    static WeakCache<Specification, shared_ptr<Texture> > s_cache;

    Texture
       (const String&                  name,
        int                            width,
        int                            height,
        int                            depth,
        Dimension                      dimension,
        const Encoding&                encoding,
        int                            numSamples,
        bool                           needsForce);

    /** If the underlying texture has not yet been uploaded to the GPU, then this method immediately 
        blocks on the loading thread and does not return until the upload is completed. Otherwise it 
        does nothing. This should be called on the OpenGL thread. 
        
        \sa m_loadingThread, m_loadingGLCallback, m_loadingMutex, m_needsForce */
    void force() const;

    friend class BufferTexture;
    
    /**
     Helper method. Returns a new OpenGL texture ID that is not yet managed by a G3D Texture.
     Used by Texture and G3D::BufferTexture only.
     */
    static unsigned int newGLTextureID();

    /** Perform the next CPU steps specified in m_loadingInfo.
        Blocks. Does nothing if GPU loading is already complete. */
    void completeCPULoading();

    /** Perform the final GPU step specified in m_loadingInfo.
        It assumes that CPU loading has been completed and that
        the caller is on the GL thread.
        
        deletes m_loadingInfo.
    */
    void completeGPULoading();

public:

    /** Used to display this Texture in a GuiTextureBox  */
    Visualization                       visualization;

    static void getAllTextures(Array<shared_ptr<Texture>>& textures);
    static void getAllTextures(Array<weak_ptr<Texture>>& textures);

    inline bool appearsInTextureBrowserWindow() const {
        return m_appearsInTextureBrowserWindow;
    }

    inline bool hasMipMaps() const {
        force();
        return m_hasMipMaps;
    }

    /** Returns true if this texture might have non-unit alpha without forcing lazy loading to complete. */
    bool conservativelyHasNonUnitAlpha() const {
        return m_conservativelyHasNonUnitAlpha;
    }

    /** Returns true if this texture cannot have non-unit alpha, without forcing lazy loading to complete. */
    bool conservativelyHasUnitAlpha() const {
        return m_conservativelyHasUnitAlpha;
    }

    inline int numMipMapLevels() {
        if (! hasMipMaps()) {
            return 1;
        } else {
            // compute mipmap level according to : numLevels = 1 + floor(log2(max(w, h, d)))
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_non_power_of_two.txt
            //
            const int numMipMaps = int(floor(log2(float(std::max(m_width, std::max(m_height, m_depth)))))) + 1;
            return numMipMaps;
        }
    }

    void setName(const String& n) {
        m_name = n;
    }

    /** Deprecated, use dimension() == Texture::DIM_CUBE_MAP instead */
    inline bool isCubeMap() const {
        return (m_dimension == DIM_CUBE_MAP);
    }

    /** The value that AlphaFilter::DETECT should use for this texture when applied 
        to a G3D::UniversalMaterial.
        If this returns AlphaFilter::DETECT, then detection has not been executed, likely
        because the texture was not in an 8-bit format. */
    AlphaFilter alphaFilter() const {
        return opaque() ? AlphaFilter::ONE : m_detectedHint;
    }

    TexelType texelType() const;
    

    static shared_ptr<Texture> create(const Specification& s);

    /** 
        Returns a pointer to a texture with the name textureName, if such a texture exists. Returns null otherwise.
        If multiple textures have the name textureName, one is chosen arbitrarily.
        
        This function is not performant, it is linear in the number of existing textures; it is provided for convenience.
    */
    static shared_ptr<Texture> getTextureByName(const String& textureName);

    /**
       \deprecated Use toPixelTransferBuffer
    */
    [[deprecated]] void getTexImage(void* data, const ImageFormat* desiredFormat, CubeFace face = CubeFace::POS_X, int mipLevel = 0) const;

    /** Reads back a single texel.  This is slow because it stalls the CPU on the GPU.
        However, it requires less bandwidth than reading an entire image.
         

        \a rd If nullptr, set to RenderDevice::current;

        \sa toPixelTransferBuffer
    */
    Color4 readTexel(int ix, int iy, class RenderDevice* rd = nullptr, int mipLevel = 0, int iz = 0, CubeFace face = CubeFace::POS_X) const;

    /**
     Creates an empty texture (useful for later reading from the screen).
     
     \sa clear()
     */
    static shared_ptr<Texture> createEmpty
       (const String&                   name,
        int                             width,
        int                             height,
        const Encoding&                 encoding        = Encoding(ImageFormat::RGBA8()),
        Dimension                       dimension       = DIM_2D,
        bool                            generateMipMaps = false,
        int                             depth           = 1,
        int                             numSamples      = 1);

    /** Clear the texture to 0's (typically after creation, so that it does not contain unitialized data).
    */
    void clear(int mipLevel = 0);

    /** 
      Copies this texture over \a dest, allocating or resizing dest as needed. (It reallocs if the internal formats mismatch or dest is null)
      Uses a shader and makes a draw call. If src is a depth texture, mipLevel must be 0.

      DEPRECATED: To be replaced with a clearer API before G3D10 final release.

      \return true if dest was allocated/reallocated
    */
    [[deprecated]] bool copyInto(shared_ptr<Texture>& dest, CubeFace cf = CubeFace::POS_X, int mipLevel = 0, RenderDevice* rd = nullptr) const;

    /** 
      Copies src to dst, resizing if \a resize == true (the default). Both src and dst image formats are preserved.

      At least one of the following must hold: either dstMipLevel == 0, 
      or srcMipLevel == dstMipLevel.

      Uses a shader and makes a draw call in some cases, and executes a direct copy in others.

      \param scale Source coordinates are multipled by this before shift is added

      API subject to change

      \param shift Add This to destination coordinates to find source coordinates.  For example, 
      use the negative guard band thickness to crop a guard band. 
      
    */
    static void copy
       (shared_ptr<Texture> src, 
        shared_ptr<Texture> dst, 
        int                 srcMipLevel = 0, 
        int                 dstMipLevel = 0,
        float               scale       = 1.0f,
        const Vector2int16& shift       = Vector2int16(0,0),
        CubeFace            srcCubeFace = CubeFace::POS_X, 
        CubeFace            dstCubeFace = CubeFace::POS_X, 
        RenderDevice*       rd          = nullptr,
        bool                resize      = true,
        int                 srcLayer    = 0,
        int                 dstLayer    = 0);

    /** Resize the underlying OpenGL texture memory buffer, without
        reallocating the OpenGL texture ID.  This does not scale the
        contents; the contents are undefined after resizing.  This is
        only useful for textures that are render targets. */
    void resize(int w, int h);

    /** New resize for 3D textures */
    void resize(int w, int h, int d);


    /**
     Wrap will override the existing parameters on the
     GL texture.

     \param name Arbitrary name for this texture to identify it
     \param textureID Set to newGLTextureID() to create an empty texture.
     \param destroyGLTextureInDestructor If true, deallocate the OpenGL texture when the G3D::Texture is destroyed
     */
    static shared_ptr<Texture> fromGLTexture
    (const String&                   name,
     GLuint                          textureID,
     Encoding                        encoding,
     AlphaFilter                     alphaFilter,
     Dimension                       dimension                      = DIM_2D,
     bool                            destroyGLTextureInDestructor   = true,
     int                             numSamples                     = 1,
     int                             width                          = -1,
     int                             height                         = -1,
     int                             depth                          = -1,
     bool                            hasMIPMaps                     = false);
    
    /**
     Creates a texture from a single image or a set of files for a texture array or cube
     map based on a wildcard specification
     such as `foo-*.png`. Cube maps can have any common cube map specifier in the wildcard,
     such as "bk", "+z", or "pz". Texture arrays must have sequential integers in the
     wildcard.
     The image must have a format understood by G3D::Image or be a DirectDraw Surface (DDS).    
     */    
    static shared_ptr<Texture> fromFile
    (const String&                   filenameSpec,
     const Encoding                  encoding           = Encoding(),
     Dimension                       dimension          = DIM_2D,
     bool                            generateMipMaps    = true,
     const Preprocess&               preprocess         = Preprocess::defaults(),
     bool                            preferSRGBForAuto  = false);
    
    /**
     Creates a texture from the colors of filename and takes the alpha values
     from the red channel of alpha filename if useAlpha is set to true, it will 
     use the alpha channel instead. See G3D::RenderDevice::setBlendFunc
     for important information about turning on alpha blending. 
     */
    static shared_ptr<Texture> fromTwoFiles
    (const String&                   filename,
     const String&                   alphaFilename,
     Encoding                        encoding       = Encoding(),
     Dimension                       dimension      = DIM_2D,
     bool                            generateMipMaps = true,
     const Preprocess&               preprocess     = Preprocess::defaults(),
     bool                            preferSRGBForAuto  = true,
     const bool                      useAlpha       = false);

    /**
    Construct from an explicit set of (optional) mipmaps and (optional) cubemap faces.

    bytes[miplevel][cubeface] is a pointer to the bytes
    for that miplevel and cube
    face. If the outer array has only one element and the
    interpolation mode is
    TRILINEAR_MIPMAP, mip-maps are automatically generated from
    the level 0 mip-map.

    There must be exactly
    6 cube faces per mip-level if the dimensions are
    DIM_CUBE and and 1 per
    mip-level otherwise. You may specify compressed and
    uncompressed formats for
    both the bytesformat and the desiredformat.
   
    3D Textures may not use mip-maps.

    Data is converted between normalized fixed point and floating point as described in section 2.1.5 of the OpenGL 3.2 specification.
    Specifically, uint8 values are converted to floating point by <code>v' = v / 255.0f</code>.  Note that this differs from
    how G3D::Color1uint8 converts to G3D::Color3.

    Note: OpenGL stores values at texel centers.  Thus element at integer position (x, y) in the input "image" is stored at
    texture coordinate ((x + 0.5) / width, (x + 0.5) / height).

    */
    static shared_ptr<Texture> fromMemory
       (const String&                       name,
        const Array< Array<const void*> >&  bytes,
        const ImageFormat*                  bytesFormat,
        int                                 width,
        int                                 height,
        int                                 depth,
        int                                 numSamples      = 1,
        Encoding                            encoding        = Encoding(ImageFormat::AUTO()),
        Dimension                           dimension       = DIM_2D,
        bool                                generateMipMaps = true,
        const Preprocess&                   preprocess      = Preprocess::defaults(),
        bool                                preferSRGBForAuto = false);

    /** Construct from a single packed 2D or 3D data set.
        Shorthand for `Texture::fromMemory(name, Array<Array<const void*>> bytes, ...)` */
    static shared_ptr<Texture> fromMemory
       (const String&                       name,
        const void*                         bytes,
        const ImageFormat*                  bytesFormat,
        int                                 width,
        int                                 height,
        int                                 depth,
        int                                 numSamples      = 1,
        Encoding                            encoding        = Encoding(),
        Dimension                           dimension       = DIM_2D,
        bool                                generateMipMaps = true,
        const Preprocess&                   preprocess      = Preprocess::defaults(),
        bool                                preferSRGBForAuto = false);

    static shared_ptr<Texture> fromPixelTransferBuffer
       (const String&                       name,
        const shared_ptr<PixelTransferBuffer>& image,
        Encoding                            desiredEncoding = Encoding(),
        Dimension                           dimension       = DIM_2D,
        bool                                generateMipMaps = true,
        const Preprocess&                   preprocess      = Preprocess::defaults());

    /** Shorthand for `Texture::fromPixelTransferBuffer(name, image->toPixelTransferBuffer(), ...)`
        \sa Texture::fromMemory  */
    static shared_ptr<Texture> fromImage
       (const String&                   name,
        const shared_ptr<Image>&        image,
        const ImageFormat*              desiredFormat   = ImageFormat::AUTO(),
        Dimension                       dimension       = DIM_2D,
        bool                            generateMipMaps = true,
        const Preprocess&               preprocess      = Preprocess::defaults());

    /** Creates another texture that is the same as this one but contains only
        an alpha channel.  Alpha-only textures are useful as mattes.  
        
        If the current texture is opaque(), returns nullptr (since it is not useful
        to construct an alpha-only version of a texture without an alpha channel).
        
        Like all texture construction methods, this is fairly
        slow and should not be called every frame during interactive rendering.*/
    shared_ptr<Texture> alphaOnlyVersion() const;

    /**
     Copies data from screen into an existing texture (replacing whatever was
     previously there).  The dimensions must be powers of two or a texture 
     rectangle will be created (not supported on some cards).

     <i>This call is provided for backwards compatibility on old cards. It is 
      substantially slower than simply rendering to a
     G3D::Texture using a G3D::Framebuffer.</i>

     The (x, y) coordinates are in OpenGL coordinates.  If a FrameBuffer is bound then (0, 0) is the top left
     of the screen.  When rendering directly to a window, (0,0) is the lower left.
     Use RenderDevice::copyTextureFromScreen to obtain consistent coordinates.

     The texture dimensions will be updated but all other properties will be preserved:
     The previous wrap mode will be preserved.
     The interpolation mode will be preserved (unless it required a mipmap,
     in which case it will be set to BILINEAR_NO_MIPMAP).  The previous color m_depth
     and alpha m_depth will be preserved.  Texture compression is not supported for
     textures copied from the screen.

     To copy a depth texture, first create an empty depth texture then copy into it.

     If you invoke this method on a texture that is currently set on RenderDevice,
     the texture will immediately be updated (there is no need to rebind).

     \param rect The rectangle to copy (relative to the viewport)

     \param fmt If nullptr, uses the existing texture format, otherwise 
     forces this texture to use the specified format.

     @sa RenderDevice::screenShotPic
     @sa RenderDevice::setReadBuffer

     \deprecated
     */
    [[deprecated]] void copyFromScreen(const Rect2D& rect, const ImageFormat* fmt = nullptr);

    /**
     Copies into the specified face of a cube map.  Because cube maps can't have
     the Y direction inverted (and still do anything useful), you should render
     the cube map faces <B>upside-down</B> before copying them into the map.  This
     is an unfortunate side-effect of OpenGL's cube map convention.  
     
     Use G3D::Texture::getCubeMapRotation to generate the (upside-down) camera
     orientations.
     */
    void copyFromScreen(const Rect2D& rect, CubeFace face);

    /**
     How much (texture) memory this texture occupies.  OpenGL backs
     video memory textures with main memory, so the total memory 
     is actually twice this number.
     */
    int sizeInMemory() const;

    /**
     Video memory occupied by all OpenGL textures allocated using Texture
     or maintained by pointers to a Texture.
     */
    inline static int64 sizeOfAllTexturesInMemory() {
        return m_sizeOfAllTexturesInMemory;
    }

    /**
     True if this texture was created with an alpha channel.  Note that
     a texture may have a format that is not opaque (e.g., RGBA8) yet still
     have a completely opaque alpha channel, causing texture->opaque to
     be true.  This is just a flag set for the user's convenience-- it does
     not affect rendering in any way.
     See G3D::RenderDevice::setBlendFunc
     for important information about turning on alpha blending. 
     */
    inline bool opaque() const {
        if (m_encoding.readMultiplyFirst.a < 1.0f) {
            return false;
        } else if (m_conservativelyHasUnitAlpha) {
            return true;
        } else if (m_conservativelyHasNonUnitAlpha) {
            return false;
        } else {
            force();
            return m_opaque;
        }
    }

    shared_ptr<GLPixelTransferBuffer> toPixelTransferBuffer(const ImageFormat* outFormat = ImageFormat::AUTO(), int mipLevel = 0, CubeFace face = CubeFace::POS_X) const;
    // Overload writes output to passed in pixelTransferBuffer
    void toPixelTransferBuffer(shared_ptr<GLPixelTransferBuffer>& buffer, const ImageFormat* outFormat = ImageFormat::AUTO(), int mipLevel = 0, CubeFace face = CubeFace::POS_X, bool runMapHooks = true) const;
    shared_ptr<Image> toImage(const ImageFormat* outFormat = ImageFormat::AUTO(), int mipLevel = 0, CubeFace face = CubeFace::POS_X) const;

private:
    
    static shared_ptr<Texture> loadTextureFromSpec(const Texture::Specification& s);

    // These methods are all deprecated.  They are here only until Component and MapComponent are rewritten.

    template<class C, class I> friend class Component;
    template<class I> friend class MapComponent;

    shared_ptr<Image4> toImage4() const;
        
    /** Extracts the data as ImageFormat::RGB32F. */
    shared_ptr<Image3> toImage3(CubeFace face = CubeFace::POS_X, int mip = 0) const;    
    
    /** Extracts the data as ImageFormat::L32F.   @deprecated */
    shared_ptr<Image1> toImage1() const;    
    
    /** Extracts the data as ImageFormat::DEPTH32F. @deprecated */
    shared_ptr<Image1> toDepthImage1() const;
    
    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. @deprecated */
    inline void getImage(shared_ptr<Image4>& im) const {
        im = toImage4();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. @deprecated */
    inline void getImage(shared_ptr<Image3>& im) const {
        im = toImage3();
    }

    /** Reassigns the im pointer; does not write to the data currently in it.  Useful when working with a templated image. @deprecated */
    inline void getImage(shared_ptr<Image1>& im) const {
        im = toImage1();
    }
    
public:

    /** If this texture was loaded from an uncompressed format in memory or disk (and not rendered to), 
       this is the smallest value in the texture, modified by the encoding's scale and bias. NaN if unknown. */
    inline Color4 min() const {
        force();
        return m_min * m_encoding.readMultiplyFirst + m_encoding.readAddSecond;
    }

    /** If this texture was loaded from an uncompressed format in memory or disk (and not
       rendered to), then this is the largest value in the texture, modified by the encoding's scale
       and bias. NaN if unknown. */
    inline Color4 max() const {
        force();
        return m_max * m_encoding.readMultiplyFirst + m_encoding.readAddSecond;
    }

    /** If this texture was loaded from an uncompressed format in memory or disk (and not
       rendered to), then this is the average value in the texture, modified by the encoding's scale
       and bias. NaN if unknown. */
    inline Color4 mean() const {
        force();
        return m_mean * m_encoding.readMultiplyFirst + m_encoding.readAddSecond;
    }

    /** Extracts the data as ImageFormat::DEPTH32F. */
    shared_ptr<Map2D<float>> toDepthMap() const;
    
    /** Extracts the data as ImageFormat::DEPTH32F and converts to 8-bit. Note that you may
        want to call Image1unorm8::flipVertical afterward. */
    shared_ptr<Image1unorm8> toDepthImage1unorm8() const;
    
    inline unsigned int openGLID() const {
        force();
        return m_textureID;
    }

    /** Number of horizontal texels in the level 0 mipmap */
    inline int width() const {
        return m_width;
    }

    /** Number of horizontal texels in the level 0 mipmap */
    inline int height() const {
        return m_height;
    }

    inline int depth() const {
        return m_depth;
    }

    inline Vector2 vector2Bounds() const {
        return Vector2((float)m_width, (float)m_height);
    }

    /** Returns a rectangle whose m_width and m_height match the dimensions of the texture. */
    Rect2D rect2DBounds() const;

    /** For debugging purposes, appears in the texture viewer. \sa caption */
    const String& name() const {
        return m_name;
    }

    /** Suggested caption for screen capture. If empty, returns name(). \sa name, setName */
    String caption() const;

    void setCaption(const String& c) {
        m_caption = c;
    }

    /** \deprecated Use encoding().format */
    const ImageFormat* format() const {
        return m_encoding.format;
    }

    const Encoding& encoding() const {
        return m_encoding;
    }
    
    Dimension dimension() const {
        return m_dimension;
    }

    int numSamples(){
        return m_numSamples;
    }

    /**
     Deallocates the OpenGL texture.
     */
    virtual ~Texture();

    /**
     The OpenGL texture target this binds (e.g. GL_TEXTURE_2D)
     */
    unsigned int openGLTextureTarget() const;

    /** For a texture with that supports the FrameBufferObject extension, 
       generate mipmaps from the level 0 mipmap immediately.  For other textures, does nothing.
       */
    void generateMipMaps();

    /**
       Upload new data from the CPU to this texture.  Corresponds to <a
       href="http://www.opengl.org/sdk/docs/man/xhtml/glTexSubImage2D.xml">glTexSubImage2D</a>.
       If \a src is smaller than the current dimensions of \a this, only
       part of \a this is updated.

       This routine does not provide the same protections as creating a
       new Texture from memory: you must handle scaling and ensure
       compatible formats yourself.

       \param face If specified, determines the cubemap face to copy into

       \param src GLPixelTransferBuffer is handled specially to support updates when the buffer is not yet PixelTransferBuffer::readyToMap.  All other
       PixelTransferBuffer%s are mapped for read.  The upload to the GPU is asynchronous (assuming that the GPU driver is reasonable) and the 
       buffer may be modified immediately on return.
    */
    void update(const shared_ptr<PixelTransferBuffer>& src,
                int mipLevel = 0,
                CubeFace face = CubeFace::POS_X,
                bool runMapHooks = true,
                size_t byteOffset = 0,
                bool resizeTexture = true);

    /**
        Binds the following uniforms:

        - samplerX prefix\#\#buffer;
        - vecY prefix\#\#size;
        - vecY prefix\#\#invSize;
        - vec4 prefix\#\#readMultiplyFirst;
        - vec4 prefix\#\#readAddSecond;

        to \a args, with X and Y being chosen based on the texture dimension.

        If prefix contains no period (deprecated), then the macro is bound:

        * \#define prefix\#\#notNull 1

        otherwise, 

        * bool prefix\#\#notNull

        is set to true.

        Inside a shader, these arguments can be automatically defined using the macro (deprecated)
        
        \code
        #include &lt;Texture/Texture.glsl&gt;
        uniform_Texture(samplerType, name_)
        \endcode

        or by using a struct 

        \code
        #include &lt;Texture/Texture.glsl&gt;
        uniform Texture2D name;
        \endcode

        \sa uniform_Texture
    */
    void setShaderArgs(class UniformTable& args, const String& prefix, const Sampler& sampler);

    /** Returns a texture of 1024^2 oct32-encoded cosine-weighted hemispherical random vectors about the
        positive z-axis.
        Use octDecode(texelFetch(g3d_cosHemiRandom, pos, 0).xy) in a shader to decode these.
        This is automatically bound as g3d_cosHemiRandom in shaders that use it.
    */
    static shared_ptr<Texture> cosHemiRandom();

    /** Returns a texture of 1024^2 oct32-encoded uniformly distributed random vectors on the sphere.
        Use octDecode(texelFetch(g3d_sphereRandom, pos, 0).xy) in a shader to decode these.
        This is automatically bound as g3d_sphereRandom in shaders that use it.
    */
    static shared_ptr<Texture> sphereRandom();

    /** Returns a texture of 1024^2 RG16 vectors of uniformly distributed random numbers: ([0,1], [0,1]). 
        This is automatically bound as g3d_uniformRandom in shaders that use it. */
    static shared_ptr<Texture> uniformRandom();

    shared_ptr<class CubeMap> toCubeMap() const;

    void registerReallocationHook(std::function<void(GLuint)> reallocateHook) const {
        m_reallocateHook = std::make_shared<std::function<void(GLuint)>> (reallocateHook);
    }

private:

    void reallocateHook(const GLuint glID) {
        if (notNull(m_reallocateHook)) {
            m_reallocateHook->operator()(glID);
        }
    }

    void computeMinMaxMean();

    static void setDepthReadMode(GLenum target, DepthReadMode depthReadMode);
    /** Allows forcing a change to the depthReadMode of the texture currently bound to the target. */
    static void setDepthTexParameters(GLenum target, DepthReadMode depthReadMode);
    static void setAllSamplerParameters(GLenum target,  const Sampler& settings);
    void updateSamplerParameters(const Sampler& settings);

#ifdef G3D_ENABLE_CUDA

private:
    //Everything needed for CUDA mapping
    CUgraphicsResource          m_cudaTextureResource;
    CUarray                     m_cudaTextureArray;

    unsigned int                m_cudaUsageFlags;
    bool                        m_cudaIsMapped;
public:
    
    CUarray &cudaMap(unsigned int   usageflags);

    void cudaUnmap();

#endif

};

} // namespace

template <> struct HashTrait<shared_ptr<G3D::Texture> > {
    static size_t hashCode(const shared_ptr<G3D::Texture>& key) { return reinterpret_cast<size_t>(key.get()); }
};

template <> struct HashTrait<G3D::Texture::Specification> {
    static size_t hashCode(const G3D::Texture::Specification& key) { return key.hashCode(); }
};

