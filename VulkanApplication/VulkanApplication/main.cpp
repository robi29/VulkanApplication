#include <vulkan/vulkan.h>

#include <iostream>
#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

enum class StatusCode : uint32_t
{
    Success = 0,
    Fail
};

class Application
{
private:
    uint32_t    m_Width;
    uint32_t    m_Height;
    GLFWwindow* m_Window;
    std::string m_WindowName;

public:
    Application()
        : m_Width( 800 )
        , m_Height( 600 )
        , m_Window( nullptr )
        , m_WindowName( "Vulkan Application" )
    {
    }

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

    StatusCode Initialize()
    {
        StatusCode result = StatusCode::Success;

        result = InitializeWindow();
        if( result != StatusCode::Success )
        {
            return result;
        }

        result = InitializeVulkan();
        if( result != StatusCode::Success )
        {
            return result;
        }

        return result;
    }

    StatusCode Run()
    {
        StatusCode result = StatusCode::Success;

        result = MainLoop();
        if( result != StatusCode::Success )
        {
            return result;
        }

        result = Cleanup();
        if( result != StatusCode::Success )
        {
            return result;
        }

        return result;
    }

private:
    StatusCode InitializeWindow()
    {
        if( glfwInit() != GLFW_TRUE )
        {
            return StatusCode::Fail;
        }

        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

        m_Window = glfwCreateWindow( m_Width, m_Height, m_WindowName.c_str(), nullptr, nullptr );

        if( m_Window == nullptr )
        {
            return StatusCode::Fail;
        }

        return StatusCode::Success;
    }

    StatusCode InitializeVulkan()
    {
        return StatusCode::Success;
    }

    StatusCode MainLoop()
    {
        while( !glfwWindowShouldClose( m_Window ) )
        {
            glfwPollEvents();
        }

        return StatusCode::Success;
    }

    StatusCode Cleanup()
    {
        glfwDestroyWindow( m_Window );
        glfwTerminate();

        return StatusCode::Success;
    }
};

int32_t main()
{
    Application application( 800, 600, "Vulkan Application" );

    application.Initialize();

    application.Run();

    return static_cast<int32_t>( StatusCode::Success );
}
