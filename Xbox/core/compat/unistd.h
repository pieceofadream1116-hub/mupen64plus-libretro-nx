/* The few POSIX bits the core asks for (the Xbox app builds it with clang-cl, for UWP) */
#pragma once
#include <io.h>
#include <process.h>
#include <direct.h>
#include <stdlib.h>
#ifndef F_OK
#define F_OK 0
#define W_OK 2
#define R_OK 4
#endif
