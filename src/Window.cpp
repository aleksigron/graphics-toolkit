#include "Window.hpp"

#define GLFW_INCLUDE_GLCOREARB
#include "glfw/glfw3.h"

#include "KeyboardInput.hpp"
#include "PointerInput.hpp"

Window::Window() :
	windowHandle(nullptr),
	keyboardInput(nullptr),
	pointerInput(nullptr)
{
}

Window::~Window()
{
	glfwTerminate();

	delete pointerInput;
	delete keyboardInput;
}

bool Window::Initialize(const char* windowTitle)
{
	if (glfwInit() == GL_TRUE)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		windowHandle = glfwCreateWindow(960, 640, windowTitle, NULL, NULL);
		
		if (windowHandle != nullptr)
		{
			keyboardInput = new KeyboardInput;
			keyboardInput->Initialize(windowHandle);

			pointerInput = new PointerInput;
			pointerInput->Initialize(windowHandle);

			glfwSetWindowUserPointer(windowHandle, this);
			glfwMakeContextCurrent(windowHandle);

			glfwSwapInterval(1);

			return true;
		}
		
		glfwTerminate();
	}
	
	return false;

}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(windowHandle);
}

void Window::UpdateInput()
{
	if (keyboardInput != nullptr)
	{
		keyboardInput->Update();
	}

	if (pointerInput != nullptr)
	{
		pointerInput->Update();
	}
}

void Window::Swap()
{
	glfwSwapBuffers(windowHandle);
	glfwPollEvents();
}

Vec2f Window::GetFrameBufferSize()
{
	int width, height;
	glfwGetFramebufferSize(windowHandle, &width, &height);

	return Vec2f(width, height);
}

Window* Window::GetWindowObject(GLFWwindow* windowHandle)
{
	return static_cast<Window*>(glfwGetWindowUserPointer(windowHandle));
}
