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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "main.h"

#include "stb_image_library.h"
#include "tiny_obj_loader_library.h"

#ifdef _DEBUG
constexpr bool EnableBestPracticesValidation = false;
#endif

constexpr uint64_t Kilobyte          = 1024;
constexpr uint64_t Megabyte          = 1024 * Kilobyte;
constexpr uint32_t MaxFramesInFlight = 2;

////////////////////////////////////////////////////////////
/// GetBindingDescription.
////////////////////////////////////////////////////////////
auto Vertex::GetBindingDescription()
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
auto Vertex::GetAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    // Position.
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof( Vertex, position );

    // Color.
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof( Vertex, color );

    // Texture coordinates.
    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof( Vertex, textureCoordinate );

    return attributeDescriptions;
}

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
/* const std::vector<Vertex> Vertices = {
    // First square.
    { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
    { { -0.5f, 0.5f, 0.0f }, { 0.3f, 0.0f, 1.0f }, { 1.0f, 1.0f } },

    // Second square.
    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 0.3f, 0.0f, 1.0f }, { 0.0f, 0.0f } }
};*/
std::vector<Vertex> Vertices = {};

////////////////////////////////////////////////////////////
/// Indices.
////////////////////////////////////////////////////////////
/* const std::vector<uint32_t> Indices = {
    // First square.
    0,
    1,
    2,
    2,
    3,
    0,

    // Second square.
    4,
    5,
    6,
    6,
    7,
    4
};*/
std::vector<uint32_t> Indices = {};

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
    VkInstance                                     m_Instance;
    VkSurfaceKHR                                   m_Surface;
    VkPhysicalDevice                               m_PhysicalDevice;
    VkPhysicalDeviceFeatures                       m_PhysicalDeviceFeatures;
    VkDevice                                       m_Device;
    VkQueue                                        m_GraphicsQueue;
    VkQueue                                        m_ComputeQueue;
    VkQueue                                        m_CopyQueue;
    VkQueue                                        m_PresentQueue;
    VkSwapchainKHR                                 m_SwapChain;
    std::vector<VkImage>                           m_SwapChainImages;
    VkFormat                                       m_SwapChainImageFormat;
    VkExtent2D                                     m_SwapChainExtent;
    std::vector<VkImageView>                       m_SwapChainImageViews;
    VkRenderPass                                   m_RenderPass;
    VkDescriptorSetLayout                          m_DescriptorSetLayout;
    VkDescriptorPool                               m_DescriptorPool;
    std::vector<VkDescriptorSet>                   m_DescriptorSets;
    VkPipelineLayout                               m_GraphicsPipelineLayout;
    VkPipeline                                     m_GraphicsPipeline;
    std::vector<VkFramebuffer>                     m_SwapChainFramebuffers;
    VkCommandPool                                  m_CommandPoolGraphics;
    VkCommandPool                                  m_CommandPoolCopy;
    VkImage                                        m_TextureImage;
    std::pair<uint32_t, VkDeviceSize>              m_TextureImageGpuMemoryOffset;
    VkImageView                                    m_TextureImageView;
    VkSampler                                      m_TextureSampler;
    VkBuffer                                       m_VertexBuffer;
    std::pair<uint32_t, VkDeviceSize>              m_VertexBufferGpuMemoryOffset;
    VkBuffer                                       m_IndexBuffer;
    std::pair<uint32_t, VkDeviceSize>              m_IndexBufferGpuMemoryOffset;
    VkImage                                        m_DepthImage;
    std::pair<uint32_t, VkDeviceSize>              m_DepthImageGpuMemoryOffset;
    VkImageView                                    m_DepthImageView;
    std::vector<VkDeviceMemory>                    m_BufferGpuMemoryLocal;
    std::vector<VkDeviceSize>                      m_BufferGpuMemoryLocalSize;
    std::vector<VkDeviceSize>                      m_BufferGpuMemoryLocalUsage;
    std::vector<VkDeviceMemory>                    m_BufferGpuMemoryCpuVisible;
    std::vector<VkDeviceSize>                      m_BufferGpuMemoryCpuVisibleSize;
    std::vector<VkDeviceSize>                      m_BufferGpuMemoryCpuVisibleUsage;
    std::vector<VkBuffer>                          m_UniformBuffers;
    std::vector<std::pair<uint32_t, VkDeviceSize>> m_UniformBuffersGpuMemoryOffsets;
    std::vector<VkCommandBuffer>                   m_CommandBuffers;
    std::vector<VkSemaphore>                       m_ImageAvailableSemaphores;
    std::vector<VkSemaphore>                       m_RenderFinishedSemaphores;
    std::vector<VkFence>                           m_InFlightFences;
    std::vector<VkFence>                           m_ImagesInFlight;
    QueueFamilyIndices                             m_QueueFamilyIndices;
    uint32_t                                       m_CurrentFrame;
    bool                                           m_IsFrameBufferResized;
    float                                          m_MaxSamplerAnisotropy;

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
        , m_TextureImageGpuMemoryOffset{}
        , m_TextureImageView( VK_NULL_HANDLE )
        , m_TextureSampler( VK_NULL_HANDLE )
        , m_VertexBuffer( VK_NULL_HANDLE )
        , m_VertexBufferGpuMemoryOffset{}
        , m_IndexBuffer( VK_NULL_HANDLE )
        , m_IndexBufferGpuMemoryOffset{}
        , m_BufferGpuMemoryLocal{}
        , m_BufferGpuMemoryLocalSize{}
        , m_BufferGpuMemoryLocalUsage{}
        , m_BufferGpuMemoryCpuVisible{}
        , m_BufferGpuMemoryCpuVisibleSize{}
        , m_BufferGpuMemoryCpuVisibleUsage{}
        , m_UniformBuffers{}
        , m_UniformBuffersGpuMemoryOffsets{}
        , m_CommandBuffers{}
        , m_ImageAvailableSemaphores{}
        , m_RenderFinishedSemaphores{}
        , m_InFlightFences{}
        , m_ImagesInFlight{}
        , m_QueueFamilyIndices{}
        , m_CurrentFrame( 0 )
        , m_IsFrameBufferResized( false )
        , m_MaxSamplerAnisotropy( 1.0f )
        , m_PhysicalDeviceExtensions{}
    {
        m_PhysicalDeviceExtensions.emplace_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

#ifdef _DEBUG
        m_ValidationLayers.emplace_back( "VK_LAYER_KHRONOS_validation" );
        m_DebugMessenger = VK_NULL_HANDLE;
#endif

        m_BufferGpuMemoryLocal.emplace_back( VK_NULL_HANDLE );
        m_BufferGpuMemoryLocalSize.emplace_back( 0 );
        m_BufferGpuMemoryLocalUsage.emplace_back( 0 );
        m_BufferGpuMemoryCpuVisible.emplace_back( VK_NULL_HANDLE );
        m_BufferGpuMemoryCpuVisibleSize.emplace_back( 0 );
        m_BufferGpuMemoryCpuVisibleUsage.emplace_back( 0 );
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

        result = LoadModel();
        if( result != StatusCode::Success )
        {
            std::cerr << "Loading model creation failed!" << std::endl;
            return result;
        }

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

        result = CreateDescriptorSetLayout();
        if( result != StatusCode::Success )
        {
            std::cerr << "Descriptor set layout creation failed!" << std::endl;
            return result;
        }

        result = CreateGraphicsPipeline();
        if( result != StatusCode::Success )
        {
            std::cerr << "Graphics pipeline creation failed!" << std::endl;
            return result;
        }

        result = CreateCommandPools();
        if( result != StatusCode::Success )
        {
            std::cerr << "Command pool creation failed!" << std::endl;
            return result;
        }

        result = CreateDepthResources();
        if( result != StatusCode::Success )
        {
            std::cerr << "Depth resources creation failed!" << std::endl;
            return result;
        }

        result = CreateFramebuffers();
        if( result != StatusCode::Success )
        {
            std::cerr << "Frame buffers creation failed!" << std::endl;
            return result;
        }

        result = CreateTextureImage();
        if( result != StatusCode::Success )
        {
            std::cerr << "Texture image creation failed!" << std::endl;
            return result;
        }

        result = CreateTextureImageView();
        if( result != StatusCode::Success )
        {
            std::cerr << "Texture image view creation failed!" << std::endl;
            return result;
        }

        result = CreateTextureSampler();
        if( result != StatusCode::Success )
        {
            std::cerr << "Texture sampler creation failed!" << std::endl;
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
    /// Loads a model.
    ////////////////////////////////////////////////////////////
    StatusCode LoadModel()
    {
        return TinyObjLoader::loadModel( "Models/viking_room.obj", Vertices, Indices )
            ? StatusCode::Success
            : StatusCode::Fail;
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

        if( EnableBestPracticesValidation )
        {
            const VkValidationFeatureEnableEXT enables  = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;
            VkValidationFeaturesEXT            features = {};

            features.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            features.enabledValidationFeatureCount = 1;
            features.pEnabledValidationFeatures    = &enables;

            createInfo.pNext = &features;
        }
        else
        {
            createInfo.pNext = &debugCreateInfo;
        }
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
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
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

        // Get max sampler anisotropy.
        if( m_PhysicalDeviceFeatures.samplerAnisotropy )
        {
            m_MaxSamplerAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
        }

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
    /// Finds a given supported format.
    ////////////////////////////////////////////////////////////
    VkFormat FindSupportedFormat(
        const std::vector<VkFormat>& candidates,
        const VkImageTiling          tiling,
        const VkFormatFeatureFlags   features )
    {
        // List of candidate formats in order from most desirable to least desirable.
        for( VkFormat format : candidates )
        {
            VkFormatProperties props = {};
            vkGetPhysicalDeviceFormatProperties( m_PhysicalDevice, format, &props );

            if( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
            {
                return format;
            }
            else if( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
            {
                return format;
            }
        }

        return VK_FORMAT_UNDEFINED;
    }

    ////////////////////////////////////////////////////////////
    /// Finds a supported depth format.
    ////////////////////////////////////////////////////////////
    VkFormat FindDepthFormat()
    {
        return FindSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
    }

    ////////////////////////////////////////////////////////////
    /// Checks if a given format has stencil component.
    ////////////////////////////////////////////////////////////
    bool HasStencilComponent( const VkFormat format )
    {
        return ( format == VK_FORMAT_D32_SFLOAT_S8_UINT ) ||
            ( format == VK_FORMAT_D24_UNORM_S8_UINT );
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
        const uint32_t swapChainImageCount = static_cast<uint32_t>( m_SwapChainImages.size() );

        // Match swap chain image views to swap chain images.
        m_SwapChainImageViews.resize( swapChainImageCount );

        for( uint32_t i = 0; i < swapChainImageCount; ++i )
        {
            // For stereographics 3d applications a swap chain with multiple layers is required and
            // with multiple image views as well.
            const StatusCode result = CreateImageView(
                m_SwapChainImages[i],
                m_SwapChainImageFormat,
                VK_IMAGE_ASPECT_COLOR_BIT,
                m_SwapChainImageViews[i] );

            if( result != StatusCode::Success )
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
        // Populate color attachment description information.
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format                  = m_SwapChainImageFormat;
        colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Populate color attachment reference information.
        VkAttachmentReference colorAttachmentReference = {};
        colorAttachmentReference.attachment            = 0;
        colorAttachmentReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Populate depth attachment description information.
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format                  = FindDepthFormat();
        depthAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Populate depth attachment reference information.
        VkAttachmentReference depthAttachmentReference = {};
        depthAttachmentReference.attachment            = 1;
        depthAttachmentReference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Populate subpass information.
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentReference;
        subpass.pDepthStencilAttachment = &depthAttachmentReference;

        // Populate subpass dependency information.
        VkSubpassDependency dependency = {};
        dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass          = 0;
        dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask       = 0;
        dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // Populate render pass information.
        const std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount        = static_cast<uint32_t>( attachments.size() );
        renderPassInfo.pAttachments           = attachments.data();
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
    /// Creates descriptor set layout.
    ////////////////////////////////////////////////////////////
    StatusCode CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding     = {};
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};

        // Uniform layout.
        uboLayoutBinding.binding            = 0;
        uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount    = 1;
        uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional.

        // Sampler layout.
        samplerLayoutBinding.binding            = 1;
        samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount    = 1;
        samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional.

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};

        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>( bindings.size() );
        layoutInfo.pBindings    = bindings.data();

        if( vkCreateDescriptorSetLayout( m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create descriptor set layout!" << std::endl;
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
        vertexInputInfo.pVertexBindingDescriptions           = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount      = static_cast<uint32_t>( attributeDescriptions.size() );
        vertexInputInfo.pVertexAttributeDescriptions         = attributeDescriptions.data();

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

        // Populate depth and stencil state.
        VkPipelineDepthStencilStateCreateInfo depthStencil = {};

        depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable       = VK_TRUE;
        depthStencil.depthWriteEnable      = VK_TRUE;
        depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds        = 0.0f; // Optional.
        depthStencil.maxDepthBounds        = 1.0f; // Optional.
        depthStencil.stencilTestEnable     = VK_FALSE;
        depthStencil.front                 = {}; // Optional.
        depthStencil.back                  = {}; // Optional.

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
        pipelineInfo.pDepthStencilState           = &depthStencil;
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
            const std::array<VkImageView, 2> attachments = { m_SwapChainImageViews[i], m_DepthImageView };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass              = m_RenderPass;
            framebufferInfo.attachmentCount         = static_cast<uint32_t>( attachments.size() );
            framebufferInfo.pAttachments            = attachments.data();
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
        const VkDeviceSize                 size,
        const VkBufferUsageFlags           usage,
        const VkMemoryPropertyFlags        properties,
        VkBuffer&                          buffer,
        std::pair<uint32_t, VkDeviceSize>& bufferGpuMemoryOffsets )
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

        // Get gpu memory requirements for the buffer.
        VkMemoryRequirements gpuMemoryRequirements = {};

        vkGetBufferMemoryRequirements( m_Device, buffer, &gpuMemoryRequirements );

        // Check if buffer has enough free space.
        if( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        {
            if( ( m_BufferGpuMemoryLocalUsage.back() + gpuMemoryRequirements.size ) > m_BufferGpuMemoryLocalSize.back() )
            {
                m_BufferGpuMemoryLocal.emplace_back( VK_NULL_HANDLE );
                m_BufferGpuMemoryLocalSize.emplace_back( 0 );
                m_BufferGpuMemoryLocalUsage.emplace_back( 0 );
            }
        }
        else
        {
            if( ( m_BufferGpuMemoryCpuVisibleUsage.back() + gpuMemoryRequirements.size ) > m_BufferGpuMemoryCpuVisibleSize.back() )
            {
                m_BufferGpuMemoryCpuVisible.emplace_back( VK_NULL_HANDLE );
                m_BufferGpuMemoryCpuVisibleSize.emplace_back( 0 );
                m_BufferGpuMemoryCpuVisibleUsage.emplace_back( 0 );
            }
        }

        // Allocate buffer.
        VkDeviceMemory& bufferGpuMemory = ( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            ? m_BufferGpuMemoryLocal.back()
            : m_BufferGpuMemoryCpuVisible.back();

        VkDeviceSize& bufferGpuMemorySize = ( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            ? m_BufferGpuMemoryLocalSize.back()
            : m_BufferGpuMemoryCpuVisibleSize.back();

        VkDeviceSize& bufferGpuMemoryUsage = ( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            ? m_BufferGpuMemoryLocalUsage.back()
            : m_BufferGpuMemoryCpuVisibleUsage.back();

        if( bufferGpuMemoryUsage == 0 )
        {
            // Allocate gpu memory for the buffer.
            VkMemoryAllocateInfo allocationInfo = {};

            const VkDeviceSize bytesToAllocate = ( ( gpuMemoryRequirements.size / Megabyte ) + 1 ) * Megabyte;

            allocationInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocationInfo.allocationSize  = bytesToAllocate;
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
                std::cerr << "Failed to allocate buffer memory!" << std::endl;
                return StatusCode::Fail;
            }

            bufferGpuMemorySize = bytesToAllocate;
        }

        // Bind allocated gpu memory with the buffer.
        if( bufferGpuMemoryUsage % gpuMemoryRequirements.alignment != 0 )
        {
            std::cerr << "Buffer offset      = " << bufferGpuMemoryUsage << std::endl;
            std::cerr << "Required alignment = " << gpuMemoryRequirements.alignment << std::endl;
            std::cerr << "Gpu memory offset is not aligned!" << std::endl;
            return StatusCode::Fail;
        }

        if( vkBindBufferMemory( m_Device, buffer, bufferGpuMemory, bufferGpuMemoryUsage ) != VK_SUCCESS )
        {
            std::cerr << "Cannot bind buffer gpu memory!" << std::endl;
            return StatusCode::Fail;
        }

        if( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        {
            bufferGpuMemoryOffsets = std::make_pair( static_cast<uint32_t>( m_BufferGpuMemoryLocal.size() - 1 ), bufferGpuMemoryUsage );
        }
        else
        {
            bufferGpuMemoryOffsets = std::make_pair( static_cast<uint32_t>( m_BufferGpuMemoryCpuVisible.size() - 1 ), bufferGpuMemoryUsage );
        }

        bufferGpuMemoryUsage += gpuMemoryRequirements.size;

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates an image view.
    ////////////////////////////////////////////////////////////
    StatusCode CreateImageView(
        const VkImage      image,
        const VkFormat     format,
        VkImageAspectFlags aspectFlags,
        VkImageView&       imageView )
    {
        VkImageViewCreateInfo viewInfo = {};

        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

        if( vkCreateImageView( m_Device, &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create image view!" << std::endl;
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
        const VkImage       image,
        const VkFormat      format,
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
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        // For depth testing.
        if( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if( HasStencilComponent( format ) )
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

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
        else if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
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
        const uint32_t                     textureWidth,
        const uint32_t                     textureHeight,
        const VkFormat                     format,
        const VkImageTiling                tiling,
        const VkImageUsageFlags            usage,
        const VkMemoryPropertyFlags        properties,
        VkImage&                           image,
        std::pair<uint32_t, VkDeviceSize>& bufferGpuMemoryOffsets )
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

        // Get gpu memory requirements for the image.
        VkMemoryRequirements gpuMemoryRequirements = {};

        vkGetImageMemoryRequirements( m_Device, image, &gpuMemoryRequirements );

        // Check if buffer has enough free space.
        if( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        {
            if( ( m_BufferGpuMemoryLocalUsage.back() + gpuMemoryRequirements.size ) > m_BufferGpuMemoryLocalSize.back() )
            {
                m_BufferGpuMemoryLocal.emplace_back( VK_NULL_HANDLE );
                m_BufferGpuMemoryLocalSize.emplace_back( 0 );
                m_BufferGpuMemoryLocalUsage.emplace_back( 0 );
            }
        }
        else
        {
            if( ( m_BufferGpuMemoryCpuVisibleUsage.back() + gpuMemoryRequirements.size ) > m_BufferGpuMemoryCpuVisibleSize.back() )
            {
                m_BufferGpuMemoryCpuVisible.emplace_back( VK_NULL_HANDLE );
                m_BufferGpuMemoryCpuVisibleSize.emplace_back( 0 );
                m_BufferGpuMemoryCpuVisibleUsage.emplace_back( 0 );
            }
        }

        // Allocate buffer.
        VkDeviceMemory& bufferGpuMemory = ( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            ? m_BufferGpuMemoryLocal.back()
            : m_BufferGpuMemoryCpuVisible.back();

        VkDeviceSize& bufferGpuMemorySize = ( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            ? m_BufferGpuMemoryLocalSize.back()
            : m_BufferGpuMemoryCpuVisibleSize.back();

        VkDeviceSize& bufferGpuMemoryUsage = ( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            ? m_BufferGpuMemoryLocalUsage.back()
            : m_BufferGpuMemoryCpuVisibleUsage.back();

        if( bufferGpuMemoryUsage == 0 )
        {
            VkMemoryAllocateInfo allocationInfo = {};

            const VkDeviceSize bytesToAllocate = ( ( gpuMemoryRequirements.size / Megabyte ) + 1 ) * Megabyte;

            allocationInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocationInfo.allocationSize  = bytesToAllocate;
            allocationInfo.memoryTypeIndex = FindGpuMemoryType( gpuMemoryRequirements.memoryTypeBits, properties );

            if( allocationInfo.memoryTypeIndex == UINT32_MAX )
            {
                std::cerr << "Cannot find gpu memory type!" << std::endl;
                return StatusCode::Fail;
            }

            if( vkAllocateMemory( m_Device, &allocationInfo, nullptr, &bufferGpuMemory ) != VK_SUCCESS )
            {
                std::cerr << "Cannot allocate image memory!" << std::endl;
                return StatusCode::Fail;
            }

            bufferGpuMemorySize = bytesToAllocate;
        }

        // Bind allocated gpu memory with the given buffer.
        if( bufferGpuMemoryUsage % gpuMemoryRequirements.alignment != 0 )
        {
            std::cerr << "Buffer offset      = " << bufferGpuMemoryUsage << std::endl;
            std::cerr << "Required alignment = " << gpuMemoryRequirements.alignment << std::endl;
            std::cerr << "Gpu memory offset is not aligned!" << std::endl;
            return StatusCode::Fail;
        }

        if( vkBindImageMemory( m_Device, image, bufferGpuMemory, bufferGpuMemoryUsage ) != VK_SUCCESS )
        {
            std::cerr << "Cannot bind buffer gpu memory!" << std::endl;
            return StatusCode::Fail;
        }

        if( properties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        {
            bufferGpuMemoryOffsets = std::make_pair( static_cast<uint32_t>( m_BufferGpuMemoryLocal.size() - 1 ), bufferGpuMemoryUsage );
        }
        else
        {
            bufferGpuMemoryOffsets = std::make_pair( static_cast<uint32_t>( m_BufferGpuMemoryCpuVisible.size() - 1 ), bufferGpuMemoryUsage );
        }

        bufferGpuMemoryUsage += gpuMemoryRequirements.size;

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates depth resources.
    ////////////////////////////////////////////////////////////
    StatusCode CreateDepthResources()
    {
        StatusCode     result      = StatusCode::Success;
        const VkFormat depthFormat = FindDepthFormat();

        result = CreateImage(
            m_SwapChainExtent.width,
            m_SwapChainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImage,
            m_DepthImageGpuMemoryOffset );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create image for depth!" << std::endl;
            return StatusCode::Fail;
        }

        result = CreateImageView(
            m_DepthImage,
            depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            m_DepthImageView );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create image view for depth!" << std::endl;
            return StatusCode::Fail;
        }

        TransitionImageLayout(
            m_DepthImage,
            depthFormat,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

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
        //std::string        textureFileName = "Textures/texture.jpg";
        std::string textureFileName = "Textures/viking_room.png";
        StatusCode  result          = StatusCode::Success;

        Pixels* pixels = StbImage::loadRgba( textureFileName, textureWidth, textureHeight, textureChannels );

        if( pixels == nullptr )
        {
            std::cerr << "Failed to load texture image!" << std::endl;
            return StatusCode::Fail;
        }

        const VkDeviceSize imageSize = textureWidth * textureHeight * bytesPerPixel;

        VkBuffer                          stagingBuffer        = VK_NULL_HANDLE;
        std::pair<uint32_t, VkDeviceSize> stagingBufferOffsets = {};

        result = CreateBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferOffsets );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create staging buffer for texture image!" << std::endl;
            return StatusCode::Fail;
        }

        // Copy the texture to staging buffer.
        void* data                  = nullptr;
        auto  bufferGpuMemory       = m_BufferGpuMemoryCpuVisible[std::get<0>( stagingBufferOffsets )];
        auto  bufferGpuMemoryOffset = std::get<1>( stagingBufferOffsets );

        vkMapMemory( m_Device, bufferGpuMemory, bufferGpuMemoryOffset, imageSize, 0, &data );
        memcpy( data, pixels, static_cast<size_t>( imageSize ) );
        vkUnmapMemory( m_Device, bufferGpuMemory );

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
            m_TextureImageGpuMemoryOffset );

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

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates texture image view.
    ////////////////////////////////////////////////////////////
    StatusCode CreateTextureImageView()
    {
        const StatusCode result = CreateImageView(
            m_TextureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT,
            m_TextureImageView );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create texture image view!" << std::endl;
            return StatusCode::Fail;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////
    /// Creates texture sampler.
    ////////////////////////////////////////////////////////////
    StatusCode CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo = {};

        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = VK_FILTER_LINEAR;
        samplerInfo.minFilter               = VK_FILTER_LINEAR;
        samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.minLod                  = 0.0f;
        samplerInfo.maxLod                  = 0.0f;
        samplerInfo.maxAnisotropy           = m_MaxSamplerAnisotropy;

        if( m_MaxSamplerAnisotropy > 1.0f )
        {
            samplerInfo.anisotropyEnable = VK_TRUE;
        }
        else
        {
            samplerInfo.anisotropyEnable = VK_FALSE;
        }

        if( vkCreateSampler( m_Device, &samplerInfo, nullptr, &m_TextureSampler ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create texture sampler!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates vertex buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateVertexBuffer()
    {
        StatusCode                        result               = StatusCode::Success;
        VkBuffer                          stagingBuffer        = VK_NULL_HANDLE;
        std::pair<uint32_t, VkDeviceSize> stagingBufferOffsets = {};
        const VkDeviceSize                bufferSize           = sizeof( Vertices[0] ) * Vertices.size();

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferOffsets );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create staging buffer for vertex buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Fill the vertex buffer.
        void* data                  = nullptr;
        auto  bufferGpuMemory       = m_BufferGpuMemoryCpuVisible[std::get<0>( stagingBufferOffsets )];
        auto  bufferGpuMemoryOffset = std::get<1>( stagingBufferOffsets );

        vkMapMemory( m_Device, bufferGpuMemory, bufferGpuMemoryOffset, bufferSize, 0, &data );
        memcpy_s( data, bufferSize, Vertices.data(), bufferSize );
        vkUnmapMemory( m_Device, bufferGpuMemory );

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_VertexBuffer,
            m_VertexBufferGpuMemoryOffset );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create buffer for vertex buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Copy data from the staging buffer to vertex buffer.
        CopyBuffer( stagingBuffer, m_VertexBuffer, bufferSize );

        vkDestroyBuffer( m_Device, stagingBuffer, nullptr );

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Creates index buffers.
    ////////////////////////////////////////////////////////////
    StatusCode CreateIndexBuffer()
    {
        StatusCode                        result               = StatusCode::Success;
        VkBuffer                          stagingBuffer        = VK_NULL_HANDLE;
        std::pair<uint32_t, VkDeviceSize> stagingBufferOffsets = {};
        const VkDeviceSize                bufferSize           = sizeof( Indices[0] ) * Indices.size();

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferOffsets );

        if( result != StatusCode::Success )
        {
            std::cerr << "Cannot create staging buffer for index buffer!" << std::endl;
            return StatusCode::Fail;
        }

        // Fill the index buffer.
        void* data                  = nullptr;
        auto  bufferGpuMemory       = m_BufferGpuMemoryCpuVisible[std::get<0>( stagingBufferOffsets )];
        auto  bufferGpuMemoryOffset = std::get<1>( stagingBufferOffsets );

        vkMapMemory( m_Device, bufferGpuMemory, bufferGpuMemoryOffset, bufferSize, 0, &data );
        memcpy_s( data, bufferSize, Indices.data(), bufferSize );
        vkUnmapMemory( m_Device, bufferGpuMemory );

        result = CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_IndexBuffer,
            m_IndexBufferGpuMemoryOffset );

        // Copy data from the staging buffer to index buffer.
        CopyBuffer( stagingBuffer, m_IndexBuffer, bufferSize );

        vkDestroyBuffer( m_Device, stagingBuffer, nullptr );

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
        m_UniformBuffersGpuMemoryOffsets.resize( swaChainImageCount );

        for( uint32_t i = 0; i < swaChainImageCount; ++i )
        {
            // TODO: try to use already freed uniform buffers.
            const StatusCode result = CreateBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_UniformBuffers[i],
                m_UniformBuffersGpuMemoryOffsets[i] );

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
        const uint32_t                      descriptorCount = static_cast<uint32_t>( m_SwapChainImages.size() );
        std::array<VkDescriptorPoolSize, 2> poolSizes       = {};
        VkDescriptorPoolCreateInfo          poolInfo        = {};

        // For uniform.
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = descriptorCount;

        // For sampler.
        poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = descriptorCount;

        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>( poolSizes.size() );
        poolInfo.pPoolSizes    = poolSizes.data();
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

            VkDescriptorImageInfo imageInfo = {};

            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView   = m_TextureImageView;
            imageInfo.sampler     = m_TextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet           = m_DescriptorSets[i];
            descriptorWrites[0].dstBinding       = 0;
            descriptorWrites[0].dstArrayElement  = 0;
            descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount  = 1;
            descriptorWrites[0].pBufferInfo      = &bufferInfo;
            descriptorWrites[0].pImageInfo       = nullptr; // Optional.
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional.

            descriptorWrites[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet           = m_DescriptorSets[i];
            descriptorWrites[1].dstBinding       = 1;
            descriptorWrites[1].dstArrayElement  = 0;
            descriptorWrites[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount  = 1;
            descriptorWrites[1].pBufferInfo      = nullptr; // Optional.
            descriptorWrites[1].pImageInfo       = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr; // Optional.

            vkUpdateDescriptorSets(
                m_Device,
                static_cast<uint32_t>( descriptorWrites.size() ),
                descriptorWrites.data(),
                0,
                nullptr );
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

            // Define a clear color and depth.
            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color.float32[0]         = 0.1f; // Red channel.
            clearValues[0].color.float32[1]         = 0.4f; // Green channel.
            clearValues[0].color.float32[2]         = 0.5f; // Blue channel.
            clearValues[0].color.float32[3]         = 1.0f; // Alpha channel.
            clearValues[1].depthStencil.depth       = 1.0f;
            clearValues[1].depthStencil.stencil     = 0;

            // Populate render pass begin information.
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass            = m_RenderPass;
            renderPassInfo.framebuffer           = m_SwapChainFramebuffers[i];
            renderPassInfo.renderArea.offset     = { 0, 0 };
            renderPassInfo.renderArea.extent     = m_SwapChainExtent;
            renderPassInfo.clearValueCount       = static_cast<uint32_t>( clearValues.size() );
            renderPassInfo.pClearValues          = clearValues.data();

            // Begin the render pass.
            vkCmdBeginRenderPass( m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

            // Bind the graphics pipeline.
            vkCmdBindPipeline( m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline );

            // Bind the vertex buffers.
            const VkBuffer     vertexBuffers[] = { m_VertexBuffer };
            const VkDeviceSize offsets[]       = { 0 };
            vkCmdBindVertexBuffers( m_CommandBuffers[i], 0, 1, vertexBuffers, offsets );

            // Bind the index buffer.
            vkCmdBindIndexBuffer( m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

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

        result = CreateDepthResources();
        if( result != StatusCode::Success )
        {
            std::cerr << "Depth resources creation failed!" << std::endl;
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
        void* data                         = nullptr;
        auto  uniformBufferGpuMemoryOffset = m_UniformBuffersGpuMemoryOffsets[currentImage];
        auto  bufferGpuMemory              = m_BufferGpuMemoryCpuVisible[std::get<0>( uniformBufferGpuMemoryOffset )];
        auto  bufferGpuMemoryOffset        = std::get<1>( uniformBufferGpuMemoryOffset );

        vkMapMemory( m_Device, bufferGpuMemory, bufferGpuMemoryOffset, sizeof( ubo ), 0, &data );
        memcpy( data, &ubo, sizeof( ubo ) );
        vkUnmapMemory( m_Device, bufferGpuMemory );
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
        // Destroy depth image view.
        vkDestroyImageView( m_Device, m_DepthImageView, nullptr );

        // Destroy depth image.
        vkDestroyImage( m_Device, m_DepthImage, nullptr );

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

        // Destroy texture sampler.
        vkDestroySampler( m_Device, m_TextureSampler, nullptr );

        // Destroy texture image view.
        vkDestroyImageView( m_Device, m_TextureImageView, nullptr );

        // Destroy texture image.
        vkDestroyImage( m_Device, m_TextureImage, nullptr );

        // Destroy description set layout.
        vkDestroyDescriptorSetLayout( m_Device, m_DescriptorSetLayout, nullptr );

        // Destroy index buffer.
        vkDestroyBuffer( m_Device, m_IndexBuffer, nullptr );

        // Destroy vertex buffer.
        vkDestroyBuffer( m_Device, m_VertexBuffer, nullptr );

        // Free gpu memory associated with local memory.
        for( auto& bufferGpuMemoryLocal : m_BufferGpuMemoryLocal )
        {
            vkFreeMemory( m_Device, bufferGpuMemoryLocal, nullptr );
        }

        // Free gpu memory associated with cpu visible memory.
        for( auto& bufferGpuMemoryCpuVisible : m_BufferGpuMemoryCpuVisible )
        {
            vkFreeMemory( m_Device, bufferGpuMemoryCpuVisible, nullptr );
        }

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
