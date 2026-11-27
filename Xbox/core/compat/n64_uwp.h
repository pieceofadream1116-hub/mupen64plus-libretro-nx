/* Force-included into every file of the core (UWP build with clang-cl) */
#pragma once
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifdef __cplusplus
/* GLideN64 takes this one from desktop GL's glext.h; ANGLE's headers do not have it */
#include <GLES3/gl3.h>
typedef void (GL_APIENTRYP PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC)(GLuint texture, GLenum target, GLsizei samples,
	GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
#endif
