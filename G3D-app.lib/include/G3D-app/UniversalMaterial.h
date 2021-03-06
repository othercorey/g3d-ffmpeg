/**
  \file G3D-app.lib/include/G3D-app/UniversalMaterial.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#pragma once

#include "G3D-base/platform.h"
#include "G3D-base/enumclass.h"
#include "G3D-base/lazy_ptr.h"
#include "G3D-base/HashTrait.h"
#include "G3D-base/constants.h"
#include "G3D-base/ReferenceCount.h"
#include "G3D-gfx/Sampler.h"
#include "G3D-app/Component.h"
#include "G3D-app/UniversalBSDF.h"
#include "G3D-app/BumpMap.h"
#include "G3D-app/Material.h"
#include "G3D-app/Surfel.h"

namespace G3D {
class Any;
class Args;


/** 
  \brief Description of a surface for rendering purposes.

  Encodes a BSDF, bump map, emission function, and flags.

  Note that for real-time rendering most translucent surfaces should
  be two-sided and have comparatively low diffuse terms.  They should
  also be applied to convex objects (subdivide non-convex objects) to
  prevent rendering surfaces out of order.  For ray tracing, implement
  translucent surfaces as two single-sided surfaces: one for entering
  the material and one for exiting it (i.e., the "backfaces").  The
  eta of the exiting surface should be that of the medium that is
  being exited into--typically, air.  So a glass sphere is a set of
  front faces with eta ~= 1.3 and a set of backfaces with eta = 1.0.


  \sa G3D::UniversalBSDF, G3D::Component, G3D::Texture,
  G3D::BumpMap, G3D::ArticulatedModel, G3D::GBuffer
  */
class UniversalMaterial : public Material {
public:

    /** \brief Specification of a material; used for loading.  
    
        Can be written to a file or constructed from a series of calls.
        
        The following terminology for photon scattering is used in the
        G3D::UniversalMaterial::Specification and G3D::UniversalBSDF
        classes and their documentation:

        \htmlonly <img src="scatter-terms.png" width="800px"> \endhtmlonly
    */
    class Specification {
    private:
        friend class UniversalMaterial;

        Texture::Specification  m_lambertian;
        shared_ptr<Texture>     m_lambertianTex;

        Texture::Specification  m_glossy;
        shared_ptr<Texture>     m_glossyTex;

        Texture::Specification  m_transmissive;
        shared_ptr<Texture>     m_transmissiveTex;

        float                   m_etaTransmit;
        Color3                  m_extinctionTransmit;

        float                   m_etaReflect;
        Color3                  m_extinctionReflect;

        Texture::Specification  m_emissive;
        shared_ptr<Texture>     m_emissiveTex;

        String                  m_customShaderPrefix;

        BumpMap::Specification  m_bump;

        /** Preferred level of refraction quality. The actual level available depends on the renderer.*/
        RefractionHint          m_refractionHint;

        /** Preferred level of mirror reflection quality. The actual level available depends on the renderer.*/
        MirrorQuality           m_mirrorHint;

        int                     m_numLightMapDirections;
        shared_ptr<Texture>     m_lightMap[3];

        Table<String, double>   m_constantTable;

        AlphaFilter             m_alphaFilter;

        Sampler                 m_sampler;

        /** \see Surfel::flags */
        uint8                   m_flags = 0;

        /** boolean or "AUTO" */
        Any                     m_inferAmbientOcclusionAtTransparentPixels;

        Component4 loadLambertian() const;
        Component4 loadGlossy() const;
        Component3 loadTransmissive() const;
        Component3 loadEmissive() const;

    public:

        Specification();

        void setSampler(const Sampler& sampler) {
            m_sampler = sampler;
        }

        const Sampler& sampler() const {
            return m_sampler;
        }

        void setFlags(uint8 f) { m_flags = f; }
        uint8 flags() const { return m_flags; }

        /** 
\brief Construct a UniversalMaterial::Specification from an Any, typically loaded by parsing a file.
Some simple examples follow.

<b>All fields as texture maps:</b>
\code
UniversalMaterial::Specification {
    lambertian = "diffusemap.png";

    alphaFilter = BINARY;
 
    // Put smoothness in the alpha channel
    glossy = "specmap.png";

    transmissive = "transmap.png"; // Simple transmission

    emissive = "emitmap.png";
    emissive = mul("emitmap.png", 3.0); // various options for emissive
    emissive = Color3(0.5);

    bump = "bumpmap.png";  // see BumpMap::Specification

    // Sophisticated transmission:
    // Properties of the back-face side:
    etaTransmit = 1.0;
    extinctionTransmit = Color3(1,1,1);

    flags = 0;
      
    // Properties of the front-face side:
    etaReflect = 1.0;
    extinctionReflect = Color3(1,1,1);
              
    // Hints and hacks
    refractionHint = "DYNAMIC_FLAT";
    mirrorHint = "STATIC_PROBE";
    customShaderPrefix = "";    

    // Should transparent pixels guess their AO value from nearby opaque ones?
    // This looks good on tree leaves and bad on glass or smoke
    inferAmbientOcclusionAtTransparentPixels = false;

    sampler = Sampler {
       maxMipMap = 4;
    };

    // Arbitrary fields for use in experiments and prototypes.
    // These are bound to macros in shaders and stored in a double. Setting 
    // a constant to nan() unsets it.
    constantTable = {
        ANISO = 1;
        HEIGHT = 3.7;
        ITERATIONS = 1;
    };
}
\endcode
<b>Diffuse yellow (any Color3 automatically coerces to a pure Lambertian surface):</b>
\code
Color3(1,1,0)
\endcode

<b>Simple diffuse texture (any string coerces to a filename for the Lambertian texture):</b>
\code
"dirt.jpg"
\endcode

<b>Mirror:</b>
\code
UniversalMaterial::Specification {
    lambertian = Color3(0.01);
    glossy = Color4(Color3(0.95), 1.0);
}
\endcode

<b>Red plastic:</b>
\code
UniversalMaterial::Specification {
    lambertian = Color3(0.95, 0.2, 0.05);
    glossy = Color4(Color3(0.3), 0.5);
}
\endcode
          
<b>Green glass:</b>
\code
UniversalMaterial::Specification {
    lambertian = Color3(0.01, 0.1, 0.05);
    transmissive = Color3(0.01, 0.9, 0.01);
    glossy = Color4(Color3(0.4), 1.0);
}            
\endcode

Any component can be a Texture::Specification, Color3/Color4, or table of <code>{texture = ...; constant = ...}</code>.

\sa G3D::RefractionHint, \sa G3D::MirrorQuality, \sa G3D::BumpMapSpecification
 */
        Specification(const Any& any);

        Specification(const Color4& lambertian);

        bool operator==(const Specification& s) const;

        Any toAny() const;

        bool operator!=(const Specification& s) const {
            return !((*this) == s);
        }

        /** Load from a file created by save(). */
        void load(const String& filename);

        void setCustomShaderPrefix(const String& s) {
            m_customShaderPrefix = s;
        }

        void setConstant(const String& name, float c) {
            m_constantTable.set(name, c);
        }

        void setAlphaHint(const AlphaFilter& a) {
            m_alphaFilter = a;
        }

        AlphaFilter alphaFilter() const {
            return m_alphaFilter;
        }

        Any inferAmbientOcclusionAtTransparentPixels() const {
            return m_inferAmbientOcclusionAtTransparentPixels;
        }

        /** boolean or "auto" */
        void setinferAmbientOcclusionAtTransparentPixels(Any b) {
            m_inferAmbientOcclusionAtTransparentPixels = b;
        }

        void setConstant(const String& name, int c) {
            m_constantTable.set(name, double(c));
        }

        void setConstant(const String& name, bool c) {
            m_constantTable.set(name, double(c));
        }

        /** Filename of Lambertian ("diffuse") term, empty if none. The
            alpha channel is a mask that will be applied to all maps
            for coverage.  That is, alpha = 0 indicates holes in the
            surface.  Alpha is for partial coverage. Do not use alpha
            for transparency; set transmissiveFilename instead.
            
            The image file is assumed to be in the sRGB color space.
            The constant is multiplied in "linear" space, after sRGB->RGB
            conversion.
            */
        void setLambertian(const Texture::Specification& spec);
        
        void setLambertian(const shared_ptr<Texture>& tex);

        /** Makes the surface opaque black. */
        void removeLambertian();

        void setEmissive(const Texture::Specification& spec);
        void setEmissive(const shared_ptr<Texture>& tex);

        void removeEmissive();

        /** Sets the diffuse light map for this surface. Pass shared_ptr<Texture>() 
            to erase an existing light map.*/
        void setLightMaps(const shared_ptr<Texture>& lightMap);

        /** Sets the radiosity normal map for this material specification. */
        void setLightMaps(const shared_ptr<Texture> lightMap[3]);

        /** Sets the radiosity normal map for this material specification from the other material. */
        void setLightMaps(const shared_ptr<UniversalMaterial>& otherMaterial);

        /** Mirror reflection or glossy reflection.
            This actually specifies 
            the \f$F_0\f$ term, which is the minimum reflectivity of the surface.  At 
            glancing angles it will increase towards white.
            
            The alpha channel specifies the smoothness of the surface on [0, 1].
            Smoother materials have sharp, bright highlights. Setting this to zero disables
            glossy reflection.
            */
        void setGlossy(const Texture::Specification& spec);
        void setGlossy(const shared_ptr<Texture>& tex);

        /** */
        void removeGlossy();

        /** This is an approximation of attenuation due to extinction
           while traveling through a translucent material.  Note that
           no real material is transmissive without also being at
           least slightly glossy.
           
           The image file is assumed to be in the sRGB color space.
            The constant is multiplied in "linear" space, after sRGB->RGB
            conversion.*/
        void setTransmissive(const Texture::Specification& spec);
        void setTransmissive(const shared_ptr<Texture>& tex);

        void removeTransmissive();

        /** Set the index of refraction. Not used unless transmissive is non-zero. */
        void setEta(float etaTransmit, float etaReflect);

        void setExtinction(const Color3& extinctionTransmit, const Color3& extinctionReflect) {
            m_extinctionTransmit = extinctionTransmit;
            m_extinctionReflect  = extinctionReflect;
        }

        /**
           The image is assumed to be in linear (R) space.

           @param normalMapWhiteHeightInPixels When loading normal
              maps, argument used for G3D::GImage::computeNormalMap()
              whiteHeightInPixels.  Default is 2.0f
              \deprecated
        */
        [[deprecated]] void setBump
        (const String&              filename,
         const BumpMap::Settings&   settings = BumpMap::Settings(),
         float                      normalMapWhiteHeightInPixels = 4.0f,
         bool                       invertPrecomputedNormalYAxis = false);

        void setBump(const BumpMap::Specification& bump) {
            m_bump = bump;
        }

        void removeBump();

        /** Defaults to G3D::RefractionHint::DYNAMIC_FLAT */
        void setRefractionHint(RefractionHint q) {
            m_refractionHint = q;
        }

        /** Defaults to G3D::MirrorQuality::STATIC_PROBE */
        void setMirrorHint(MirrorQuality q) {
            m_mirrorHint = q;
        }

        size_t hashCode() const;
    };

protected:

    String                      m_name;

    /** Scattering function */
    shared_ptr<UniversalBSDF>   m_bsdf;

    /** Emission map.  This emits radiance uniformly in all directions. */
    Component3                  m_emissive;

    int                         m_numLightMapDirections;

    /**
       \copydoc lightMap
    */
    Component3                  m_lightMap[3];

    /** Bump map */
    shared_ptr<BumpMap>         m_bump;

    /** For experimentation.  This code (typically macro definitions)
        is injected into the shader code after the material constants.
      */
    String                      m_customShaderPrefix;

    /** Preferred level of refraction quality. The actual level
        available depends on the renderer.*/
    RefractionHint              m_refractionHint;

    /** Preferred level of mirror reflection quality. The actual level
        available depends on the renderer.*/
    MirrorQuality               m_mirrorHint;

    bool                        m_inferAmbientOcclusionAtTransparentPixels;

    /** These constants are also in the m_macros string. */
    Table<String, double>       m_constantTable;

    /** Are the macros out of date? */
    bool                        m_needsComputeDefines = true;
    String                      m_macros;

    AlphaFilter                 m_alphaFilter;

    uint8                       m_flags;

    Sampler                     m_sampler;

    UniversalMaterial();

public:

    /** \brief Constructs an empty UniversalMaterial, which has no
       BSDF. This is provided mainly for efficiency when constructing
       a UniversalMaterial manually. Use UniversalMaterial::create()
       to create a default material. */
    static shared_ptr<UniversalMaterial> createEmpty();

    virtual const String& name() const override {
       return m_name;
    }

    /** The sampler used for all Texture%s */
    const Sampler& sampler() const {
        return m_sampler;
    }
    
    /** 
        Returns the dimension of the textures in the material
    */
    Texture::Dimension textureDimension() const;

    bool hasTransmissive() const;

    bool hasEmissive() const;

    /** \sa Surfel::flags */
    uint8 flags() const {
        return m_flags;
    }

    bool inferAmbientOcclusionAtTransparentPixels() const {
        return m_inferAmbientOcclusionAtTransparentPixels;
    }

    /** The UniversalMaterial::create(const Settings& settings) factor method is recommended 
       over this one because it performs caching and argument validation. 
       \deprecated
       */ 
    [[deprecated]] static shared_ptr<UniversalMaterial> create
    (const shared_ptr<UniversalBSDF>&    bsdf,
     const Component3&                   emissive           = Component3(),
     const shared_ptr<BumpMap>&          bump               = shared_ptr<BumpMap>(),
     const Array<Component3>&            lightMaps          = Array<Component3>(),
     const String&                       customShaderPrefix = "",
     const AlphaFilter                   alphaFilter        = AlphaFilter::DETECT,
     const uint8                         flags              = 0);
    
    /** Returns nan() if the constant is not bound. */
    double constant(const String& name) const {
        double* v = m_constantTable.getPointer(name);
        if (v) {
            return *v;
        } else {
            return nan();
        }
    }

    /** 
        Checks that all textures in the material use the same dimensions, 
        returns true if they do.
     */
    bool validateTextureDimensions() const;

    AlphaFilter alphaFilter() const {
        return m_alphaFilter;
    }

    /**
       Caches previously created Materials, and the textures 
       within them, to minimize loading time.

       Materials are initially created with all data stored exclusively 
       on the GPU. Call setStorage() to move or copy data to the CPU 
       (note: it will automatically copy to the CPU as needed, but that 
       process is not threadsafe).
     */
    static shared_ptr<UniversalMaterial> create(const Specification& settings = Specification());

    static shared_ptr<UniversalMaterial> create(const String& name, const Specification& settings);

    /**
     Create a G3D::UniversalMaterial using a Lambertian (pure diffuse) G3D::BSDF with color @a p_Lambertian.
     */
    static shared_ptr<UniversalMaterial> createDiffuse(const Color3& p_Lambertian);
    
    static shared_ptr<UniversalMaterial> createDiffuse(const String& textureFilename);

    /**
     Create a G3D::UniversalMaterial using the \a texture as the Lambertian (pure diffuse), with no emissive or bump.
     */
    static shared_ptr<UniversalMaterial> createDiffuse(const shared_ptr<Texture>& texture);

    /** Flush the material cache.  If you're editing texture maps on
        disk and want to reload them, invoke this first. */
    static void clearCache();
   

protected:

    /** Appends a string of GLSL macros (e.g., "#define LAMBERTIANMAP\n") to
        @a defines that
        describe the specified components of this G3D::UniversalMaterial, as used by 
        G3D::Shader.
        \sa macros()
      */
    void computeDefines(String& defines) const;

public:

    void setStorage(ImageStorage s) const override;

    /** Never nullptr */
    const shared_ptr<UniversalBSDF>& bsdf() const {
        return m_bsdf;
    }

    /** May be nullptr */
    const shared_ptr<BumpMap>& bump() const {
        return m_bump;
    }

    /** \copydoc m_customShaderPrefix */
    const String& customShaderPrefix() const {
        return m_customShaderPrefix;
    }

    bool hasAlpha() const;

    /** An emission function.

        Dim emission functions are often used for "glow", where a
        surface is bright independent of external illumination but
        does not illuminate other surfaces.

        Bright emission functions are used for light sources under the
        photon mapping algorithm.

        The result is not a pointer because G3D::Component3 is
        immutable and already indirects the
        G3D::Component::MapComponent inside of it by a pointer.*/
    inline const Component3& emissive() const {
        return m_emissive;
    }


    /** 0, 1, or 3.  \sa lightMap */
    int numLightMapDirections() const {
        return m_numLightMapDirections;
    }

    /** 
        Directional light maps.  These are treated as additional
        <i>incoming</i> light on the surface.

        If numLightMapDirections is 0, this is unused.
        
        If numLightMapDirections is 1, incident light is stored in lightMap[0] and 
        is assumed to be at normal incidence, i.e., coming from wi = (0, 0, 1) in tangent space, where the axes are t1, t2, and n.

        If numLightMapDirections is 3, incident light is stored in lightMap[0..3]
        and follows the HL2 basis (see "Radiosity normal mapping" http://www2.ati.com/developer/gdc/D3DTutorial10_Half-Life2_Shading.pdf):

        - lightMap[0] is light incoming from (sqrt(2/3), 0, 1/sqrt(3)),
        - lightMap[1] is light incoming from (-1/sqrt(6),  1/sqrt(2), 1/sqrt(3)),
        - lightMap[2] is light incoming from (-1/sqrt(6), -1/sqrt(2), 1/sqrt(3)).

     */
    inline const Component3* lightMap() const {
        return m_lightMap;
    }
    
    /** \brief Preprocessor macros for GLSL defining the fields used.*/
    const String& macros() const {
        if (m_needsComputeDefines) {
            UniversalMaterial* me = const_cast<UniversalMaterial*>(this);
            me->m_needsComputeDefines = false;
            me->computeDefines(me->m_macros);
        }
        return m_macros;
    }

    /** \sa uniform_UniversalMaterial */
    void setShaderArgs(class UniformTable& a, const String& prefix = "") const;

    virtual bool coverageLessThanEqual(const float alphaThreshold, const Point2& texCoord) const override;
    virtual bool hasPartialCoverage() const override;

    /** 
     To be identical, two materials must not only have the same images in their
     textures but must share pointers to the same underlying G3D::Texture objects.
     */
    bool operator==(const UniversalMaterial& other) const {
        return 
            (this == &other) ||
            ((m_bsdf == other.m_bsdf) &&
             (m_emissive == other.m_emissive) &&
             (m_bump == other.m_bump) &&
             (m_numLightMapDirections == other.m_numLightMapDirections) &&
             (m_lightMap[0] == other.m_lightMap[0]) &&
             (m_lightMap[1] == other.m_lightMap[1]) &&
             (m_lightMap[2] == other.m_lightMap[2]) &&
             (m_sampler == other.m_sampler) &&
             (m_flags == other.m_flags)
            );
    }

    /** Preferred type of refraction quality. The actual type available depends on the renderer.*/
    RefractionHint refractionHint() const {
        return m_refractionHint;
    }

    /** Preferred type of mirror reflection quality. The actual type available depends on the renderer.*/
    MirrorQuality mirrorHint() const {
        return m_mirrorHint;
    }

    /** Re-uses the existing \a surfel if it is a UniversalSurfel */
    virtual void sample(const Tri& tri, float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backside, shared_ptr<Surfel>& surfel, float du = 0, float dv = 0, bool twoSided = true) const override;
};

}

template <>
struct HashTrait<G3D::UniversalMaterial::Specification> {
    static size_t hashCode(const G3D::UniversalMaterial::Specification& key) {
        return key.hashCode();
    }
};


template <>
struct HashTrait<shared_ptr<G3D::UniversalMaterial> > {
    static size_t hashCode(const shared_ptr<G3D::UniversalMaterial>& key) {
        return reinterpret_cast<size_t>(key.get());
    }
};
