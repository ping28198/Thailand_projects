// cutParcel.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "cutParcel.h"
#include "CutParcelBox.h"
#include <string>
int cutParcel(char* srcPath, char* dstPath, int applied_rotate, double boxSizeThreshold, double binaryThreshold,int is_top)
{
	CutParcelBox cutparcel;
	int res = 0;
	try
	{
		res = cutparcel.getMailBox_c(srcPath, dstPath, applied_rotate, boxSizeThreshold, binaryThreshold, is_top);
	}
	catch (...)
	{
		
	}
	
	return res;
}
