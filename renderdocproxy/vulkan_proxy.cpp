#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../renderdoc/api/replay/renderdoc_names.h"
#include "../renderdoc/driver/vulkan/official/vulkan.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

static INIT_ONCE g_initOnce = INIT_ONCE_STATIC_INIT;
static HMODULE g_realVulkan = NULL;

template <typename T>
static T LoadRealProc(const char *name)
{
  if(g_realVulkan == NULL)
    return NULL;

  return (T)GetProcAddress(g_realVulkan, name);
}

static void BootstrapCore()
{
  wchar_t selfPath[MAX_PATH] = {};
  GetModuleFileNameW((HMODULE)&__ImageBase, selfPath, MAX_PATH - 1);

  wchar_t *slash = wcsrchr(selfPath, L'\\');

  if(slash == NULL)
    return;

  *slash = 0;

  wchar_t manifestPath[MAX_PATH] = {};
  wcscpy_s(manifestPath, selfPath);
  wcscat_s(manifestPath, L"\\");
  wcscat_s(manifestPath, RENDERDOC_PROXY_MANIFEST_NAME_W);

  wchar_t corePath[MAX_PATH] = {};

  FILE *manifest = NULL;
  _wfopen_s(&manifest, manifestPath, L"rt");

  if(manifest == NULL)
    return;

  char line[4096] = {};

  while(fgets(line, sizeof(line), manifest))
  {
    if(strncmp(line, "core_dll_path=", 14) == 0)
    {
      const char *hex = line + 14;
      size_t len = strlen(hex);

      while(len > 0 && (hex[len - 1] == '\n' || hex[len - 1] == '\r'))
        len--;

      if(len >= 2 && (len % 2) == 0)
      {
        int out = 0;
        char utf8[MAX_PATH * 4] = {};

        for(size_t i = 0; i < len && out < (int)ARRAYSIZE(utf8) - 1; i += 2)
        {
          char chars[3] = {hex[i + 0], hex[i + 1], 0};
          utf8[out++] = (char)strtoul(chars, NULL, 16);
        }

        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, corePath, MAX_PATH - 1);
      }

      break;
    }
  }

  fclose(manifest);

  if(corePath[0] == 0)
    return;

  HMODULE core = LoadLibraryW(corePath);

  if(core == NULL)
    return;

  typedef uint32_t(__cdecl *BootstrapFn)(const wchar_t *manifestPath);
  BootstrapFn bootstrap = (BootstrapFn)GetProcAddress(core, RENDERDOC_PROXY_BOOTSTRAP_NAME);

  if(bootstrap)
    bootstrap(manifestPath);
}

static BOOL CALLBACK InitProxy(PINIT_ONCE, PVOID, PVOID *)
{
  wchar_t systemDir[MAX_PATH] = {};
  GetSystemDirectoryW(systemDir, MAX_PATH - 1);

  wchar_t realVulkanPath[MAX_PATH] = {};
  wcscpy_s(realVulkanPath, systemDir);
  wcscat_s(realVulkanPath, L"\\");
  wcscat_s(realVulkanPath, RENDERDOC_VULKAN_PROXY_DLL_W);

  g_realVulkan = LoadLibraryW(realVulkanPath);

  if(g_realVulkan != NULL)
    BootstrapCore();

  return TRUE;
}

static void EnsureProxyInitialised()
{
  InitOnceExecuteOnce(&g_initOnce, InitProxy, NULL, NULL);
}

#define VK_FORWARD_RESULT(ret, fail, name, params, args) \
  extern "C" VKAPI_ATTR ret VKAPI_CALL name params       \
  {                                                      \
    EnsureProxyInitialised();                            \
    PFN_##name real = LoadRealProc<PFN_##name>(#name);   \
    if(real == NULL)                                     \
      return fail;                                       \
    return real args;                                    \
  }

#define VK_FORWARD_VOID(name, params, args)            \
  extern "C" VKAPI_ATTR void VKAPI_CALL name params    \
  {                                                    \
    EnsureProxyInitialised();                          \
    PFN_##name real = LoadRealProc<PFN_##name>(#name); \
    if(real == NULL)                                   \
      return;                                          \
    real args;                                         \
  }

VK_FORWARD_RESULT(PFN_vkVoidFunction, NULL, vkGetInstanceProcAddr,
                  (VkInstance instance, const char *pName), (instance, pName));
VK_FORWARD_RESULT(PFN_vkVoidFunction, NULL, vkGetDeviceProcAddr,
                  (VkDevice device, const char *pName), (device, pName));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkCreateInstance,
                  (const VkInstanceCreateInfo *pCreateInfo,
                   const VkAllocationCallbacks *pAllocator, VkInstance *pInstance),
                  (pCreateInfo, pAllocator, pInstance));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED,
                  vkEnumerateInstanceExtensionProperties,
                  (const char *pLayerName, uint32_t *pPropertyCount,
                   VkExtensionProperties *pProperties),
                  (pLayerName, pPropertyCount, pProperties));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkEnumerateInstanceLayerProperties,
                  (uint32_t *pPropertyCount, VkLayerProperties *pProperties),
                  (pPropertyCount, pProperties));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkEnumerateInstanceVersion,
                  (uint32_t *pApiVersion), (pApiVersion));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkCreateDevice,
                  (VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                   const VkAllocationCallbacks *pAllocator, VkDevice *pDevice),
                  (physicalDevice, pCreateInfo, pAllocator, pDevice));
VK_FORWARD_VOID(vkDestroyInstance,
                (VkInstance instance, const VkAllocationCallbacks *pAllocator),
                (instance, pAllocator));
VK_FORWARD_VOID(vkDestroyDevice, (VkDevice device, const VkAllocationCallbacks *pAllocator),
                (device, pAllocator));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkEnumeratePhysicalDevices,
                  (VkInstance instance, uint32_t *pPhysicalDeviceCount,
                   VkPhysicalDevice *pPhysicalDevices),
                  (instance, pPhysicalDeviceCount, pPhysicalDevices));
VK_FORWARD_VOID(vkGetPhysicalDeviceFeatures,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures),
                (physicalDevice, pFeatures));
VK_FORWARD_VOID(vkGetPhysicalDeviceFeatures2,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures),
                (physicalDevice, pFeatures));
VK_FORWARD_VOID(vkGetPhysicalDeviceProperties,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties),
                (physicalDevice, pProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceProperties2,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties),
                (physicalDevice, pProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceFormatProperties,
                (VkPhysicalDevice physicalDevice, VkFormat format,
                 VkFormatProperties *pFormatProperties),
                (physicalDevice, format, pFormatProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceFormatProperties2,
                (VkPhysicalDevice physicalDevice, VkFormat format,
                 VkFormatProperties2 *pFormatProperties),
                (physicalDevice, format, pFormatProperties));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkGetPhysicalDeviceImageFormatProperties,
                  (VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
                   VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
                   VkImageFormatProperties *pImageFormatProperties),
                  (physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED,
                  vkGetPhysicalDeviceImageFormatProperties2,
                  (VkPhysicalDevice physicalDevice,
                   const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
                   VkImageFormatProperties2 *pImageFormatProperties),
                  (physicalDevice, pImageFormatInfo, pImageFormatProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceQueueFamilyProperties,
                (VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
                 VkQueueFamilyProperties *pQueueFamilyProperties),
                (physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceQueueFamilyProperties2,
                (VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
                 VkQueueFamilyProperties2 *pQueueFamilyProperties),
                (physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceMemoryProperties,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties),
                (physicalDevice, pMemoryProperties));
VK_FORWARD_VOID(vkGetPhysicalDeviceMemoryProperties2,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties),
                (physicalDevice, pMemoryProperties));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED,
                  vkEnumerateDeviceExtensionProperties,
                  (VkPhysicalDevice physicalDevice, const char *pLayerName,
                   uint32_t *pPropertyCount, VkExtensionProperties *pProperties),
                  (physicalDevice, pLayerName, pPropertyCount, pProperties));
VK_FORWARD_VOID(vkGetDeviceQueue,
                (VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex,
                 VkQueue *pQueue),
                (device, queueFamilyIndex, queueIndex, pQueue));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkQueueSubmit,
                  (VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits,
                   VkFence fence),
                  (queue, submitCount, pSubmits, fence));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkQueuePresentKHR,
                  (VkQueue queue, const VkPresentInfoKHR *pPresentInfo),
                  (queue, pPresentInfo));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkCreateWin32SurfaceKHR,
                  (VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface),
                  (instance, pCreateInfo, pAllocator, pSurface));
VK_FORWARD_RESULT(VkBool32, VK_FALSE, vkGetPhysicalDeviceWin32PresentationSupportKHR,
                  (VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex),
                  (physicalDevice, queueFamilyIndex));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkGetPhysicalDeviceSurfaceSupportKHR,
                  (VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                   VkSurfaceKHR surface, VkBool32 *pSupported),
                  (physicalDevice, queueFamilyIndex, surface, pSupported));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED,
                  vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
                  (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                   VkSurfaceCapabilitiesKHR *pSurfaceCapabilities),
                  (physicalDevice, surface, pSurfaceCapabilities));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED,
                  vkGetPhysicalDeviceSurfaceCapabilities2KHR,
                  (VkPhysicalDevice physicalDevice,
                   const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo,
                   VkSurfaceCapabilities2KHR *pSurfaceCapabilities),
                  (physicalDevice, pSurfaceInfo, pSurfaceCapabilities));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkGetPhysicalDeviceSurfaceFormatsKHR,
                  (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                   uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats),
                  (physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkGetPhysicalDeviceSurfaceFormats2KHR,
                  (VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo,
                   uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats),
                  (physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED,
                  vkGetPhysicalDeviceSurfacePresentModesKHR,
                  (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                   uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes),
                  (physicalDevice, surface, pPresentModeCount, pPresentModes));
VK_FORWARD_VOID(vkDestroySurfaceKHR,
                (VkInstance instance, VkSurfaceKHR surface,
                 const VkAllocationCallbacks *pAllocator),
                (instance, surface, pAllocator));
VK_FORWARD_RESULT(VkResult, VK_ERROR_INITIALIZATION_FAILED, vkAcquireNextImageKHR,
                  (VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
                   VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex),
                  (device, swapchain, timeout, semaphore, fence, pImageIndex));

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
  if(reason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(module);

  return TRUE;
}
