#include "PeMem.h"

/*
----->Please, read comments on PeMem.h<---------
*/


int main() {
	/*Process. Float value is optional. By default, it is set to 0.100f (0.1 seconds)*/
	pemem::Process process("Resident Evil 4", 
		L"bio4.exe", L"bio4.exe", 0.50f); 
	if (!process.isOpen()) {
		auto processname = process.GetProcessName();
		MessageBoxA(NULL, "Process NOT OPEN", "ERROR!", MB_ICONERROR | MB_OK);//error box
		return 0; //quit program if target window is not open
	}
	/*Address. Reads from a txt file called "addresses". Must create this file and file.*/
	pemem::Address address(&process);
	/*Time. It's a trivial class. Not recommended. Use SFML clock and time classes instead.*/
	pemem::Time time;

	/*Quick test, it will READ player's health*/
	short health = address.ReadMemory<short>(address.GetAddress("Health"));
	std::cout << "Player's health: " << health << "\n";

	/*Quick test, it will write to the player's health*/
	short maxHealth = 2400; //value we want to write to address
	address.WriteMemory<short>(address.GetAddress("Health"), &maxHealth);//Writing to address


	/*Main loop. Alive while we have target process open.*/
	while (process.isOpen()) {
		time.Update();//Reseting our elapsed time every tick.
		process.Update(time.GetElapsed());//Checking if our program is open every x seconds.
	}

	/*Program will terminate when target process is closed.*/
	return 0;
}