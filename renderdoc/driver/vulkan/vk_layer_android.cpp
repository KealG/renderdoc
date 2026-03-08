/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>

// RenderDoc Includes

#include "vk_common.h"
#include "vk_core.h"
#include "vk_hookset_defs.h"
#include "vk_resources.h"

#undef VK_LAYER_EXPORT
#define VK_LAYER_EXPORT __attribute__((visibility("default")))

// The android loader has limitations at present that require the enumerate functions
// to be exported with the precise canonical names. We just forward them to the
// layer-named functions

#if DISABLED(RDOC_ANDROID)
#error "This file should only be compiled on android!"
#endif

extern "C" {

// these are in vk_layer.cpp
VK_LAYER_EXPORT VkResult VKAPI_CALL RENDERDOC_VULKAN_LAYER_FUNC(EnumerateDeviceLayerProperties)(
    VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties);

VK_LAYER_EXPORT VkResult VKAPI_CALL RENDERDOC_VULKAN_LAYER_FUNC(EnumerateDeviceExtensionProperties)(
    VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount,
    VkExtensionProperties *pProperties);

VK_LAYER_EXPORT VkResult VKAPI_CALL RENDERDOC_VULKAN_LAYER_FUNC(EnumerateInstanceExtensionProperties)(
    const VkEnumerateInstanceExtensionPropertiesChain *pChain, const char *pLayerName,
    uint32_t *pPropertyCount, VkExtensionProperties *pProperties);

VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                                                     uint32_t *pPropertyCount,
                                                                     VkLayerProperties *pProperties)
{
  return RENDERDOC_VULKAN_LAYER_FUNC(EnumerateDeviceLayerProperties)(physicalDevice, pPropertyCount,
                                                                  pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI_CALL
vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                     uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
  return RENDERDOC_VULKAN_LAYER_FUNC(EnumerateDeviceExtensionProperties)(physicalDevice, pLayerName,
                                                                      pPropertyCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
                                                                       VkLayerProperties *pProperties)
{
  // RENDERDOC_VULKAN_LAYER_FUNC(EnumerateDeviceLayerProperties) ignores the physicalDevice parameter
  // since the layer properties are static
  return RENDERDOC_VULKAN_LAYER_FUNC(EnumerateDeviceLayerProperties)(VK_NULL_HANDLE, pPropertyCount,
                                                                  pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
  return RENDERDOC_VULKAN_LAYER_FUNC(EnumerateInstanceExtensionProperties)(
      NULL, pLayerName, pPropertyCount, pProperties);
}
}
