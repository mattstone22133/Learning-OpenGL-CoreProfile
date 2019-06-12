#include "SAWindow.h"

#include <stdexcept>
#include <iostream>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "OpenGLHelpers.h"

#define MAP_GLFWWINDOW_TO_WINDOWOBJ

static bool gladLoaded = false;

namespace SA
{


	/** 
		Simple concept to pimple, but all windows share this implementation 
		The goal is to hide the interface between the C api and the exposed C++ interface
		this class should not be useable outside of the this translation unit!
	*/
	class WindowStaticsImplementation
	{
	public:

		inline Window& findWindow(GLFWwindow* window)
		{
#ifdef MAP_GLFWWINDOW_TO_WINDOWOBJ
			auto winIter = windowMap.find(window);
			if (winIter != windowMap.end())
			{
				Window* winObj = winIter->second;
				return *winObj;
			}
#else 
			NOT_IMPLEMENTED;
#endif
			//fall through to return nothing if no window found
			throw std::runtime_error("FATAL: no window matching window from callback");
		}

		/** This is a raw pointer so it can be passed form Window's constructor */
		inline void trackWindow(GLFWwindow* rawWindow, Window* windowObj)
		{
#ifdef MAP_GLFWWINDOW_TO_WINDOWOBJ
			windowMap.insert({ rawWindow, windowObj });
			bindWindowCallbacks(rawWindow);
#else
			NOT_IMPLEMENTED;
#endif
		}

		inline void stopTrackingWindow(GLFWwindow* rawWindow)
		{
#ifdef MAP_GLFWWINDOW_TO_WINDOWOBJ
			windowMap.erase(rawWindow);
#else
			NOT_IMPLEMENTED;
#endif
		}
	private:
		void bindWindowCallbacks(GLFWwindow* window);

	private:
#ifdef MAP_GLFWWINDOW_TO_WINDOWOBJ
		std::unordered_map<GLFWwindow*, Window*> windowMap;
#else 
		std::vector<Window*> windows;
#endif

	public:
		uint32_t getWindowInstances() { 
#ifdef MAP_GLFWWINDOW_TO_WINDOWOBJ
			return static_cast<uint32_t>(windowMap.size());
#else
			NOT_IMPLEMENTED;
#endif
		};
	};
	static WindowStaticsImplementation windowStatics;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// C CALLBACK MANAGEMENT
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static void c_callback_CursorPos(GLFWwindow* window, double xpos, double ypos)
	{
		Window& windowObj = windowStatics.findWindow(window);
		windowObj.cursorPosEvent.broadcast(xpos, ypos);
	}

	static void c_callback_CursorEnter(GLFWwindow* window, int enter)
	{
		Window& windowObj = windowStatics.findWindow(window);
		windowObj.mouseLeftEvent.broadcast(enter);
	}

	static void c_callback_Scroll(GLFWwindow* window, double xOffset, double yOffset)
	{
		Window& windowObj = windowStatics.findWindow(window);
		windowObj.scrollChanged.broadcast(xOffset, yOffset);
	}

	static void c_callback_FramebufferSize(GLFWwindow* window, int width, int height)
	{
		Window& windowObj = windowStatics.findWindow(window);
		windowObj.framebufferSizeChanged.broadcast(width, height);
	}

	static void c_callback_KeyCallback(GLFWwindow* window, int key, int scanecode, int action, int mods)
	{
		Window& windowObj = windowStatics.findWindow(window);
		windowObj.onKeyInput.broadcast(key, scanecode, action, mods);
	}

	static void c_callback_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		Window& windowObj = windowStatics.findWindow(window);
		windowObj.onMouseButtonInput.broadcast(button, action, mods);
	}

	///this must come after callbacks for proper definition order, otherwise forward declarations are going to be needed
	void WindowStaticsImplementation::bindWindowCallbacks(GLFWwindow* window)
	{
		//unbinding I believe is done implicitly by GLFW destroy window
		glfwSetCursorPosCallback(window, &c_callback_CursorPos);
		glfwSetCursorEnterCallback(window, &c_callback_CursorEnter);
		glfwSetScrollCallback(window, &c_callback_Scroll);
		glfwSetFramebufferSizeCallback(window, &c_callback_FramebufferSize);
		glfwSetKeyCallback(window, &c_callback_KeyCallback);
		glfwSetMouseButtonCallback(window, &c_callback_MouseButtonCallback);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//STATIC GLFW MANAGEMENT
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Window::startUp()
	{
		if (windowStatics.getWindowInstances() == 0)
		{
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		}
	}
	void Window::tryShutDown()
	{
		if (windowStatics.getWindowInstances() == 0)
		{
			glfwTerminate();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Window Instances
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Window::Window(uint32_t width, uint32_t height)
	{
		startUp();

		window = glfwCreateWindow((int)width, (int)height, "OpenGL Window", nullptr, nullptr);
		if (!window)
		{
			glfwTerminate();
			throw std::runtime_error("FATAL: FAILED TO CREATE WINDOW");
		}

		//must be done everytime something is rendered to this window
		glfwMakeContextCurrent(window);
		
		//I believe GLAD proc address only needs to happen once
		if (!gladLoaded)
		{
			gladLoaded = true;

			if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress)))
			{
				std::cerr << "Failed to set up GLAD" << std::endl;
				glfwTerminate();
				throw std::runtime_error("FATAL: FAILED GLAD LOAD PROC");
			}
		}

		windowStatics.trackWindow(window, this);

	}

	void Window::postConstruct()
	{
		//it is weird that it listens to its own delegate, but the alternative is to friend the
		//interface between the C api that calls the event. But I also wanted to test the system for
		//letting objects subscrib at construction. So this acts as sa test for that.
		framebufferSizeChanged.addWeakObj(sp_this(), &Window::handleFramebufferSizeChanged);
	}

	Window::~Window()
	{
		windowStatics.stopTrackingWindow(window);
		glfwDestroyWindow(window);
		tryShutDown();
	}

	void Window::markWindowForClose(bool bClose)
	{
		glfwSetWindowShouldClose(window, bClose);
	}

	bool Window::shouldClose()
	{
		return glfwWindowShouldClose(window);
	}

	float Window::getAspect()
	{
		int width, height;

		//unsure if this should be framebuffer size or actual window size
		glfwGetFramebufferSize(window, &width, &height);
		//glfwGetWindowSize(window, &width, &height);

		float aspect = static_cast<float>(width) / height;
		if (aspect != 0.0f) 
		{
			//make sure user can't do scale window in a way that has 0 as aspect in some sort.
			assert(true);
		}
		return aspect;
	}

	void Window::setViewportToWindowSize()
	{
		//window size is not in pixels, but frame size returns the framebuffer size which is pixels.
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		ec(glViewport(0, 0, width, height));
	}

	void Window::handleFramebufferSizeChanged(int width, int height)
	{
		//update view port to support resizing
		ec(glViewport(0, 0, width, height));
	}

}
