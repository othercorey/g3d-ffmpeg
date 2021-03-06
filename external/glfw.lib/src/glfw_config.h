//========================================================================
// GLFW 3.3 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2010-2016 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================
// As glfw_config.h.in, this file is used by CMake to produce the
// glfw_config.h configuration header file.  If you are adding a feature
// requiring conditional compilation, this is where to add the macro.
//========================================================================
// As glfw_config.h, this file defines compile-time option macros for a
// specific platform and development environment.  If you are using the
// GLFW CMake files, modify glfw_config.h.in instead of this file.  If you
// are using your own build system, make this file define the appropriate
// macros in whatever way is suitable.
//========================================================================

#ifdef _MSC_VER
// Define this to 1 if building GLFW for Win32/WGL
#define _GLFW_WIN32 1
#define _GLFW_WGL   1
#elif __APPLE__
// Define this to 1 if building GLFW for Cocoa/NSOpenGL
#define _GLFW_NSGL 1
#define _GLFW_COCOA 1
#else 
#define _GLFW_X11 1
#define _GLFW_GLX 1
#define _GLFW_HAS_GLXGETPROCADDRESS
#endif



// Define this to 1 if building as a shared library / dynamic library / DLL
/* #undef _GLFW_BUILD_DLL */
// Define this to 1 to use Vulkan loader linked statically into application
/* #undef _GLFW_VULKAN_STATIC */

// Define this to 1 to force use of high-performance GPU on hybrid systems
/* #undef _GLFW_USE_HYBRID_HPG */

// Define this to 1 if xkbcommon supports the compose key
/* #undef HAVE_XKBCOMMON_COMPOSE_H */

// Define this to 1 if the Xxf86vm X11 extension is available
#define _GLFW_HAS_XF86VM

// Define this to 1 if glfwInit should change the current directory
#define _GLFW_USE_CHDIR
// Define this to 1 if glfwCreateWindow should populate the menu bar
#define _GLFW_USE_MENUBAR

// Define this to 1 if windows should use full resolution on Retina displays
#define _GLFW_USE_RETINA 0
