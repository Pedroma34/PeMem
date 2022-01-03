```
(IMPORTANT) ADD shlwapi.lib TO YOUR DEPENDENCIES!
```
# GO TO main.cpp FOR A GOOD EXAMPLE

# Descripition

Simple, automated, and unfinished single header that aims to read and write to memory addresses in a facile way.

# Easy way to access addresses
PeMem reads from a text file called addresses.txt, which is **MUST** be located at your program's exe folder. 
The txt file should contain. 
```Ruby
|NAME|ADDRESS
MaxHealth 0x85BE96
PosX 0x86CE6C
PosY 0x86CE5C
PosX 0x86CE4C
AmmoDecrease 0x3087C3
ScreenWidth 0x8282CC
ScreenHeight 0x8282D0
|END|
```

The '|' is purely for comments. All these addresses are static, of course. The module address will be added automatically. For example, if you find the your health address trough cheat engine, it will most likely be: "bio4.exe" + 0x85BE96. bio4.exe is the module. The way you tell PeMem the module is via the Process class constructor. As of right now, it only supports one module per process. However, it is in my to do list to change that.

# How to add an address that has multiple offsets (pointers)?
All the addresses above are solo, static addresses. Meaning that only adding the module to it will be enough (this is done in the background.)
## Adding layred addresses
First, you need to identify it by typing Pointer. Second, type how many offsets there are. Third, they name you want to give for this address. Fourth, the base address. Finally, you want to type all the offsets separated by spaces.
For example:
```Ruby
|NAME|ADDRESS
Pointer 1 Health 0x805F3C 0x4FB4
Pointer 3 Ammo 0x806D8C 0xA 0x04 0xC 0x6
Pointer 5 Stamina 0x807C8C 0x2 0x4 0x6 0x8 0xA
Money 0x858CAC
|END|
```
Simple enough, guys. If your address is not a pointer, like "biot.exe" + 0x858CAC, just give it a name and paste the address.

# Alright, how do I use it?

# CODE

```c++
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
```c++
/*WRITING*/
float maxHealth = 2400; //value that we want to write
address.WriteMemory<float>(healthAddress, &maxHealth); //Same as reading, we do care about the type. Easy enough.
```
# Using In Main Loop
What if you want to use it in your main loop?
```c++
pemem::Process process("Skyrim", "eo5.exe", "eo5.dll") // I made these names up lolz
if(process.isOpen() == false)
    return 0; //Game is not open. Let's abort.
pemem::Address address(&process);
pemem::Time time;

float newHealth = 100;
while(process.isOpen()){ //Loop will be alive as long as game is open
  time.Update();//Updating time count
  process.Update(); //It will keep track of your game.
  if(Numb1 == Pressed){
    address.WriteMemory<float>(address.GetAddress("Health"), &newHealth); //Regenerate player's health
  }
}
return 0; //Game was closed, let's close the program
```
Note that Update() will be looking for your game every x seconds, so it doesn't consume all your cpu power. If you want to be crazy and just to update every tick:
```c++
pemem::Process process("Skyrim", "eo5.exe", "eo5.dll", 0)
```
The float 0 will be the cooldown, which in this case is none. So on the update meothod, it will update every tick.

# How to know the name of the module of interest?
In cheat engine, or whatever program you prefer, once you find an static address or pointer, it will display as such:
```Ruby
"modulename.dll" + "address"
```
That address, let's say, stores the player's health. All you need to do is to pass that module name to the constructor and add that address to your address.txt file with the name "Health" before it.
#### The Current Problem
Some games have more than one module. So different addresses are in different modules. As of right now, PeMem does not support more than one module.
