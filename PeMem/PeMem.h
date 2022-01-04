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

/*
[Brasil and U.S.A] Created by Pedro Sérgio de Castro Sarmento Filho. Current located in the U.S, contact
pedroma-guild@hotmail.com 
*/

/*Simple, automated, and unfinished set of classes that aims to read and write to addresses in a facile way.
It contains the Process class, which will be constantly looking for the target process.
It contains the Address class, which reads from a file that contains names, used as
"keys", and addresses of interest. That file can and should be edited and updated with addresses.
It is possible to store addresses that contains multiple offsets (Pointers)*/

/*Your addresses.txt should look something like: */
/*
|NAME| |ADDRESS|
Health 0x00000
Ammo 0x00000
Stamina 0x00000
*/ //Note that these addresses do not contains offsets(pointers)
   //Also, those are based on the module specified in the Process class constructor
/*What if you want to store addresses that contain multiple offsets, like a pointer? Easy enough*/
/*
	Pointer 2 Health 0x0000 0x1 0x2
	Pointer 1 Ammo 0x0000 0x1
	Pointer 5 Stamina 0x0000 0x1 0x4 0x1 0x2 0x1
*/
/*The first indicator must ALWAYS be named "Pointer" (case sensitive.)
Second indicator is how many offsets that address is pointing to.
Third is the base address.
Fourth and beyond are the offsets.*/

namespace pemem {

	/*Returns the path to where the exe of your program is located.*/
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

	/*Class responsible to find or(and) keep track of a process.*/
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

	/*Class will keep track of all the addresses of our game, read as listed in addresses.txt file.*/
	class Address {
		/*Contains name(key) and addresses*/
		std::map<std::string, uintptr_t> m_addresses;
		/*Used to keep track of bytes written, like in a nop*/
		struct MemoryData {
			uintptr_t address;
			unsigned int size;
			std::vector<BYTE> bytes;
		};
		/*Key and byte data*/
		std::map<std::string, MemoryData> m_bytes;
		Process* m_process;
	public:
		Address(Process* l_process) : m_process(l_process) {
			Setup();
		}
		~Address() {}

		template <typename T>
		/*Returns the value pointed by that address*/
		T ReadMemory(uintptr_t l_address) {
			T tmpValue;
			ReadProcessMemory(m_process->GetProcessHandle(), (BYTE*)l_address, &tmpValue, sizeof(tmpValue), 0);

			return tmpValue;
		}

		template <typename T>
		/*Writes to that address*/
		void WriteMemory(uintptr_t l_where, T* l_value) {
			WriteProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)l_where, l_value, sizeof(*l_value), NULL);
		}

		/*Writes 0x90 an x amount of times to that address and stores the original bytes in m_bytes*/
		void Nop(const std::string& l_key, uintptr_t l_where, const unsigned int& l_amountOfNops) {
			auto itr = m_bytes.find(l_key);
			if (itr != m_bytes.end())
				return;//if it's already in container, cancel.

			std::vector<BYTE> bytes;
			std::vector<BYTE> oldBytes;
			for (int i = 0; i < l_amountOfNops; i++) {
				bytes.push_back(0x90);
				oldBytes.push_back(0x0);
			}

			ReadProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)l_where, (BYTE*)&oldBytes.at(0), l_amountOfNops, NULL); //storing original bytes 
			WriteProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)l_where, (BYTE*)&bytes.at(0), l_amountOfNops, NULL);

			MemoryData memoryData;
			memoryData.address = l_where;
			memoryData.bytes = oldBytes;
			memoryData.size = l_amountOfNops;
			m_bytes.emplace(l_key, memoryData);
		}

		/*Writes the original bytes to where it was nopped before*/
		void Unop(const std::string& l_key) {
			auto itr = m_bytes.find(l_key);
			if (itr == m_bytes.end())
				return;// not in memory or wrong key, cancel.

			MemoryData* memoryData = &itr->second;

			WriteProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)memoryData->address, (BYTE*)&memoryData->bytes.at(0), memoryData->size, NULL);
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

				/*If address has offsets*/
				if (name == "Pointer") { 
					unsigned int depth; //offset count
					keystream >> depth;
					std::string key; //name of our address, used as key in container
					keystream >> key; 
					std::string baseAddressString;
					keystream >> baseAddressString; //Passing address as a string
					uintptr_t baseAddress = ConvertStringToPtr(baseAddressString); //Our address in uinptr_t
					baseAddress = m_process->GetModuleBase() + baseAddress; //Adding module
					std::vector<unsigned int> offsets;
					for (int i = 0; i < depth; i++) { //Looping through all the offsets
						std::string offsetString;
						keystream >> offsetString;
						uintptr_t offset = ConvertStringToPtr(offsetString);
						offsets.push_back(offset);
					}
					uintptr_t finalAddress = GetAddressPointer(baseAddress, offsets);
					m_addresses.emplace(key, finalAddress); //No need to add module, since we already added before
					continue; //Job done with this line, skipping this loop.
				}

				/*If address does not have offsets*/
				std::string addressString;
				keystream >> addressString;
				uintptr_t address = ConvertStringToPtr(addressString);
				m_addresses.emplace(name, m_process->GetModuleBase() + address); //Adding module to get static address
			}
			file.close();
		}

		/*Helper function to get address that has multiple offsets.*/
		uintptr_t GetAddressPointer(uintptr_t l_baseAddress, const std::vector<unsigned int> &l_offsets) {
			uintptr_t address = l_baseAddress;
			for (int i = 0; i < l_offsets.size(); i++) {
				ReadProcessMemory(m_process->GetProcessHandle(), (BYTE*)address, &address, sizeof(address), 0);
				address += l_offsets[i];
			}
			return address;
		}
		/*Helper function that converts a string to uintptr_t*/
		uintptr_t ConvertStringToPtr(std::string &l_addressString) {
			uintptr_t address;
			std::istringstream ss(&l_addressString[2]);
			ss >> std::hex >> address;
			return address;
		}
	};

	/*Trivial class used to create a time that we can use as a timer. Must call update function in main loop to work.*/
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
