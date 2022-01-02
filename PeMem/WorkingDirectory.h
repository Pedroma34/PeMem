#pragma once
#include <string>
#include <iostream>
#include<Windows.h>
#include <Shlwapi.h>

namespace pemem {
	inline std::string GetDirectory() {
		HMODULE hModule = GetModuleHandle(nullptr); //get module for this .exe file
		if (hModule) {
			char path[256];
			GetModuleFileNameA(hModule, path, sizeof(path));
			PathRemoveFileSpecA(path);
			strcat_s(path, "\\");
			return std::string(path);
		}
		return "";
	}
}