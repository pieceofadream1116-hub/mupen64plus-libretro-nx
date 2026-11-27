#include "GlesContext.h"

#include "UwpPlatform.h"

#include <algorithm>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#ifndef EGL_PLATFORM_ANGLE_ANGLE
#define EGL_PLATFORM_ANGLE_ANGLE 0x3202
#endif
#ifndef EGL_PLATFORM_ANGLE_TYPE_ANGLE
#define EGL_PLATFORM_ANGLE_TYPE_ANGLE 0x3203
#endif
#ifndef EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE
#define EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE 0x3208
#endif
#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

GlesContext::~GlesContext()
{
	Destroy();
}

bool GlesContext::Create(std::string& error)
{
	Destroy();
	EGLDisplay display = EGL_NO_DISPLAY;
	auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
	if(getPlatformDisplay) {
		const EGLint attributes[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, EGL_NONE};
		display = getPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, attributes);
	}
	if(display == EGL_NO_DISPLAY) {
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	}
	EGLint major = 0, minor = 0;
	if(display == EGL_NO_DISPLAY || !eglInitialize(display, &major, &minor)) {
		error = "OpenGL ES (ANGLE) could not start: eglInitialize failed (" + std::to_string(eglGetError()) + ")";
		return false;
	}
	_display = display;
	uwp::Log("EGL " + std::to_string(major) + "." + std::to_string(minor) + " (" +
		std::string(eglQueryString(display, EGL_VENDOR) ? eglQueryString(display, EGL_VENDOR) : "?") + ")");

	int version = 3;
	EGLConfig config = nullptr;
	EGLint count = 0;
	for(; version >= 2; version--) {
		const EGLint attributes[] = {EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
			EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
			EGL_RENDERABLE_TYPE, version == 3 ? EGL_OPENGL_ES3_BIT_KHR : EGL_OPENGL_ES2_BIT, EGL_NONE};
		if(eglChooseConfig(display, attributes, &config, 1, &count) && count > 0) {
			break;
		}
	}
	if(count <= 0) {
		error = "OpenGL ES (ANGLE): no usable configuration";
		Destroy();
		return false;
	}
	const EGLint surfaceAttributes[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
	_surface = eglCreatePbufferSurface(display, config, surfaceAttributes);
	const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, version, EGL_NONE};
	_context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
	if(!_surface || !_context || !eglMakeCurrent(display, _surface, _surface, _context)) {
		error = "OpenGL ES (ANGLE): the context could not be made (" + std::to_string(eglGetError()) + ")";
		Destroy();
		return false;
	}
	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	uwp::Log(std::string("OpenGL ES: ") + (glVersion ? glVersion : "?") + ", " + (renderer ? renderer : "?"));
	return true;
}

void GlesContext::Destroy()
{
	if(!_display) {
		return;
	}
	if(_context && eglGetCurrentContext() == _context) {
		if(_fbo) {
			glDeleteFramebuffers(1, &_fbo);
		}
		if(_color) {
			glDeleteRenderbuffers(1, &_color);
		}
		if(_depth) {
			glDeleteRenderbuffers(1, &_depth);
		}
	}
	_fbo = _color = _depth = 0;
	eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if(_context) {
		eglDestroyContext(_display, _context);
	}
	if(_surface) {
		eglDestroySurface(_display, _surface);
	}
	eglTerminate(_display);
	_display = _surface = _context = nullptr;
}

bool GlesContext::MakeFramebuffer(unsigned width, unsigned height, bool depth, bool stencil)
{
	if(_fbo && width <= _width && height <= _height) {
		return true;
	}
	if(_fbo) {
		glDeleteFramebuffers(1, &_fbo);
		glDeleteRenderbuffers(1, &_color);
		if(_depth) {
			glDeleteRenderbuffers(1, &_depth);
		}
		_fbo = _color = _depth = 0;
	}
	_width = width;
	_height = height;
	glGenFramebuffers(1, &_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glGenRenderbuffers(1, &_color);
	glBindRenderbuffer(GL_RENDERBUFFER, _color);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _color);
	if(depth || stencil) {
		glGenRenderbuffers(1, &_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, _depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depth);
	}
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	uwp::Log("Core framebuffer " + std::to_string(width) + "x" + std::to_string(height) +
		(status == GL_FRAMEBUFFER_COMPLETE ? "" : " [!] incomplete " + std::to_string(status)));
	return status == GL_FRAMEBUFFER_COMPLETE;
}

void* GlesContext::ProcAddress(const char* name)
{
	return reinterpret_cast<void*>(eglGetProcAddress(name));
}

void GlesContext::ReadPicture(unsigned width, unsigned height, std::vector<uint32_t>& out)
{
	width = std::min(width, _width);
	height = std::min(height, _height);
	_rgba.resize((size_t)width * height * 4);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, _rgba.data());
	out.resize((size_t)width * height);
	// GL's rows go bottom to top
	for(unsigned y = 0; y < height; y++) {
		const uint8_t* in = _rgba.data() + (size_t)(height - 1 - y) * width * 4;
		uint32_t* row = out.data() + (size_t)y * width;
		for(unsigned x = 0; x < width; x++, in += 4) {
			row[x] = 0xFF000000u | ((uint32_t)in[0] << 16) | ((uint32_t)in[1] << 8) | in[2];
		}
	}
}
