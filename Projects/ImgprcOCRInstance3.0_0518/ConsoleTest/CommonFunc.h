#pragma once
#include "pch.h"
#include <string>
#include <vector>
#include "windows.h"
#include "tchar.h"
#include <iostream>
#include <atlconv.h>  
using namespace std;

class CommonFunc
{
public:
	CommonFunc();




	//************************************
	// 函数名:    getAllFilesNameInDir
	// 描述：    获取文件路径下的所有文件,可指定文件名后缀.不包括：“.”和“..”
	// 引用类型:    public static 
	// 返回值:   int :获得的文件名数量,没有获得文件名为0.
	// 参数: string dir :输入文件路径，可以是目录，也可以是带通配文件名，例如：/*.jpg /*.*
	// 参数: vector<string> & filenames :返回文件名信息
	// 参数: bool isIncludeSubDir :是否包括子目录，默认不包括
	// 参数: bool isReturnPath : 返回文件名中是否包括文件路径，默认不包括
	//************************************
	static int getAllFilesNameInDir(string dir, vector<string> &filenames,bool isIncludeSubDir=false, bool isReturnPath=false);

};