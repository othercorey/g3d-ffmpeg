/**
  \file G3D-gfx.lib/source/GLSamplerObject.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-gfx/GLSamplerObject.h"
#include "G3D-gfx/GLCaps.h"
#include "G3D-gfx/glcalls.h"
#include "G3D-gfx/Texture.h"

namespace G3D {

static Array<GLuint> s_freeList;

WeakCache<Sampler, shared_ptr<GLSamplerObject> > GLSamplerObject::s_cache;

void GLSamplerObject::clearCache() {
    s_cache.clear();
}


shared_ptr<GLSamplerObject> GLSamplerObject::create(const Sampler& s) {  
    shared_ptr<GLSamplerObject> cachedValue = s_cache[s];
    if (isNull(cachedValue)) {
        cachedValue = shared_ptr<GLSamplerObject>(new GLSamplerObject(s));
        s_cache.set(s, cachedValue);
    }
    return cachedValue;
}


void GLSamplerObject::setDepthSamplerParameters(GLuint samplerID, DepthReadMode depthReadMode) {
    if (GLCaps::supports_GL_ARB_shadow()) {
        if (depthReadMode == DepthReadMode::DEPTH_NORMAL) {
            glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        } else {
            glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_FUNC, 
                (depthReadMode == DepthReadMode::DEPTH_LEQUAL) ? GL_LEQUAL : GL_GEQUAL);
        }
    }
}


static GLenum toGLenum(WrapMode m) {
    switch (m) {
    case WrapMode::TILE:
        return GL_REPEAT;

    case WrapMode::CLAMP:  
        return GL_CLAMP_TO_EDGE;

    case WrapMode::ZERO:
        return GL_CLAMP_TO_BORDER;

    default:
        //debugAssertM(Texture::supportsWrapMode(m), "Unsupported wrap mode for Texture");
        return GL_NONE;
    }
}


void GLSamplerObject::setParameters(const Sampler& settings) {
    // All platforms now support sampler objects and it is no longer declared as an extension
    // debugAssertM( GLCaps::supports("GL_ARB_sampler_objects"), "OpenGL Sampler Objects not supported on this device");

    // Set the wrap and interpolate state
    GLenum sMode = toGLenum(settings.xWrapMode);
    GLenum tMode = toGLenum(settings.yWrapMode);
    
    glSamplerParameteri(m_glSamplerID, GL_TEXTURE_WRAP_S, sMode);
    glSamplerParameteri(m_glSamplerID, GL_TEXTURE_WRAP_T, tMode);
    glSamplerParameteri(m_glSamplerID, GL_TEXTURE_WRAP_R, tMode);    
        
    if (settings.interpolateMode.requiresMipMaps() &&
        (GLCaps::supports("GL_EXT_texture_lod") || 
         GLCaps::supports("GL_SGIS_texture_lod"))) {

        // I can find no documentation for GL_EXT_texture_lod even though many cards claim to support it - Morgan 6/30/06
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAX_LOD_SGIS, settings.maxMipMap);
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_LOD_SGIS, settings.minMipMap);
    }


    switch (settings.interpolateMode) {
    case InterpolateMode::TRILINEAR_MIPMAP:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;

    case InterpolateMode::BILINEAR_MIPMAP:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;

    case InterpolateMode::NEAREST_MIPMAP:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;

    case InterpolateMode::BILINEAR_NO_MIPMAP:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        break;

    case InterpolateMode::NEAREST_NO_MIPMAP:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;

    case InterpolateMode::NEAREST_MIPMAP_LINEAR:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        break;

    case InterpolateMode::LINEAR_MIPMAP_NEAREST:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;

    case InterpolateMode::NEAREST_MAGNIFICATION_TRILINEAR_MIPMAP_MINIFICATION:
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
        glSamplerParameteri(m_glSamplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;

    default:
        debugAssert(false);
    }
    
    
    static const bool anisotropic = GLCaps::supports("GL_EXT_texture_filter_anisotropic");

    if (anisotropic && settings.interpolateMode.requiresMipMaps()) {
        glSamplerParameterf(m_glSamplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, settings.maxAnisotropy);
    }

    if (settings.mipBias != 0.0f) {
        glSamplerParameterf(m_glSamplerID, GL_TEXTURE_LOD_BIAS, settings.mipBias);
    }
    
    GLSamplerObject::setDepthSamplerParameters(m_glSamplerID, settings.depthReadMode);
        
}


GLSamplerObject::GLSamplerObject(const Sampler& sampler) :
    m_sampler(sampler), m_glSamplerID(GL_NONE) {

    // Allocate the OpenGL Sampler Object
    if (s_freeList.size() == 0) {
        glGenSamplers(1, &s_freeList.next());
    }
    m_glSamplerID = s_freeList.pop();

    setParameters(sampler);
}


GLSamplerObject::~GLSamplerObject() {
    // Free the OpenGL Sampler Object
    s_freeList.push(m_glSamplerID);
    m_glSamplerID = GL_NONE;
}

}
