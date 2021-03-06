/**
  \file G3D-app.lib/source/LightingEnvironment.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#include "G3D-base/Any.h"
#include "G3D-base/FileSystem.h"
#include "G3D-app/LightingEnvironment.h"
#include "G3D-app/AmbientOcclusion.h"
#include "G3D-app/Light.h"
#include "G3D-gfx/Texture.h"
#include "G3D-gfx/Sampler.h"
#include "G3D-app/ShadowMap.h"
#include "G3D-gfx/Args.h"
#include "G3D-gfx/RenderDevice.h"
#include "G3D-app/DDGIVolume.h"

#ifndef _MSC_VER
   #define _timeb timeb
   #define _ftime ftime
#endif
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4305)
#endif

namespace G3D {

LightingEnvironment::LightingEnvironment() {
}


void LightingEnvironment::copyScreenSpaceBuffers(const shared_ptr<Framebuffer>& framebuffer, const Vector2int16 colorGuardBand, const Vector2int16 depthGuardBand) {

    if (isNull(m_copiedScreenColorTexture) || (framebuffer->texture(0)->vector2Bounds() != m_copiedScreenColorTexture->vector2Bounds())) {
        m_copiedScreenColorTexture = Texture::createEmpty("G3D::LightingEnvironment::m_copiedScreenColorTexture", framebuffer->texture(0)->width(), framebuffer->texture(0)->height(), framebuffer->texture(0)->format(), Texture::DIM_2D, false);
        // m_copiedScreenDepthTexture = Texture::createEmpty("G3D::LightingEnvironment::m_copiedScreenDepthTexture", depthSource->width(), depthSource->height(), depthSource->format(), Texture::DIM_2D, false);
    }

    m_copiedScreenDepthGuardBand = depthGuardBand;
    m_copiedScreenColorGuardBand = colorGuardBand;

    static shared_ptr<Framebuffer> copyFB = Framebuffer::create("LightingEnvironment::copyScreenSpaceBuffers");
    copyFB->set(Framebuffer::COLOR0, m_copiedScreenColorTexture);
    copyFB->set(Framebuffer::DEPTH, m_copiedScreenDepthTexture);
    
    const bool invertY              = false;
    const bool linearInterpolation  = false;
    const bool blitColor            = true;
    const bool blitStencil          = false;
    const bool blitDepth            = notNull(m_copiedScreenDepthTexture);

    framebuffer->blitTo(RenderDevice::current, copyFB, invertY, linearInterpolation, blitDepth, blitStencil, blitColor);

    // Intentionally leave the attachments bound on the copyFB...although that prevents them from being collected if there is never
    // another copy call, it will be faster in the common case of executing it once per frame.
    //
    // Also, invoking copyFB->clear() here erases the screen color texture to black (or prevents the blit from occuring---it is asyncrhonous, so I can't tell).
    // That shouldn't happen, but simply removing the call avoids the bug.
}


void LightingEnvironment::setToDemoLightingEnvironment() {
    *this = LightingEnvironment();

    lightArray.append(Light::directional("Sun",  Vector3( 1.0f,  2.0f,  1.0f), Color3::fromARGB(0xfcf6eb), true));
    lightArray.append(Light::directional("Fill", Vector3(-1.0f, -0.5f, -1.0f), Color3::fromARGB(0x1e324d), false));
    // Perform our own search first, since we have a better idea of where this directory might be
    // than the general System::findDataFile.  This speeds up loading of the starter app.
    String cubePath = "cubemap";
    if (! FileSystem::exists(cubePath)) {
        cubePath = "../data-files/cubemap";
        if (! FileSystem::exists(cubePath)) {
            cubePath = System::findDataFile("cubemap");
        }
    }

    environmentMapArray.clear();
    environmentMapWeightArray.clear();
    const bool generateMipMaps = true;

    Texture::Encoding e;
    e.format = TextureFormat::RGB8();
    environmentMapArray.append(Texture::fromFile(FilePath::concat(cubePath, "noonclouds/noonclouds_*.png"), 
                          e, Texture::DIM_CUBE_MAP, generateMipMaps));
}


int LightingEnvironment::numShadowCastingLights() const {
    int n = 0;
    for (int i = 0; i < lightArray.size(); ++i) {
        n += lightArray[i]->shadowsEnabled() ? 1 : 0;
    }
    return n;
}


void LightingEnvironment::getNonShadowCastingLights(Array<shared_ptr<Light> >& array) const {
    for (int i = 0; i < lightArray.size(); ++i) {
        if (lightArray[i]->shadowsEnabled()) {
            array.append(lightArray[i]);
        }
    }
}


void LightingEnvironment::getIndirectIlluminationProducingLights(Array< shared_ptr<Light> >& array) const {
    for (int i = 0; i < lightArray.size(); ++i) {
        if (lightArray[i]->producesIndirectIllumination()) {
            array.append(lightArray[i]);
        }
    }
}


void LightingEnvironment::removeShadowCastingLights() {
    for (int i = 0; i < lightArray.size(); ++i) {
        if (lightArray[i]->shadowsEnabled()) {
            lightArray.fastRemove(i);
            --i;
        }
    }
}


LightingEnvironment::LightingEnvironment(const Any& any) {
    *this = LightingEnvironment();
    m_any = any;
    any.verifyName("LightingEnvironment");

    AnyTableReader r(any);

    Any evt;
    if (r.getIfPresent("environmentMap", evt) || r.getIfPresent("environmentMapArray", evt)) {
        if ((evt.type() == Any::ARRAY) && (evt.name() == "")) {
            // Array of environment maps
            for (int i = 0; i < evt.length(); ++i) {
                Texture::Specification s(evt[i], true, Texture::DIM_CUBE_MAP);
                environmentMapArray.append(Texture::create(s));
            }
        } else {
            // Single environment map
            Texture::Specification s(evt, true, Texture::DIM_CUBE_MAP);
            environmentMapArray.append(Texture::create(s));
        }
    }

    r.getIfPresent("ambientOcclusionSettings", ambientOcclusionSettings);

    ddgiVolumeArray.fastClear();
    ddgiVolumeSpecificationArray.fastClear();
    // Irradiance volume
    if (any.containsKey("ddgiVolumeArray")) {
        Any ddgiVolumes = any["ddgiVolumeArray"];
        if (ddgiVolumes.size() > 0) {
            for (Any::AnyTable::Iterator it = ddgiVolumes.table().begin(); it.isValid(); ++it) {
                ddgiVolumeSpecificationArray.append(DDGIVolumeSpecification(it->value));
            }
        }
        r.setReadStatus("ddgiVolumeArray", true);
    }

    r.verifyDone();
}


Any LightingEnvironment::toAny() const {
    Any a = m_any;

    if (a.isNil()) {
        a = Any(Any::TABLE,  "LightingEnvironment");
    }

    // (Save environment maps and weights here)

    a["ambientOcclusionSettings"] = ambientOcclusionSettings;
    // TODO: ddgi serialize one or more ddgivolumes
    //a["ddgiVolumeSpecification"]  = ddgiVolumeSpecification;

    /*
    Any lights = Any(Any::TABLE);
    for (int i = 0; i < lightArray.size(); ++i) {
        const shared_ptr<Light>& L = lightArray[i];
        lights[L->name()] = L->toAny();
    }
    a["lights"] = lights;
    */

    return a;
}



void LightingEnvironment::setShaderArgs(UniformTable& args, const String& prefix) const {
    // Cache these strings to avoid invoking format() every frame
    static Array<String> SYMBOL_light, SYMBOL_environmentMap;
    
    // Direct lights
    int shaderLightIndex = 0;
    for (int L = 0; L < lightArray.size(); ++L) {
        const shared_ptr<Light> light = lightArray[L];

        if (light->enabled()) {
            if (shaderLightIndex >= SYMBOL_light.size()) {
                SYMBOL_light.resize(shaderLightIndex + 1);
                SYMBOL_light[shaderLightIndex] = format("light%d_", shaderLightIndex);
            }

            light->setShaderArgs(args, prefix + SYMBOL_light[shaderLightIndex]);
            ++shaderLightIndex;
        }
    }
    args.setMacro(prefix + "NUM_LIGHTS", shaderLightIndex);

    // Environment maps
    for (int e = 0; e < environmentMapArray.size(); ++e) {
        if (e >= SYMBOL_environmentMap.size()) {
            SYMBOL_environmentMap.resize(e + 1);
            SYMBOL_environmentMap[e] = format("environmentMap%d_", e);
        }

        const shared_ptr<Texture>& evt = environmentMapArray[e];
        const String& s = prefix + SYMBOL_environmentMap[e];
        debugAssert(notNull(evt));

        evt->setShaderArgs(args, s, Sampler::cubeMap());

        // The PI factor is built into G3D's definition of the environment maps
        args.setUniform(s + "readMultiplyFirst",  Color4(evt->encoding().readMultiplyFirst.rgb() * pif() * ((environmentMapWeightArray.size() > e) ? environmentMapWeightArray[e] : 1.0f), evt->encoding().readMultiplyFirst.a));
        debugAssertM(evt->encoding().readAddSecond == Color4::zero(), "LightingEnvironment requires that environment maps have no bias term.");

        args.setUniform(s + "glossyMIPConstant", float(log2(sqrt(3.0) * evt->width())));
    }
    args.setMacro(prefix + "NUM_ENVIRONMENT_MAPS", environmentMapArray.size());


    // Ambient occlusion
    if (notNull(ambientOcclusion) && ambientOcclusionSettings.enabled) {
        ambientOcclusion->setShaderArgs(args, prefix + "ambientOcclusion_");
    }

    // Diffuse GI
    args.setMacro("NUM_DDGIVOLUMES", ddgiVolumeArray.size());
    for (int i = 0; i < ddgiVolumeArray.size(); ++i) {
        ddgiVolumeArray[i]->setShaderArgs(args, format("ddgiVolumeArray[%d].", i));
    }

    if (notNull(uniformTable)) {
        args.append(*uniformTable, prefix);
    }

    // Bind dummy arguments for lights that do not have shadow maps.
    // We always burn two samplers on this, even when all lights have
    // shadow maps and it is unnecessary...but (a) that will compile out
    // because the uniforms won't be read, and (b) in that case, we have
    // a lot of available samplers anyway.
    args.setUniform("dummyLightSampler2DShadow", Texture::white(), Sampler::shadow());

    // To reduce the number of samplers consumed by the shaders, don't pass this one
    //    args.setUniform("dummyLightSampler2D", Texture::white(), Sampler::buffer());
}


} // G3D

#ifdef _MSC_VER
#   pragma warning(pop)
#endif
