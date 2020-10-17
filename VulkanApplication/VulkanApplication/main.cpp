#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdlib>

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
    VkInstance               m_Instance;
    VkPhysicalDevice         m_PhysicalDevice;
    VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
    VkDevice                 m_Device;
    VkQueue                  m_GraphicsQueue;
    VkSurfaceKHR             m_Surface;

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
        , m_PhysicalDevice( VK_NULL_HANDLE )
        , m_PhysicalDeviceFeatures{}
        , m_Device( VK_NULL_HANDLE )
        , m_GraphicsQueue( VK_NULL_HANDLE )
        , m_Surface( VK_NULL_HANDLE )
    {
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

        return result;
    }

    ////////////////////////////////////////////////////////////
    /// Main loop of the application.
    ////////////////////////////////////////////////////////////
    StatusCode MainLoop()
    {
        // Check for events.
        while( !glfwWindowShouldClose( m_Window ) )
        {
            glfwPollEvents();
        }

        return StatusCode::Success;
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
        bool foundRequiredExtensions = false;

        for( const auto& requiredExtension : extensions )
        {
            auto findRequiredExtension = [&requiredExtension]( const auto& extension ) {
                return std::string( extension.extensionName ) == requiredExtension;
            };

            auto iterator = std::find_if( availableExtensions.begin(), availableExtensions.end(), findRequiredExtension );

            if( iterator == availableExtensions.end() )
            {
                std::cerr << "Cannot find extension: " << iterator->extensionName << std::endl;
                return StatusCode::Fail;
            }
            else
            {
                foundRequiredExtensions = true;
            }
        }

        if( !foundRequiredExtensions )
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
        bool foundRequiredValidationLayers = false;

        for( const auto& requiredValidationLayer : m_ValidationLayers )
        {
            auto findRequiredValidationLayers = [&requiredValidationLayer]( const auto& validationLayer ) {
                return std::string( validationLayer.layerName ) == requiredValidationLayer;
            };

            auto iterator = std::find_if( availableLayers.begin(), availableLayers.end(), findRequiredValidationLayers );

            if( iterator == availableLayers.end() )
            {
                std::cerr << "Cannot find validation layer: " << iterator->layerName << std::endl;
                return StatusCode::Fail;
            }
            else
            {
                foundRequiredValidationLayers = true;
            }
        }

        if( !foundRequiredValidationLayers )
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

        FindQueueFamilies( m_PhysicalDevice );

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
    /// Checks if physical device is suitable.
    ////////////////////////////////////////////////////////////
    bool IsPhysicalDeviceSuitable( VkPhysicalDevice physicalDevice )
    {
        QueueFamilyIndices indices = FindQueueFamilies( physicalDevice );

        return indices.m_HasGraphicsFamily;
    }

    ////////////////////////////////////////////////////////////
    /// Creates a logical device.
    ////////////////////////////////////////////////////////////
    StatusCode CreateLogicalDevice()
    {
        // Get graphics queue family index.
        QueueFamilyIndices indices = FindQueueFamilies( m_PhysicalDevice );

        // Set the queue priority.
        const float queuePriority = 1.0f;

        // Populate queue create information.
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex        = indices.m_GraphicsFamily;
        queueCreateInfo.queueCount              = 1;
        queueCreateInfo.pQueuePriorities        = &queuePriority;

        // Populate logical device create information.
        VkDeviceCreateInfo deviceCreateInfo   = {};
        deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos    = &queueCreateInfo;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pEnabledFeatures     = &m_PhysicalDeviceFeatures;
#ifdef _DEBUG
        deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>( m_ValidationLayers.size() );
        deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif

        if( vkCreateDevice( m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device ) != VK_SUCCESS )
        {
            std::cerr << "Cannot create a logical device!" << std::endl;
            return StatusCode::Fail;
        }

        vkGetDeviceQueue( m_Device, indices.m_GraphicsFamily, 0, &m_GraphicsQueue );
        if( m_GraphicsQueue == nullptr )
        {
            std::cerr << "Cannot obtain graphics queue!" << std::endl;
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Find different queue families.
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
            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && !indices.m_HasGraphicsFamily )
            {
                indices.m_GraphicsFamily    = index;
                indices.m_HasGraphicsFamily = true;
            }
            if( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && !indices.m_HasComputeFamily )
            {
                indices.m_ComputeFamily    = index;
                indices.m_HasComputeFamily = true;
            }
            if( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !indices.m_HasMemoryTransferFamily )
            {
                indices.m_MemoryTransferFamily    = index;
                indices.m_HasMemoryTransferFamily = true;
            }

            ++index;
        }

        return indices;
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
        vkDestroyDevice( m_Device, nullptr );

        vkDestroySurfaceKHR( m_Instance, m_Surface, nullptr );

#ifdef _DEBUG
        vkDestroyDebugUtilsMessengerExtension( m_Instance, m_DebugMessenger, nullptr );
#endif

        vkDestroyInstance( m_Instance, nullptr );

        return StatusCode::Success;
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
