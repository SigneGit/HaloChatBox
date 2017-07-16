#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include <Psapi.h>



TCHAR* GetProcessName()
{
	TCHAR szExeFileName[MAX_PATH]; 
	GetModuleFileName(NULL, szExeFileName, MAX_PATH);

	std::string Text = "";
	//char buf[256];
	//sprintf(buf,"%S", szExeFileName);
	Text = szExeFileName;

	//MessageBox(NULL, buf, "Title", MB_OK);

	if(Text.find("halo.exe") != std::string::npos){
		return "halo.exe";
	}
	if(Text.find("haloce.exe") != std::string::npos){
		return "haloce.exe";
	}
	return "NULL";
}

bool is_file_exist (const char *fileName) {
	std::ifstream infile(fileName);
	return infile.good();
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}


DWORD FindDmaAddy(int PointerLevel, DWORD Offsets[], DWORD BaseAddress)
{
	//DEFINES OUR ADDRESS to write to
	//if statements are crucial to make sure that the address is valid to write
	//otherwise we crash. Address will not be valid when things like map changes or game loads are happening
	DWORD Ptr = *(DWORD*)(BaseAddress); //Base Address
	if(Ptr == 0) return NULL;//prevent crash

	//this is done to allow us to have pointers up to many levels e.g.10
	for(int i = 0; i < PointerLevel; i ++)
	{
		//if it = PointerLevel-1 then it reached the last element of the array
		//therefore check if that address plus the offset is valid and leave the loop
		if(i == PointerLevel-1)
		{
			//!!make sure the last address doesnt have the asterisk on DWORD otherwise incoming crash
			Ptr = (DWORD)(Ptr+Offsets[i]);  //Add the final offset to the pointer
			if(Ptr == 0) return NULL;//prevent crash
			//we here return early because when it hits the last element
			//we want to leave the loop, specially adapted for offsets of 1
			return Ptr;
		}
		else
		{
			//if its just a normal offset then add it to the address
			Ptr = *(DWORD*)(Ptr+Offsets[i]); //Add the offsets
			if(Ptr == 0) return NULL;//prevent crash
		}
	}
	return Ptr;
}

//Print our pattern scan results if necessary
void MsgBoxAddy(DWORD addy)
{
	char szBuffer[1024];
	sprintf(szBuffer, "Addy: %02x", addy);
	MessageBox(NULL, szBuffer, "Title", MB_OK);

}


//Get all module related info, this will include the base DLL. 
//and the size of the module
MODULEINFO GetModuleInfo( char *szModule )
{
	MODULEINFO modinfo = {0};
	HMODULE hModule = GetModuleHandle(szModule);
	if(hModule == 0) 
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}


void WriteToMemory(uintptr_t addressToWrite, char* valueToWrite, int byteNum)
{
	//used to change our file access type, stores the old
	//access type and restores it after memory is written
	unsigned long OldProtection;
	//give that address read and write permissions and store the old permissions at oldProtection
	VirtualProtect((LPVOID)(addressToWrite), byteNum, PAGE_EXECUTE_READWRITE, &OldProtection);

	//write the memory into the program and overwrite previous value
	memcpy( (LPVOID)addressToWrite, valueToWrite, byteNum);

	//reset the permissions of the address back to oldProtection after writting memory
	VirtualProtect((LPVOID)(addressToWrite), byteNum, OldProtection, NULL);
}


DWORD FindPattern(char *module, char *pattern, char *mask)
{
	//Get all module related information
	MODULEINFO mInfo = GetModuleInfo(module);

	//Assign our base and module size
	//Having the values right is ESSENTIAL, this makes sure
	//that we don't scan unwanted memory and leading our game to crash
	DWORD base = (DWORD)mInfo.lpBaseOfDll;
	DWORD size =  (DWORD)mInfo.SizeOfImage;

	//Get length for our mask, this will allow us to loop through our array
	DWORD patternLength = (DWORD)strlen(mask);

	for(DWORD i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for(DWORD j = 0; j < patternLength; j++)
		{
			//if we have a ? in our mask then we have true by default, 
			//or if the bytes match then we keep searching until finding it or not
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		//found = true, our entire pattern was found
		//return the memory addy so we can write to it
		if(found) 
		{
			return base + i;
		}
	}

	return NULL;
} 