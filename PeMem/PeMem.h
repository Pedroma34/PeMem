#pragma once
#include <string>
#include <fstream>
#include <ostream>
#include <sstream>
#include <map>
#include <vector>
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <chrono>
#include <Shlwapi.h> //must add shlwapi.lib to dependencies

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
	class Process {
		const wchar_t* m_processName;
		const wchar_t* m_moduleName;
		const char* m_windowName;
		HANDLE m_processHandle;
		DWORD m_processId;
		HWND m_windowHandle;
		uintptr_t m_moduleBase;
		bool m_isOpen;
		float m_maxCoolDown;//In seconds, how ofter we will look for target process in our main loop
		float m_coolDown;
	public:
		Process(const char * l_windowName, const wchar_t* l_process, 
			const wchar_t* l_module, float l_coolDown = 0.100f)
			: m_processName(l_process), m_moduleName(l_module), m_windowName(l_windowName), m_processHandle(0), m_processId(0),
			m_windowHandle(0), m_moduleBase(0), m_maxCoolDown(l_coolDown), m_coolDown(m_maxCoolDown) {

			m_isOpen = isReady() ? true : false;

		}
		~Process() {}

		/*Checkin very x seconds if process is still open*/
		void Update(const float& l_time) {
			m_coolDown -= l_time;

			//Updating every (m_maxCoolDown) seconds
			if (m_coolDown >= 0)
				return;
			m_coolDown = m_maxCoolDown;
			if (!FindProcessId()) {
				m_isOpen = false;
				return;
			}
			FindProcessHandle();
			FindWindowHandle();
			if (FindModuleBase())
				m_isOpen = true;
			else
				m_isOpen = false;
		}

		//Getters

		const wchar_t* GetProcessName() { return m_processName; }
		const wchar_t* GetModuleName() { return m_moduleName; }
		HANDLE GetProcessHandle() { return m_processHandle; }
		DWORD GetProcessId() { return m_processId; }
		HWND GetWindowHandle() { return m_windowHandle; }
		uintptr_t GetModuleBase() { return m_moduleBase; }
		const bool& isOpen() { return m_isOpen; }
	private:
		/*Checking if target is open. Used while this class is constructing.*/
		bool isReady() {
			if (!FindProcessId())
				return false;
			FindProcessHandle();
			FindWindowHandle();
			if (!FindModuleBase())
				return false;

			return true;
		}
		bool FindProcessId() {
			bool found = false;
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnap != INVALID_HANDLE_VALUE) {
				PROCESSENTRY32 procEntry;
				procEntry.dwSize = sizeof(procEntry);

				if (Process32First(hSnap, &procEntry)) {
					do {
						if (!_wcsicmp(procEntry.szExeFile, m_processName)) {
							m_processId = procEntry.th32ProcessID;
							found = true;
							break;
						}
					} while (Process32Next(hSnap, &procEntry));
				}
			}
			CloseHandle(hSnap);
			if (m_processId == 0 || !found) {
				std::cout << "[1]!ERROR! m_processId = 0\n";
				std::wcout << "[2] m_processName = " << static_cast<std::wstring>(m_processName) << "\n";
				return false;
			}
			else
				return true;
		}
		bool FindProcessHandle() {
			m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, m_processId);
			return m_processHandle ? true : false;
		}
		bool FindWindowHandle() {
			m_windowHandle = FindWindowA(NULL, m_windowName);
			return m_windowHandle ? true : false;
		}
		bool FindModuleBase() {
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_processId);
			if (hSnap != INVALID_HANDLE_VALUE) {
				MODULEENTRY32 modEntry;
				modEntry.dwSize = sizeof(modEntry);
				if (Module32First(hSnap, &modEntry)) {
					do {
						if (!_wcsicmp(modEntry.szModule, m_moduleName)) {
							m_moduleBase = (uintptr_t)modEntry.modBaseAddr;
							break;
						}
					} while (Module32Next(hSnap, &modEntry));
				}
			}
			CloseHandle(hSnap);
			if (m_moduleBase == 0) {
				std::cout << "[1]!ERROR! m_moduleBase = 0\n";
				std::cout << "[2] m_moduleName = " << static_cast<std::wstring>(m_moduleName).c_str() << "\n";
				std::cout << "[3] m_processId = " << m_processId << "\n";
				std::cout << "[4] m_processName = " << static_cast<std::wstring>(m_processName).c_str() << "\n";
				return false;
			}
			else
				return true;
		}
	};
	class Address {
		/*Contains name(key) and addresses*/
		std::map<std::string, uintptr_t> m_addresses;
		/*To be updated*/
		std::map<std::string, std::vector<uintptr_t>> m_offsets;
		Process* m_process;
	public:
		Address(Process* l_process) : m_process(l_process) {
			Setup();
		}
		~Address() {}

		template <typename T>
		//Returns value pointed by the address
		T ReadMemory(uintptr_t l_address) {
			T tmpValue;
			ReadProcesspemem(m_process->GetProcessHandle(), (BYTE*)l_address, &tmpValue, sizeof(tmpValue), 0);

			return tmpValue;
		}

		template <typename T>
		void WriteMemory(uintptr_t l_where, T* l_value) {
			WriteProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)l_where, l_value, sizeof(*l_value), NULL);
		}

		//Getters
		const uintptr_t& GetAddress(const std::string& l_name) {
			auto itr = m_addresses.find(l_name);
			if (itr == m_addresses.end())
				return 0;

			return itr->second;
		}
	private:
		/*Reads a file with keys(names) and addresses and stores it in m_addresses container*/
		void Setup() {
			std::ifstream file;
			file.open(pemem::GetDirectory() + "addresses.txt");
			if (!file) {
				std::cout << "!ERROR! addresses.addy not found!\n";
				return;
			}

			std::string line;
			while (std::getline(file, line)) {
				if (line[0] == '|') continue;
				std::stringstream keystream(line);
				std::string name;
				keystream >> name;
				std::string addressString;
				keystream >> addressString;
				uintptr_t address;
				std::istringstream ss(&addressString[2]);
				ss >> std::hex >> address;
				m_addresses.emplace(name, m_process->GetModuleBase() + address);
			}
			file.close();
		}
	};
	class Time {
		std::chrono::system_clock::time_point m_time1;
		std::chrono::system_clock::time_point m_time2;
		float m_elapsed;
	public:
		Time() : m_time1(std::chrono::system_clock::now()), m_time2(std::chrono::system_clock::now()),
			m_elapsed(0) {
		}
		~Time() {}
		void Update() {
			m_time2 = std::chrono::system_clock::now();
			std::chrono::duration<float> elapsed = m_time2 - m_time1;
			m_time1 = m_time2;
			m_elapsed = elapsed.count();
		}

		const float& GetElapsed() {
			return m_elapsed;
		}
	};
}