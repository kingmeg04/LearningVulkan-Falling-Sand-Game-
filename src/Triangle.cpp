#include "pch.h"
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        // Vertex binding describes at which rate to load data from memory throughout the vertices
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        // There are two attribute descriptions, position and color
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        // Set attributes of position
        attributeDescriptions[0].binding = 0;                       // Which binding does the per-vertex data come from
        attributeDescriptions[0].location = 0;                      // Which layout location is the attribute present at
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);    // How many bytes from the start of the per-vertex data to read from
        // Set attributes for color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

class HelloTriangleApplication {
public:
    const std::string compilerOutput = "../cmake-build-debug/";

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const int MAX_FRAMES_IN_FLIGHT = 2;     // Not too large so the cpu doesn't get too far ahead of the GPU, which would cause latency

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT; //| VK_QUEUE_TRANSFER_BIT; // VK_QUEUE_TRANSFER_BIT is implied by VK_QUEUE_GRAPHICS_BIT so it's not really necessary

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

    // ========================= Setup Section ========================= //
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    std::vector<std::pair<uint32_t, VkQueueFlags>> selectedQueueFamilies;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // hint to not use OpenGL (as this is the standard choice)
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // hint to make the window non-resizeable

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);      // Attaches a pointer to "this" to associate the app instance with the window so callbacks can retrieve it
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPools();
        createVertexBuffer();
        createIndexBuffer();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
    }

    void cleanup() {
        vkDeviceWaitIdle(device);   // wait for all GPU work to finish before destroying anything (Avoids validation layers complaining when closing the program)

        cleanupSwapChain();

        vkDestroyBuffer(device, indexBuffer, nullptr);              // clear th index buffer
        vkFreeMemory(device, indexBufferMemory, nullptr);           // free the memory allocated for the index buffer
        vkDestroyBuffer(device, vertexBuffer, nullptr);             // clear the vertex buffer
        vkFreeMemory(device, vertexBufferMemory, nullptr);          // free the memory allocated for the vertex buffer

        vkDestroyPipeline(device, graphicsPipeline, nullptr);       // clear the graphics pipeline
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);   // destroy the pipeline layout (used for "uniforms" and "push constants")

        vkDestroyRenderPass(device, renderPass, nullptr);           // destroy the render pass

        // Destroy synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, transferCommandPool, nullptr); // destroy the transfer command pool
        vkDestroyCommandPool(device, graphicsCommandPool, nullptr); // destroy the graphics command pool

        vkDestroyDevice(device, nullptr);                           // destroy the logical device

        vkDestroySurfaceKHR(instance, surface, nullptr);            // destroy the surface
        vkDestroyInstance(instance, nullptr);                       // destroy the instance

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
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);      // My application's version
        appInfo.pEngineName = "No Engine";                          // Engine (running the application, aka me)
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);           // Engine's version
        appInfo.apiVersion = VK_API_VERSION_1_3;                    // The version of Vulkan being used

        // create Info
        VkInstanceCreateInfo createInfo{};

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());  // # of validation layers
            createInfo.ppEnabledLayerNames = validationLayers.data();                       // passing validation layers to createInfo
        } else {
            createInfo.enabledLayerCount = 0;
        }

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;                          // struct type
        createInfo.pApplicationInfo = &appInfo;                                             // reference appInfo defined above

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

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
    }

    void createLogicalDevice() {
        std::vector<uint32_t> indices = findQueueFamilies(physicalDevice, requiredQueueFlags);
        if (indices.size() <= 0)
            throw std::runtime_error("No compatible queue family found!");



        // Check if any of the families with support also support present
        VkBool32 presentSupport = false;
        for (uint32_t candidate : indices) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, candidate, surface, &presentSupport);
            if (presentSupport) {
                selectedQueueFamilies.emplace_back(candidate, requiredQueueFlags);
                selectedQueueFamilies.emplace_back(findQueueFamilies(physicalDevice, VK_QUEUE_TRANSFER_BIT)[0], VK_QUEUE_TRANSFER_BIT);
                // Guaranteed to succeed since VK_QUEUE_GRAPHICS_BIT implies VK_QUEUE_TRANSFER_BIT //
                break;
            }
        }

        // If no ideal family was found, find another family with present support
        if (!presentSupport) {

            selectedQueueFamilies.emplace_back(indices[0], requiredQueueFlags);
            selectedQueueFamilies.emplace_back(findQueueFamilies(physicalDevice, VK_QUEUE_TRANSFER_BIT)[0], VK_QUEUE_TRANSFER_BIT);
            // Guaranteed to succeed since VK_QUEUE_GRAPHICS_BIT implies VK_QUEUE_TRANSFER_BIT //

            indices = findQueueFamilies(physicalDevice, 0);
            for (uint32_t candidate : indices) {
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, candidate, surface, &presentSupport);
                if (presentSupport) {
                    selectedQueueFamilies.emplace_back(candidate, 0);
                    break;
                }
            }

            if (!presentSupport)
                throw std::runtime_error("failed to find a supporting device!");
        }



        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f; // (range from 0.0 - 1.0)

        for (const auto& [family, flags] : selectedQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};                                      // Creating the queues for the queueFamily
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = family;
            queueCreateInfo.queueCount = 1;                                                 // How many Queues should the family have
            queueCreateInfo.pQueuePriorities = &queuePriority;                              // The priority of the queue
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};                                          // Features that should be enabled
        deviceFeatures.geometryShader = VK_TRUE;

        VkDeviceCreateInfo createInfo{};                                                    // Creating the actual device
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());   // How many VkDeviceQueueCreateInfo's are being provided

        createInfo.pEnabledFeatures = &deviceFeatures;                                      // Which features should the logical device enable

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());  // How many extensions do we want to enable
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();                       // Extensions to enable

        createInfo.enabledLayerCount = 0;                                                   // Disable validation layers for logic device


        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // Assign the graphicsQueue to the Queue (This must have present support)
        //vkGetDeviceQueue(device, *familyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, selectedQueueFamilies[0].first, 0, &graphicsQueue);
        vkGetDeviceQueue(device, selectedQueueFamilies[1].first, 0, &transferQueue);

        if (selectedQueueFamilies.size() > 2) {
            vkGetDeviceQueue(device, selectedQueueFamilies[2].first, 0, &presentQueue);
        } else {
            vkGetDeviceQueue(device, selectedQueueFamilies[0].first, 0, &presentQueue);  // same family, same queue
        }


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
            deviceExtensionSupport(device, std::set<std::string>(deviceExtensions.begin(), deviceExtensions.end())) &&
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
    bool deviceExtensionSupport(VkPhysicalDevice device, std::set<std::string> requiredExtensions) {
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
    std::vector<uint32_t> convertQueueFamiliesFormat(const std::vector<std::pair<uint32_t, VkQueueFlags>>& queueFamilies) {
        std::vector<uint32_t> queueFamilyIndices;
        queueFamilyIndices.reserve(selectedQueueFamilies.size());

        for (const auto& [family, flags] : selectedQueueFamilies) {
            if (std::find(queueFamilyIndices.begin(), queueFamilyIndices.end(), family)
                == queueFamilyIndices.end()) {
                queueFamilyIndices.push_back(family);
                }
        }
        return queueFamilyIndices;
    }

    // alternative to just checking if a device is suitable, but finding the best candidate
    uint32_t rateDeviceSuitability(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

        vkGetPhysicalDeviceProperties(device, &deviceProperties);   // Get the properties of the device
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);       // Additional features supported by the device

        if (!(
            devicePropertySupport(deviceProperties) &&          // check if device supports required properties
            deviceFeatureSupport(deviceFeatures)    &&          // check if device supports required features
            deviceFamilySupport(device)             &&          // check if device has a matching Family
            // check if device supports required extensions
            deviceExtensionSupport(device, std::set<std::string>(deviceExtensions.begin(), deviceExtensions.end())) &&
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

    // ========================= Presentation Section ========================= //
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // Flag used for resizing the window to notify that swap chain needs to be recreated
    // (Most platforms trigger VK_ERROR_OUT_OF_DATE_KHR automatically, but isn't guaranteed, which is why we do this)
    bool framebufferResized = false;

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


        std::vector<uint32_t> queueFamilyIndices = convertQueueFamiliesFormat(selectedQueueFamilies);

        if (queueFamilyIndices.size() > 1) {
            // present family is in a separate queueFamily than graphics familyW
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            // Images can be used across multiple queue families without explicit ownership transfers.
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            // present is contained within graphics family
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
    void recreateSwapChain() {
        // This first part handles the scenario where the window is minimized
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);  // Avoid touching resources that are in use

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
        createRenderFinishedSemaphores();   // Adjust semaphores dependent on swap chain
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

    // The method used for queuing images to be displayed
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

    void cleanupSwapChain() {
        // Clean the swap chain
        for (auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        for (auto imageView : swapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        // Adjust semaphores, which depend on swap chain
        for (auto semaphore : renderFinishedSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);
        renderFinishedSemaphores.clear();

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    // Declared as static, since GLFW doesn't know how to call a member function using "this" pointer
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    // ========================= Graphics Pipeline Section ========================= //
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile(compilerOutput + "shaders/shader2.vert.spv");
        auto fragShaderCode = readFile(compilerOutput + "shaders/shader2.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // To actually use the shaders we need to assign them to a specific pipeline stage
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;             // Which pipeline stage is the shader in
        // Next two lines can be used to differentiate between shaders by setting different "entry points"
        vertShaderStageInfo.module = vertShaderModule;                      // which shaderModule does this pipeline stage use
        vertShaderStageInfo.pName = "main";                                 // Name of the pipeline stage (standard entry point "main")

        // Same process for fragment shader
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Put the pipeline stages into an array to be used in a later step
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // Vertex input TODO: complete comments
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // VkPipelineInputAssemblyStateCreateInfo struct defines what kind of geometry will be drawn from the vertices
        // and if primitive restart should be enabled
        // primitive restart lets a special index value (0xFFFF/0xFFFFFFFF) split one draw call into multiple strips without ending the draw
        // (only relevant with indexed draws and strip/fan topologies)
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;       // Type of geometry to be drawn
        inputAssembly.primitiveRestartEnable = VK_FALSE;                    // Is primitive restart enabled

        // Viewport is the region of the framebuffer that the output will be rendered to
        // This will almost always be 0,0 to (width, height), so it has the same size
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor Rectangles define the region where pixels should be stored, any pixels outside this region will be discarded by the rasterizer
        // This is why we define a scissor rectangle to cover the entire frameBuffer, since we want to draw the entire framebuffer
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        // Determines which states of the pipeline can be changed dynamically (e.g. window size)
        // Since we want dynamic viewport and scissor rectangle, we must enable their dynamic states in the pipeline
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        // Define the dynamic states of the pipeline
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Specify the count of viewports and scissors at pipeline creation time
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Without the dynamic state the viewport and scissor rectangle would need to be set the following way, making them immutable
        /*
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        */

        // The rasterizer converts geometry into fragments so the fragment shader can color them later (Fragments are converted to pixels later)
        // It can be configured to output fragments that fill the entire polygon or just the edges (wireframe)
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;          // If this is enabled, the rasterizer discards the output to the framebuffer
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;          // The mode for rendering polygons (alternatives are line and point modes)
        rasterizer.lineWidth = 1.0f;                            // Line width if using a mode other than fill (Any thickness greater than 1.0f requires enabling wideLines GPU feature)
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;            // The type of face culling (back or front face culling)
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;         // Specifies the vertex order to be considered front/back facing
        rasterizer.depthBiasEnable = VK_FALSE;                  // The rasterizer has the ability to bias depth values if enabled
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // VkPipelineMultisampleStateCreateInfo configures multisampling, which is used for antialiasing
        // It combines fragment shader results to rasterize to the same pixel to reduce the hardness of the pixel edges
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // The output of the rasterizer needs to be blended with the color already present in the frame buffer. There's two options:
        // 1. Mix old and new
        // 2. Combine old and new with bitwise operation

        // VkPipelineColorBlendAttachmentState defines the configuration per attached frame buffer TODO: complete comments
        // Since we only have one frame buffer this is only defined once
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // Alpha blending:
        // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
        // finalColor.a = newAlpha.a;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // VkPipelineColorBlendStateCreateInfo defines the global configuration for color blending
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Uniform values used in shaders behave like globals and are commonly used for passing samples or transformation matrices from one CPU to GPU
        // These uniforms need to be specified during the pipeline creation using VkPipelineLayout TODO: complete comments
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // Finally create the graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        // reference the array of VkPipelineShaderStageCreateInfo structs
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;                                       // Index of the subpass to be used
        // These allow a pipeline to be derived from an existing pipeline, potentially allowing Vulkan to reuse some of its state/implementation
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // We can directly clear the shader modules again as it is only need to create the pipeline and not used after that
        // This means we're allowed to clear them directly after the graphics pipeline has been created
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createRenderPass() {
        // Single color buffer attachment with an image from the swap chain
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;                  // Match the image format of the swap chain
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                // No multisampling so only 1 sample
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;           // Determines what to do with data before rendering
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;         // Determines what to do with data after rendering
        // No stencils are used so we do nothing with them
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Present the image to swap chain

        // Subpasses can be reordered within a single pass by Vulkan to preserve memory bandwidth
        // Every subpass references one or more attachments, described by VkAttachmentReference
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // Specify the type of subpass
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;                // referenced in fragment shader by: layout(location = 0) out vec4 outColor

        // Add a dependency to the subpass
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        // What operations to wait on and in which stages they occur
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        // What operations should wait on this stage/operation to complete
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


        // Finally fill the renderPassInfo object to create the render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        // Since data is stored in a std::vector, the alignment requirements satisfy the worst case scenario //

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    // Read shader files
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        // ate: start reading from end of file | binary: read file as binary file to avoid text transformation //
        // ate is useful as it lets us allocate a buffer to be large enough for the entire file without needing to reallocate

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);     // Allocate a buffer with the size of the file for reading

        file.seekg(0);                       // Go back to the beginning of the file
        file.read(buffer.data(), fileSize);   // Read the file into the buffer

        file.close();

        return buffer;
    }

    // ----- Vertex Buffers ----- //
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    // Using uint16_t for less than 65535 unique vertices
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        std::vector<uint32_t> queueFamilyIndices = convertQueueFamiliesFormat(selectedQueueFamilies);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        // Allocate a temporary command buffer to submit the memory transfer operation
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = transferCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        // Immediately start recording this command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;      // Hint to the driver that we will wait for copy to finish and then submit once

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        // Actual copy operation for copying the source buffer into the destination buffer is vkCmdCopyBuffer(...)
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        // TODO: cleaner fence implementation
        // Create fence for this submission
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence;
        if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence!");
        }

        // We don't need to wait on anything, so we execute the transfer on the buffers immediately
        // It's possible to use a fence or wait for transfer queue to become idle, so we know when the operation has completed
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Associate the fence with this submission
        if (vkQueueSubmit(transferQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit transfer command buffer!");
        }

        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);        // Wait for THIS submission to finish
        vkDestroyFence(device, fence, nullptr);

        // vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);         // Submit this command
        // vkQueueWaitIdle(transferQueue); // easier implementation (no gain from asynchronous implementation)

        vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);   // Cleanup
    }
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        // Find memory suitable for the buffer
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (
            typeFilter & (1 << i)   &&                                                  // Is the bitflag for the required memory type set
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties     // Search for memory with the requested properties
            ) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Staging buffer is used to copy vertex data from host-visible memory into device-local memory
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // Clear and free the temporary staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    // Similar to creating the vertex buffer
    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }





    // ========================= Drawing Section ========================= //
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool graphicsCommandPool;
    VkCommandPool transferCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());   // Resize the container to hold all framebuffers

        // Iterate through all image views and create frame buffers from them
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            // Create the frame buffer
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPools() {
        // Create command pool for graphics family
        VkCommandPoolCreateInfo graphicsPoolInfo{};
        graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        graphicsPoolInfo.queueFamilyIndex = selectedQueueFamilies[0].first;

        if (vkCreateCommandPool(device, &graphicsPoolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

        // Create command pool for transfer family
        VkCommandPoolCreateInfo transferPoolInfo{};
        transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        for (const auto& [family, flags] : selectedQueueFamilies) {
            if (flags == VK_QUEUE_TRANSFER_BIT) {
                transferPoolInfo.queueFamilyIndex = family;
                break;
            }
        }
        if (vkCreateCommandPool(device, &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = graphicsCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;      // Is the command buffer primary or secondary (secondary buffers can only be called by primary buffers)
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional                        // Flags specify how we're going to use the command buffer
        beginInfo.pInheritanceInfo = nullptr; // Optional

        // Start recording the command buffer
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // Start the rendering pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        // We created a framebuffer for each swap chain image where it is specified as a color attachment //
        // Define the size of the render area, should match in size for best performance
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        // clearColor is roughly the same as a background color if nothing else is rendered on top
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Begin render pass, can be defined inline (render pass commands embedded into command buffer)
        // or secondary (render pass commands will be executed from a secondary command buffer)
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind the graphics pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Set the viewport and scissor rectangle for the commandBuffer
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind vertex buffer during draw
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        // Bind the index and vertex buffers
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);      // End the render pass

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);     // Check if all fences have been passed for current frame
        // vkResetFences(device, 1, &inFlightFences[currentFrame]);                                                // Reset all fences for current frame

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Check if Swap chain is out-of-date or suboptimal
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);                                         // Reset fences after we're certain that work will be submitted with it

        vkResetCommandBuffer(commandBuffers[currentFrame],  0);                                               // Make sure the command buffer is available
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);                                             // Set the command Buffer to use the swap chain image at imageIndex

        // Submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};                        // Which pipeline stage waits on semaphore
        submitInfo.waitSemaphoreCount = 1;                                                                          // How many Semaphores to wait for
        submitInfo.pWaitSemaphores = waitSemaphores;                                                                // What to wait for
        submitInfo.pWaitDstStageMask = waitStages;
        // Which command buffers submit for execution
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        // Which semaphore should be signaled to once the command buffers have finished execution
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // Wait for rendering to finish before presenting
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        // Which swap chain should the images be presented to and the index of the image to be presented
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional     // Specify an array of VkResult values to check for every individual swap chain if presentation was successful

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // Same thing as above with vkAcquireNextImageKHR: check if swap chain is out-of-date or suboptimal
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // Finally advance to next frame
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;   // using % for looping the frame counter
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        // Create semaphores
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Create fences
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;         // Initialize the fences as passed so the first frame can render (otherwise deadlock as nothing ever resets the fences)

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
            ) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }

        createRenderFinishedSemaphores();
    }
    // Separate creation of RenderFinishedSemaphores (So it can be called separately on recreateSwapChain()
    void createRenderFinishedSemaphores() {
        renderFinishedSemaphores.resize(swapChainImages.size());
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
};