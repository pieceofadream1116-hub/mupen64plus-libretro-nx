#include "GlesContext.h"

#include "UwpPlatform.h"

#include <algorithm>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#ifndef GL_GLES_PROTOTYPES
#define GL_GLES_PROTOTYPES 1
#endif
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


namespace {

// The few GL functions the app itself calls, taken from ANGLE at run time (the same way the core gets them)
struct GlFunctions
{
	const GLubyte*(GL_APIENTRY* GetString)(GLenum) = nullptr;
	void(GL_APIENTRY* DeleteFramebuffers)(GLsizei, const GLuint*) = nullptr;
	void(GL_APIENTRY* DeleteRenderbuffers)(GLsizei, const GLuint*) = nullptr;
	void(GL_APIENTRY* GenFramebuffers)(GLsizei, GLuint*) = nullptr;
	void(GL_APIENTRY* BindFramebuffer)(GLenum, GLuint) = nullptr;
	void(GL_APIENTRY* GenRenderbuffers)(GLsizei, GLuint*) = nullptr;
	void(GL_APIENTRY* BindRenderbuffer)(GLenum, GLuint) = nullptr;
	void(GL_APIENTRY* RenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei) = nullptr;
	void(GL_APIENTRY* FramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint) = nullptr;
	GLenum(GL_APIENTRY* CheckFramebufferStatus)(GLenum) = nullptr;
	void(GL_APIENTRY* ClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
	void(GL_APIENTRY* Clear)(GLbitfield) = nullptr;
	void(GL_APIENTRY* PixelStorei)(GLenum, GLint) = nullptr;
	void(GL_APIENTRY* ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*) = nullptr;

	bool Load()
	{
		GetString = reinterpret_cast<decltype(GetString)>(eglGetProcAddress("glGetString"));
		DeleteFramebuffers = reinterpret_cast<decltype(DeleteFramebuffers)>(eglGetProcAddress("glDeleteFramebuffers"));
		DeleteRenderbuffers = reinterpret_cast<decltype(DeleteRenderbuffers)>(eglGetProcAddress("glDeleteRenderbuffers"));
		GenFramebuffers = reinterpret_cast<decltype(GenFramebuffers)>(eglGetProcAddress("glGenFramebuffers"));
		BindFramebuffer = reinterpret_cast<decltype(BindFramebuffer)>(eglGetProcAddress("glBindFramebuffer"));
		GenRenderbuffers = reinterpret_cast<decltype(GenRenderbuffers)>(eglGetProcAddress("glGenRenderbuffers"));
		BindRenderbuffer = reinterpret_cast<decltype(BindRenderbuffer)>(eglGetProcAddress("glBindRenderbuffer"));
		RenderbufferStorage = reinterpret_cast<decltype(RenderbufferStorage)>(eglGetProcAddress("glRenderbufferStorage"));
		FramebufferRenderbuffer = reinterpret_cast<decltype(FramebufferRenderbuffer)>(eglGetProcAddress("glFramebufferRenderbuffer"));
		CheckFramebufferStatus = reinterpret_cast<decltype(CheckFramebufferStatus)>(eglGetProcAddress("glCheckFramebufferStatus"));
		ClearColor = reinterpret_cast<decltype(ClearColor)>(eglGetProcAddress("glClearColor"));
		Clear = reinterpret_cast<decltype(Clear)>(eglGetProcAddress("glClear"));
		PixelStorei = reinterpret_cast<decltype(PixelStorei)>(eglGetProcAddress("glPixelStorei"));
		ReadPixels = reinterpret_cast<decltype(ReadPixels)>(eglGetProcAddress("glReadPixels"));
		return GetString && DeleteFramebuffers && DeleteRenderbuffers && GenFramebuffers && BindFramebuffer && GenRenderbuffers && BindRenderbuffer && RenderbufferStorage && FramebufferRenderbuffer && CheckFramebufferStatus && ClearColor && Clear && PixelStorei && ReadPixels;
	}
};

GlFunctions gl;

} // namespace

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
	if(!gl.Load()) {
		error = "OpenGL ES (ANGLE): its functions could not be found";
		Destroy();
		return false;
	}
	const char* renderer = reinterpret_cast<const char*>(gl.GetString(GL_RENDERER));
	const char* glVersion = reinterpret_cast<const char*>(gl.GetString(GL_VERSION));
	uwp::Log(std::string("OpenGL ES: ") + (glVersion ? glVersion : "?") + ", " + (renderer ? renderer : "?"));
	return true;
}

void GlesContext::Destroy()
{
	if(!_display) {
		return;
	}
	if(_context && gl.DeleteFramebuffers && eglGetCurrentContext() == _context) {
		if(_fbo) {
			gl.DeleteFramebuffers(1, &_fbo);
		}
		if(_color) {
			gl.DeleteRenderbuffers(1, &_color);
		}
		if(_depth) {
			gl.DeleteRenderbuffers(1, &_depth);
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
		gl.DeleteFramebuffers(1, &_fbo);
		gl.DeleteRenderbuffers(1, &_color);
		if(_depth) {
			gl.DeleteRenderbuffers(1, &_depth);
		}
		_fbo = _color = _depth = 0;
	}
	_width = width;
	_height = height;
	gl.GenFramebuffers(1, &_fbo);
	gl.BindFramebuffer(GL_FRAMEBUFFER, _fbo);
	gl.GenRenderbuffers(1, &_color);
	gl.BindRenderbuffer(GL_RENDERBUFFER, _color);
	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _color);
	if(depth || stencil) {
		gl.GenRenderbuffers(1, &_depth);
		gl.BindRenderbuffer(GL_RENDERBUFFER, _depth);
		gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);
		gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depth);
	}
	GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
	gl.ClearColor(0, 0, 0, 1);
	gl.Clear(GL_COLOR_BUFFER_BIT);
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
	gl.BindFramebuffer(GL_FRAMEBUFFER, _fbo);
	gl.PixelStorei(GL_PACK_ALIGNMENT, 4);
	gl.ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, _rgba.data());
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
