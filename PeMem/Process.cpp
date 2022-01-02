#include "Process.h"

pemem::Process::Process(const wchar_t* l_process, const wchar_t* l_module, const float& l_coolDown)
	: m_processName(l_process), m_moduleName(l_module), m_processHandle(0), m_processId(0), 
m_windowHandle(0), m_moduleBase(0), m_maxCoolDown(l_coolDown), m_coolDown(m_maxCoolDown) {

	m_isOpen = isReady() ? true : false;

}



pemem::Process::~Process() {}

void pemem::Process::Update(const float& l_time) {
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

const wchar_t* pemem::Process::GetProcessName() {
	return m_processName;
}

const wchar_t* pemem::Process::GetModuleName() {
	return m_moduleName;
}

HANDLE pemem::Process::GetProcessHandle() {
	return m_processHandle;
}

DWORD pemem::Process::GetProcessId() {
	return m_processId;
}

HWND pemem::Process::GetWindowHandle() {
	return m_windowHandle;
}

uintptr_t pemem::Process::GetModuleBase() {
	return m_moduleBase;
}

const bool& pemem::Process::isOpen() {
	return m_isOpen;
}

bool pemem::Process::isReady() {
	if (!FindProcessId())
		return false;
	FindProcessHandle();
	FindWindowHandle();
	if (!FindModuleBase())
		return false;

	return true;
}

bool pemem::Process::FindProcessId() {
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

bool pemem::Process::FindProcessHandle() {
	m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, m_processId);
	return m_processHandle ? true : false;
}

bool pemem::Process::FindWindowHandle() {
	m_windowHandle = FindWindowA(NULL, "Resident Evil 4");
	return m_windowHandle ? true : false;
}

bool pemem::Process::FindModuleBase() {
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