#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdlib>

#include <fstream>
#include <set>
#include <vector>
#include <array>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image_library.h"

constexpr uint32_t MaxFramesInFlight = 2;

////////////////////////////////////////////////////////////
/// Vertex structure.
////////////////////////////////////////////////////////////
struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    ////////////////////////////////////////////////////////////
    /// GetBindingDescription.
    ////////////////////////////////////////////////////////////
    static auto GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};

        bindingDescription.binding   = 0; // Index used in attribute descriptions.
        bindingDescription.stride    = sizeof( Vertex );
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    ////////////////////////////////////////////////////////////
    /// GetAttributeDescriptions.
    ////////////////////////////////////////////////////////////
    static auto GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

        // Position.
        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof( Vertex, position );

        // Color.
        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof( Vertex, color );

        return attributeDescriptions;
    }
};

////////////////////////////////////////////////////////////
/// UniformBufferObject.
////////////////////////////////////////////////////////////
struct UniformBufferObject
{
    alignas( 16 ) glm::mat4 model;
    alignas( 16 ) glm::mat4 view;
    alignas( 16 ) glm::mat4 proj;
};

////////////////////////////////////////////////////////////
/// Vertices.
////////////////////////////////////////////////////////////
const std::vector<Vertex> Vertices = {
    { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
    { { -0.5f, 0.5f }, { 0.3f, 0.0f, 1.0f } }
};

////////////////////////////////////////////////////////////
/// Indices.
////////////////////////////////////////////////////////////
const std::vector<uint16_t> Indices = {
    0,
    1,
    2,
    2,
    3,
    0
};

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
    bool m_HasGraphicsFamily;
    bool m_HasComputeFamily;
    bool m_HasCopyFamily;
    bool m_HasPresentFamily;

    uint32_t m_GraphicsFamily;
    uint32_t m_ComputeFamily;
    uint32_t m_CopyFamily;
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
    VkQueue                      m_ComputeQueue;
    VkQueue                      m_CopyQueue;
    VkQueue                      m_PresentQueue;
    VkSwapchainKHR               m_SwapChain;
    std::vector<VkImage>         m_SwapChainImages;
    VkFormat                     m_SwapChainImageFormat;
    VkExtent2D                   m_SwapChainExtent;
    std::vector<VkImageView>     m_SwapChainImageViews;
    VkRenderPass                 m_RenderPass;
    VkDescriptorSetLayout        m_DescriptorSetLayout;
    VkDescriptorPool             m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    VkPipelineLayout             m_GraphicsPipelineLayout;
    VkPipeline                   m_GraphicsPipeline;
    std::vector<VkFramebuffer>   m_SwapChainFramebuffers;
    VkCommandPool                m_CommandPoolGraphics;
    VkCommandPool                m_CommandPoolCopy;
    VkImage                      m_TextureImage;
    VkDeviceMemory               m_TextureImageGpuMemory;
    VkBuffer                     m_VertexBuffer;
    VkDeviceMemory               m_VertexBufferGpuMemory;
    VkBuffer                     m_IndexBuffer;
    VkDeviceMemory               m_IndexBufferGpuMemory;
    std::vector<VkBuffer>        m_UniformBuffers;
    std::vector<VkDeviceMemory>  m_UniformBuffersGpuMemory;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore>     m_ImageAvailableSemaphores;
    std::vector<VkSemaphore>     m_RenderFinishedSemaphores;
    std::vector<VkFence>         m_InFlightFences;
    std::vector<VkFence>         m_ImagesInFlight;
    QueueFamilyIndices           m_QueueFamilyIndices;
    uint32_t                     m_CurrentFrame;
    bool                         m_IsFrameBufferResized;

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
        , m_ComputeQueue( VK_NULL_HANDLE )
        , m_CopyQueue( VK_NULL_HANDLE )
        , m_PresentQueue( VK_NULL_HANDLE )
        , m_SwapChain( VK_NULL_HANDLE )
        , m_SwapChainImages{}
        , m_SwapChainImageFormat( VK_FORMAT_UNDEFINED )
        , m_SwapChainExtent{}
        , m_SwapChainImageViews{}
        , m_RenderPass( VK_NULL_HANDLE )
        , m_DescriptorSetLayout( VK_NULL_HANDLE )
        , m_DescriptorPool( VK_NULL_HANDLE )
        , m_DescriptorSets{}
        , m_GraphicsPipelineLayout( VK_NULL_HANDLE )
        , m_GraphicsPipeline( VK_NULL_HANDLE )
        , m_SwapChainFramebuffers{}
        , m_CommandPoolGraphics( VK_NULL_HANDLE )
        , m_CommandPoolCopy( VK_NULL_HANDLE )
        , m_TextureImage( VK_NULL_HANDLE )
        , m_TextureImageGpuMemory( VK_NULL_HANDLE )
        , m_VertexBuffer( VK_NULL_HANDLE )
        , m_VertexBufferGpuMemory( VK_NULL_HANDLE )
        , m_IndexBuffer( VK_NULL_HANDLE )
        , m_IndexBufferGpuMemory( VK_NULL_HANDLE )
        , m_UniformBuffers{}
        , m_UniformBuffersGpuMemory{}
        , m_CommandBuffers{}
        , m_ImageAvailableSemaphores{}
        , m_RenderFinishedSemaphores{}
        , m_InFlightFences{}
        , m_ImagesInFlight{}
        , m_QueueFamilyIndices{}
        , m_CurrentFrame( 0 )
        , m_IsFrameBufferResized( false )
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

        // Initialize the window.
        m_Window = glfwCreateWindow( m_Width, m_Height, m_WindowName.c_str(), nullptr, nullptr );
        if( m_Window == nullptr )
        {
            std::cerr << "Cannot create window!" << std::endl;
            return StatusCode::Fail;
        }

        // Set a callback for window resizing.
        glfwSetWindowUserPointer( m_Window, this );
        glfwSetFramebufferSizeCallback( m_Window, static_cast<GLFWframebuffersizefun>( FrameBufferResizeCallback ) );

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

        result = CreateDescriptionSetLayout();
        if( result != StatusCode::Success )
        {
            std::cerr << "Description set layout creation failed!" << std::endl;
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

        result = CreateCommandPools();
        if( result != StatusCode::Success )
        {
            std::cerr << "Command pool creation failed!" << std::endl;
            return result;
        }

        result = CreateTextureImage();
        if( result != StatusCode::Success )
        {
            std::cerr << "Texture Image creation failed!" << std::endl;
            return result;
        }

        result = CreateVertexBuffer();
        if( result != StatusCode::Success )
        {
            std::cerr << "Vertex buffers creation failed!" << std::endl;
            return result;
        }

        result = CreateIndexBuffer();
        if( result != StatusCode::Success )
        {
            std::cerr << "Index buffers creation failed!" << std::endl;
            return result;
        }

        result = CreateUniformBuffers();
        if( result != StatusCode::Success )
        {
            std::cerr << "Uniform buffers creation failed!" << std::endl;
            return result;
        }

        result = CreateDescriptorPool();
        if( result != StatusCode::Success )
        {
            std::cerr << "Descriptor pool creation failed!" << std::endl;
            return result;
        }

        result = CreateDescriptorSets();
        if( result != StatusCode::Success )
        {
            std::cerr << "Descriptor sets creation failed!" << std::endl;
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

        result = CreateFences();
        if( result != StatusCode::Success )
        {
            std::cerr << "Fences creation failed!" << std::endl;
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
        m_QueueFamilyIndices = FindQueueFamilies( m_PhysicalDevice );

        // Set the queue priority.
        const float queuePriority = 1.0f;

        // Graphics and present queue create information.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t>                   uniqueQueueFamilies = {
            m_QueueFamilyIndices.m_GraphicsFamily,
            m_QueueFamilyIndices.m_ComputeFamily,
            m_QueueFamilyIndices.m_PresentFamily,
            m_QueueFamilyIndices.m_CopyFamily
        };

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

        // Get graphics/compute queue.
        if( m_QueueFamilyIndices.m_GraphicsFamily == m_QueueFamilyIndices.m_ComputeFamily )
        {
            vkGetDeviceQueue( m_Device, m_QueueFamilyIndices.m_GraphicsFamily, 0, &m_GraphicsQueue );
            if( m_GraphicsQueue == nullptr )
            {
                std::cerr << "Cannot obtain graphics queue!" << std::endl;
                return StatusCode::Fail;
            }

            m_ComputeQueue = m_GraphicsQueue;
        }
        else
        {
            vkGetDeviceQueue( m_Device, m_QueueFamilyIndices.m_GraphicsFamily, 0, &m_GraphicsQueue );
            if( m_GraphicsQueue == nullptr )
            {
                std::cerr << "Cannot obtain graphics queue!" << std::endl;
                return StatusCode::Fail;
            }

            vkGetDeviceQueue( m_Device, m_QueueFamilyIndices.m_ComputeFamily, 0, &m_ComputeQueue );
            if( m_ComputeQueue == nullptr )
            {
                std::cerr << "Cannot obtain compute queue!" << std::endl;
                return StatusCode::Fail;
            }
        }

        // Get copy family.
        vkGetDeviceQueue( m_Device, m_QueueFamilyIndices.m_CopyFamily, 0, &m_CopyQueue );
        if( m_CopyQueue == nullptr )
        {
            std::cerr << "Cannot obtain copy queue!" << std::endl;
            return StatusCode::Fail;
        }

        // Get present family.
        if( m_QueueFamilyIndices.m_GraphicsFamily != m_QueueFamilyIndices.m_PresentFamily )
        {
            vkGetDeviceQueue( m_Device, m_QueueFamilyIndices.m_PresentFamily, 0, &m_PresentQueue );
            if( m_PresentQueue == nullptr )
            {
                std::cerr << "Cannot obtain present queue!" << std::endl;
                return StatusCode::Fail;
            }
        }
        else
        {
            m_PresentQueue = m_GraphicsQueue;
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
            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                !indices.m_HasGraphicsFamily )
            {
                indices.m_GraphicsFamily    = index;
                indices.m_HasGraphicsFamily = true;
            }

            // Compute queue family.
            if( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT &&
                !indices.m_HasComputeFamily )
            {
                indices.m_ComputeFamily    = index;
                indices.m_HasComputeFamily = true;
            }

            // Memory transfer queue family (explicitly).
            if( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                !( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) &&
                !( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT ) &&
                !indices.m_HasCopyFamily )
            {
                indices.m_CopyFamily    = index;
                indices.m_HasCopyFamily = true;
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

        std::vector<uint32_t> queueFamilyIndices = {};

        if( m_QueueFamilyIndices.m_PresentFamily != m_QueueFamilyIndices.m_GraphicsFamily )
        {
            // Add both queues if they are in different queue families.
            queueFamilyIndices.emplace_back( m_QueueFamilyIndices.m_GraphicsFamily );
            queueFamilyIndices.emplace_back( m_QueueFamilyIndices.m_PresentFamily );
        }
        else
        {
            // Add one queue if graphics and present queues are in the same queue family.
            queueFamilyIndices.emplace_back( m_QueueFamilyIndices.m_GraphicsFamily );
        }

        if( m_QueueFamilyIndices.m_ComputeFamily != m_QueueFamilyIndices.m_GraphicsFamily )
        {
            // Add compute queue family if it is in different queue family then graphics one.
            queueFamilyIndices.emplace_back( m_QueueFamilyIndices.m_ComputeFamily );
        }

        if( m_QueueFamilyIndices.m_CopyFamily != m_QueueFamilyIndices.m_GraphicsFamily ||
            m_QueueFamilyIndices.m_CopyFamily != m_QueueFamilyIndices.m_ComputeFamily )
        {
            // Add copy queue family if it is in different queue family then graphics or compute one.
            queueFamilyIndices.emplace_back( m_QueueFamilyIndices.m_CopyFamily );
        }

        if( queueFamilyIndices.size() > 1 )
        {
            // Graphics, compute, copy and present queues are in different queue families.
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>( queueFamilyIndices.size() );
            createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
        }
        else
        {
            // Graphics, compute, copy and present queues are in the same queue family.
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // Optional.
            createInfo.pQueueFamilyIndices   = nullptr; // Optional.
        }

        // Check supported formats.
        VkImageFormatProperties imageFormatProperties = {};
        if( vkGetPhysicalDeviceImageFormatProperties(
                m_PhysicalDevice,
                createInfo.imageFormat,
                VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_OPTIMAL,
                createInfo.imageUsage,
                0,
                &imageFormatProperties ) != VK_SUCCESS )
        {
            std::cerr << "Cannot get physical device image format properties!" << std::endl;
            return StatusCode::Fail;
        }

        if( imageFormatProperties.maxExtent.height < extent.height || imageFormatProperties.maxExtent.width < extent.width )
        {
            std::cerr << "Image extent is bigger than max extent!" << std::endl;
            return StatusCode::Fail;
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
    /// Creates description set layout.
    ////////////////////////////////////////////////////////////
    StatusCode CreateDescriptionSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};

        uboLayoutBinding.binding            = 0;
        uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount    = 1;
        uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional.

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};

        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &uboLayoutBinding;

        if( vkCreateDescriptorSetLayout( m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create description set layout!" << std::endl;
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
        const auto bindingDescription    = Vertex::GetBindingDescription();
        const auto attributeDescriptions = Vertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount        = 1;
        vertexInputInfo.pVertexBindingDescriptions           = &bindingDescription; // Optional.
        vertexInputInfo.vertexAttributeDescriptionCount      = static_cast<uint32_t>( attributeDescriptions.size() );
        vertexInputInfo.pVertexAttributeDescriptions         = attributeDescriptions.data(); // Optional.

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
        rasterizer.frontFace                              = VK_FRONT_FACE_CLOCKWISE; // VK_FRONT_FACE_COUNTER_CLOCKWISE
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
        pipelineLayoutInfo.setLayoutCount             = 1;
        pipelineLayoutInfo.pSetLayouts                = &m_DescriptorSetLayout;
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
    /// Creates command pools.
    ////////////////////////////////////////////////////////////
    StatusCode CreateCommandPools()
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex        = m_QueueFamilyIndices.m_GraphicsFamily;
        poolInfo.flags                   = 0; // Optional.

        // Graphics command pool.
        if( vkCreateCommandPool( m_Device, &poolInfo, nullptr, &m_CommandPoolGraphics ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create command pool!" << std::endl;
            return StatusCode::Fail;
        }

        // Copy command pool.
        poolInfo.queueFamilyIndex = m_QueueFamilyIndices.m_CopyFamily;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // Short-lived command buffers.

        if( vkCreateCommandPool( m_Device, &poolInfo, nullptr, &m_CommandPoolCopy ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create command pool!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates a buffer.
    ////////////////////////////////////////////////////////////
    StatusCode CreateBuffer(
        const VkDeviceSize          size,
        const VkBufferUsageFlags    usage,
        const VkMemoryPropertyFlags properties,
        VkBuffer&                   buffer,
        VkDeviceMemory&             bufferGpuMemory )
    {
        VkBufferCreateInfo bufferInfo = {};

        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = size;
        bufferInfo.usage       = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if( vkCreateBuffer( m_Device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create vertex buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Get gpu memory requirements for the vertex buffer.
        VkMemoryRequirements gpuMemoryRequirements = {};

        vkGetBufferMemoryRequirements( m_Device, buffer, &gpuMemoryRequirements );

        // Allocate gpu memory for the vertex buffer.
        VkMemoryAllocateInfo allocationInfo = {};

        allocationInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize  = gpuMemoryRequirements.size;
        allocationInfo.memoryTypeIndex = FindGpuMemoryType(
            gpuMemoryRequirements.memoryTypeBits,
            properties );

        if( allocationInfo.memoryTypeIndex == UINT32_MAX )
        {
            std::cerr << "Cannot find gpu memory type!" << std::endl;
            return StatusCode::Fail;
        }

        if( vkAllocateMemory( m_Device, &allocationInfo, nullptr, &bufferGpuMemory ) != VK_SUCCESS )
        {
            std::cerr << "Failed to allocate vertex buffer memory!" << std::endl;
            return StatusCode::Fail;
        }

        // Bind allocated gpu memory with the vertex buffer.
        const VkDeviceSize gpuMemoryOffset = 0;

        // if( gpuMemoryOffset != 0 )
        // {
        //     if( gpuMemoryOffset % gpuMemoryRequirements.alignment != 0 )
        //     {
        //         std::cerr << "Gpu memory offset is not aligned!" << std::endl;
        //         return StatusCode::Fail;
        //     }
        // }

        if( vkBindBufferMemory( m_Device, buffer, bufferGpuMemory, gpuMemoryOffset ) != VK_SUCCESS )
        {
            std::cerr << "Cannot bind buffer gpu memory!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Begins single time gpu commands within command buffer.
    ////////////////////////////////////////////////////////////
    VkCommandBuffer BeginSingleTimeCommands( const bool isCopyQueueIsUsed )
    {
        VkCommandBufferAllocateInfo allocationInfo = {};
        VkCommandPool               commandPool    = isCopyQueueIsUsed ? m_CommandPoolCopy : m_CommandPoolGraphics;

        allocationInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocationInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocationInfo.commandPool        = commandPool;
        allocationInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        vkAllocateCommandBuffers( m_Device, &allocationInfo, &commandBuffer );

        VkCommandBufferBeginInfo beginInfo = {};

        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Tells the driver that our intent is to use the command buffer once
                                                                       // and wait with returning from the function until the copy operation has finished executing.

        vkBeginCommandBuffer( commandBuffer, &beginInfo );

        return commandBuffer;
    }

    ////////////////////////////////////////////////////////////
    /// End single time gpu commands within command buffer.
    ////////////////////////////////////////////////////////////
    void EndSingleTimeCommands( const bool isCopyQueueIsUsed, VkCommandBuffer commandBuffer )
    {
        vkEndCommandBuffer( commandBuffer );

        VkSubmitInfo  submitInfo  = {};
        VkQueue       queue       = isCopyQueueIsUsed ? m_CopyQueue : m_GraphicsQueue;
        VkCommandPool commandPool = isCopyQueueIsUsed ? m_CommandPoolCopy : m_CommandPoolGraphics;

        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit( queue, 1, &submitInfo, VK_NULL_HANDLE );

        // Wait for the queue to become idle.
        vkQueueWaitIdle( queue );

        vkFreeCommandBuffers( m_Device, commandPool, 1, &commandBuffer );
    }

    ////////////////////////////////////////////////////////////
    /// Copies data from one buffer to another.
    ////////////////////////////////////////////////////////////
    void CopyBuffer( const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size )
    {
        const bool      isCopyQueueIsUsed = true;
        VkCommandBuffer commandBuffer     = BeginSingleTimeCommands( isCopyQueueIsUsed );

        VkBufferCopy copyRegion = {};

        copyRegion.srcOffset = 0; // Optional.
        copyRegion.dstOffset = 0; // Optional.
        copyRegion.size      = size;

        vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

        EndSingleTimeCommands( isCopyQueueIsUsed, commandBuffer );
    }

    ////////////////////////////////////////////////////////////
    /// Handles image layout transitions .
    ////////////////////////////////////////////////////////////
    StatusCode TransitionImageLayout(
        const VkImage image,
        const VkFormat /* format*/,
        const VkImageLayout oldLayout,
        const VkImageLayout newLayout )
    {
        const bool      isCopyQueueIsUsed = false;
        VkCommandBuffer commandBuffer     = BeginSingleTimeCommands( isCopyQueueIsUsed );

        VkImageMemoryBarrier barrier          = {};
        VkPipelineStageFlags sourceStage      = 0;
        VkPipelineStageFlags destinationStage = 0;

        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = oldLayout;
        barrier.newLayout                       = newLayout;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            std::cerr << "Unsupported layout transition!" << std::endl;
            return StatusCode::Fail;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier );

        EndSingleTimeCommands( isCopyQueueIsUsed, commandBuffer );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Copies data from a buffer to an image.
    ////////////////////////////////////////////////////////////
    void CopyBufferToImage(
        const VkBuffer buffer,
        const VkImage  image,
        const uint32_t width,
        const uint32_t height )
    {
        const bool      isCopyQueueIsUsed = true;
        VkCommandBuffer commandBuffer     = BeginSingleTimeCommands( isCopyQueueIsUsed );

        VkBufferImageCopy region = {};

        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region );

        EndSingleTimeCommands( isCopyQueueIsUsed, commandBuffer );
    }

    ////////////////////////////////////////////////////////////
    /// Creates texture image.
    ////////////////////////////////////////////////////////////
    StatusCode CreateImage(
        const uint32_t              textureWidth,
        const uint32_t              textureHeight,
        const VkFormat              format,
        const VkImageTiling         tiling,
        const VkImageUsageFlags     usage,
        const VkMemoryPropertyFlags properties,
        VkImage&                    image,
        VkDeviceMemory&             imageMemory )
    {
        VkImageCreateInfo imageInfo = {};

        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = textureWidth;
        imageInfo.extent.height = textureHeight;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usage;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags         = 0; // Optional.

        if( vkCreateImage( m_Device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create image!" << std::endl;
            return StatusCode::Fail;
        }

        VkMemoryRequirements gpuMemoryRequirements = {};

        vkGetImageMemoryRequirements( m_Device, image, &gpuMemoryRequirements );

        VkMemoryAllocateInfo allocationInfo = {};

        allocationInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize  = gpuMemoryRequirements.size;
        allocationInfo.memoryTypeIndex = FindGpuMemoryType( gpuMemoryRequirements.memoryTypeBits, properties );

        if( vkAllocateMemory( m_Device, &allocationInfo, nullptr, &imageMemory ) != VK_SUCCESS )
        {
            std::cerr << "Cannot allocate image memory!" << std::endl;
            return StatusCode::Fail;
        }

        vkBindImageMemory( m_Device, image, imageMemory, 0 );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates texture image.
    ////////////////////////////////////////////////////////////
    StatusCode CreateTextureImage()
    {
        constexpr uint32_t bytesPerPixel   = 4;
        int32_t            textureWidth    = 0;
        int32_t            textureHeight   = 0;
        int32_t            textureChannels = 0;
        std::string        textureFileName = "Textures/texture.jpg";
        StatusCode         result          = StatusCode::Success;

        Pixels* pixels = StbImage::loadRgba( textureFileName, textureWidth, textureHeight, textureChannels );

        if( pixels == nullptr )
        {
            std::cerr << "Failed to load texture image!" << std::endl;
            return StatusCode::Fail;
        }

        const VkDeviceSize imageSize = textureWidth * textureHeight * bytesPerPixel;

        VkBuffer       stagingBuffer          = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferGpuMemory = VK_NULL_HANDLE;

        result = CreateBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferGpuMemory );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create staging buffer for texture image!" << std::endl;
            return StatusCode::Fail;
        }

        // Copy the texture to staging buffer.
        void* data = nullptr;

        vkMapMemory( m_Device, stagingBufferGpuMemory, 0, imageSize, 0, &data );
        memcpy( data, pixels, static_cast<size_t>( imageSize ) );
        vkUnmapMemory( m_Device, stagingBufferGpuMemory );

        // Free texture.
        StbImage::unloadRbga( pixels );

        // Create image.
        result = CreateImage(
            textureWidth,
            textureHeight,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_TextureImage,
            m_TextureImageGpuMemory );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create image for texture image!" << std::endl;
            return StatusCode::Fail;
        }

        // Transition image layout from undefined to transfer destination.
        result = TransitionImageLayout(
            m_TextureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot handle image layout transition!" << std::endl;
            return StatusCode::Fail;
        }

        // Copy buffer to image.
        CopyBufferToImage(
            stagingBuffer,
            m_TextureImage,
            static_cast<uint32_t>( textureWidth ),
            static_cast<uint32_t>( textureHeight ) );

        // Transition image layout from transfer destination to shader read only.
        result = TransitionImageLayout(
            m_TextureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot handle image layout transition!" << std::endl;
            return StatusCode::Fail;
        }

        vkDestroyBuffer( m_Device, stagingBuffer, nullptr );
        vkFreeMemory( m_Device, stagingBufferGpuMemory, nullptr );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates vertex buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateVertexBuffer()
    {
        StatusCode         result                 = StatusCode::Success;
        VkBuffer           stagingBuffer          = VK_NULL_HANDLE;
        VkDeviceMemory     stagingBufferGpuMemory = VK_NULL_HANDLE;
        const VkDeviceSize bufferSize             = sizeof( Vertices[0] ) * Vertices.size();

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferGpuMemory );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create staging buffer for vertex buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Fill the vertex buffer.
        void* data = nullptr;

        vkMapMemory( m_Device, stagingBufferGpuMemory, 0, bufferSize, 0, &data );
        memcpy_s( data, bufferSize, Vertices.data(), bufferSize );
        vkUnmapMemory( m_Device, stagingBufferGpuMemory );

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_VertexBuffer,
            m_VertexBufferGpuMemory );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create buffer for vertex buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Copy data from the staging buffer to vertex buffer.
        CopyBuffer( stagingBuffer, m_VertexBuffer, bufferSize );

        vkDestroyBuffer( m_Device, stagingBuffer, nullptr );
        vkFreeMemory( m_Device, stagingBufferGpuMemory, nullptr );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates index buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateIndexBuffer()
    {
        StatusCode         result                 = StatusCode::Success;
        VkBuffer           stagingBuffer          = VK_NULL_HANDLE;
        VkDeviceMemory     stagingBufferGpuMemory = VK_NULL_HANDLE;
        const VkDeviceSize bufferSize             = sizeof( Indices[0] ) * Indices.size();

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferGpuMemory );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create staging buffer for index buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Fill the index buffer.
        void* data = nullptr;

        vkMapMemory( m_Device, stagingBufferGpuMemory, 0, bufferSize, 0, &data );
        memcpy_s( data, bufferSize, Indices.data(), bufferSize );
        vkUnmapMemory( m_Device, stagingBufferGpuMemory );

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_IndexBuffer,
            m_IndexBufferGpuMemory );

        // Copy data from the staging buffer to index buffer.
        CopyBuffer( stagingBuffer, m_IndexBuffer, bufferSize );

        vkDestroyBuffer( m_Device, stagingBuffer, nullptr );
        vkFreeMemory( m_Device, stagingBufferGpuMemory, nullptr );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates uniform buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateUniformBuffers()
    {
        const uint32_t     swaChainImageCount = static_cast<uint32_t>( m_SwapChainImages.size() );
        const VkDeviceSize bufferSize         = sizeof( UniformBufferObject );

        m_UniformBuffers.resize( swaChainImageCount );
        m_UniformBuffersGpuMemory.resize( swaChainImageCount );

        for( uint32_t i = 0; i < swaChainImageCount; ++i )
        {
            const StatusCode result = CreateBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersGpuMemory[i] );
            if( result != StatusCode::Success )
            {
                std::cerr << "Cannot create buffer for uniform buffer!" << std::endl;
                return StatusCode::Fail;
            }
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates descriptor pool.
    ////////////////////////////////////////////////////////////
    StatusCode CreateDescriptorPool()
    {
        const uint32_t             descriptorCount = static_cast<uint32_t>( m_SwapChainImages.size() );
        VkDescriptorPoolSize       poolSize        = {};
        VkDescriptorPoolCreateInfo poolInfo        = {};

        poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = descriptorCount;

        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSize;
        poolInfo.maxSets       = descriptorCount;

        if( vkCreateDescriptorPool( m_Device, &poolInfo, nullptr, &m_DescriptorPool ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create descriptor pool!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates descriptor sets.
    ////////////////////////////////////////////////////////////
    StatusCode CreateDescriptorSets()
    {
        const uint32_t                     descriptorSetCount = static_cast<uint32_t>( m_SwapChainImages.size() );
        VkDescriptorSetAllocateInfo        allocationInfo     = {};
        std::vector<VkDescriptorSetLayout> layouts( descriptorSetCount, m_DescriptorSetLayout );

        allocationInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocationInfo.descriptorPool     = m_DescriptorPool;
        allocationInfo.descriptorSetCount = descriptorSetCount;
        allocationInfo.pSetLayouts        = layouts.data();

        m_DescriptorSets.resize( descriptorSetCount );

        if( vkAllocateDescriptorSets( m_Device, &allocationInfo, m_DescriptorSets.data() ) != VK_SUCCESS )
        {
            std::cerr << "Cannot allocate descriptor sets!" << std::endl;
            return StatusCode::Fail;
        }

        for( uint32_t i = 0; i < descriptorSetCount; ++i )
        {
            VkDescriptorBufferInfo bufferInfo = {};

            bufferInfo.buffer = m_UniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range  = sizeof( UniformBufferObject );

            VkWriteDescriptorSet descriptorWrite = {};

            descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet           = m_DescriptorSets[i];
            descriptorWrite.dstBinding       = 0;
            descriptorWrite.dstArrayElement  = 0;
            descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount  = 1;
            descriptorWrite.pBufferInfo      = &bufferInfo;
            descriptorWrite.pImageInfo       = nullptr; // Optional.
            descriptorWrite.pTexelBufferView = nullptr; // Optional.

            vkUpdateDescriptorSets( m_Device, 1, &descriptorWrite, 0, nullptr );
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Finds gpu memory type.
    ////////////////////////////////////////////////////////////
    uint32_t FindGpuMemoryType( const uint32_t typeFilter, const VkMemoryPropertyFlags properties )
    {
        VkPhysicalDeviceMemoryProperties gpuMemoryProperties = {};

        // Get gpu memory properties.
        vkGetPhysicalDeviceMemoryProperties( m_PhysicalDevice, &gpuMemoryProperties );

        for( uint32_t i = 0; i < gpuMemoryProperties.memoryTypeCount; i++ )
        {
            if( ( typeFilter & ( 1 << i ) ) &&
                ( gpuMemoryProperties.memoryTypes[i].propertyFlags & properties ) == properties )
            {
                return i;
            }
        }

        std::cerr << "Failed to find suitable memory type!" << std::endl;

        return UINT32_MAX;
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
        allocInfo.commandPool                 = m_CommandPoolGraphics;
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

            // Bind the vertex buffers.
            const VkBuffer     vertexBuffers[] = { m_VertexBuffer };
            const VkDeviceSize offsets[]       = { 0 };
            vkCmdBindVertexBuffers( m_CommandBuffers[i], 0, 1, vertexBuffers, offsets );

            // Bind the index buffer.
            vkCmdBindIndexBuffer( m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16 );

            // Bind the descriptor sets.
            vkCmdBindDescriptorSets( m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr );

            // Draw.
            //vkCmdDraw( m_CommandBuffers[i], static_cast<uint32_t>( Vertices.size() ), 1, 0, 0 );
            vkCmdDrawIndexed( m_CommandBuffers[i], static_cast<uint32_t>( Indices.size() ), 1, 0, 0, 0 );

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
    /// Recreates swap chain.
    ////////////////////////////////////////////////////////////
    StatusCode RecreateSwapChain()
    {
        int32_t width  = 0;
        int32_t height = 0;

        glfwGetFramebufferSize( m_Window, &width, &height );

        while( width == 0 || height == 0 )
        {
            // Pause until the windows is minimized.
            glfwGetFramebufferSize( m_Window, &width, &height );
            glfwWaitEvents();
        }

        if( vkDeviceWaitIdle( m_Device ) != VK_SUCCESS )
        {
            std::cerr << "Failed to waiting for device!" << std::endl;
            return StatusCode::Fail;
        }

        CleanupSwapChain();

        StatusCode result = StatusCode::Success;

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

        result = CreateUniformBuffers();
        if( result != StatusCode::Success )
        {
            std::cerr << "Unifrom buffers creation failed!" << std::endl;
            return result;
        }

        result = CreateDescriptorPool();
        if( result != StatusCode::Success )
        {
            std::cerr << "Descriptor pool creation failed!" << std::endl;
            return result;
        }

        result = CreateDescriptorSets();
        if( result != StatusCode::Success )
        {
            std::cerr << "Descriptor sets creation failed!" << std::endl;
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

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates semaphores to perform gpu-gpu synchronization.
    ////////////////////////////////////////////////////////////
    StatusCode CreateSemaphores()
    {
        m_ImageAvailableSemaphores.resize( MaxFramesInFlight );
        m_RenderFinishedSemaphores.resize( MaxFramesInFlight );

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for( uint32_t i = 0; i < MaxFramesInFlight; ++i )
        {
            if( vkCreateSemaphore( m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i] ) != VK_SUCCESS ||
                vkCreateSemaphore( m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i] ) != VK_SUCCESS )
            {
                std::cerr << "Failed to create semaphores!" << std::endl;
                return StatusCode::Fail;
            }
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates fences to perform cpu-gpu synchronization.
    ////////////////////////////////////////////////////////////
    StatusCode CreateFences()
    {
        m_InFlightFences.resize( MaxFramesInFlight );
        m_ImagesInFlight.resize( m_SwapChainImages.size(), VK_NULL_HANDLE );

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

        for( auto& inFlightFence : m_InFlightFences )
        {
            if( vkCreateFence( m_Device, &fenceInfo, nullptr, &inFlightFence ) != VK_SUCCESS )
            {
                std::cerr << "Failed to create fences!" << std::endl;
                return StatusCode::Fail;
            }
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Updates uniform buffer.
    ////////////////////////////////////////////////////////////
    void UpdateUniformBuffer( const uint32_t currentImage )
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        const auto  currentTime = std::chrono::high_resolution_clock::now();
        const float time        = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

        UniformBufferObject ubo = {};

        // Rotate only z-axis.
        ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

        // Eye position, center position, up axis.
        ubo.view = glm::lookAt( glm::vec3( 2.0f, 2.0f, 2.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, -1.0f ) );

        // Field of view, aspect ratio, near view plane, far view plane.
        ubo.proj = glm::perspective( glm::radians( 45.0f ), m_SwapChainExtent.width / static_cast<float>( m_SwapChainExtent.height ), 0.1f, 10.0f );

        //ubo.proj[1][1] *= -1;

        // Copy data to buffer.
        void* data = nullptr;

        vkMapMemory( m_Device, m_UniformBuffersGpuMemory[currentImage], 0, sizeof( ubo ), 0, &data );
        memcpy( data, &ubo, sizeof( ubo ) );
        vkUnmapMemory( m_Device, m_UniformBuffersGpuMemory[currentImage] );
    }

    ////////////////////////////////////////////////////////////
    /// Draws a frame.
    ////////////////////////////////////////////////////////////
    StatusCode DrawFrame()
    {
        VkResult result = VK_SUCCESS;

        // Wait for fences.
        vkWaitForFences( m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX );

        // Acquire an image from the swap chain.
        uint32_t imageIndex = 0;
        result              = vkAcquireNextImageKHR( m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex );

        // Check swap chain status and recreate it if needed.
        if( result == VK_ERROR_OUT_OF_DATE_KHR )
        {
            RecreateSwapChain();
            return StatusCode::Success;
        }
        else if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) // Suboptimal swap chain is ok.
        {
            std::cerr << "Failed to acquire swap chain image!" << std::endl;
            return StatusCode::Fail;
        }

        // Update uniform buffer.
        UpdateUniformBuffer( imageIndex );

        // Check if a previous frame is using this image (i.e. there is its fence to wait on).
        if( m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE )
        {
            vkWaitForFences( m_Device, 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX );
        }

        // Mark the image as now being in use by this frame
        m_ImagesInFlight[imageIndex] = m_InFlightFences[m_CurrentFrame];

        // Submit the command buffer.
        VkSubmitInfo         submitInfo = {};
        VkPipelineStageFlags waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &m_ImageAvailableSemaphores[m_CurrentFrame];
        submitInfo.pWaitDstStageMask    = &waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &m_CommandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &m_RenderFinishedSemaphores[m_CurrentFrame];

        // Reset fences before using them.
        vkResetFences( m_Device, 1, &m_InFlightFences[m_CurrentFrame] );

        if( vkQueueSubmit( m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame] ) != VK_SUCCESS )
        {
            std::cerr << "Failed to submit draw command buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Present the swap chain.
        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &m_RenderFinishedSemaphores[m_CurrentFrame];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &m_SwapChain;
        presentInfo.pImageIndices      = &imageIndex;
        presentInfo.pResults           = nullptr; // Optional.

        result = vkQueuePresentKHR( m_PresentQueue, &presentInfo );

        // Check swap chain status and recreate it if needed.
        if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_IsFrameBufferResized )
        {
            m_IsFrameBufferResized = false;
            RecreateSwapChain();
        }
        else if( result != VK_SUCCESS )
        {
            std::cerr << "Failed to present swap chain image!" << std::endl;
            return StatusCode::Fail;
        }

        m_CurrentFrame = ( m_CurrentFrame + 1 ) % MaxFramesInFlight;

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
#else
        (void) createInfo;
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
    /// Cleanups swap chain.
    ////////////////////////////////////////////////////////////
    void CleanupSwapChain()
    {
        // Destroy frame buffers.
        for( auto& framebuffer : m_SwapChainFramebuffers )
        {
            vkDestroyFramebuffer( m_Device, framebuffer, nullptr );
        }

        // Destroy uniform buffers.
        for( auto& uniformBuffer : m_UniformBuffers )
        {
            vkDestroyBuffer( m_Device, uniformBuffer, nullptr );
        }

        // Free uniform buffers gpu memory.
        for( auto& uniformBufferGpuMemory : m_UniformBuffersGpuMemory )
        {
            vkFreeMemory( m_Device, uniformBufferGpuMemory, nullptr );
        }

        // Destroy descriptor pool.
        vkDestroyDescriptorPool( m_Device, m_DescriptorPool, nullptr );

        // Free command buffers.
        vkFreeCommandBuffers( m_Device, m_CommandPoolGraphics, static_cast<uint32_t>( m_CommandBuffers.size() ), m_CommandBuffers.data() );

        // Destroy graphics pipeline.
        vkDestroyPipeline( m_Device, m_GraphicsPipeline, nullptr );

        // Destroy graphics pipeline layout.
        vkDestroyPipelineLayout( m_Device, m_GraphicsPipelineLayout, nullptr );

        // Destroy render pass.
        vkDestroyRenderPass( m_Device, m_RenderPass, nullptr );

        // Destroy image views.
        for( auto& imageView : m_SwapChainImageViews )
        {
            vkDestroyImageView( m_Device, imageView, nullptr );
        }

        // Destroy swap chain.
        vkDestroySwapchainKHR( m_Device, m_SwapChain, nullptr );
    }

    ////////////////////////////////////////////////////////////
    /// Cleanups Vulkan api.
    ////////////////////////////////////////////////////////////
    StatusCode CleanupVulkan()
    {
        CleanupSwapChain();

        // Destroy image.
        vkDestroyImage( m_Device, m_TextureImage, nullptr );

        // Free gpu memory associated with destroyed image.
        vkFreeMemory( m_Device, m_TextureImageGpuMemory, nullptr );

        // Destroy description set layout.
        vkDestroyDescriptorSetLayout( m_Device, m_DescriptorSetLayout, nullptr );

        // Destroy index buffer.
        vkDestroyBuffer( m_Device, m_IndexBuffer, nullptr );

        // Free gpu memory associated with destroyed index buffer.
        vkFreeMemory( m_Device, m_IndexBufferGpuMemory, nullptr );

        // Destroy vertex buffer.
        vkDestroyBuffer( m_Device, m_VertexBuffer, nullptr );

        // Free gpu memory associated with destroyed vertex buffer.
        vkFreeMemory( m_Device, m_VertexBufferGpuMemory, nullptr );

        for( auto& inFlightFence : m_InFlightFences )
        {
            vkDestroyFence( m_Device, inFlightFence, nullptr );
        }

        for( auto& renderFinishedSemaphore : m_RenderFinishedSemaphores )
        {
            vkDestroySemaphore( m_Device, renderFinishedSemaphore, nullptr );
        }

        for( auto& imageAvailableSemaphore : m_ImageAvailableSemaphores )
        {
            vkDestroySemaphore( m_Device, imageAvailableSemaphore, nullptr );
        }

        vkDestroyCommandPool( m_Device, m_CommandPoolCopy, nullptr );

        vkDestroyCommandPool( m_Device, m_CommandPoolGraphics, nullptr );

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
            std::cerr << "[VULKAN] "
                      << callbackData->pMessage
                      << std::endl
                      << std::endl;
        }
#else
        (void) messageSeverity;
        (void) callbackData;
#endif
        (void) messageType;
        (void) userData;

        return VK_FALSE;
    }

    ////////////////////////////////////////////////////////////
    /// Frame buffer resize callback.
    ////////////////////////////////////////////////////////////
    static void __stdcall FrameBufferResizeCallback(
        GLFWwindow* window,
        int32_t     width,
        int32_t     height )
    {
        auto app                    = reinterpret_cast<Application*>( glfwGetWindowUserPointer( window ) );
        app->m_IsFrameBufferResized = true;

        (void) width;
        (void) height;
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
