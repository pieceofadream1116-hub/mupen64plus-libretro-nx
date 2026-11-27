#pragma once
// OpenGL ES 3 for the core (GLideN64), through ANGLE (OpenGL ES on Direct3D 11, made for UWP apps).
//
// Everything runs on the emulation thread: the context is made there, the core draws into an offscreen framebuffer,
// and after each frame the picture is read back into memory, where the app shows it like any other core's.

#include <cstdint>
#include <string>
#include <vector>

class GlesContext
{
public:
	~GlesContext();

	/// Makes the display, a context and a small offscreen surface, and makes them current on this thread.
	bool Create(std::string& error);
	void Destroy();
	bool Ready() const { return _context != nullptr; }

	/// The framebuffer the core draws into (sized for the biggest picture it will make).
	bool MakeFramebuffer(unsigned width, unsigned height, bool depth, bool stencil);
	uintptr_t Framebuffer() const { return _fbo; }

	/// For the core: the address of a GL function.
	static void* ProcAddress(const char* name);

	/// Copies the top-left width x height of the framebuffer into out as 0xAARRGGBB, top row first.
	void ReadPicture(unsigned width, unsigned height, std::vector<uint32_t>& out);

private:
	void* _display = nullptr;
	void* _surface = nullptr;
	void* _context = nullptr;
	unsigned _fbo = 0, _color = 0, _depth = 0;
	unsigned _width = 0, _height = 0;
	std::vector<uint8_t> _rgba;
};
