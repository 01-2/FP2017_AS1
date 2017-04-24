/**
* USB Backup tools
* @Version : 1.0
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

#pragma comment(lib, "shlwapi")

using namespace std;


typedef struct {
	TCHAR fileName[MAX_PATH];
	FILETIME LastWriteTime;
}lightFS;

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
				listFile(newpath, flist);
			}
		}
		else {
			wsprintf(fname, L"%s%s%s", drive, dir, fileData.cFileName);
			_tcscpy(component.fileName, fname);
			component.LastWriteTime = fileData.ftLastWriteTime;
			flist.push_back(component);
		}
		bResult = FindNextFile(hSrch, &fileData);
	}
	FindClose(hSrch);
}

void BackUpFile(HANDLE hOut, TCHAR* dest, vector<lightFS> &src_list) {
	HANDLE hFind;
	HANDLE hTarget;

	WIN32_FIND_DATA target;
	WIN32_FIND_DATA nTarget;
	DWORD dwWrite;
	DWORD temp;

	TCHAR ext[_MAX_EXT];
	TCHAR fname[_MAX_FNAME];
	TCHAR newpath[MAX_PATH];

	vector<lightFS>::iterator itor;
	itor = src_list.begin();

	for (itor = src_list.begin(); itor != src_list.end(); itor++) {
		hFind = FindFirstFile(itor->fileName, &target);

		_tsplitpath(itor->fileName, NULL, NULL, fname, ext);
		wsprintf(newpath, L"%s\\%s%s", dest, fname, ext);

		hTarget = FindFirstFile(newpath, &nTarget);
		// it there is no file
		if (hTarget == INVALID_HANDLE_VALUE){
			CopyFile(itor->fileName, newpath, FALSE);
			WriteFile(hOut, itor->fileName, wcslen(itor->fileName)*sizeof(TCHAR), &dwWrite, NULL);
			WriteFile(hOut, "\r\n", strlen("\r\n"), &temp, NULL);
		}
		// check target is a file and time
		else if (target.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
			if (1 == CompareFileTime(&(target.ftLastWriteTime), &(nTarget.ftLastWriteTime))) {
				CopyFile(itor->fileName, newpath, FALSE);
				WriteFile(hOut, itor->fileName, wcslen(itor->fileName)*sizeof(TCHAR), &dwWrite, NULL);
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
	BackUpFile(hfile, dst, srcFileList);
	CloseHandle(hfile);
	return 0;
}