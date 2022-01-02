#include "Address.h"

pemem::Address::Address(Process* l_process) : m_process(l_process) {
	Setup();
}

pemem::Address::~Address() {}

const uintptr_t& pemem::Address::GetAddress(const std::string& l_name) {
	auto itr = m_addresses.find(l_name);
	if (itr == m_addresses.end())
		return 0;

	return itr->second;
}

void pemem::Address::Setup() {
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
