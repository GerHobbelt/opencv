// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2018, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.

#include "../../precomp.hpp"
#include "../vulkan/vk_loader.hpp"
#include "common.hpp"
#include "context.hpp"

namespace cv { namespace dnn { namespace vkcom {

#ifdef HAVE_VULKAN

std::shared_ptr<Context> kCtx;
bool enableValidationLayers = false;
VkInstance kInstance;
VkPhysicalDevice kPhysicalDevice;
VkDevice kDevice;
VkQueue kQueue;
VkCommandPool kCmdPool;
VkDebugReportCallbackEXT kDebugReportCallback;
uint32_t kQueueFamilyIndex;
std::vector<const char *> kEnabledLayers;
std::map<std::string, std::vector<uint32_t>> kShaders;
cv::Mutex kContextMtx;

static uint32_t getComputeQueueFamilyIndex()
{
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice, &queueFamilyCount, NULL);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice,
                                             &queueFamilyCount,
                                             queueFamilies.data());

    uint32_t i = 0;
    for (; i < queueFamilies.size(); ++i)
    {
        VkQueueFamilyProperties props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            break;
        }
    }

    if (i == queueFamilies.size())
    {
        throw std::runtime_error("could not find a queue family that supports operations");
    }

    return i;
}

bool checkExtensionAvailability(const char *extension_name,
                                const std::vector<VkExtensionProperties> &available_extensions)
{
    for( size_t i = 0; i < available_extensions.size(); ++i )
    {
      if( strcmp( available_extensions[i].extensionName, extension_name ) == 0 )
      {
        return true;
      }
    }
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData)
{
        std::cout << "Debug Report: " << pLayerPrefix << ":" << pMessage << std::endl;
        return VK_FALSE;
}

// internally used
void createContext()
{
    cv::AutoLock lock(kContextMtx);
    if (!kCtx)
    {
        kCtx.reset(new Context());
    }
}

<<<<<<< HEAD
bool isAvailable()
{
    try
    {
        createContext();
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "Failed to init Vulkan environment. " << e.what());
        return false;
=======
        if (result != VK_SUCCESS)
        {
            CV_Error(cv::Error::StsError, "Vulkan: vkEnumerateInstanceLayerProperties failed!");
            return;
        }

        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerPropertyCount);
        result = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, instanceLayerProperties.data());

        if (result != VK_SUCCESS)
        {
            CV_Error(cv::Error::StsError, "Vulkan: vkEnumerateInstanceLayerProperties failed!");
            return;
        }

        for (uint32_t i = 0; i < instanceLayerPropertyCount; i++)
        {
            const VkLayerProperties& lp = instanceLayerProperties[i];
            CV_LOG_INFO(NULL, "instance layer "<< lp.layerName << lp.implementationVersion);

            if (strcmp(lp.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
            {
                kEnabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");
            }
            if (strcmp(lp.layerName, "VK_LAYER_LUNARG_parameter_validation") == 0)
            {
                kEnabledLayers.push_back("VK_LAYER_LUNARG_parameter_validation");
            }
            if (strcmp(lp.layerName, "VK_LAYER_KHRONOS_validation") == 0)
            {
                kEnabledLayers.push_back("VK_LAYER_KHRONOS_validation");
            }
        }
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    }

    return true;
}

Context::Context()
{
    if(!loadVulkanLibrary())
    {
        CV_Error(Error::StsError, "loadVulkanLibrary failed");
        return;
    }
    else if (!loadVulkanEntry())
    {
        CV_Error(Error::StsError, "loadVulkanEntry failed");
        return;
    }
    else if (!loadVulkanGlobalFunctions())
    {
        CV_Error(Error::StsError, "loadVulkanGlobalFunctions failed");
        return;
    }

    // create VkInstance, VkPhysicalDevice
    std::vector<const char *> enabledExtensions;
    if (enableValidationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);

        std::vector<VkLayerProperties> layerProperties(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

        bool foundLayer = false;
        for (VkLayerProperties prop : layerProperties)
        {
            if (strcmp("VK_LAYER_LUNARG_standard_validation", prop.layerName) == 0)
            {
                foundLayer = true;
                break;
            }
        }

        if (!foundLayer)
        {
            throw std::runtime_error("Layer VK_LAYER_LUNARG_standard_validation not supported\n");
        }
        kEnabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");

        uint32_t extensionCount;

        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, NULL);
        std::vector<VkExtensionProperties> extensionProperties(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

        bool foundExtension = false;
        for (VkExtensionProperties prop : extensionProperties)
        {
            if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0)
            {
                foundExtension = true;
                break;
            }
        }

        if (!foundExtension) {
            throw std::runtime_error("Extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not supported\n");
        }
        enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "VkCom Library";
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName = "vkcom";
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &applicationInfo;

    // Give our desired layers and extensions to vulkan.
    createInfo.enabledLayerCount = kEnabledLayers.size();
    createInfo.ppEnabledLayerNames = kEnabledLayers.data();
    createInfo.enabledExtensionCount = enabledExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, NULL, &kInstance));

    if (!loadVulkanFunctions(kInstance))
    {
        CV_Error(Error::StsError, "loadVulkanFunctions failed");
        return;
    }

    if (enableValidationLayers && vkCreateDebugReportCallbackEXT)
    {
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                           VK_DEBUG_REPORT_WARNING_BIT_EXT |
                           VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        createInfo.pfnCallback = &debugReportCallbackFn;

        // Create and register callback.
        VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(kInstance, &createInfo,
                                                       NULL, &kDebugReportCallback));
    }

    // find physical device
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(kInstance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
<<<<<<< HEAD
        throw std::runtime_error("could not find a device with vulkan support");
=======
        CV_Error(cv::Error::StsError, "Vulkan Backend: could not find a device with vulkan support!");
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(kInstance, &deviceCount, devices.data());

    for (VkPhysicalDevice device : devices)
    {
        if (true)
        {
            kPhysicalDevice = device;
            break;
        }
    }

    kQueueFamilyIndex = getComputeQueueFamilyIndex();

    // create device, queue, command pool
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = kQueueFamilyIndex;
    queueCreateInfo.queueCount = 1; // create one queue in this family. We don't need more.
    float queuePriorities = 1.0;  // we only have one queue, so this is not that important.
    queueCreateInfo.pQueuePriorities = &queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo = {};

    // Specify any desired device features here. We do not need any for this application, though.
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.enabledLayerCount = kEnabledLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = kEnabledLayers.data();
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VK_CHECK_RESULT(vkCreateDevice(kPhysicalDevice, &deviceCreateInfo, NULL, &kDevice));

    // Get a handle to the only member of the queue family.
    vkGetDeviceQueue(kDevice, kQueueFamilyIndex, 0, &kQueue);

<<<<<<< HEAD
    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // the queue family of this command pool. All command buffers allocated from this command pool,
    // must be submitted to queues of this family ONLY.
    commandPoolCreateInfo.queueFamilyIndex = kQueueFamilyIndex;
    VK_CHECK_RESULT(vkCreateCommandPool(kDevice, &commandPoolCreateInfo, NULL, &kCmdPool));
=======
    // Step4: Create CommandPool and PipelineFactory
    if (!cmdPoolPtr)
        cmdPoolPtr = CommandPool::create(kQueue, kQueueFamilyIndex);
    else
        CV_Error(cv::Error::StsError, "cmdPoolPtr has been created before!!");

    pipelineFactoryPtr = PipelineFactory::create();
}

GPUInfo Context::parseGPUInfo(VkPhysicalDevice& kPhysicalDevice)
{
    GPUInfo info;

    // device type
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(kPhysicalDevice, &physicalDeviceProperties);

    // GPU reference.
    // mali
    // t760 = 0x13b5 0x7500001 / 0x7501000
    // t860 = 0x13b5 0x8602000
    // t880 = 0x13b5 0x8800020
    // g31  = 0x13b5 0x70930000
    // g51  = 0x13b5 0x70901010
    // g52  = 0x13b5 0x74021000 / 0x72120000
    // g71  = 0x13b5 0x60a00002
    // g72  = 0x13b5 0x62210001
    // g76  = 0x13b5 0x72110000
    // g77  = 0x13b5 0x90800011

    // adreno
    // 506 = 0x5143 0x5000600
    // 510 = 0x5143 0x5010000
    // 512 = 0x5143 0x5010200
    // 530 = 0x5143 0x5030004
    // 540 = 0x5143 0x5040001
    // 616 = 0x5143 0x6010600
    // 630 = 0x5143 0x6030001
    // 640 = 0x5143 0x6040001
    // 650 = 0x5143 0x6050002

    CV_LOG_INFO(NULL, "Begin parse GPU Info "<<gpuInfoList.size()<<"...");
    CV_LOG_INFO(NULL, "API version is  "
        <<VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion)<<"."
        <<VK_VERSION_MINOR(physicalDeviceProperties.apiVersion)<<"."
        <<VK_VERSION_PATCH(physicalDeviceProperties.apiVersion)<<".");
    CV_LOG_INFO(NULL, "Driver version is  "
            <<VK_VERSION_MAJOR(physicalDeviceProperties.driverVersion)<<"."
            <<VK_VERSION_MINOR(physicalDeviceProperties.driverVersion)<<"."
            <<VK_VERSION_PATCH(physicalDeviceProperties.driverVersion)<<".");
    CV_LOG_INFO(NULL, "Vendor ID:"<<physicalDeviceProperties.vendorID<<".");
    CV_LOG_INFO(NULL, "Device ID:"<<physicalDeviceProperties.deviceID<<".");
    CV_LOG_INFO(NULL, "Device name:"<<physicalDeviceProperties.deviceName<<".");

    // info
    info.apiVersion = physicalDeviceProperties.apiVersion;
    info.driverVersion = physicalDeviceProperties.driverVersion;
    info.vendorId = physicalDeviceProperties.vendorID;
    info.deviceId = physicalDeviceProperties.deviceID;
    memcpy(info.deviceName, physicalDeviceProperties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    memcpy(info.pipelineCacheUUID, physicalDeviceProperties.pipelineCacheUUID, VK_UUID_SIZE);

    if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        info.type = GPU_TYPE_DISCRETE;
    else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        info.type = GPU_TYPE_INTEGRATED;
    else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        info.type = GPU_TYPE_VIRTUAL;
    else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        info.type = GPU_TYPE_CPU_ONLY;
    else
        info.type = GPU_TYPE_NOFOUND;

    // device capability
    info.maxSharedMemorySize = physicalDeviceProperties.limits.maxComputeSharedMemorySize;

    info.maxWorkgroupCount_x = physicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
    info.maxWorkgroupCount_y = physicalDeviceProperties.limits.maxComputeWorkGroupCount[1];
    info.maxWorkgroupCount_z = physicalDeviceProperties.limits.maxComputeWorkGroupCount[2];

    info.maxWorkgroup_invocations = physicalDeviceProperties.limits.maxComputeWorkGroupInvocations;

    info.maxWorkgroupSize_x = physicalDeviceProperties.limits.maxComputeWorkGroupSize[0];
    info.maxWorkgroupSize_y = physicalDeviceProperties.limits.maxComputeWorkGroupSize[1];
    info.maxWorkgroupSize_z = physicalDeviceProperties.limits.maxComputeWorkGroupSize[2];

    info.memoryMapAlignment = physicalDeviceProperties.limits.minMemoryMapAlignment;
    info.bufferOffsetAlignment = physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
    info.non_coherent_atom_size = physicalDeviceProperties.limits.nonCoherentAtomSize;
    info.bufferImageGranularity = physicalDeviceProperties.limits.bufferImageGranularity;
    info.maxImageDimension_1d = physicalDeviceProperties.limits.maxImageDimension1D;
    info.maxImageDimension_1d = physicalDeviceProperties.limits.maxImageDimension2D;
    info.maxImageDimension_1d = physicalDeviceProperties.limits.maxImageDimension3D;

    info.timestampPeriod = physicalDeviceProperties.limits.timestampPeriod;

    CV_LOG_INFO(NULL, "maxSharedMemorySize = "<<info.maxSharedMemorySize<<".");
    CV_LOG_INFO(NULL, "maxWorkgroupCount ( "
            <<info.maxWorkgroupSize_x<<", "<<info.maxWorkgroupSize_y<<", "<<info.maxWorkgroupSize_z<<").");
    CV_LOG_INFO(NULL, "maxWorkgroup_invocations = "<<info.maxWorkgroup_invocations<<".");
    CV_LOG_INFO(NULL, "maxWorkgroupSize ( "
            <<info.maxWorkgroupSize_x<<", "<<info.maxWorkgroupSize_y<<", "<<info.maxWorkgroupSize_z<<").");
    CV_LOG_INFO(NULL, "memoryMapAlignment = "<<info.memoryMapAlignment<<".");
    CV_LOG_INFO(NULL, "bufferOffsetAlignment = "<<info.bufferOffsetAlignment<<".");

    // find compute queue
    uint32_t queueFamilyPropertiesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice, &queueFamilyPropertiesCount, 0);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());

    info.computeQueueFamilyIndex = findDeviceComputeQueue(queueFamilyProperties);
    info.graphicsQueueFamilyIndex = findDeviceGraphicsQueue(queueFamilyProperties);
    info.transferQueueFamilyIndex = findDeviceTransferQueue(queueFamilyProperties);

    info.computeQueueCount = queueFamilyProperties[info.computeQueueFamilyIndex].queueCount;
    info.graphicsQueueCount = queueFamilyProperties[info.graphicsQueueFamilyIndex].queueCount;
    info.transferQueueCount = queueFamilyProperties[info.transferQueueFamilyIndex].queueCount;

    info.unifiedComputeTransferQueue = info.computeQueueFamilyIndex == info.transferQueueFamilyIndex;

    // additional device properties
    info.subgroupSize = 64;
    info.supportSubgroupBasic = false;
    info.supportSubgroupVote = false;
    info.supportSubgroupBallot = false;
    info.supportSubgroupShuffle = false;

    if (support_VK_KHR_get_physical_device_properties2)
    {
        void* queryDeviceProperties = 0;

        // query subgroup
        VkPhysicalDeviceSubgroupProperties physicalDeviceSubgroupProperties;
        physicalDeviceSubgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
        physicalDeviceSubgroupProperties.pNext = queryDeviceProperties;
        if (VK_VERSION_MAJOR(instanceApiVersion) >= 1 && VK_VERSION_MINOR(instanceApiVersion) >= 1)
        {
            queryDeviceProperties = &physicalDeviceSubgroupProperties;
        }

        VkPhysicalDeviceProperties2KHR queryProperties;
        queryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        queryProperties.pNext = queryDeviceProperties;
        vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(kInstance, "vkGetPhysicalDeviceProperties2KHR");
        vkGetPhysicalDeviceProperties2KHR(kPhysicalDevice, &queryProperties);

        if (VK_VERSION_MAJOR(instanceApiVersion) >= 1 && VK_VERSION_MINOR(instanceApiVersion) >= 1)
        {
            info.subgroupSize = physicalDeviceSubgroupProperties.subgroupSize;
            if (physicalDeviceSubgroupProperties.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT)
            {
                info.supportSubgroupBasic = physicalDeviceSubgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT;
                info.supportSubgroupVote = physicalDeviceSubgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT;
                info.supportSubgroupBallot = physicalDeviceSubgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT;
                info.supportSubgroupShuffle = physicalDeviceSubgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT;
            }
        }
        else
        {
            if (physicalDeviceProperties.vendorID == 0x5143) // qcom adreno prefer very large workgroup :P
                info.subgroupSize = 128;
            if (physicalDeviceProperties.vendorID == 0x13b5) // arm mali
                info.subgroupSize = 16;
            if (physicalDeviceProperties.vendorID == 0x1010) // imgtec powervr
                info.subgroupSize = 32;
            if (physicalDeviceProperties.vendorID == 0x1002) // amd
                info.subgroupSize = 64;
            if (physicalDeviceProperties.vendorID == 0x10de) // nvidia
                info.subgroupSize = 32;
            if (physicalDeviceProperties.vendorID == 0x8086) // intel
                info.subgroupSize = 32;
        }
    }

    // cache memory properties
    vkGetPhysicalDeviceMemoryProperties(kPhysicalDevice, &info.physicalDeviceMemoryProperties);

    // get device extension
    uint32_t deviceExtensionPropertyCount = 0;
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(kPhysicalDevice, NULL, &deviceExtensionPropertyCount, NULL));

    std::vector<VkExtensionProperties> deviceExtensionProperties(deviceExtensionPropertyCount);
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(kPhysicalDevice, NULL, &deviceExtensionPropertyCount, deviceExtensionProperties.data()));

    // extension capability
    info.support_VK_KHR_8bit_storage = 0;
    info.support_VK_KHR_16bit_storage = 0;
    info.support_VK_KHR_bind_memory2 = 0;
    info.support_VK_KHR_create_renderpass2 = 0;
    info.support_VK_KHR_dedicated_allocation = 0;
    info.support_VK_KHR_descriptor_update_template = 0;
    info.support_VK_KHR_external_memory = 0;
    info.support_VK_KHR_get_memory_requirements2 = 0;
    info.support_VK_KHR_maintenance1 = 0;
    info.support_VK_KHR_maintenance2 = 0;
    info.support_VK_KHR_maintenance3 = 0;
    info.support_VK_KHR_multiview = 0;
    info.support_VK_KHR_portability_subset = 0;
    info.support_VK_KHR_push_descriptor = 0;
    info.support_VK_KHR_sampler_ycbcr_conversion = 0;
    info.support_VK_KHR_shader_float16_int8 = 0;
    info.support_VK_KHR_shader_float_controls = 0;
    info.support_VK_KHR_storage_buffer_storage_class = 0;
    info.support_VK_KHR_swapchain = 0;
    info.support_VK_EXT_descriptor_indexing = 0;
    info.support_VK_EXT_memory_budget = 0;
    info.support_VK_EXT_queue_family_foreign = 0;
#if defined(__ANDROID_API__) && __ANDROID_API__ >= 26
    gpu_info.support_VK_ANDROID_external_memory_android_hardware_buffer = 0;
#endif // __ANDROID_API__ >= 26
    info.support_VK_NV_cooperative_matrix = 0;
    for (uint32_t j = 0; j < deviceExtensionPropertyCount; j++)
    {
        const VkExtensionProperties& exp = deviceExtensionProperties[j];

        if (strcmp(exp.extensionName, "VK_KHR_8bit_storage") == 0)
            info.support_VK_KHR_8bit_storage = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_16bit_storage") == 0)
            info.support_VK_KHR_16bit_storage = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_bind_memory2") == 0)
            info.support_VK_KHR_bind_memory2 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_create_renderpass2") == 0)
            info.support_VK_KHR_create_renderpass2 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_dedicated_allocation") == 0)
            info.support_VK_KHR_dedicated_allocation = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_descriptor_update_template") == 0)
            info.support_VK_KHR_descriptor_update_template = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_external_memory") == 0)
            info.support_VK_KHR_external_memory = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_get_memory_requirements2") == 0)
            info.support_VK_KHR_get_memory_requirements2 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_maintenance1") == 0)
            info.support_VK_KHR_maintenance1 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_maintenance2") == 0)
            info.support_VK_KHR_maintenance2 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_maintenance3") == 0)
            info.support_VK_KHR_maintenance3 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_multiview") == 0)
            info.support_VK_KHR_multiview = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_portability_subset") == 0)
            info.support_VK_KHR_portability_subset = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_push_descriptor") == 0)
            info.support_VK_KHR_push_descriptor = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_sampler_ycbcr_conversion") == 0)
            info.support_VK_KHR_sampler_ycbcr_conversion = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_shader_float16_int8") == 0)
            info.support_VK_KHR_shader_float16_int8 = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_shader_float_controls") == 0)
            info.support_VK_KHR_shader_float_controls = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_storage_buffer_storage_class") == 0)
            info.support_VK_KHR_storage_buffer_storage_class = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_KHR_swapchain") == 0)
            info.support_VK_KHR_swapchain = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_EXT_descriptor_indexing") == 0)
            info.support_VK_EXT_descriptor_indexing = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_EXT_memory_budget") == 0)
            info.support_VK_EXT_memory_budget = exp.specVersion;
        else if (strcmp(exp.extensionName, "VK_EXT_queue_family_foreign") == 0)
            info.support_VK_EXT_queue_family_foreign = exp.specVersion;
#if defined(__ANDROID_API__) && __ANDROID_API__ >= 26
        else if (strcmp(exp.extensionName, "VK_ANDROID_external_memory_android_hardware_buffer") == 0)
            info.support_VK_ANDROID_external_memory_android_hardware_buffer = exp.specVersion;
#endif // __ANDROID_API__ >= 26
        else if (strcmp(exp.extensionName, "VK_NV_cooperative_matrix") == 0)
            info.support_VK_NV_cooperative_matrix = exp.specVersion;
    }

    // check features
    info.support_fp16_packed = true;
    info.support_fp16_storage = false;
    info.support_fp16_arithmetic = false;
    info.support_int8_packed = true;
    info.support_int8_storage = false;
    info.support_int8_arithmetic = false;

    if (support_VK_KHR_get_physical_device_properties2)
    {
        void* queryExtensionFeatures = 0;

        VkPhysicalDevice8BitStorageFeaturesKHR query8BitStorageFeatures;
        query8BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR;
        query8BitStorageFeatures.pNext = 0;
        if (info.support_VK_KHR_8bit_storage)
        {
            query8BitStorageFeatures.pNext = queryExtensionFeatures;
            queryExtensionFeatures = &query8BitStorageFeatures;
        }

        // query fp16/int16 storage
        VkPhysicalDevice16BitStorageFeaturesKHR query16BitStorageFeatures;
        query16BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR;
        query16BitStorageFeatures.pNext = 0;
        if (info.support_VK_KHR_16bit_storage)
        {
            query16BitStorageFeatures.pNext = queryExtensionFeatures;
            queryExtensionFeatures = &query16BitStorageFeatures;
        }

        // query fp16/int8 arithmetic
        VkPhysicalDeviceFloat16Int8FeaturesKHR queryFloat16Int8Features;
        queryFloat16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
        queryFloat16Int8Features.pNext = 0;
        if (info.support_VK_KHR_shader_float16_int8)
        {
            queryFloat16Int8Features.pNext = queryExtensionFeatures;
            queryExtensionFeatures = &queryFloat16Int8Features;
        }

        VkPhysicalDeviceFeatures2KHR queryFeatures;
        queryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        queryFeatures.pNext = queryExtensionFeatures;

        vkGetPhysicalDeviceFeatures2KHR(kPhysicalDevice, &queryFeatures);

        if (info.support_VK_KHR_8bit_storage)
        {
            info.support_int8_storage = query8BitStorageFeatures.storageBuffer8BitAccess;
        }
        if (info.support_VK_KHR_16bit_storage && queryFeatures.features.shaderStorageImageExtendedFormats)
        {
            // shaderStorageImageExtendedFormats enables r16f format in storage image
            info.support_fp16_storage = query16BitStorageFeatures.storageBuffer16BitAccess;
        }
        if (info.support_VK_KHR_shader_float16_int8)
        {
            info.support_fp16_arithmetic = queryFloat16Int8Features.shaderFloat16;
            info.support_int8_arithmetic = queryFloat16Int8Features.shaderInt8;
        }
    }

    if (physicalDeviceProperties.vendorID == 0x13b5 && physicalDeviceProperties.apiVersion < VK_MAKE_VERSION(1, 0, 82))
    {
        // the 16bit_storage implementation of arm mali driver is buggy :[
        info.support_fp16_storage = false;
    }

    if (physicalDeviceProperties.vendorID == 0x10002 && physicalDeviceProperties.deviceID == 0x70006214 && physicalDeviceProperties.apiVersion == VK_MAKE_VERSION(1, 1, 82))
    {
        // the 16bit_storage implementation of vivante gc1700 driver is buggy :[
        info.support_fp16_storage = false;
    }
    CV_LOG_INFO(NULL, "GPU id "<<gpuInfoList.size()<<", name = "<<physicalDeviceProperties.deviceName<<
        ", queueCompute = ["<<info.computeQueueFamilyIndex<<","<<info.computeQueueCount<<"]"<<
        ", queueGraphics = ["<<info.graphicsQueueFamilyIndex<<","<<info.graphicsQueueCount<<"]"<<
        ", queueTransfer = ["<<info.transferQueueFamilyIndex<<","<<info.transferQueueCount<<"].");

    CV_LOG_INFO(NULL, "fp16-packed/storage/arithmetic = "<<info.support_fp16_packed<<"/"
        <<info.support_VK_KHR_16bit_storage<<"/"<<info.support_fp16_arithmetic<<".");
    CV_LOG_INFO(NULL, "int8-packed/storage/arithmetic = "<<info.support_int8_packed<<"/"
        <<info.support_int8_storage<<"/"<<info.support_int8_arithmetic<<".");

    CV_LOG_INFO(NULL, "subgroup = "<<info.subgroupSize<<", basic = "
        <<info.supportSubgroupBasic<<", vote = "<<info.support_int8_arithmetic<<".");

    return info;
}

int Context::findBestPhysicalGPUIndex()
{
    // first try, discrete gpu
    for (int i = 0; i < gpuInfoList.size(); i++)
    {
        if (gpuInfoList[i].type == GPU_TYPE_DISCRETE)
            return i;
    }

    // second try, integrated gpu
    for (int i = 0; i < gpuInfoList.size(); i++)
    {
        if (gpuInfoList[i].type == GPU_TYPE_INTEGRATED)
            return i;
    }

    // third try, any probed device
    if (gpuInfoList.size() > 0)
        return 0;

    CV_LOG_ERROR(NULL, "no vulkan device");
    return -1;
}

void Context::reset()
{
    cmdPoolPtr->reset();
    pipelineFactoryPtr->reset();
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
}

Context::~Context()
{
    vkDestroyCommandPool(kDevice, kCmdPool, NULL);
    vkDestroyDevice(kDevice, NULL);

    if (enableValidationLayers) {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT)
            vkGetInstanceProcAddr(kInstance, "vkDestroyDebugReportCallbackEXT");
        if (func == nullptr)
        {
            CV_LOG_FATAL(NULL, "Could not load vkDestroyDebugReportCallbackEXT");
        }
        else
        {
            func(kInstance, kDebugReportCallback, NULL);
        }
    }
    kShaders.clear();
    vkDestroyInstance(kInstance, NULL);

    return;
}

#endif // HAVE_VULKAN

}}} // namespace cv::dnn::vkcom
