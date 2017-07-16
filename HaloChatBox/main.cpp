#include "Main.h"
#include <windows.h>
#include "d3dhook.h"
#include "Memory.h"

#include "IniWriter.h"
#include "IniReader.h"




//These two functions are here so Halo loads the dll without needing to injecting it
//http://www.modacity.net/forums/showthread.php?24128-Halo-Custom-Edition-1.09-Sightjacker/page2
void OnRegister(LPDWORD lpModuleBase)
{
	// Fix the module base for controls.dll
	// Hard coding 'controls.dll' defeats the purpose of this whole routine, but idk why would rename it anyway
	*lpModuleBase = (DWORD)GetModuleHandleA("controls.dll");
}
DWORD* dwModuleBase = 0, Register_ret = 0;

// Note that halo calls this function twice.
extern "C" __declspec(dllexport, naked) bool Register()
{
	__asm pop Register_ret
	__asm mov dwModuleBase, ebx
	__asm add dwModuleBase, 20h

	OnRegister(dwModuleBase);

	__asm push Register_ret
	__asm mov al, 1

	__asm ret
}

void LogChat(std::string Text)
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X ", &tstruct);

	CreateDirectory(".\\ChatLogs", NULL);

	char ServerNameBuf[80];
	strftime(ServerNameBuf, sizeof(ServerNameBuf), "%Y-%m-%d-", &tstruct);
	std::string _ServerNameTemp = ServerNameBuf;
	_ServerNameTemp += (char*)CurrentServerIP;
	//_ServerNameTemp = _ServerNameTemp.substr(0, _ServerNameTemp.find(":", 0));
	replace(_ServerNameTemp,":"," ");
	_ServerNameTemp += ".txt";

	

	std::ofstream log(".\\ChatLogs\\" + _ServerNameTemp, std::ios_base::app | std::ios_base::out);
	log << buf + Text + "\n";
}









uintptr_t ContinueChat; //0x4AB4C7

typedef void (*pChatHandler)(const wchar_t* message, bool chat);
pChatHandler oChatHandler; 


PlayerInfoBackup PlayerBackup[16];

void chatHandler(const wchar_t* message, bool chat) {

	std::string Text = "";
	char buf[256];
	sprintf(buf,"%S", message);
	Text += buf;
	if(IniSettings.LogChat)
		LogChat(Text);



	std::string PlayerName0 = "";
	bool PlayerTeam0;
	std::string PlayerName1 = "";
	bool PlayerTeam1;

	D3DCOLOR Color[4];
	int size = 0;

	std::vector<char*> FormattedText;

	bool KillMessage = false; //Used to check if the current message is a kill message
	bool ChatMessage = false; //Used to check if the current message is a chat message
	bool TeamMessage = false; //Used to check if the current message is a team message
	bool LocalMessage = false; //Used to check if the current message is a local chat message
	bool TeamLocalMessage = false; //Used to check if the current message is a local team chat message

	//pull out the players names
	for(unsigned short i = 0;i < StaticPlayerHeader->MaxSlots;i++)
	{
		if(GetLocalPlayer(Local->PlayerIndex))
		{
			if(GetPlayerByIndex(i))
			{
				bool UsedBackup = false;
				std::string PlayerName = "";
				char PlayerBuf[256];
				sprintf(PlayerBuf,"%S", StaticPlayer->PlayerName1);
				PlayerName = PlayerBuf;



				if(PlayerName.size() == 0){ //when a player dies there name and team messes up? Needs to use a backup
					if(PlayerBackup[i].PlayerName.size() > 0)
					{
						PlayerName = PlayerBackup[i].PlayerName;
						UsedBackup = true;
					}
				}
				else
				{

					PlayerBackup[i].PlayerName = StringToChar(PlayerName);
					PlayerBackup[i].PlayerTeam = StaticPlayer->Team;

				}
				//Should be the first player in the message
				if(StartsWith(Text,PlayerName)) {
					if(StartsWith(Text,PlayerName + ": "))
					{
						if(MutedMenu.mi[i].on){ //should be if the current player is muted then just return
							return;
						}
					}

					PlayerName0 = PlayerName;
					if(!UsedBackup)
						PlayerTeam0 = StaticPlayer->Team;
					else
						PlayerTeam0 = PlayerBackup[i].PlayerTeam;

					continue; //continue here because it wont be the second player in the message
				}
				//Should be the second player in a death message
				//example "Example1 was killed by Example2"

				if(StartsWith(Text,"[" + PlayerName + "]: ")){
					if(MutedMenu.mi[i].on){ //should be if the current player is muted then just return
						return;
					}

					PlayerName0 = PlayerName;
					if(!UsedBackup)
						PlayerTeam0 = StaticPlayer->Team;
					else
						PlayerTeam0 = PlayerBackup[i].PlayerTeam;
					TeamMessage = true;

					continue;
				}

				if(Text.find(PlayerName) != std::string::npos && 
					!StartsWith(Text,"[" + PlayerName + "]: ")) { 
						PlayerName1 = PlayerName;
						if(!UsedBackup)
							PlayerTeam1 = StaticPlayer->Team;
						else
							PlayerTeam1 = PlayerBackup[i].PlayerTeam;
						KillMessage = true;
				}
			}
		}
	}

	if(PlayerName0.size() == 0 || PlayerName1.size() == 0) {
		for(unsigned int i = 0;i < 16;i++){


			if(StartsWith(Text,PlayerBackup[i].PlayerName)){
				if(PlayerBackup[i].PlayerName.size() > 0)
				{
					if(MutedMenu.mi[i].on){ //should be if the current player is muted then just return
						return;
					}
					PlayerName0 = PlayerBackup[i].PlayerName;
					PlayerTeam0 = PlayerBackup[i].PlayerTeam;
					continue;
				}
			}

			if(StartsWith(Text,"[" + PlayerBackup[i].PlayerName + "]: ")){
				if(MutedMenu.mi[i].on){
					return;
				}
				PlayerName0 = PlayerBackup[i].PlayerName;
				PlayerTeam0 = PlayerBackup[i].PlayerTeam;
				TeamMessage = true;
				continue;
			}



			if(Text.find(PlayerBackup[i].PlayerName) != std::string::npos){
				if(PlayerBackup[i].PlayerName.size() > 0)
				{
					PlayerName1 = PlayerBackup[i].PlayerName;
					PlayerTeam1 = PlayerBackup[i].PlayerTeam;
				}
			}






		}
	}


#pragma region Kills Region
	if(PlayerName0.size() > 0 && 
		PlayerName1.size() > 0)
	{
		if  (Text.find("was killed by") != std::string::npos) //These messages SHOULD be the only messages that have both player names
		{
			KillMessage = true;

			if(PlayerTeam0)
				Color[0] = tBlue;
			else
				Color[0] = tRed;

			Color[1] = tWhite;

			if(PlayerTeam1)
				Color[2] = tBlue;
			else
				Color[2] = tRed;
			size = 3;

			std::string Temp = Text;
			FormattedText.push_back(StringToChar(PlayerName0));

			Temp = Temp.erase(0,PlayerName0.size());
			Temp = Temp.erase(13,PlayerName1.size());
			FormattedText.push_back(" was killed by ");

			FormattedText.push_back(StringToChar(" " + PlayerName1));
		}

		if(Text.find("was betrayed by") != std::string::npos){
			KillMessage = true;

			if(PlayerTeam0)
				Color[0] = tBlue;
			else
				Color[0] = tRed;

			Color[1] = tWhite;

			if(PlayerTeam1)
				Color[2] = tBlue;
			else
				Color[2] = tRed;
			size = 3;
			std::string Temp = Text;
			FormattedText.push_back(StringToChar(PlayerName0));
			Temp = Temp.erase(0,PlayerName0.size());
			Temp = Temp.erase(15,PlayerName1.size());
			FormattedText.push_back( " was betrayed by ");
			FormattedText.push_back(StringToChar(" " + PlayerName1));
		}



	}

	if(PlayerName1.size() > 0){

		//if(Text.find("died") != std::string::npos)
		if(StartsWith(Text,PlayerName1 + " died"))
		{
			Color[1] = tWhite;
			if(PlayerTeam1)
				Color[0] = tBlue;
			else
				Color[0] = tRed;
			size = 2;

			std::string Temp = Text.erase(0, PlayerName1.size());
			FormattedText.push_back(StringToChar(Temp));
			FormattedText.push_back(StringToChar(" " + PlayerName1));
		}
		if(StartsWith(Text,"You killed"))
		{
			Color[0] = tWhite;
			if(PlayerTeam1)
				Color[1] = tBlue;
			else
				Color[1] = tRed;
			size = 2;

			std::string Temp = Text.erase(11, PlayerName1.size());
			FormattedText.push_back(StringToChar(Temp));
			FormattedText.push_back(StringToChar(" " + PlayerName1));
		}
	}
	if(PlayerName0.size() > 0){
		//if(Text.find("committed suicide") != std::string::npos)
		if(StartsWith(Text,PlayerName0 + " committed suicide"))
		{
			Color[1] = tWhite;
			if(PlayerTeam1)
				Color[0] = tBlue;
			else
				Color[0] = tRed;
			size = 2;

			std::string Temp = Text.erase(0, PlayerName0.size());
			FormattedText.push_back(StringToChar(PlayerName0 + " "));
			FormattedText.push_back(StringToChar(Temp));

		}
	}


#pragma endregion

#pragma region ChatMessages

	if(Text.find(": ") != std::string::npos)
	{
		if(StartsWith(Text,PlayerName0 + ": ")) //regular chat message
		{
			ChatMessage = true;
			if(PlayerTeam0)
				Color[0] = tBlue;
			else
				Color[0] = tRed;
			Color[1] = tWhite;
			size = 2;
			std::string Temp = Text.erase(0,PlayerName0.size());

			FormattedText.push_back(StringToChar(PlayerName0));
			FormattedText.push_back(StringToChar(Temp));
		}
		else if(StartsWith(Text, "[" + PlayerName0 + "]: ") && TeamMessage) //team chat message
		{
			ChatMessage = true;

			Color[0] = tGreen;
			if(PlayerTeam0)
				Color[1] = tBlue;
			else
				Color[1] = tRed;
			Color[2] = tGreen;
			Color[3] = tWhite;
			size = 4;
			std::string Temp = Text.erase(0,PlayerName0.size() + 2);

			FormattedText.push_back("[");
			FormattedText.push_back(StringToChar(PlayerName0));
			FormattedText.push_back("]");
			FormattedText.push_back(StringToChar(Temp));
		}
	}
#pragma endregion

#pragma region LocalChat

	if(PlayerName0.size() == 0)
	{
		GetLocalPlayer(Local->PlayerIndex);
		std::string LocalPlayerName = "";
		char LocalPlayerBuf[256];
		sprintf(LocalPlayerBuf,"%S", LocalPlayer->PlayerName0);
		LocalPlayerName = LocalPlayerBuf;

		if  (StartsWith(Text,"Welcome " + LocalPlayerName)) {

			Color[0] = tWhite;

			if(LocalPlayer->Team)
				Color[1] = tBlue;
			else
				Color[1] = tRed;
			size = 2;

			std::string Temp = Text.erase(8, LocalPlayerName.size());
			FormattedText.push_back(StringToChar(Temp));
			FormattedText.push_back(StringToChar(" " + LocalPlayerName));
		}

		if(StartsWith(Text,LocalPlayerName)
			&& (Text.find("was killed by") == std::string::npos) 
			&& (Text.find("was betrayed by") == std::string::npos)){
				PlayerName0 = LocalPlayerName;
				PlayerTeam0 = LocalPlayer->Team;
				ChatMessage = true;
				LocalMessage = true;

				if(PlayerTeam0)
					Color[0] = tBlue;
				else
					Color[0] = tRed;
				Color[1] = tWhite;
				size = 2;
				std::string Temp = Text.erase(0,PlayerName0.size());

				FormattedText.push_back(StringToChar(PlayerName0));
				FormattedText.push_back(StringToChar(Temp));
		}
		else if(StartsWith(Text,"[" + LocalPlayerName + "]: ")) {

			PlayerName0 = LocalPlayerName;
			PlayerTeam0 = LocalPlayer->Team;
			ChatMessage = true;
			TeamLocalMessage = true;

			Color[0] = tGreen;
			if(PlayerTeam0)
				Color[1] = tBlue;
			else
				Color[1] = tRed;
			Color[2] = tGreen;
			Color[3] = tWhite;
			size = 4;
			std::string Temp = Text.erase(0,PlayerName0.size() + 2);

			FormattedText.push_back("[");
			FormattedText.push_back(StringToChar(PlayerName0));
			FormattedText.push_back("]");
			FormattedText.push_back(StringToChar(Temp));
		}

		if(PlayerName1.size() > 0 && (Text.find("was killed by") != std::string::npos || 
			Text.find("was betrayed by") != std::string::npos))
		{
			PlayerName0 = LocalPlayerName;
			PlayerTeam0 = LocalPlayer->Team;
			KillMessage = true;

			if(PlayerTeam0)
				Color[0] = tBlue;
			else
				Color[0] = tRed;

			Color[1] = tWhite;

			if(PlayerTeam1)
				Color[2] = tBlue;
			else
				Color[2] = tRed;

			size = 3;
			std::string Temp = Text;
			FormattedText.push_back(StringToChar(PlayerName0));
			if(Text.find("was killed by") != std::string::npos){
				Temp = Temp.erase(0,PlayerName0.size());
				Temp = Temp.erase(13,PlayerName1.size());
				FormattedText.push_back(" was killed by ");
			}
			else{
				Temp = Temp.erase(0,PlayerName0.size());
				Temp = Temp.erase(15,PlayerName1.size());
				FormattedText.push_back( " was betrayed by ");
			}

			FormattedText.push_back(StringToChar(" " + PlayerName1));
			//FormattedText.push_back(StringToChar(Temp));
		}



	}







#pragma endregion

#pragma region Welcome / quit

	//Broken for some reason? The player isn't actually assigned a team or name on this frame but the message gets sent?
	if (StartsWith(Text,"Welcome " + PlayerName1)) {
		//if(Text.find("quit") != std::string::npos){

		Color[0] = tWhite;
		if(PlayerTeam1)
			Color[1] = tBlue;
		else
			Color[1] = tRed;
		size = 2;

		std::string Temp = Text.erase(8,PlayerName1.size());
		FormattedText.push_back(StringToChar(Temp));
		FormattedText.push_back(StringToChar(" " + PlayerName1));
	}

	if(Text.find("quit") != std::string::npos 
		&& (Text.find(":") == std::string::npos)){
			if(PlayerName0.size() == 0){
				for(unsigned int i = 0;i < 16;i++){
					if(StartsWith(Text,PlayerBackup[i].PlayerName)){
						if(PlayerBackup[i].PlayerName.size() > 0)
						{
							PlayerName0 = PlayerBackup[i].PlayerName;
							PlayerTeam0 = PlayerBackup[i].PlayerTeam;
							PlayerBackup[i].PlayerName = "";
						}
					}
				}
			}
			if(PlayerTeam0)
				Color[0] = tBlue;
			else
				Color[0] = tRed;
			Color[1] = tWhite;
			size = 2;

			std::string Temp = Text.erase(0,PlayerName0.size());
			FormattedText.push_back(StringToChar(PlayerName0));
			FormattedText.push_back(StringToChar(Temp));
	}



#pragma endregion

	if(KillMessage){ //sanity check incase it can't find the players name for whatever reason (i've seen it happen)
		if(PlayerName0.size() == 0 || 
			PlayerName1.size() == 0) 
			KillMessage = false;
	}

	if(FormattedText.size() > 4){
		for(unsigned int i = 0;i < FormattedText.size();i++)
			FormattedText.erase(FormattedText.begin());
		hkTextSend("An error occurred (array higher than 4) while formatting this message:");
		hkTextSend(StringToChar(Text + " " + PlayerName0 + " " + PlayerName1));
	}

	ChatMenu.AddItemToChat(Text,FormattedText,Color,size, PlayerName0,PlayerTeam0,PlayerName1,PlayerTeam1,KillMessage,ChatMessage,TeamMessage);
}

void __declspec(naked) chatHandlerStub() {
	__asm {
		pushad
			pushfd
			xor ecx, ecx
			test ebx, ebx
			jne not_chat
			mov cl, 1
not_chat:
		lea ebx, [esp + 0x30]
		mov ebx, [ebx]
		push ecx
			push ebx
			call chatHandler
			add esp, 8
			popfd
			popad
			xor eax, eax
			jmp ContinueChat
	}
}




DWORD WINAPI initChat(LPVOID)
{
	DWORD VTable[5] = {0};

	while(GetModuleHandleA("d3d9.dll")==NULL)
		Sleep(250);

	DX_Init(VTable);

	TCHAR* ProcessName = GetProcessName();

	DWORD dwOldProtect = 0;
	DWORD ChatHand = FindPattern(ProcessName,"\xA1\x00\x00\x00\x00\x85\xC0\x74\x69\x8B\x0D\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00","x????xxxxxx????xx????");

	if(ChatHand != NULL){
		VirtualProtect((void*)ChatHand, 10, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		oTextOut = (pTextOut)DetourFunc((PBYTE)ChatHand,(PBYTE)&chatHandlerStub, 6);
		VirtualProtect((void*)ChatHand, 10, dwOldProtect, &dwOldProtect);
	}

	DWORD ChatHand1 = FindPattern(ProcessName,"\xA1\x00\x00\x00\x00\x85\xC0\x74\x66\x8B\x0D\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00","x????xxxxxx????xx????");
	if(ChatHand1 != NULL){
		WriteToMemory(ChatHand1,"\x90\x90\x90\x90\x90\x90\x90\xEB",8);
	}
	DWORD ChatHand2 = FindPattern(ProcessName,"\xA1\x00\x00\x00\x00\x85\xC0\x74\x6E\x8B\x0D\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00","x????xxxxxx????xx????");;

	if(ChatHand2 != NULL){
		WriteToMemory(ChatHand2,"\x90\x90\x90\x90\x90\x90\x90\xEB",8);
	}

	dwTextOut = FindPattern(ProcessName,"\x83\xEC\x10\x57\x8B\xF8\xA0\xFC\x2E\x6B\x00\x84\xC0","xxxxxxxxxxxxx");

	ContinueChat = FindPattern(ProcessName,"\x85\xC0\x74\x69\x8B\x0D\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00","xxxxxx????xx????");

	CurrentServerIP = 0x6A3F38;


	//DWORD LocalAddy = FindPattern(ProcessName,"\x00\x00\x70\xEC\xFF\xFF\xFF\xFF\x01\x00\x00\x00\x00\x00\xFF\xFF","xxxxxxxxxxxxxxxx");

	//MsgBoxAddy(LocalAddy);


	/*DWORD dwOldProtect = 0;
	DWORD ChatHand = 0x4AB4C2;

	VirtualProtect((void*)ChatHand, 10, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	oTextOut = (pTextOut)DetourFunc((PBYTE)ChatHand,(PBYTE)&chatHandlerStub, 6);
	VirtualProtect((void*)ChatHand, 10, dwOldProtect, &dwOldProtect);


	HANDLE hand;

	DWORD ChatHand1 = 0x4AB24C;
	BYTE buffer[8] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90,0x90,0xEB};

	DWORD fOld = 0;
	hand = GetCurrentProcess();
	VirtualProtectEx(hand, (void*)ChatHand1, 8,PAGE_EXECUTE_READWRITE,&fOld);
	WriteProcessMemory(hand, (void*)ChatHand1,buffer,8,nullptr);
	VirtualProtectEx(hand, (void*)ChatHand1, 8,fOld,&fOld);

	DWORD ChatHand2 = 0x4AB400;

	VirtualProtectEx(hand, (void*)ChatHand2, 8,PAGE_EXECUTE_READWRITE,&fOld);
	WriteProcessMemory(hand, (void*)ChatHand2,buffer,8,nullptr);
	VirtualProtectEx(hand, (void*)ChatHand2, 8,fOld,&fOld);

	*/

	HOOK(EndScene,VTable[ES]);
	HOOK(Reset,VTable[RS]);
	HOOK(SetViewport,VTable[SV]);

	Sleep(3000);
	hkTextSend("BetterChatbox by Xzero213 version 0.1!");

	DWORD LocalAddyOffset[] = {0x4};


	LocalAddy = FindDmaAddy(1,LocalAddyOffset,0x87A478);

	//GetIniValues();

	if(is_file_exist(".\\CONTROLS\\HaloChatBox.ini"))
	{
		CIniReader iniReader(".\\CONTROLS\\HaloChatBox.ini");
		char* _Font = iniReader.ReadString("settings","font","Ariel");
		IniSettings.Font = _Font;
		int BigFontSize = iniReader.ReadInteger("settings","bigfontsize",15);
		IniSettings.BigFontSize = BigFontSize;
		int SmallFontSize = iniReader.ReadInteger("settings","smallfontsize",13);
		IniSettings.SmallFontSize = SmallFontSize;
		char* _LogChat = iniReader.ReadString("multiplayer","logchat","false");
		IniSettings.LogChat = _LogChat;
	}else{

		std::ofstream log(".\\CONTROLS\\HaloChatBox.ini", std::ios_base::app | std::ios_base::out);
		log << "//This file contains all the settings for the chatbox\n";
		log << "//Everything under Chatbox settings should be left on default unless you know what your doing\n";
		log << "//If you change anything in here and it breaks simply delete this file\n";
		log << "//a new one will be created when you start Halo again\n";
		log << "\n";
		log << "//Chatbox settings\n";
		log << "//Font - Any font you have installed\n";
		log << "//BigFontSize - anything higher than 20 is to big\n";
		log << "//SmallFontSize - Anything smaller than 10 is to small\n";
		log << "\n";
		log << "//Multiplayer Options\n";
		log << "//logchat - True/False enables logging of chat\n";


		CIniWriter iniWriter(".\\CONTROLS\\HaloChatBox.ini");

		



		iniWriter.WriteString("settings","font","Ariel");
		iniWriter.WriteInteger("settings","bigfontsize",15);
		iniWriter.WriteInteger("settings","smallfontsize",13);
		iniWriter.WriteString("multiplayer","logchat","false");

		IniSettings.Font = "Ariel";
		IniSettings.BigFontSize = 15;
		IniSettings.SmallFontSize = 13;
		IniSettings.LogChat = "false";

		hkTextSend("Inifile created in CONTROLS\\HaloChatBox.ini");
	}






	//hkTextSend(IniSettings.BigFontSize);
	//hkTextSend(IniSettings.SmallFontSize);
	//hkTextSend(IniSettings.LogChat);


	//0x87A478



	//hkTextSend(IntToChar(test));

	//TCHAR szExeFileName[MAX_PATH]; 
	//GetModuleFileName(NULL, szExeFileName, MAX_PATH);

	//hkTextSend(GetProcessName());


	return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ulReason, LPVOID lpReserved )
{
	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread( 0, 0, initChat, 0, 0, 0 );
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}











