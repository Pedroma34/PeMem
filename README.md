Simple, automated, and unfinished single header that aims to read and write to memory addresses in a facile way.
As of right now, it does not support layered addresses, like pointers. But it is in my to do list.
It requires a txt file in the same folder as the exe. The txt file should contain, for example:
```
|NAME|ADDRESS
Health 0x85BE94
MaxHealth 0x85BE96
PosX 0x86CE6C
PosY 0x86CE5C
PosX 0x86CE4C
AmmoDecrease 0x3087C3
ScreenWidth 0x8282CC
ScreenHeight 0x8282D0
|END|
```
The '|' is purely for commentary. Of course, these addresses are static.

EXAMPLE:
If you want to read the health of your player, first, you must know the type (float, int, short(2 byte), etc.)
Next, call ReadMemory from the address class to get the VALUE pointed by that address.
```
/*READING*/
pemem::Process process("Resident Evil 4", L"bio4.exe", L"bio4.exe");
pemem::Address address(&process);
auto healthAddress = address.GetAddress("Health"); //holding the address where our health is located.
//Note that we don't care about the type of our health value.
float healthValue = address.ReadMemory<float>(healthAddress); //We are reading the value that 
//this address is holding and copying to healthValue. Note that
//here we DO care about the type. If, for instance, the player's health is stored as a 2 byte value, we'd use short:
// short healthValue = address.ReadMemory<short>(healthAddress);
//or int
// int healthValue = address.ReadMemory<int>(healthAddress);
//You get the idea.
```

Writing is the same idea. We need an address and a reference to a variable that holds the value we want to write to that address.
```
/*WRITING*/
float maxHealth = 2400; //value that we want to write
address.WriteMemory<float>(healthAddress, &maxHealth); //Same as reading, we do care about the type. Easy enough.
```
