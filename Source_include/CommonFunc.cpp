#include "CommonFunc.h"
#include "tchar.h"
#include <iostream>
#include <atlconv.h>

using namespace std;

CommonFunc::CommonFunc()
{

}


int CommonFunc::getAllFilesNameInDir(string dir, vector<string> &filenames, bool isIncludeSubDir/*=false*/, bool isReturnPath/*=false*/)
{
	HANDLE hFind;
	WIN32_FIND_DATA findData;
	//LARGE_INTEGER size;
	string base_dir,new_dir,suffix;
	//dir.replace(dir.begin(), dir.end(), '\\', '/');
	while (true)
	{
		int mpos = dir.find('\\');
		if (mpos!=dir.npos)
		{
			dir.replace(mpos, 1, "/");
		}
		else
		{
			break;
		}
	}
	while (true)
	{
		if (dir.find_last_of('/')!=(dir.size()-1))
		{
			break;
		}
		dir.erase(dir.size() - 1);
	}
	base_dir = dir;
	if (base_dir.find('*') != base_dir.npos || base_dir.find('?') != base_dir.npos)
	{
		int slashPos = dir.find_last_of('/');
		if (slashPos!=dir.npos)
		{
			suffix = base_dir.substr(slashPos+1);
			base_dir = base_dir.substr(0, slashPos);
		}
	}
	else
	{
		suffix = "*.*";
	}
	if (isIncludeSubDir)
	{
		new_dir = base_dir + "/" + "*.*";
	}
	else
	{
		new_dir = base_dir + "/" + suffix;
	}
	USES_CONVERSION;
	hFind = FindFirstFileW(A2T(new_dir.c_str()), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		//cout << "Failed to find first file!\n";
	}
	else
	{
		do
		{
			// 忽略"."和".."两个结果 
			string str1 = T2A(findData.cFileName);
			if (strcmp(str1.c_str(), ".") == 0 || strcmp(str1.c_str(), "..") == 0)
				continue;
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    // 是否是目录 
			{
				if (isIncludeSubDir)
				{
					getAllFilesNameInDir(base_dir + '/' + str1+'/'+suffix, filenames, isIncludeSubDir, isReturnPath);
				}
			}
		} while (FindNextFile(hFind, &findData));
	}

	//查找文件
	new_dir.clear();
	new_dir = base_dir + "/" + suffix;
	hFind = FindFirstFileW(A2T(new_dir.c_str()), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		cout << "Failed to find first file!\n";
	}
	else
	{
		do
		{
			string str1 = T2A(findData.cFileName);
			if (strcmp(str1.c_str(), ".") == 0 || strcmp(str1.c_str(), "..") == 0)
				continue;
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    // 是否是目录 
			{
				continue;
			}
			else
			{
				if (isReturnPath)
				{
					filenames.push_back(base_dir + '/' + str1);
				}
				else
				{
					filenames.push_back(str1);
				}
			}
		} while (FindNextFile(hFind, &findData));
	}

	return filenames.size();
}

std::string CommonFunc::get_exe_dir()
{
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	//str_url = szFilePath;  // 例如str_url==e:\program\Debug\  //
	USES_CONVERSION;
	std::string exe_dir = T2A(szFilePath);
	return exe_dir;
}

