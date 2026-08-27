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

class HelloTriangleApplication {
public:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT;

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
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // hint to not use OpenGL (as this is the standard choice)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // hint to make the window non-resizeable

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        //setupDebugMessanger(); // this is for custom control over the debug output from validation layers
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyDevice(device, nullptr);       // clear the logical device

        vkDestroyInstance(instance, nullptr);   // clear the instance

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

        uint32_t familyIndex = indices[0];

        float queuePriority = 0.7f; // (range from 0.0 - 1.0)

        VkDeviceQueueCreateInfo queueCreateInfo{};                                  // Creating the queues for the queueFamily
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        //queueCreateInfo.queueFamilyIndex = *familyIndex;
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueCreateInfo.queueCount = 1;                                             // How many Queues should the family have
        queueCreateInfo.pQueuePriorities = &queuePriority;                          // The priority of the queue

        VkPhysicalDeviceFeatures deviceFeatures{};                                  // Features that should be enabled
        deviceFeatures.geometryShader = true;

        VkDeviceCreateInfo createInfo{};                                            // Creating the actual device
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;                                        // How many VkDeviceQueueCreateInfo's are being provided

        createInfo.pEnabledFeatures = &deviceFeatures;                              // Which features should the logical device enable

        // Rest is similar to VkInstanceCreateInfo
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // Assign the graphicsQueue to the Queue
        //vkGetDeviceQueue(device, *familyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, familyIndex, 0, &graphicsQueue);
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

        vkGetPhysicalDeviceProperties(device, &deviceProperties);   // Get the properties of the device
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);       // Additional features supported by the device

        return (
            devicePropertySupport(deviceProperties) &&              // check if device has correct properties
            deviceFeatureSupport(deviceFeatures)    &&              // check if device has correct features
            deviceFamilySupport(device)                             // check if device has a queue with correct support
            );
    }

    // These three are "semi"-hardcoded methods, so I can easily have a validity check for device anywhere without redundancy
    bool devicePropertySupport(VkPhysicalDeviceProperties deviceProperties) {
        return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    }

    bool deviceFeatureSupport(VkPhysicalDeviceFeatures deviceFeatures) {
        return (deviceFeatures.geometryShader);
    }

    bool deviceFamilySupport(VkPhysicalDevice device) {
        return findQueueFamily(device, requiredQueueFlags).has_value();
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
            devicePropertySupport(deviceProperties) &&              // check if device has correct properties
            deviceFeatureSupport(deviceFeatures)    &&              // check if device has correct features
            deviceFamilySupport(device)                             // check if device has a matching Family
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

};

