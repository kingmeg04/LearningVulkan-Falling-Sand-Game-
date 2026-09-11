#pragma once

// Vulkan / GLFW platform configuration
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define NOMINMAX  // prevent windows.h's min/max macros from clashing with std::min/max, std::numeric_limits<>::max()

// Vulkan
#include <vulkan/vulkan.h>

// GLFW
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// GLM
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// STB
#include "../lib/stb/stb_image.h"

// TinyObjLoader
#include "../lib/tinyobjloader/tiny_obj_loader.h"

// Standard libraries
// Types
#include <vector>
#include <optional>
#include <map>
#include <set>
#include <bit>
#include <algorithm>
#include <limits>
#include <chrono>

// Console/Filesystems
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <cstring>