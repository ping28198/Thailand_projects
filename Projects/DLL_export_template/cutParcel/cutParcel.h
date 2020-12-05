#pragma once


#ifdef CUTPARCEL_EXPORTS
#define DLL_FUNC_API extern "C" __declspec(dllexport)
#else
#define DLL_FUNC_API extern "C" __declspec(dllimport)
#endif



DLL_FUNC_API int cutParcel(char* srcPath,char* dstPath,int applied_rotate,double boxSizeThreshold, double binaryThreshold, int is_top);