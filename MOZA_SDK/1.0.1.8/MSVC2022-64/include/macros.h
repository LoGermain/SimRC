#pragma once

#ifdef DLL_EXPORTS
#define WIN_API __declspec(dllexport)
#else
#define WIN_API __declspec(dllimport)
#endif
