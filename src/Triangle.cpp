#include "pch.h"

// Queue Families are sets of queues, which each support a specific set of functions, checked by control bits (e.g. line 25)
/*
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    // Check whether graphics family has already been found yet
    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);   // How many queue families are there

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, &queueFamilies[0]);             // What are those families

    // find at least one queue family supporting graphics
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & requiredQueueFlags)
            indices.graphicsFamily = i;                                                                 // set the graphics family to the index, so we know it's been found
        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}
*/

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    // ------------------------- Setup Section ------------------------- //
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    std::vector<uint32_t> queueFamilyIndices;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // hint to not use OpenGL (as this is the standard choice)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // hint to make the window non-resizeable

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        //setupDebugMessanger(); // this is for custom control over the debug output from validation layers
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        for (auto imageView : swapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr); // clear the images of the swap chain


        vkDestroySwapchainKHR(device, swapChain, nullptr);  // clear the swap chain

        vkDestroyDevice(device, nullptr);                   // clear the logical device

        vkDestroySurfaceKHR(instance, surface, nullptr);    // clear the surface (must be before clearing instance)

        vkDestroyInstance(instance, nullptr);               // clear the instance

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        // This is mostly metadata for the application
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;         // struct type
        appInfo.pApplicationName = "Hello Triangle";                // Name
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);      // My applications version
        appInfo.pEngineName = "No Engine";                          // Engine (running the application, aka me)
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);           // Engine's version
        appInfo.apiVersion = VK_API_VERSION_1_3;                    // The version of Vulkan being used

        // create Info
        VkInstanceCreateInfo createInfo{};

        // Validation layers (used primarily for development/debugging of Vulkan errors)
        /*
         * const char* validationLayers[] = {
            "VK_LAYER_KHRONOS_validation"
        };  // this is a less useful implementation

        createInfo.enabledLayerCount = (uint32_t)sizeof(validationLayers) / sizeof(char*);  // # Layers in (char*) validationLayers[]
        createInfo.ppEnabledLayerNames = validationLayers.data();                           // passing (char*) validationLayers to createInfo
        */

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());  // # of validation layers
            createInfo.ppEnabledLayerNames = validationLayers.data();                       // passing validation layers to createInfo
        } else {
            createInfo.enabledLayerCount = 0;
        }

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;                          // struct type
        createInfo.pApplicationInfo = &appInfo;                                             // reference appInfo defined above

        // Extensions (not relevant atm)
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        // createInfo.enabledLayerCount = 0;

        // We've defined everything Vulkan needs to create an instance
        // VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

        // Check if instance was created and stored successfully
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    bool checkValidationLayerSupport() { // pretty much checks: "Can I enable everything I want?"
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);                   // How many Layers can we find?

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());    // What are those layers we found?

        // Compare the layers we found against validationLayers
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    void pickPhysicalDevice() {
        physicalDevice = VK_NULL_HANDLE;

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);        // How many options are there?

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); // What are the available devices?

        // Check for suitable devices
        /*for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }*/

        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<uint32_t, VkPhysicalDevice> candidates;

        for (const auto& device : devices) {
            uint32_t score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }

        // Check for best valid candidate
        if (candidates.rbegin()->first > 0)
            physicalDevice = candidates.rbegin()->second;
        else
            throw std::runtime_error("failed to find a suitable GPU!");
    }

    void createLogicalDevice() {
        // alternative approach just choosing the first family found with correct flags
        /*
        std::optional<uint32_t> familyIndex = findQueueFamily(physicalDevice, requiredQueueFlags);  // find the first queueFamily with required flags

        if (!familyIndex)
            throw std::runtime_error("No compatible queue family found!");
        */

        std::vector<uint32_t> indices = findQueueFamilies(physicalDevice, requiredQueueFlags);
        if (indices.size() <= 0)
            throw std::runtime_error("No compatible queue family found!");

        // Check if any of the families with support also support present
        VkBool32 presentSupport = false;
        for (uint32_t candidate : indices) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, candidate, surface, &presentSupport);
            if (presentSupport) {
                queueFamilyIndices.push_back(candidate);
                break;
            }
        }

        // If no ideal family was found, find another family with present support
        if (!presentSupport) {
            queueFamilyIndices.push_back(indices[0]);
            indices = findQueueFamilies(physicalDevice, 0);
            for (uint32_t candidate : indices) {
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, candidate, surface, &presentSupport);
                if (presentSupport) {
                    queueFamilyIndices.push_back(candidate);
                    break;
                }
            }

            if (!presentSupport)
                throw std::runtime_error("failed to find a supporting device!");
        }


        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f; // (range from 0.0 - 1.0)

        for (uint32_t family : queueFamilyIndices) {
            VkDeviceQueueCreateInfo queueCreateInfo{};                                      // Creating the queues for the queueFamily
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            //queueCreateInfo.queueFamilyIndex = *familyIndex;
            queueCreateInfo.queueFamilyIndex = family;
            queueCreateInfo.queueCount = 1;                                                 // How many Queues should the family have
            queueCreateInfo.pQueuePriorities = &queuePriority;                              // The priority of the queue
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};                                          // Features that should be enabled
        deviceFeatures.geometryShader = true;

        VkDeviceCreateInfo createInfo{};                                                    // Creating the actual device
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());   // How many VkDeviceQueueCreateInfo's are being provided

        createInfo.pEnabledFeatures = &deviceFeatures;                                      // Which features should the logical device enable

        // Rest is similar to VkInstanceCreateInfo
        /*
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }
        */
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());  // How many extensions do we want to enable
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();                       // Extensions to enable

        createInfo.enabledLayerCount = 0;                                                   // Disable validation layers for logic device


        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // Assign the graphicsQueue to the Queue (This must have present support)
        //vkGetDeviceQueue(device, *familyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices[0], 0, &graphicsQueue);

        if (queueFamilyIndices.size() > 1)
            vkGetDeviceQueue(device, queueFamilyIndices[1], 0, &presentQueue);
        else
            vkGetDeviceQueue(device, queueFamilyIndices[0], 0, &presentQueue);  // same family, same queue
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

        vkGetPhysicalDeviceProperties(device, &deviceProperties);   // Get the properties of the device
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);       // Additional features supported by the device

        return (
            devicePropertySupport(deviceProperties) &&          // check if device supports required properties
            deviceFeatureSupport(deviceFeatures)    &&          // check if device supports required features
            deviceFamilySupport(device)             &&          // check if device has a matching Family
            // check if device supports required extensions
            deviceExensionSupport(device, std::set<std::string>(deviceExtensions.begin(), deviceExtensions.end())) &&
            deviceSwapChainSupport(device)                      // check if device has suitable swap chain
            );
    }

    // These three are "semi"-hardcoded methods, so I can easily have a validity check for device anywhere without redundancy
    // Check for all properties required from the device
    bool devicePropertySupport(VkPhysicalDeviceProperties deviceProperties) {
        return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    }

    // Check all features required from the device
    bool deviceFeatureSupport(VkPhysicalDeviceFeatures deviceFeatures) {
        return (deviceFeatures.geometryShader);
    }

    // Check all families required from the device
    bool deviceFamilySupport(VkPhysicalDevice device) {
        return findQueueFamily(device, requiredQueueFlags).has_value();
    }

    // Check all extensions required from the device
    bool deviceExensionSupport(VkPhysicalDevice device, std::set<std::string> requiredExtensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);                    // How many extensions are there

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()); // What extensions are available to us

        //std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());                            // Copy the required extensions

        // Remove each extension found in available extensions from required extensions and check if requiredExtensions is empty ==> all required extensions are satisfied
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    // Check if required swap chain details are supported by the device
    bool deviceSwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty(); // return true if there are formats and present modes
    }
    // end of "semi"-hardcoded methods


    std::vector<uint32_t> findQueueFamilies(VkPhysicalDevice device, VkQueueFlags requiredFlags) {
        // Output ordered by score: each additional, non-required flag -> + 1 score
        std::multimap<uint32_t, uint32_t> score_index;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);               // How many queue families are there

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());  // What are those families
        // (use queueFamilies.data() instead of &queueFamilies[0] as they produce identical results, but &queueFamilies[0] breaks when vector is empty) //

        // Find all families supporting the required flags and add their indices to the output
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            VkQueueFlags flags = queueFamilies[i].queueFlags;
            if ((flags & requiredFlags) == requiredFlags) {
                score_index.insert(std::make_pair(std::popcount(flags & ~requiredFlags), i));
            }
        }

        std::vector<uint32_t> indices;
        indices.reserve(score_index.size());
        for (const auto& [score, idx] : score_index) {
            indices.push_back(idx);
        }
        return indices;
    }
    std::optional<uint32_t> findQueueFamily(VkPhysicalDevice device, VkQueueFlags requiredFlags) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);               // How many queue families are there

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());  // What are those families
        // (use queueFamilies.data() instead of &queueFamilies[0] as they produce identical results, but &queueFamilies[0] breaks when vector is empty) //

        // Find all families supporting the required flags and add their indices to the output
        for (uint32_t i = 0; i < queueFamilyCount; i++)
            if ((queueFamilies[i].queueFlags & requiredFlags) == requiredFlags)
                return i;

        return std::nullopt;
    }

    // alternative to just checking if a device is suitable, but finding the best candidate
    uint32_t rateDeviceSuitability(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

        vkGetPhysicalDeviceProperties(device, &deviceProperties);   // Get the properties of the device
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);       // Additional features supported by the device

        // QueueFamilyIndices indices = findQueueFamilies(device);

        if (!(
            devicePropertySupport(deviceProperties) &&          // check if device supports required properties
            deviceFeatureSupport(deviceFeatures)    &&          // check if device supports required features
            deviceFamilySupport(device)             &&          // check if device has a matching Family
            // check if device supports required extensions
            deviceExensionSupport(device, std::set<std::string>(deviceExtensions.begin(), deviceExtensions.end())) &&
            deviceSwapChainSupport(device)                      // check if device has suitable swap chain
            ))
            return 0;


        uint32_t score = 1;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;


        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        return score;
    }

    // ------------------------- Presentation Section ------------------------- //
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    void createSurface() {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;     // Use the windows specific extension
        createInfo.hwnd = glfwGetWin32Window(window);                           // hwnd: handle to a window
        createInfo.hinstance = GetModuleHandle(nullptr);                        // hinstance: handle to an instance of an application

        if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // Use helper functions below to define the swap chain
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;      // # of images to have in the swapChain (+1 because we otherwise could need to wait for driver to complete internal operations)
        // Make sure that imageCount doesn't exceed the maximum possible # of images the swap chain could support
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            imageCount = swapChainSupport.capabilities.maxImageCount;

        // Set all information for createInfo
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;                                // # of Layers each image contains (equivalent to z layers)
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // Defines what kind of operations the images will be used for (e.g. if an image is used in post-processing)


        if (queueFamilyIndices.size() > 1) {
            // present family is in a separate queueFamily than graphics family
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            // Images can be used across multiple queue families without explicit ownership transfers.
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            // present is contained withing graphics family
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            // An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family.
            // This option offers the best performance.
            createInfo.queueFamilyIndexCount = 0;       // optional
            createInfo.pQueueFamilyIndices = nullptr;   // optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;   // What transforms should be applied to images in the swap chain
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;              // Should alpha channel be used for blending with other windows
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;                                               // If true, we don't care about colors of obscured pixels
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        // Assign the swap chain
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        // Retrieving handles
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // Assign the remaining variables
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());     // Match the size with the swapChainImages size

        // Iterate over all swap chain images
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;            // How to treat images (1D, 2D, 3D textures or cube maps)
            createInfo.format = swapChainImageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // subresourceRange describes the purpose of an image
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }

        }

    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        // Get the Surface capabilities of the logical device
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // Get the supported formats of the logical device
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);                       // How many formats are there

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());    // What are the formats
        }

        // Get the present modes of the logical device
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);                         // How many present modes are there

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data()); // What the present modes
        }

        return details;
    }

    // Colorspace of Surface (e.g. SRGB)
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0]; // If no ideal format is found return what we've got
    }

    // The method used for queuin images to be displayed
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR; // return VK_PRESENT_MODE_FIFO_KHR as this mode is guaranteed to be available
    }

    // Resolution of swap chain images displayed in window
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            // Adjust the swap chain width and height to use correct coordinates as on some systems screen coordinates != pixel coordinates
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }


};

