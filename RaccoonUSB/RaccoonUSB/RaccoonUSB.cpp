/**
* USB Backup tools
* @Version : 2.0
* @author : Young Il Seo
*/

#include <iostream>
#include <cstdio>
#include <Windows.h>
#include <tchar.h>
#include "Shlwapi.h"
#include <vector>
#include <cstdlib>
#include <io.h>
#include <string>

#pragma comment(lib, "shlwapi")

using namespace std;

typedef std::basic_string<TCHAR> tstring;

typedef struct {
	TCHAR fileName[MAX_PATH];
	FILETIME LastWriteTime;
	DWORD dwFileAttributes;
}lightFS;

TCHAR* StringToTCHAR(string& s)
{
	tstring tstr;
	const char* all = s.c_str();
	int len = 1 + strlen(all);
	wchar_t* t = new wchar_t[len];
	if (NULL == t) throw std::bad_alloc();
	mbstowcs(t, all, len);
	return (TCHAR*)t;
}

string TCHARToString(const TCHAR* ptsz)
{
	int len = wcslen((wchar_t*)ptsz);
	char* psz = new char[2 * len + 1];
	wcstombs(psz, (wchar_t*)ptsz, 2 * len + 1);
	std::string s = psz;
	delete[] psz;
	return s;
}

void listFile(TCHAR *path, vector<lightFS> &flist) {
	WIN32_FIND_DATA fileData;
	HANDLE hSrch;
	BOOL bResult = TRUE;
	lightFS component;

	TCHAR fname[MAX_PATH];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[MAX_PATH];
	TCHAR newpath[MAX_PATH];

	hSrch = FindFirstFile(path, &fileData);
	if (hSrch == INVALID_HANDLE_VALUE) return;
	_tsplitpath(path, drive, dir, NULL, NULL);

	while (bResult) {
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (lstrcmp(fileData.cFileName, L".") && lstrcmp(fileData.cFileName, L"..")) {
				wsprintf(newpath, L"%s%s%s\\*.*", drive, dir, fileData.cFileName);
				wsprintf(fname, L"%s%s%s", drive, dir, fileData.cFileName);
				_tcscpy(component.fileName, fname);
				component.LastWriteTime = fileData.ftLastWriteTime;
				component.dwFileAttributes = fileData.dwFileAttributes;
				flist.push_back(component);

				listFile(newpath, flist);
			}
		}
		else {
			wsprintf(fname, L"%s%s%s", drive, dir, fileData.cFileName);
			_tcscpy(component.fileName, fname);
			component.LastWriteTime = fileData.ftLastWriteTime;
			component.dwFileAttributes = fileData.dwFileAttributes;
			flist.push_back(component);
		}
		bResult = FindNextFile(hSrch, &fileData);
	}
	FindClose(hSrch);
}

void BackUpFile(HANDLE hOut, TCHAR* src, TCHAR* dest, vector<lightFS> &src_list) {
	HANDLE hFind;
	HANDLE hTarget;

	WIN32_FIND_DATA target;
	WIN32_FIND_DATA nTarget;
	DWORD dwWrite;
	DWORD temp;

	TCHAR newpath[MAX_PATH];

	vector<lightFS>::iterator itor;
	itor = src_list.begin();

	for (itor = src_list.begin(); itor != src_list.end(); itor++) {
		hFind = FindFirstFile(itor->fileName, &target);
		
		string itorFile = TCHARToString(itor->fileName);
		string itorSrc = TCHARToString(src);
		string strDirectory = itorFile.substr(itorSrc.length(), itorFile.length());
		
		string itorDest = TCHARToString(dest);
		string finalDirectory = itorDest.append(strDirectory);
		_tcscpy(newpath, StringToTCHAR(finalDirectory));
		hTarget = FindFirstFile(newpath, &nTarget);

		// it there is no file
		if (hTarget == INVALID_HANDLE_VALUE) {
			if (itor->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				CreateDirectory(newpath, NULL);
			}
			else{
				CopyFile(itor->fileName, newpath, FALSE);
				WriteFile(hOut, itor->fileName, wcslen(itor->fileName) * sizeof(TCHAR), &dwWrite, NULL);
				WriteFile(hOut, "\r\n", strlen("\r\n"), &temp, NULL);
			}
		}
		// check target is a file and time
		else if (target.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
			if (1 == CompareFileTime(&(target.ftLastWriteTime), &(nTarget.ftLastWriteTime))) {
				CopyFile(itor->fileName, newpath, FALSE);
				WriteFile(hOut, itor->fileName, wcslen(itor->fileName) * sizeof(TCHAR), &dwWrite, NULL);
				WriteFile(hOut, "\r\n", strlen("\r\n"), &temp, NULL);
			}
		}
	}
}

// Usage : program src_path, dst_path
int _tmain(int argc, TCHAR* argv[]) {

	// 1. read file list and add vector
	// 2. compare two files and copy(conditional)
	// 3. write backup log

	TCHAR src[MAX_PATH];
	TCHAR dst[MAX_PATH];
	HANDLE hfile;



	if (argc != 3) {
		cout << "Need more argument : program src_directory dest_directory" << endl;
		return 1;
	}
	else {
		// backup original argv
		lstrcpy(src, argv[1]);
		lstrcpy(dst, argv[2]);
	}

	if ((-1 == _taccess(argv[1], 0)) || (-1 == _taccess(argv[2], 0))) {
		cout << "invalid path" << endl;
		return 1;
	}
	else {
		hfile = CreateFile(_T("mybackup.log"), GENERIC_READ | GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, 0, NULL);
	}

	vector<lightFS> srcFileList;
	TCHAR* first_path;

	first_path = argv[1];
	lstrcat(first_path, L"\\*.*");
	listFile(first_path, srcFileList);
	BackUpFile(hfile, src, dst, srcFileList);
	CloseHandle(hfile);
	return 0;
}