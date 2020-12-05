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
	// ������:    getAllFilesNameInDir
	// ������    ��ȡ�ļ�·���µ������ļ�,��ָ���ļ�����׺.����������.���͡�..��
	// ��������:    public static 
	// ����ֵ:   int :��õ��ļ�������,û�л���ļ���Ϊ0.
	// ����: string dir :�����ļ�·����������Ŀ¼��Ҳ�����Ǵ�ͨ���ļ��������磺/*.jpg /*.*
	// ����: vector<string> & filenames :�����ļ�����Ϣ
	// ����: bool isIncludeSubDir :�Ƿ������Ŀ¼��Ĭ�ϲ�����
	// ����: bool isReturnPath : �����ļ������Ƿ�����ļ�·����Ĭ�ϲ�����
	//************************************
	static int getAllFilesNameInDir(string dir, vector<string> &filenames,bool isIncludeSubDir=false, bool isReturnPath=false);

};