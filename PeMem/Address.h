#pragma once
#include <map>
#include <fstream>
#include <ostream>
#include <sstream>
#include <vector>
#include "Process.h"
#include "WorkingDirectory.h"

namespace pemem {
	class Address {
		/*Contains name(key) and addresses*/
		std::map<std::string, uintptr_t> m_addresses;
		/*To be updated*/
		std::map<std::string, std::vector<uintptr_t>> m_offsets;
		Process* m_process;
	public:
		Address(Process* l_process);
		~Address();

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
		const uintptr_t& GetAddress(const std::string& l_name);
	private:
		/*Reads a file with keys(names) and addresses and stores it in m_addresses container*/
		void Setup();
	};
}