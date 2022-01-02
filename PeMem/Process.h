#pragma once
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>

namespace pemem {
	class Process {
		const wchar_t* m_processName;
		const wchar_t* m_moduleName;
		HANDLE m_processHandle;
		DWORD m_processId;
		HWND m_windowHandle;
		uintptr_t m_moduleBase;
		bool m_isOpen;
		float m_maxCoolDown;//In seconds, how ofter we will look for target process in our main loop
		float m_coolDown;
	public:
		Process(const wchar_t* l_process, const wchar_t* l_module, const float &l_coolDown = 0.100f);
		~Process();

		/*Checkin very x seconds if process is still open*/
		void Update(const float& l_time);

		//Getters

		const wchar_t* GetProcessName();
		const wchar_t* GetModuleName();
		HANDLE GetProcessHandle();
		DWORD GetProcessId();
		HWND GetWindowHandle();
		uintptr_t GetModuleBase();
		const bool& isOpen();
	private:
		bool isReady();//Checks if target is open.
		bool FindProcessId();
		bool FindProcessHandle();
		bool FindWindowHandle();
		bool FindModuleBase();
	};
}