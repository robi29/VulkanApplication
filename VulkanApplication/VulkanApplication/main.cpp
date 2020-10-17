#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdlib>

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

////////////////////////////////////////////////////////////
/// Status code enumeration.
////////////////////////////////////////////////////////////
enum class StatusCode : uint32_t
{
    Success = 0,
    Fail
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
    VkInstance m_Instance;

public:
    ////////////////////////////////////////////////////////////
    /// Default constructor of the object.
    ////////////////////////////////////////////////////////////
    Application()
        : m_Width( 800 )
        , m_Height( 600 )
        , m_Window( nullptr )
        , m_WindowName( "Vulkan Application" )
        , m_Instance{}
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
        , m_Instance{}
    {
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

        result = CleanupVulkan();
        if( result != StatusCode::Success )
        {
            return result;
        }

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
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

        // Enumarate supported extensions.
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> extensions( extensionCount );
        vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data() );

        // Check if required extensions are supported.
        if( glfwExtensionCount > 0 && glfwExtensions != nullptr )
        {
            for( uint32_t i = 0; i < glfwExtensionCount; ++i )
            {
                const char* glfwExtension = glfwExtensions[i];

                auto findRequiredExtension = [&glfwExtension]( const auto& extension )
                {
                    return std::string( extension.extensionName ) == glfwExtension;
                };

                auto iterator = std::find_if( extensions.begin(), extensions.end(), findRequiredExtension );

                if( iterator == extensions.end() )
                {
                    return StatusCode::Fail;
                }
            }
        }

        // Sufficient information for creating an instance.
        VkInstanceCreateInfo createInfo    = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo        = &applicationInfo;
        createInfo.enabledExtensionCount   = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount       = 0;

        if( vkCreateInstance( &createInfo, nullptr, &m_Instance ) != VK_SUCCESS )
        {
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    ////////////////////////////////////////////////////////////
    /// Cleanups Vulkan api.
    ////////////////////////////////////////////////////////////
    StatusCode CleanupVulkan()
    {
        vkDestroyInstance( m_Instance, nullptr );

        return StatusCode::Success;
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
        return static_cast<int32_t>( result );
    }

    // Run the application in a loop.
    result = application.Run();
    if( result != StatusCode::Success )
    {
        return static_cast<int32_t>( result );
    }

    // Destroy the application.
    result = application.Destroy();
    if( result != StatusCode::Success )
    {
        return static_cast<int32_t>( result );
    }

    // Return success.
    return static_cast<int32_t>( result );
}
