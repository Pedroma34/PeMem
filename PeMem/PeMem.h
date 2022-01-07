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
#include <vector>
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
		Process(const char* l_windowName, const wchar_t* l_process,
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

	/*External Hook class that will be used in the Address class. It is counseled not to use this class by itself.*/
	class ExternalHook {
		std::vector<BYTE> m_ourFunctBytes; //Holds the bytes of our function, acquired trough GrabBytes()
		uintptr_t m_toHook; //address of target's instrunction
		uintptr_t m_ourFunct; //hook function
		uintptr_t m_ourCave; //Where our function is located inside targets memory
		std::vector<BYTE> m_originalBytes; //Holds the original bytes of the target's address
		uintptr_t m_size; //length of original bytes
		HANDLE m_handle; //process handle
		bool m_isHooked;
	public:
		ExternalHook(HANDLE l_handle, uintptr_t l_ourFunction, uintptr_t l_where, const int& l_size) :
			m_handle(l_handle),
			m_ourFunct(l_ourFunction),
			m_toHook(l_where),
			m_size(l_size),
			m_isHooked(false) {

			GrabBytes(); //Saving our function in bytes

		}
		~ExternalHook() {}

		void HookOn() {
			if (m_isHooked == true)
				return;
			InsertFunction();
			m_isHooked = CreateDetour();
		}
		void HookOff() {
			if (m_isHooked == false)
				return;
			m_isHooked = Restore();
		}

		const bool& isHooked() { return m_isHooked; }
	private:
		/*Saves our function's bytes to container and returns the size of our function */
		int GrabBytes() {
			for (int i = 0;; i++) {
				if (*(BYTE*)(m_ourFunct + i) == 0xCC &&
					(*(BYTE*)(m_ourFunct + i - 1)) == 0xC3 ||
					(*(BYTE*)(m_ourFunct + i + 1)) == 0xCC &&
					(*(BYTE*)(m_ourFunct + i)) == 0xCC) {
					return i; // Returns the number of bytes found
				}

				//Add the byte to the vector
				m_ourFunctBytes.push_back(*(BYTE*)(m_ourFunct + i));
			}
		}

		/*Allocates a new space in memory for our function*/
		void InsertFunction() {
			uintptr_t oldProtect, Bkup;


			int sizeOfFunction = m_ourFunctBytes.size();

			//Allocate memory in the other process to place our hook function in
			LPVOID arg = VirtualAllocEx( /*It will return the address of the allocated region*/
				m_handle, /*Target process handle*/
				NULL/*the function determines where to allocate the region*/,
				sizeOfFunction + 0x5/*Size of our function plus jump instruction*/,
				MEM_RESERVE | MEM_COMMIT, /*Allocates memory*/
				PAGE_READWRITE);


			//Change protections of the newly allocated region, so we can write our function later
			VirtualProtectEx(m_handle, arg, sizeOfFunction + 0x5, PAGE_EXECUTE_READWRITE, (PDWORD)&oldProtect);


			//Calculate the jump to the original function (a.k.a relative address)
			uintptr_t relativeAddress = (m_toHook - (uintptr_t)arg) - 5;


			//Writing our function into the newly allocated memory
			WriteProcessMemory(m_handle, arg, &m_ourFunctBytes.at(0), sizeOfFunction, NULL);


			//At the end of it, add a jump
			BYTE jump = 0xE9;
			WriteProcessMemory(m_handle, (LPVOID)((uintptr_t)arg + sizeOfFunction), &jump, sizeof(jump), NULL);
			WriteProcessMemory(m_handle, (LPVOID)((uintptr_t)arg + sizeOfFunction + 0x1), &relativeAddress, sizeof(relativeAddress), NULL);

			//Used in the CreateDetour function
			m_ourCave = (uintptr_t)arg;
		}

		/*Writes jump in the target's address to our cave*/
		bool CreateDetour() {
			uintptr_t oldProtect, Bkup, relativeAddy;

			//The hook jmp takes 5 bytes
			if (m_size < 5)
				return false;


			//Make sure we can even write to the section of memory
			if (!VirtualProtectEx(m_handle, (LPVOID)m_toHook, m_size, PAGE_EXECUTE_READWRITE, (PDWORD)&oldProtect))
				return false;


			//Just some variables needed to apply the hook
			BYTE jump = 0xE9;
			BYTE NOP = 0x90;
			BYTE NOPS[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

			//Put the original function bytes into m_restoreBytes for when we want to undo the hook
			for (int i = 0; i < m_size; i++)
				m_originalBytes.push_back(0x0);

			ReadProcessMemory(m_handle, (LPVOID)m_toHook, &m_originalBytes.front(), m_size, NULL);

			//Calculate the jump to the hook function
			relativeAddy = (m_ourCave - m_toHook) - 5;
			//Write the jump
			WriteProcessMemory(m_handle, (LPVOID)m_toHook, &jump, sizeof(jump), NULL);
			WriteProcessMemory(m_handle, (LPVOID)(m_toHook + 0x1), &relativeAddy, sizeof(relativeAddy), NULL);

			//Write all of the nops at once, instead of making multiple calls
			int new_len = m_size - 5;
			if (new_len > 0)
				WriteProcessMemory(m_handle, (LPVOID)(m_toHook + 0x5), NOPS, new_len, NULL);


			//Restore the previous protection
			if (!VirtualProtectEx(m_handle, (LPVOID)m_toHook, m_size, oldProtect, (PDWORD)&Bkup))
				return false;


			return true;
		}

		/*Get original bytes to be written back*/
		bool Restore() {
			uintptr_t oldProtect, Bkup;

			//So we can restore the bytes
			if (!VirtualProtectEx(m_handle, (LPVOID)m_toHook, m_size, PAGE_EXECUTE_READWRITE, (PDWORD)&oldProtect))
				return false;


			//Write the original bytes back into the function
			WriteProcessMemory(m_handle, (LPVOID)m_toHook, &m_originalBytes.front(), m_size, NULL);

			if (!VirtualProtectEx(m_handle, (LPVOID)m_toHook, m_size, oldProtect, (PDWORD)&Bkup))
				return false;


			//Free the memory we allocated for our hook
			VirtualFreeEx(m_handle, (LPVOID)m_ourCave, 0, MEM_RELEASE);

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

		/*Contains name(key) and hook data*/
		std::map<std::string, std::unique_ptr<ExternalHook>> m_hooks;

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

		/*Editing bytes in a specific way.*/
		void EditBytes(const std::string& l_key, uintptr_t l_where, const std::vector<BYTE>& l_bytesToWrite,
			const unsigned int& l_sizeOfOriginalBytes) {

			auto itr = m_bytes.find(l_key);
			if (itr != m_bytes.end())
				return;//if it's already in container, cancel.

			std::vector<BYTE> originalBytes;
			for (int i = 0; i < l_sizeOfOriginalBytes; i++)
				originalBytes.push_back(0x0); //allocating space of the original bytes


			ReadProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)l_where, (BYTE*)&originalBytes.at(0), originalBytes.size(), NULL); //storing original bytes
			WriteProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)l_where, (BYTE*)&l_bytesToWrite.at(0), l_bytesToWrite.size(), NULL);

			MemoryData memory;
			memory.address = l_where;
			memory.bytes = originalBytes;
			memory.size = originalBytes.size();
			m_bytes.emplace(l_key, memory);
		}

		/*Restoring original bytes*/
		void UneditBytes(const std::string& l_key) {
			auto itr = m_bytes.find(l_key);
			if (itr == m_bytes.end())
				return;// not in memory or wrong key, cancel.

			MemoryData* memoryData = &itr->second;

			WriteProcessMemory(m_process->GetProcessHandle(),
				(LPVOID*)memoryData->address, (BYTE*)&memoryData->bytes.at(0), memoryData->size, NULL);

			m_bytes.erase(itr); //removing it from container
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

			m_bytes.erase(itr); //removing it from container
		}

		/*Creates an external hook. Our hook functions must end with int 3 int 3. See example int github.*/
		void Hook(const std::string& l_key, uintptr_t l_ourFunction, uintptr_t l_where, const unsigned int& l_size) {
			auto itr = m_hooks.find(l_key);
			if (itr != m_hooks.end())
				return; //Already in memory, doesn't need to be hooked again. Call UnHook function to disabel hook.

			auto newHook = std::make_unique<ExternalHook>(m_process->GetProcessHandle(), l_ourFunction,
				l_where, l_size);
			newHook->HookOn();
			if (!newHook->isHooked())
				return;//something went wrong
			m_hooks.emplace(l_key, std::move(newHook));
		}

		/*Restores original bytes to wherever it was written*/
		void UnHook(const std::string& l_key) {
			auto itr = m_hooks.find(l_key);
			if (itr == m_hooks.end())
				return; //Not in memory
			if (!itr->second->isHooked())
				return;
			itr->second->HookOff();
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
		uintptr_t GetAddressPointer(uintptr_t l_baseAddress, const std::vector<unsigned int>& l_offsets) {
			uintptr_t address = l_baseAddress;
			for (int i = 0; i < l_offsets.size(); i++) {
				ReadProcessMemory(m_process->GetProcessHandle(), (BYTE*)address, &address, sizeof(address), 0);
				address += l_offsets[i];
			}
			return address;
		}
		/*Helper function that converts a string to uintptr_t*/
		uintptr_t ConvertStringToPtr(std::string& l_addressString) {
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
