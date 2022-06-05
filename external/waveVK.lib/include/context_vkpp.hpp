/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace nvvkpp {
/**
To run a Vulkan application, you need to create the Vulkan instance and device.
This is done using the `nvvk::Context`, which wraps the creation of `VkInstance`
and `VkDevice`.

First, any application needs to specify how instance and device should be created:
Version, layers, instance and device extensions influence the features available.
This is done through a temporary and intermediate class that will allow you to gather
all the required conditions for the device creation.
*/

//////////////////////////////////////////////////////////////////////////
/**
# struct ContextCreateInfo

This structure allows the application to specify a set of features
that are expected for the creation of
- VkInstance
- VkDevice

It is consumed by the `nvvk::Context::init` function.

Example on how to populate information in it : 

~~~~ C++
    nvvkpp::ContextCreateInfo ctxInfo;
    ctxInfo.setVersion(1, 1);
    ctxInfo.addInstanceLayer("VK_LAYER_KHRONOS_validation");
    ctxInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor", true);
    ctxInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, false);
    ctxInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, false);
    ctxInfo.addInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    ctxInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, false);
~~~~

then you are ready to create initialize `nvvk::Context`

> Note: In debug builds, the extension `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` and the layer `VK_LAYER_KHRONOS_validation` are added to help finding issues early.


*/
struct ContextCreateInfo
{
  ContextCreateInfo(bool bUseValidation = true);

  void setVersion(int major, int minor);

  void addInstanceExtension(const char* name, bool optional = false);
  void addInstanceLayer(const char* name, bool optional = false);
  void addDeviceExtension(const char* name, bool optional = false, void* pFeatureStruct = nullptr);

  void removeInstanceExtension(const char* name);
  void removeInstanceLayer(const char* name);
  void removeDeviceExtension(const char* name);


  // Configure additional device creation with these variables and functions

  // use device groups
  bool useDeviceGroups = false;

  // which compatible device or device group to pick
  // only used by All-in-one Context::init(...)
  uint32_t compatibleDeviceIndex = 0;

  // instance properties
  const char* appEngine = "nvpro-sample";
  const char* appTitle  = "nvpro-sample";

  // may impact performance hence disable by default
  bool disableRobustBufferAccess = true;

  // Information printed at Context::init time
  bool verboseCompatibleDevices = true;
  bool verboseUsed              = true;  // Print what is used
  bool verboseAvailable         =        // Print what is available
#ifdef _DEBUG
      true;
#else
      false;
#endif

  struct Entry
  {
    Entry(const char* entryName, bool isOptional = false, void* pointerFeatureStruct = nullptr)
        : name(entryName)
        , optional(isOptional)
        , pFeatureStruct(pointerFeatureStruct)
    {
    }
    const char* name{nullptr};
    bool        optional{false};
    void*       pFeatureStruct{nullptr};
  };

  int apiMajor = 1;
  int apiMinor = 1;

  using EntryArray = std::vector<Entry>;
  EntryArray instanceLayers;
  EntryArray instanceExtensions;
  EntryArray deviceExtensions;
};

//////////////////////////////////////////////////////////////////////////
/**
# class nvvk::Context

Context class helps creating the Vulkan instance and to choose the logical device for the mandatory extensions. First is to fill the `ContextCreateInfo` structure, then call:

~~~ C++
  // Creating the Vulkan instance and device
  nvvkpp::ContextCreateInfo ctxInfo;
  ... see above ...

  nvvkpp::Context vkctx;
  vkctx.init(ctxInfo);

  // after init the ctxInfo is no longer needed
~~~ 

At this point, the class will have created the `VkInstance` and `VkDevice` according to the information passed. It will also keeps track or have query the information of:
 
* Physical Device information that you can later query : `PhysicalDeviceInfo` in which lots of `VkPhysicalDevice...` are stored
* `vk::Instance` : the one instance being used for the program
* `vk::PhysicalDevice` : physical device(s) used for the logical device creation. In case of more than one physical device, we have a std::vector for this purpose...
* `vk::Device` : the logical device instantiated
* `vk::Queue` : we will enumerate all the available queues and make them available in `nvvk::Context`. Some queues are specialized, while other are for general purpose (most of the time, only one can handle everything, while other queues are more specialized). We decided to make them all available in some explicit way :
 * `Queue m_queueGCT` : Graphics/Compute/Transfer Queue + family index
 * `Queue m_queueT` : async Transfer Queue + family index
 * `Queue m_queueC` : Compute Queue + family index
* maintains what extensions are finally available
* implicitly hooks up the debug callback

## Choosing the device
When there are multiple devices, the `init` method is choosing the first compatible device available, but it is also possible the choose another one.
~~~ C++
  vkctx.initInstance(deviceInfo); 
  // Find all compatible devices
  auto compatibleDevices = vkctx.getCompatibleDevices(deviceInfo);
  assert(!compatibleDevices.empty());

  // Use first compatible device
  vkctx.initDevice(compatibleDevices[0], deviceInfo);
~~~

## Multi-GPU

When multiple graphic cards should be used as a single device, the `ContextCreateInfo::useDeviceGroups` need to be set to `true`.
The above methods will transparently create the `vk::Device` using `vk::DeviceGroupDeviceCreateInfo`.
Especially in the context of NVLink connected cards this is useful.


*/
class Context
{
public:
  Context() = default;

  struct Queue
  {
    vk::Queue queue;
    uint32_t  familyIndex = ~0;

    operator vk::Queue() const { return queue; }
    operator uint32_t() const { return familyIndex; }
  };


  vk::Instance       m_instance;
  vk::Device         m_device;
  vk::PhysicalDevice m_physicalDevice;

  Queue m_queueGCT;  // for Graphics/Compute/Transfer (must exist)
  Queue m_queueT;  // for pure async Transfer Queue (can exist, only contains transfer nothing else)
  Queue m_queueC;  // for async Compute (can exist, may contain other non-graphics support)

  operator vk::Device() const { return m_device; }

  // All-in-one instance and device creation
  bool init(const ContextCreateInfo& contextInfo);
  void deinit();

  // Individual object creation
  bool initInstance(const ContextCreateInfo& info);

  void printLayersExtensionsUsed();

  void printAllExtensions();

  void printAllLayers();

  // deviceIndex is an index either into getPhysicalDevices or getPhysicalDeviceGroups
  // depending on info.useDeviceGroups
  bool initDevice(uint32_t deviceIndex, const ContextCreateInfo& info);

  // Helpers
  std::vector<int> getCompatibleDevices(const ContextCreateInfo& info);
  bool hasMandatoryExtensions(vk::PhysicalDevice physicalDevice, const ContextCreateInfo& info);

  // Ensures the GCT queue can present to the provided surface (return false if fails to set)
  bool setGCTQueueWithPresent(vk::SurfaceKHR surface);

  // true if the context has the optional extension activated
  bool hasDeviceExtension(const char* name) const;

private:
  // New Debug system
  PFN_vkCreateDebugUtilsMessengerEXT  m_createDebugUtilsMessengerEXT  = nullptr;
  PFN_vkDestroyDebugUtilsMessengerEXT m_destroyDebugUtilsMessengerEXT = nullptr;
  VkDebugUtilsMessengerEXT            m_dbgMessenger                  = nullptr;
  void                                initDebugReport();

  VkResult fillFilteredNameArray(std::vector<const char*>&               used,
                                 const std::vector<vk::LayerProperties>& properties,
                                 const ContextCreateInfo::EntryArray&    requested);
  VkResult fillFilteredNameArray(std::vector<const char*>&                   used,
                                 const std::vector<vk::ExtensionProperties>& properties,
                                 const ContextCreateInfo::EntryArray&        requested,
                                 std::vector<void*>&                         featureStructs);
  bool     checkEntryArray(const std::vector<vk::ExtensionProperties>& properties,
                           const ContextCreateInfo::EntryArray&        requested);


  std::vector<const char*> m_usedDeviceExtensions;
  std::vector<const char*> m_usedInstanceLayers;
  std::vector<const char*> m_usedInstanceExtensions;


  //  vk::DispatchLoaderDynamic m_dispatcher;
};


}  // namespace nvvkpp

//#endif
