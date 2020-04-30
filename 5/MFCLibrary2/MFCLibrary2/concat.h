#pragma once

#include <iostream>
#include <conio.h>
#include <vector>
#include <string>
#define CONCAT_API _declspec(dllexport)
#ifdef CONCAT_EXPORTS

#else
#define CONCAT_API _declspec(dllimport)
#endif

extern "C" CONCAT_API void concat();