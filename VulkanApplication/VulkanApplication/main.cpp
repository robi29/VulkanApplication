#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdlib>

#include <fstream>
#include <set>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

////////////////////////////////////////////////////////////
/// vkCreateDebugUtilsMessengerExtension.
////////////////////////////////////////////////////////////
VkResult vkCreateDebugUtilsMessengerExtension(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks*              allocator,
    VkDebugUtilsMessengerEXT*                 debugMessenger )
{
    const auto function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if( function != nullptr )
    {
        return function( instance, createInfo, allocator, debugMessenger );
    }
    else
    {
        std::cerr << "Cannot find vkCreateDebugUtilsMessengerEXT extensions!" << std::endl;
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

////////////////////////////////////////////////////////////
/// vkDestroyDebugUtilsMessengerExtension.
////////////////////////////////////////////////////////////
void vkDestroyDebugUtilsMessengerExtension(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     debugMessenger,
    const VkAllocationCallbacks* pAllocator )
{
    const auto function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if( function != nullptr )
    {
        function( instance, debugMessenger, pAllocator );
    }
}

////////////////////////////////////////////////////////////
/// Status code enumeration.
////////////////////////////////////////////////////////////
enum class StatusCode : uint32_t
{
    Success = 0,
    Fail
};

////////////////////////////////////////////////////////////
/// Queue family indices.
////////////////////////////////////////////////////////////
struct QueueFamilyIndices
{
    // Graphics queue family.
    bool     m_HasGraphicsFamily;
    uint32_t m_GraphicsFamily;

    // Compute queue family.
    bool     m_HasComputeFamily;
    uint32_t m_ComputeFamily;

    // Memory transfer queue family.
    bool     m_HasMemoryTransferFamily;
    uint32_t m_MemoryTransferFamily;

    // Present queue family.
    bool     m_HasPresentFamily;
    uint32_t m_PresentFamily;
};

////////////////////////////////////////////////////////////
/// Swap chain support details.
////////////////////////////////////////////////////////////
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        m_Capabilities;
    std::vector<VkSurfaceFormatKHR> m_Formats;
    std::vector<VkPresentModeKHR>   m_PresentModes;
};

////////////////////////////////////////////////////////////
/// Application object.
////////////////////////////////////////////////////////////
class Application
{
private:
    ////////////////////////////////////////////////////////////
    /// Private glfw members.
    ////////////////////////////////////////////////////////////
    uint32_t    m_Width;
    uint32_t    m_Height;
    GLFWwindow* m_Window;
    std::string m_WindowName;

    ////////////////////////////////////////////////////////////
    /// Private Vulkan members.
    ////////////////////////////////////////////////////////////
    VkInstance                   m_Instance;
    VkSurfaceKHR                 m_Surface;
    VkPhysicalDevice             m_PhysicalDevice;
    VkPhysicalDeviceFeatures     m_PhysicalDeviceFeatures;
    VkDevice                     m_Device;
    VkQueue                      m_GraphicsQueue;
    VkQueue                      m_PresentQueue;
    VkSwapchainKHR               m_SwapChain;
    std::vector<VkImage>         m_SwapChainImages;
    VkFormat                     m_SwapChainImageFormat;
    VkExtent2D                   m_SwapChainExtent;
    std::vector<VkImageView>     m_SwapChainImageViews;
    VkRenderPass                 m_RenderPass;
    VkPipelineLayout             m_GraphicsPipelineLayout;
    VkPipeline                   m_GraphicsPipeline;
    std::vector<VkFramebuffer>   m_SwapChainFramebuffers;
    VkCommandPool                m_CommandPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkSemaphore                  m_ImageAvailableSemaphore;
    VkSemaphore                  m_RenderFinishedSemaphore;

    ////////////////////////////////////////////////////////////
    /// Private Vulkan extensions members.
    ////////////////////////////////////////////////////////////
    std::vector<const char*> m_PhysicalDeviceExtensions;

    ////////////////////////////////////////////////////////////
    /// Private debugging and validation layer members.
    ////////////////////////////////////////////////////////////
#ifdef _DEBUG
    std::vector<const char*> m_ValidationLayers;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif

public:
    ////////////////////////////////////////////////////////////
    /// Default constructor of the object.
    ////////////////////////////////////////////////////////////
    Application()
        : Application( 800, 600, "Vulkan Application" )
    {
    }

    ////////////////////////////////////////////////////////////
    /// Constructs the object with given parameters.
    ////////////////////////////////////////////////////////////
    Application(
        const uint32_t     width,
        const uint32_t     height,
        const std::string& windowName )
        : m_Width( width )
        , m_Height( height )
        , m_Window( nullptr )
        , m_WindowName( windowName )
        , m_Instance( VK_NULL_HANDLE )
        , m_Surface( VK_NULL_HANDLE )
        , m_PhysicalDevice( VK_NULL_HANDLE )
        , m_PhysicalDeviceFeatures{}
        , m_Device( VK_NULL_HANDLE )
        , m_GraphicsQueue( VK_NULL_HANDLE )
        , m_PresentQueue( VK_NULL_HANDLE )
        , m_SwapChain( VK_NULL_HANDLE )
        , m_SwapChainImages{}
        , m_SwapChainImageFormat( VK_FORMAT_UNDEFINED )
        , m_SwapChainExtent{}
        , m_SwapChainImageViews{}
        , m_RenderPass( VK_NULL_HANDLE )
        , m_GraphicsPipelineLayout( VK_NULL_HANDLE )
        , m_GraphicsPipeline( VK_NULL_HANDLE )
        , m_SwapChainFramebuffers{}
        , m_CommandPool( VK_NULL_HANDLE )
        , m_CommandBuffers{}
        , m_ImageAvailableSemaphore( VK_NULL_HANDLE )
        , m_RenderFinishedSemaphore( VK_NULL_HANDLE )
        , m_PhysicalDeviceExtensions{}
    {
        m_PhysicalDeviceExtensions.emplace_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

#ifdef _DEBUG
        m_ValidationLayers.emplace_back( "VK_LAYER_KHRONOS_validation" );
        m_DebugMessenger = VK_NULL_HANDLE;
#endif
    }

    ////////////////////////////////////////////////////////////
    /// Initializes the application.
    ////////////////////////////////////////////////////////////
    StatusCode Initialize()
    {
        StatusCode result = StatusCode::Success;

        // Create a window.
        result = InitializeWindow();
        if( result != StatusCode::Success )
        {
            return result;
        }

        // Initialize Vulkan api.
        result = InitializeVulkan();
        if( result != StatusCode::Success )
        {
            return result;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////
    /// Executes the main loop of the application.
    ////////////////////////////////////////////////////////////
    StatusCode Run()
    {
        StatusCode result = StatusCode::Success;

        result = MainLoop();
        if( result != StatusCode::Success )
        {
            return result;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////
    /// Destroys the application.
    ////////////////////////////////////////////////////////////
    StatusCode Destroy()
    {
        StatusCode result = StatusCode::Success;

        // Cleanups Vulkan api.
        result = CleanupVulkan();
        if( result != StatusCode::Success )
        {
            return result;
        }

        // Cleanups the window and glfw library.
        result = CleanupWindow();
        if( result != StatusCode::Success )
        {
            return result;
        }

        return result;
    }

private:
    ////////////////////////////////////////////////////////////
    /// Initialize the window using glfw.
    ////////////////////////////////////////////////////////////
    StatusCode InitializeWindow()
    {
        // Initialize glfw library.
        if( glfwInit() != GLFW_TRUE )
        {
            std::cerr << "Cannot initialize glfw library!" << std::endl;
            return StatusCode::Fail;
        }

        // Tell explicitly not to create an OpenGL context.
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

        // Disable window resizing.
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

        // Initialize the window.
        m_Window = glfwCreateWindow( m_Width, m_Height, m_WindowName.c_str(), nullptr, nullptr );
        if( m_Window == nullptr )
        {
            std::cerr << "Cannot create window!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Initializes Vulkan api.
    ////////////////////////////////////////////////////////////
    StatusCode InitializeVulkan()
    {
        StatusCode result = StatusCode::Success;

        result = CreateInstance();
        if( result != StatusCode::Success )
        {
            std::cerr << "Vulkan instance creation failed!" << std::endl;
            return result;
        }

#ifdef _DEBUG
        result = SetupDebugMessenger();
        if( result != StatusCode::Success )
        {
            std::cerr << "Setup debug messenger failed!" << std::endl;
            return result;
        }
#endif
        result = CreateSurface();
        if( result != StatusCode::Success )
        {
            std::cerr << "Surface creation failed!" << std::endl;
            return result;
        }

        result = PickPhysicalDevice();
        if( result != StatusCode::Success )
        {
            std::cerr << "Physical device picking failed!" << std::endl;
            return result;
        }

        result = CreateLogicalDevice();
        if( result != StatusCode::Success )
        {
            std::cerr << "Logical device creation failed!" << std::endl;
            return result;
        }

        result = CreateSwapChain();
        if( result != StatusCode::Success )
        {
            std::cerr << "Swap chain creation failed!" << std::endl;
            return result;
        }

        result = CreateImageViews();
        if( result != StatusCode::Success )
        {
            std::cerr << "Image views creation failed!" << std::endl;
            return result;
        }

        result = CreateRenderPass();
        if( result != StatusCode::Success )
        {
            std::cerr << "Render pass creation failed!" << std::endl;
            return result;
        }

        result = CreateGraphicsPipeline();
        if( result != StatusCode::Success )
        {
            std::cerr << "Graphics pipeline creation failed!" << std::endl;
            return result;
        }

        result = CreateFramebuffers();
        if( result != StatusCode::Success )
        {
            std::cerr << "Frame buffers creation failed!" << std::endl;
            return result;
        }

        result = CreateCommandPool();
        if( result != StatusCode::Success )
        {
            std::cerr << "Command pool creation failed!" << std::endl;
            return result;
        }

        result = CreateCommandBuffers();
        if( result != StatusCode::Success )
        {
            std::cerr << "Command buffers creation failed!" << std::endl;
            return result;
        }

        result = RecordCommandBuffers();
        if( result != StatusCode::Success )
        {
            std::cerr << "Command buffers recording failed!" << std::endl;
            return result;
        }

        result = CreateSemaphores();
        if( result != StatusCode::Success )
        {
            std::cerr << "Semaphores creation failed!" << std::endl;
            return result;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////
    /// Main loop of the application.
    ////////////////////////////////////////////////////////////
    StatusCode MainLoop()
    {
        StatusCode result = StatusCode::Success;

        // Check for events.
        while( !glfwWindowShouldClose( m_Window ) && result == StatusCode::Success )
        {
            glfwPollEvents();
            result = DrawFrame();
        }

        if( vkDeviceWaitIdle( m_Device ) != VK_SUCCESS )
        {
            std::cerr << "Failed to waiting for device!" << std::endl;
            return StatusCode::Fail;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////
    /// Cleanups the window.
    ////////////////////////////////////////////////////////////
    StatusCode CleanupWindow()
    {
        // Destroy the windows.
        glfwDestroyWindow( m_Window );

        // Destroy glfw library.
        glfwTerminate();

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates Vulkan instance.
    ////////////////////////////////////////////////////////////
    StatusCode CreateInstance()
    {
        // Information about the application.
        VkApplicationInfo applicationInfo  = {};
        applicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName   = m_WindowName.c_str();
        applicationInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
        applicationInfo.pEngineName        = "No Engine";
        applicationInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
        applicationInfo.apiVersion         = VK_API_VERSION_1_0;

        // Obtain required extensions.
        uint32_t                 glfwExtensionCount = 0;
        const char**             glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
        std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

#ifdef _DEBUG
        // Add debug messenger extension.
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif

        // Enumarate supported extensions.
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionCount );
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, availableExtensions.data() );

        // Check if required extensions are supported.
        std::set<std::string> requiredRequiredExtensions( extensions.begin(), extensions.end() );

        for( const auto& extension : availableExtensions )
        {
            requiredRequiredExtensions.erase( extension.extensionName );
        }

        if( !requiredRequiredExtensions.empty() )
        {
            std::cerr << "Cannot find required extensions!" << std::endl;
            return StatusCode::Fail;
        }

#ifdef _DEBUG
        // Enumarate supported validation layers.
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

        std::vector<VkLayerProperties> availableLayers( layerCount );
        vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

        // Check if required validation layers are supported.
        std::set<std::string> requiredValidationLayers( m_ValidationLayers.begin(), m_ValidationLayers.end() );

        for( const auto& layer : availableLayers )
        {
            requiredValidationLayers.erase( layer.layerName );
        }

        if( !requiredValidationLayers.empty() )
        {
            std::cerr << "Cannot find required validation layers!" << std::endl;
            return StatusCode::Fail;
        }
#endif

        // Sufficient information for creating an instance.
        VkInstanceCreateInfo createInfo    = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo        = &applicationInfo;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>( extensions.size() );
        createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef _DEBUG
        createInfo.enabledLayerCount   = static_cast<uint32_t>( m_ValidationLayers.size() );
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        PopulateDebugMessengerCreateInfo( debugCreateInfo );

        createInfo.pNext = &debugCreateInfo;
#else
        createInfo.enabledLayerCount       = 0;
#endif

        if( vkCreateInstance( &createInfo, nullptr, &m_Instance ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create Vulkan instance!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates Vulkan surface.
    ////////////////////////////////////////////////////////////
    StatusCode CreateSurface()
    {
        if( glfwCreateWindowSurface( m_Instance, m_Window, nullptr, &m_Surface ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create Vulkan surface using glfw library!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Picks a suitable graphics card.
    ////////////////////////////////////////////////////////////
    StatusCode PickPhysicalDevice()
    {
        // Enumerate for available graphics cards.
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices( m_Instance, &physicalDeviceCount, nullptr );

        if( physicalDeviceCount == 0 )
        {
            std::cerr << "Cannot find a physical device!" << std::endl;
            StatusCode::Fail;
        }

        std::vector<VkPhysicalDevice> physicalDevices( physicalDeviceCount );
        vkEnumeratePhysicalDevices( m_Instance, &physicalDeviceCount, physicalDevices.data() );

        // Choose a suitibale graphics card.
        uint32_t bestScore = 0;

        for( const auto& physicalDevice : physicalDevices )
        {
            if( IsPhysicalDeviceSuitable( physicalDevice ) )
            {
                uint32_t currentScore = RatePhysicalDeviceSuitability( physicalDevice );
                if( currentScore > bestScore )
                {
                    m_PhysicalDevice = physicalDevice;
                    bestScore        = currentScore;
                }
            }
        }

        if( m_PhysicalDevice == VK_NULL_HANDLE )
        {
            std::cerr << "Cannot find a suitable physical device!" << std::endl;
            StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Rates graphics card suitability.
    ////////////////////////////////////////////////////////////
    uint32_t RatePhysicalDeviceSuitability( VkPhysicalDevice physicalDevice )
    {
        // Get the physical device properties.
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties( physicalDevice, &physicalDeviceProperties );

        // Get the physical device features.
        vkGetPhysicalDeviceFeatures( physicalDevice, &m_PhysicalDeviceFeatures );

        // Do not choose a physical device without tessellation shader support.
        if( !m_PhysicalDeviceFeatures.tessellationShader )
        {
            return 0;
        }

        uint32_t score = 0;

        switch( physicalDeviceProperties.deviceType )
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                score += 2;
                break;

            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 100;
                break;

            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 1000;
                break;

            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 10;
                break;

            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 5;
                break;

            default:
                score += 1;
        }

        score += physicalDeviceProperties.limits.maxImageDimension2D;

        return score;
    }

    ////////////////////////////////////////////////////////////
    /// Checks if the physical device is suitable.
    ////////////////////////////////////////////////////////////
    bool IsPhysicalDeviceSuitable( VkPhysicalDevice physicalDevice )
    {
        const QueueFamilyIndices      indices = FindQueueFamilies( physicalDevice );
        const SwapChainSupportDetails details = QuerySwapChainSupport( physicalDevice );

        const bool areQueueFamiliesSupported =
            indices.m_HasGraphicsFamily &&
            indices.m_HasPresentFamily;

        const bool areExtensionsSupported =
            areQueueFamiliesSupported &&
            CheckPhysicalDeviceExtensionSupport( physicalDevice );

        const bool areSwapChainDetailsSupported =
            areExtensionsSupported &&
            !details.m_Formats.empty() &&
            !details.m_PresentModes.empty();

        return areSwapChainDetailsSupported;
    }

    ////////////////////////////////////////////////////////////
    /// Checks for needed physical device extensions support.
    ////////////////////////////////////////////////////////////
    bool CheckPhysicalDeviceExtensionSupport( VkPhysicalDevice physicalDevice )
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, availableExtensions.data() );

        std::set<std::string> requiredExtensions( m_PhysicalDeviceExtensions.begin(), m_PhysicalDeviceExtensions.end() );

        for( const auto& extension : availableExtensions )
        {
            requiredExtensions.erase( extension.extensionName );
        }

        return requiredExtensions.empty();
    }

    ////////////////////////////////////////////////////////////
    /// Creates a logical device.
    ////////////////////////////////////////////////////////////
    StatusCode CreateLogicalDevice()
    {
        // Get graphics queue family index.
        const QueueFamilyIndices indices = FindQueueFamilies( m_PhysicalDevice );

        // Set the queue priority.
        const float queuePriority = 1.0f;

        // Graphics and present queue create information.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t>                   uniqueQueueFamilies = { indices.m_GraphicsFamily, indices.m_PresentFamily };

        // Populate queue create information.
        for( const uint32_t queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex        = queueFamily;
            queueCreateInfo.queueCount              = 1;
            queueCreateInfo.pQueuePriorities        = &queuePriority;

            queueCreateInfos.emplace_back( queueCreateInfo );
        }

        // Populate logical device create information.
        VkDeviceCreateInfo deviceCreateInfo      = {};
        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>( queueCreateInfos.size() );
        deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures        = &m_PhysicalDeviceFeatures;
        deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>( m_PhysicalDeviceExtensions.size() );
        deviceCreateInfo.ppEnabledExtensionNames = m_PhysicalDeviceExtensions.data();
#ifdef _DEBUG
        deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>( m_ValidationLayers.size() );
        deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif

        // Create a logical device.
        if( vkCreateDevice( m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create a logical device!" << std::endl;
            return StatusCode::Fail;
        }

        // Get graphics queue.
        vkGetDeviceQueue( m_Device, indices.m_GraphicsFamily, 0, &m_GraphicsQueue );
        if( m_GraphicsQueue == nullptr )
        {
            std::cerr << "Cannot obtain graphics queue!" << std::endl;
            return StatusCode::Fail;
        }

        // Get present family.
        vkGetDeviceQueue( m_Device, indices.m_PresentFamily, 0, &m_PresentQueue );
        if( m_PresentQueue == nullptr )
        {
            std::cerr << "Cannot obtain present queue!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Finds different queue families.
    ////////////////////////////////////////////////////////////
    QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice physicalDevice )
    {
        QueueFamilyIndices indices = {};

        // Get physical device queue family properties.
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, nullptr );

        std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
        vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, queueFamilies.data() );

        // Find all needed queue families.
        uint32_t index = 0;
        for( const auto& queueFamily : queueFamilies )
        {
            // Graphics queue family.
            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && !indices.m_HasGraphicsFamily )
            {
                indices.m_GraphicsFamily    = index;
                indices.m_HasGraphicsFamily = true;
            }

            // Compute queue family.
            if( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && !indices.m_HasComputeFamily )
            {
                indices.m_ComputeFamily    = index;
                indices.m_HasComputeFamily = true;
            }

            // Memory transfer queue family.
            if( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !indices.m_HasMemoryTransferFamily )
            {
                indices.m_MemoryTransferFamily    = index;
                indices.m_HasMemoryTransferFamily = true;
            }

            // Present queue family.
            if( !indices.m_HasPresentFamily )
            {
                VkBool32 presentSupported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, index, m_Surface, &presentSupported );
                if( presentSupported )
                {
                    indices.m_PresentFamily    = index;
                    indices.m_HasPresentFamily = true;
                }
            }

            ++index;
        }

        return indices;
    }

    ////////////////////////////////////////////////////////////
    /// Creates swap chain.
    ////////////////////////////////////////////////////////////
    StatusCode CreateSwapChain()
    {
        // Get swap chain support details
        SwapChainSupportDetails details = QuerySwapChainSupport( m_PhysicalDevice );

        // Set swap chain settings.
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( details.m_Formats );
        VkPresentModeKHR   presentMode   = ChooseSwapPresentMode( details.m_PresentModes );
        VkExtent2D         extent        = ChooseSwapExtent( details.m_Capabilities );

        // Swap chain count.
        uint32_t imageCount = details.m_Capabilities.minImageCount + 1;

        if( details.m_Capabilities.maxImageCount > 0 &&
            details.m_Capabilities.maxImageCount < imageCount )
        {
            // Limit swap chain count to the maximum.
            imageCount = details.m_Capabilities.maxImageCount;
        }

        // Populate swap chain create information.
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface                  = m_Surface;
        createInfo.minImageCount            = imageCount;
        createInfo.imageFormat              = surfaceFormat.format;
        createInfo.imageColorSpace          = surfaceFormat.colorSpace;
        createInfo.imageExtent              = extent;
        createInfo.imageArrayLayers         = 1; // More than one for stereoscopic 3d applications.
        createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices              = FindQueueFamilies( m_PhysicalDevice );
        uint32_t           queueFamilyIndices[] = { indices.m_GraphicsFamily, indices.m_PresentFamily };

        if( indices.m_GraphicsFamily != indices.m_PresentFamily )
        {
            // Graphics and present queues are in different queue families.
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            // Graphics and present queues are in the same queue family.
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // Optional.
            createInfo.pQueueFamilyIndices   = nullptr; // Optional.
        }

        createInfo.preTransform   = details.m_Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = VK_TRUE;
        createInfo.oldSwapchain   = VK_NULL_HANDLE;

        // Create swap chain.
        if( vkCreateSwapchainKHR( m_Device, &createInfo, nullptr, &m_SwapChain ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create swap chain!" << std::endl;
            return StatusCode::Fail;
        }

        // Get swap chain images.
        vkGetSwapchainImagesKHR( m_Device, m_SwapChain, &imageCount, nullptr );
        m_SwapChainImages.resize( imageCount );
        vkGetSwapchainImagesKHR( m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data() );

        // Store swap chain format and extent.
        m_SwapChainImageFormat = surfaceFormat.format;
        m_SwapChainExtent      = extent;

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Queries swap chain support.
    ////////////////////////////////////////////////////////////
    SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice physicalDevice )
    {
        SwapChainSupportDetails details;

        // Get swap chain capabilities.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, m_Surface, &details.m_Capabilities );

        // Get swap chain formats.
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, m_Surface, &formatCount, nullptr );

        if( formatCount != 0 )
        {
            details.m_Formats.resize( formatCount );
            vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, m_Surface, &formatCount, details.m_Formats.data() );
        }

        // Get swap chain present modes.
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, m_Surface, &presentModeCount, nullptr );

        if( presentModeCount != 0 )
        {
            details.m_PresentModes.resize( presentModeCount );
            vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, m_Surface, &presentModeCount, details.m_PresentModes.data() );
        }

        return details;
    }

    ////////////////////////////////////////////////////////////
    /// Chooses swap chain surface format.
    ////////////////////////////////////////////////////////////
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
    {
        for( const auto& availableFormat : availableFormats )
        {
            // Use 32 bits per pixel (blue, green, red, alpha with 8 bit)/
            if( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            {
                return availableFormat;
            }
        }

        std::cerr << "Cannot choose a desire swap chain surface format! Used the first format!" << std::endl;
        return availableFormats[0];
    }

    ////////////////////////////////////////////////////////////
    /// Chooses swap chain present mode.
    ////////////////////////////////////////////////////////////
    VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes )
    {
        for( const auto& availablePresentMode : availablePresentModes )
        {
            // Triple buffering is much better than double buffering.
            if( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
            {
                return availablePresentMode;
            }
        }

        // Choose double buffering if triple buffering is not available.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    ////////////////////////////////////////////////////////////
    /// Chooses swap chain extent.
    ////////////////////////////////////////////////////////////
    VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities )
    {
        if( capabilities.currentExtent.width != UINT32_MAX )
        {
            return capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent = { m_Width, m_Height };

            actualExtent.width  = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
            actualExtent.height = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

            return actualExtent;
        }
    }

    ////////////////////////////////////////////////////////////
    /// Creates image views.
    ////////////////////////////////////////////////////////////
    StatusCode CreateImageViews()
    {
        // Match swap chain image views to swap chain images.
        m_SwapChainImageViews.resize( m_SwapChainImages.size() );

        for( size_t i = 0; i < m_SwapChainImages.size(); ++i )
        {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image                 = m_SwapChainImages[i];
            createInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                = m_SwapChainImageFormat;
            createInfo.components.r          = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g          = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b          = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a          = VK_COMPONENT_SWIZZLE_IDENTITY;

            // For stereographics 3d applications a swap chain with multiple layers is required and
            // with multiple image views as well.
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            if( vkCreateImageView( m_Device, &createInfo, nullptr, &m_SwapChainImageViews[i] ) != VK_SUCCESS )
            {
                std::cerr << "Cannot create swap chain image view!" << std::endl;
                return StatusCode::Fail;
            }
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates render pass.
    ////////////////////////////////////////////////////////////
    StatusCode CreateRenderPass()
    {
        // Populate attachment description information.
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format                  = m_SwapChainImageFormat;
        colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Populate attachment reference information.
        VkAttachmentReference colorAttachmentReference = {};
        colorAttachmentReference.attachment            = 0;
        colorAttachmentReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Populate subpass information.
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentReference;

        // Populate subpass dependency information.
        VkSubpassDependency dependency = {};
        dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass          = 0;
        dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask       = 0;
        dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Populate render pass information.
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount        = 1;
        renderPassInfo.pAttachments           = &colorAttachment;
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 1;
        renderPassInfo.pDependencies          = &dependency;

        // Create render pass.
        if( vkCreateRenderPass( m_Device, &renderPassInfo, nullptr, &m_RenderPass ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create render pass!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates graphics pipeline.
    ////////////////////////////////////////////////////////////
    StatusCode CreateGraphicsPipeline()
    {
        // Read the bytecode of shaders.
        const auto vertexShaderCode   = ReadBinaryFile( "Shaders/vert.spv" );
        const auto fragmentShaderCode = ReadBinaryFile( "Shaders/frag.spv" );

        if( vertexShaderCode.empty() || fragmentShaderCode.empty() )
        {
            std::cerr << "Empty shader files!" << std::endl;
            return StatusCode::Fail;
        }

        // Create shader modules for the shaders.
        VkShaderModule vertexShaderModule   = CreateShaderModule( vertexShaderCode );
        VkShaderModule fragmentShaderModule = CreateShaderModule( fragmentShaderCode );

        // Create vertex shader stage.
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
        vertexShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module                          = vertexShaderModule;
        vertexShaderStageInfo.pName                           = "main";

        // Create fragment shader stage.
        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
        fragmentShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module                          = fragmentShaderModule;
        fragmentShaderStageInfo.pName                           = "main";

        // Create programmable pipeline stages.
        VkPipelineShaderStageCreateInfo shaderStages[] = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        // Create vertex input state.
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount        = 0;
        vertexInputInfo.pVertexBindingDescriptions           = nullptr; // Optional.
        vertexInputInfo.vertexAttributeDescriptionCount      = 0;
        vertexInputInfo.pVertexAttributeDescriptions         = nullptr; // Optional.

        // Create input assembly state.
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable                 = VK_FALSE;

        // Viewports.
        VkViewport viewport = {};
        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = static_cast<float>( m_SwapChainExtent.width );
        viewport.height     = static_cast<float>( m_SwapChainExtent.height );
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;

        // Scissors.
        VkRect2D scissor = {};
        scissor.offset   = { 0, 0 };
        scissor.extent   = m_SwapChainExtent;

        // Create viewport state.
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount                     = 1;
        viewportState.pViewports                        = &viewport;
        viewportState.scissorCount                      = 1;
        viewportState.pScissors                         = &scissor;

        // Rasterizeration state.
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable                       = VK_FALSE;
        rasterizer.rasterizerDiscardEnable                = VK_FALSE;
        rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                              = 1.0f;
        rasterizer.cullMode                               = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable                        = VK_FALSE;
        rasterizer.depthBiasConstantFactor                = 0.0f; // Optional.
        rasterizer.depthBiasClamp                         = 0.0f; // Optional.
        rasterizer.depthBiasSlopeFactor                   = 0.0f; // Optional.

        // Multisample state.
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable                  = VK_FALSE;
        multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading                     = 1.0f;     // Optional.
        multisampling.pSampleMask                          = nullptr;  // Optional.
        multisampling.alphaToCoverageEnable                = VK_FALSE; // Optional.
        multisampling.alphaToOneEnable                     = VK_FALSE; // Optional.

        // Color blending attachment state.
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable                         = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_ONE;  // Optional.
        colorBlendAttachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO; // Optional.
        colorBlendAttachment.colorBlendOp                        = VK_BLEND_OP_ADD;      // Optional.
        colorBlendAttachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;  // Optional.
        colorBlendAttachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO; // Optional.
        colorBlendAttachment.alphaBlendOp                        = VK_BLEND_OP_ADD;      // Optional.

        // Color blending state.
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable                       = VK_FALSE;
        colorBlending.logicOp                             = VK_LOGIC_OP_COPY; // Optional.
        colorBlending.attachmentCount                     = 1;
        colorBlending.pAttachments                        = &colorBlendAttachment;
        colorBlending.blendConstants[0]                   = 0.0f; // Optional.
        colorBlending.blendConstants[1]                   = 0.0f; // Optional.
        colorBlending.blendConstants[2]                   = 0.0f; // Optional.
        colorBlending.blendConstants[3]                   = 0.0f; // Optional.

        // Create a pipeline layout.
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount             = 0;       // Optional.
        pipelineLayoutInfo.pSetLayouts                = nullptr; // Optional.
        pipelineLayoutInfo.pushConstantRangeCount     = 0;       // Optional.
        pipelineLayoutInfo.pPushConstantRanges        = nullptr; // Optional.

        if( vkCreatePipelineLayout( m_Device, &pipelineLayoutInfo, nullptr, &m_GraphicsPipelineLayout ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create pipeline layout!" << std::endl;

            // Destroy the shader modules.
            vkDestroyShaderModule( m_Device, fragmentShaderModule, nullptr );
            vkDestroyShaderModule( m_Device, vertexShaderModule, nullptr );

            return StatusCode::Fail;
        }

        // Populate graphics pipeline information.
        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount                   = 2;
        pipelineInfo.pStages                      = shaderStages;
        pipelineInfo.pVertexInputState            = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState          = &inputAssembly;
        pipelineInfo.pViewportState               = &viewportState;
        pipelineInfo.pRasterizationState          = &rasterizer;
        pipelineInfo.pMultisampleState            = &multisampling;
        pipelineInfo.pDepthStencilState           = nullptr; // Optional.
        pipelineInfo.pColorBlendState             = &colorBlending;
        pipelineInfo.pDynamicState                = nullptr; // Optional.
        pipelineInfo.layout                       = m_GraphicsPipelineLayout;
        pipelineInfo.renderPass                   = m_RenderPass;
        pipelineInfo.subpass                      = 0;
        pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE; // Optional.
        pipelineInfo.basePipelineIndex            = -1;             // Optional.

        if( vkCreateGraphicsPipelines( m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create graphics pipeline!" << std::endl;

            // Destroy the shader modules.
            vkDestroyShaderModule( m_Device, fragmentShaderModule, nullptr );
            vkDestroyShaderModule( m_Device, vertexShaderModule, nullptr );

            return StatusCode::Fail;
        }

        // Destroy the shader modules.
        vkDestroyShaderModule( m_Device, fragmentShaderModule, nullptr );
        vkDestroyShaderModule( m_Device, vertexShaderModule, nullptr );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates shader module.
    ////////////////////////////////////////////////////////////
    VkShaderModule CreateShaderModule( const std::vector<char>& code )
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize                 = code.size();
        createInfo.pCode                    = reinterpret_cast<const uint32_t*>( code.data() );

        VkShaderModule shaderModule = {};

        if( vkCreateShaderModule( m_Device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create shader module!" << std::endl;
            return shaderModule;
        }

        return shaderModule;
    }

    ////////////////////////////////////////////////////////////
    /// Creates frame buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateFramebuffers()
    {
        // Create a frame buffer for all of the images in the swap chain.
        m_SwapChainFramebuffers.resize( m_SwapChainImageViews.size() );

        for( size_t i = 0; i < m_SwapChainImageViews.size(); ++i )
        {
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass              = m_RenderPass;
            framebufferInfo.attachmentCount         = 1;
            framebufferInfo.pAttachments            = &m_SwapChainImageViews[i];
            framebufferInfo.width                   = m_SwapChainExtent.width;
            framebufferInfo.height                  = m_SwapChainExtent.height;
            framebufferInfo.layers                  = 1;

            if( vkCreateFramebuffer( m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i] ) != VK_SUCCESS )
            {
                std::cerr << "Cannot create frame buffer!" << std::endl;
                return StatusCode::Fail;
            }
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates command pool.
    ////////////////////////////////////////////////////////////
    StatusCode CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies( m_PhysicalDevice );

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex        = queueFamilyIndices.m_GraphicsFamily;
        poolInfo.flags                   = 0; // Optional.

        if( vkCreateCommandPool( m_Device, &poolInfo, nullptr, &m_CommandPool ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create command pool!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates command buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateCommandBuffers()
    {
        // Create a command buffer for all frame buffers.
        m_CommandBuffers.resize( m_SwapChainFramebuffers.size() );

        // Populate command buffer allocate information.
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool                 = m_CommandPool;
        allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount          = static_cast<uint32_t>( m_CommandBuffers.size() );

        // Create command buffers.
        if( vkAllocateCommandBuffers( m_Device, &allocInfo, m_CommandBuffers.data() ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create command buffer!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Records command buffers.
    ////////////////////////////////////////////////////////////
    StatusCode RecordCommandBuffers()
    {
        for( size_t i = 0; i < m_CommandBuffers.size(); ++i )
        {
            // Populate command buffer begin information.
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags                    = 0;       // Optional.
            beginInfo.pInheritanceInfo         = nullptr; // Optional.

            // Begin recording the command buffer.
            if( vkBeginCommandBuffer( m_CommandBuffers[i], &beginInfo ) != VK_SUCCESS )
            {
                std::cerr << "Failed to begin recording command buffer!" << std::endl;
                return StatusCode::Fail;
            }

            // Populate render pass begin information.
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass            = m_RenderPass;
            renderPassInfo.framebuffer           = m_SwapChainFramebuffers[i];
            renderPassInfo.renderArea.offset     = { 0, 0 };
            renderPassInfo.renderArea.extent     = m_SwapChainExtent;

            // Define a clear color.
            VkClearValue clearColor        = { 0.0f, 0.0f, 0.0f, 1.0f };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = &clearColor;

            // Begin the render pass.
            vkCmdBeginRenderPass( m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

            // Bind the graphics pipeline.
            vkCmdBindPipeline( m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline );

            // Draw.
            vkCmdDraw( m_CommandBuffers[i], 3, 1, 0, 0 );

            // End the render pass.
            vkCmdEndRenderPass( m_CommandBuffers[i] );

            // End recoring the command buffer.
            if( vkEndCommandBuffer( m_CommandBuffers[i] ) != VK_SUCCESS )
            {
                std::cerr << "Failed to record command buffer!" << std::endl;
                return StatusCode::Fail;
            }
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates semaphores to signal events.
    ////////////////////////////////////////////////////////////
    StatusCode CreateSemaphores()
    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if( vkCreateSemaphore( m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore ) != VK_SUCCESS ||
            vkCreateSemaphore( m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore ) != VK_SUCCESS )
        {
            std::cerr << "Failed to create semaphores!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Draws a frame.
    ////////////////////////////////////////////////////////////
    StatusCode DrawFrame()
    {
        // Acquire an image from the swap chain.
        uint32_t imageIndex = 0;
        vkAcquireNextImageKHR( m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

        // Submit the command buffer.
        VkSubmitInfo         submitInfo = {};
        VkPipelineStageFlags waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &m_ImageAvailableSemaphore;
        submitInfo.pWaitDstStageMask    = &waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &m_CommandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &m_RenderFinishedSemaphore;

        if( vkQueueSubmit( m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) != VK_SUCCESS )
        {
            std::cerr << "Failed to submit draw command buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Present the swap chain.
        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &m_RenderFinishedSemaphore;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &m_SwapChain;
        presentInfo.pImageIndices      = &imageIndex;
        presentInfo.pResults           = nullptr; // Optional.

        if( vkQueuePresentKHR( m_PresentQueue, &presentInfo ) != VK_SUCCESS )
        {
            std::cerr << "Failed to present swap chain!" << std::endl;
            return StatusCode::Fail;
        }

        if( vkQueueWaitIdle( m_PresentQueue ) != VK_SUCCESS )
        {
            std::cerr << "Failed to waiting for present queue!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Populates debug messenger create information.
    ////////////////////////////////////////////////////////////
    void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo )
    {
#ifdef _DEBUG
        createInfo                 = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        createInfo.pUserData       = nullptr; // Optional.
#endif
    }

    ////////////////////////////////////////////////////////////
    /// Setups debug messenger.
    ////////////////////////////////////////////////////////////
    StatusCode SetupDebugMessenger()
    {
#ifdef _DEBUG
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        PopulateDebugMessengerCreateInfo( createInfo );

        if( vkCreateDebugUtilsMessengerExtension( m_Instance, &createInfo, nullptr, &m_DebugMessenger ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create debug utils messenger!" << std::endl;
            return StatusCode::Fail;
        }
#endif

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Cleanups Vulkan api.
    ////////////////////////////////////////////////////////////
    StatusCode CleanupVulkan()
    {
        vkDestroySemaphore( m_Device, m_RenderFinishedSemaphore, nullptr );
        vkDestroySemaphore( m_Device, m_ImageAvailableSemaphore, nullptr );

        vkDestroyCommandPool( m_Device, m_CommandPool, nullptr );

        for( auto& framebuffer : m_SwapChainFramebuffers )
        {
            vkDestroyFramebuffer( m_Device, framebuffer, nullptr );
        }

        vkDestroyPipeline( m_Device, m_GraphicsPipeline, nullptr );

        vkDestroyPipelineLayout( m_Device, m_GraphicsPipelineLayout, nullptr );

        vkDestroyRenderPass( m_Device, m_RenderPass, nullptr );

        for( auto& imageView : m_SwapChainImageViews )
        {
            vkDestroyImageView( m_Device, imageView, nullptr );
        }

        vkDestroySwapchainKHR( m_Device, m_SwapChain, nullptr );

        vkDestroyDevice( m_Device, nullptr );

        vkDestroySurfaceKHR( m_Instance, m_Surface, nullptr );

#ifdef _DEBUG
        vkDestroyDebugUtilsMessengerExtension( m_Instance, m_DebugMessenger, nullptr );
#endif

        vkDestroyInstance( m_Instance, nullptr );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Reads binary data from files.
    ////////////////////////////////////////////////////////////
    static std::vector<char> ReadBinaryFile( const std::string& filename )
    {
        // Read file from the end of the file to determine the size of the file.
        std::ifstream     file( filename, std::ios::ate | std::ios::binary );
        std::vector<char> buffer = {};

        if( !file.is_open() )
        {
            std::cerr << "Cannot open " << filename << " file!" << std::endl;
            return buffer;
        }

        // Get the size of the binary file.
        const uint32_t fileSize = static_cast<uint32_t>( file.tellg() );

        // Resize the buffer.
        buffer.resize( fileSize );

        // Seek back to the beginning of the file.
        file.seekg( 0 );

        // Read all of the bytes at once.
        file.read( buffer.data(), fileSize );

        // Close the file.
        file.close();

        // Return the buffer.
        return buffer;
    }

    ////////////////////////////////////////////////////////////
    /// Debug Vulkan api callback.
    ////////////////////////////////////////////////////////////
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void*                                       userData )
    {
#ifdef _DEBUG
        if( messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
        {
            std::cerr << "Validation layer: " << callbackData->pMessage << std::endl;
        }
#endif

        return VK_FALSE;
    }
};

int32_t main()
{
    StatusCode result = StatusCode::Success;

    // Construct an application object with parameters.
    Application application( 800, 600, "Vulkan Application" );

    // Initialize the application.
    result = application.Initialize();
    if( result != StatusCode::Success )
    {
        std::cerr << "Initialization failed!" << std::endl;
        application.Destroy();
        return static_cast<int32_t>( result );
    }

    // Run the application in a loop.
    result = application.Run();
    if( result != StatusCode::Success )
    {
        std::cerr << "Run failed!" << std::endl;
        application.Destroy();
        return static_cast<int32_t>( result );
    }

    // Destroy the application.
    result = application.Destroy();
    if( result != StatusCode::Success )
    {
        std::cerr << "Destroy failed!" << std::endl;
        return static_cast<int32_t>( result );
    }

    // Return success.
    return static_cast<int32_t>( result );
}
