#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdlib>

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
    /// Private members.
    ////////////////////////////////////////////////////////////
    uint32_t    m_Width;
    uint32_t    m_Height;
    GLFWwindow* m_Window;
    std::string m_WindowName;

public:
    ////////////////////////////////////////////////////////////
    /// Default constructor of the object.
    ////////////////////////////////////////////////////////////
    Application()
        : m_Width( 800 )
        , m_Height( 600 )
        , m_Window( nullptr )
        , m_WindowName( "Vulkan Application" )
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

        result = Cleanup();
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
        return StatusCode::Success;
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
    /// Cleanups the application.
    ////////////////////////////////////////////////////////////
    StatusCode Cleanup()
    {
        // Destroy the windows.
        glfwDestroyWindow( m_Window );

        // Destroy glfw library.
        glfwTerminate();

        return StatusCode::Success;
    }
};

int32_t main()
{
    // Construct an application object with parameters.
    Application application( 800, 600, "Vulkan Application" );

    // Initialize the application.
    application.Initialize();

    // Run the application in a loop.
    application.Run();

    // Destroy the application.
    application.Destroy();

    // Return success.
    return static_cast<int32_t>( StatusCode::Success );
}
