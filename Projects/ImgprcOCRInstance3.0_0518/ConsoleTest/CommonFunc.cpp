#include "CommonFunc.h"


CommonFunc::CommonFunc()
{

}


int CommonFunc::getAllFilesNameInDir(string dir, vector<string> &filenames, bool isIncludeSubDir/*=false*/, bool isReturnPath/*=false*/)
{
	HANDLE hFind;
	WIN32_FIND_DATA findData;
	LARGE_INTEGER size;
	string base_dir,new_dir;
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
	new_dir = dir;
	if (base_dir.find('*') != base_dir.npos || base_dir.find('?') != base_dir.npos)
	{
		int slashPos = dir.find_last_of('/');
		if (slashPos!=dir.npos)
		{
			base_dir = base_dir.substr(0, slashPos);
		}
	}
	else
	{
		new_dir = dir + "/*.*";
	}

	USES_CONVERSION;
	hFind = FindFirstFileW(A2T(new_dir.c_str()), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		cout << "Failed to find first file!\n";
		return 0;
	}
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
				getAllFilesNameInDir(dir + '/' + str1, filenames, isIncludeSubDir, isReturnPath);
			}
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
	//cout << "Done!\n";
	return filenames.size();
}

