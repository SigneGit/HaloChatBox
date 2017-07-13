#include < Windows.h >
#include < Iostream >
#include < Assert.h >
#include < Direct.h >
#include < D3dx9.h >
#include < Fstream >
#include < Stdio.h >
#include < Vector >
#include < Time.h >
#include < D3d9.h >
#include < Winternl.h >


#define ES	0
#define RS	1
#define SV	2
#define DIP	3
#define SSS	4
#define HOOK(func,addy)	o##func = (t##func)DetourFunction((PBYTE)addy,(PBYTE)hk##func)



//Colors
#define tRed    D3DCOLOR_ARGB( 255, 255,   50,   50 )
#define tGreen  D3DCOLOR_ARGB( 255,   0, 255,   0 )
#define tBlue   D3DCOLOR_ARGB( 255,   50,   50, 255 )
#define tLightBlue   D3DCOLOR_ARGB( 255,   0,   153, 255 )
#define tYellow D3DCOLOR_ARGB( 255, 255, 255,   0 )
#define tPurple D3DCOLOR_ARGB( 255, 102,   0, 153 )
#define tWhite  D3DCOLOR_ARGB( 255, 255, 255, 255 )
#define tBlack  D3DCOLOR_ARGB( 255,   0,   0,   0 )
#define tOrange D3DCOLOR_ARGB( 255, 255, 127,   0 )
#define tPink   D3DCOLOR_ARGB( 255, 246,  90, 181 )
//End Colors

//Chatbox stuff

int MenuPosX = 50;
int MenuPosY = 300;
int TotalMenuItems = 0;
#define MAX_MENU_ITEMS 12

//End Chatbox Stuff

float g_fxCenter = 0;
float g_fyCenter = 0;



void *DetourFunc(BYTE *src, const BYTE *dst, const int len);

unsigned int g_uiStride = NULL;
static LPD3DXFONT Menu = NULL;
static LPD3DXFONT SmallFont = NULL;
static LPD3DXFONT BigFont = NULL;
D3DVIEWPORT9 pViewport;

UINT m_uiOffset = 0;
FLOAT m_fScreenX = 0;
FLOAT m_fScreenY = 0;
D3DVIEWPORT9 g_pViewPort;
ID3DXFont * g_pFont1 = NULL;
ID3DXFont * g_pFont2 = NULL;
ID3DXLine * g_pLine1 = NULL;
ID3DXSprite * g_pCursorSprite;
IDirect3DTexture9 * g_pCursorTexture;
IDirect3DVertexBuffer9 * g_pVertexBuffer;
IDirect3DTexture9 * g_pTextureRed = NULL;
IDirect3DTexture9 * g_pTextureBlue = NULL;
IDirect3DTexture9 * g_pTextureGreen = NULL;
IDirect3DTexture9 * g_pTextureYellow = NULL;


struct PlayerInfoBackup //Used to fix a bug / used to set the colors of quit
{
	std::string PlayerName;
	bool PlayerTeam;
};

struct Object_Table_Header
{
   unsigned char TName[32];          // 'object'
   unsigned short MaxObjects;        // Maximum number of objects - 0x800(2048 objects)
   unsigned short Size;                  // Size of each object array - 0x0C(12 bytes)
   unsigned long Unknown0;           // always 1?
   unsigned char Data[4];              // '@t@d' - translates to 'data'?
   unsigned short Max;                  // Max number of objects the game has reached (slots maybe?)
   unsigned short Num;                  // Number of objects in the current game
   unsigned short NextObjectIndex; // Index number of the next object to spawn
   unsigned short NextObjectID;      // ID number of the next object to spawn
   unsigned long FirstObject;          // Pointer to the first object in the table   
};
extern Object_Table_Header *ObjectTableHeader;
//-------------------------------------------
struct Object_Table_Array
{
   unsigned short ObjectID;           // Matches up to Object ID in static player table ( for players )
   unsigned short Unknown0;
   unsigned short Unknown1;
   unsigned short Size;                 // Structure size
   unsigned long Offset;                  // Pointer to the object data structure
};
extern Object_Table_Array *ObjectTableArray;
//-------------------------------------------
struct AMasterchief
{
   unsigned short BipdMetaIndex;   // [Biped]characters\cyborg_mp\cyborg_mp
   unsigned short BipdMetaID;      // [Biped]characters\cyborg_mp\cyborg_mp
   unsigned char Zeros_00[4];
   unsigned char BitFlags_00[4];
   unsigned long Timer_00;
   unsigned char BitFlags_01[4];
   unsigned long Timer_01;
   unsigned char Zeros_01[68];
   float m_World[3];
   float m_Velocity[3];
   float m_LowerRot[3];
   float m_Scale[3];
   unsigned char Zeros_02[12];
   unsigned long LocationID;
   unsigned long Pointer_00;
   float xUnknown;
   float yUnknown;
   float zUnknown;
   unsigned char Zeros_03[20];
   unsigned short PlayerIndex;
   unsigned short PlayerID;
   unsigned long Unknown00;
   unsigned char Zeros_04[4];
   unsigned short AntrMetaIndex; // [Animation Trigger]characters\cyborg\cyborg
   unsigned short AntrMetaID;   // [Animation Trigger]characters\cyborg\cyborg
   unsigned char BitFlags_02[8];
   unsigned char Unknown01[8];
   float Health;
   float Shield_00;
   unsigned long Zeros_05;
   float Unknown02;
   unsigned long Unknown03;
   float Unknown04;
   float Unknown05;
   unsigned char Unknown06[24];
   unsigned short VehicleWeaponIndex;
   unsigned short VehicleWeaponID;
   unsigned short WeaponIndex;
   unsigned short WeaponID;
   unsigned short VehicleIndex; // Ex: Turret on Warthog
   unsigned short VehicleID;
   unsigned short SeatType;
   unsigned char BitFlags_03[2];
   unsigned long Zeros_06;
   float Shield_01;
   float Flashlight_00;
   float Zeros_07;
   float Flashlight_01;
   unsigned char Zeros_08[20];
   unsigned long Unknown07;
   unsigned char Zeros_09[28];
   unsigned char Unknown08[8];
   unsigned char Unknown10[148];
   
   unsigned long IsCrouching;      // crouch = 1, jump = 2,both = 3
   unsigned char IsInvisible; // normal = 0x41 invis = 0x51 (bitfield?) //broken?

   //unsigned long IsInvisible; // normal = 0x41 invis = 0x51 (bitfield?)
   //unsigned char IsCrouching;      // crouch = 1, jump = 2


   unsigned char Unknown11[3];

  // char m_cZoom00; //Zoom -1,0,1 - not zoomed, x2,x8
   //char m_cZoom01;


   unsigned char Unknown09[884];
   float LeftThigh[13];
   float RightThigh[13];
   float Pelvis[13];
   float LeftCalf[13];
   float RightCalf[13];
   float Spine[13];
   float LeftClavicle[13];
   float LeftFoot[13];
   float Neck[13];
   float RightClavicle[13];
   float RightFoot[13];
   float Head[13];
   float LeftUpperArm[13];
   float RightUpperArm[13];
   float LeftLowerArm[13];
   float RightLowerArm[13];
   float LeftHand[13];
   float RightHand[13];
};
extern AMasterchief *Masterchief;
extern AMasterchief *LocalMC;
//-------------------------------------------
struct Static_Player_Header
{
   unsigned char TName[32]; // 'players'
   unsigned short MaxSlots; // Max number of slots/players possible
   unsigned short SlotSize; // Size of each Static_Player struct
   unsigned long Unknown; // always 1?
   unsigned char Data[4]; // '@t@d' - translated as 'data'?
   unsigned short IsInMainMenu; // 0 = in game 1 = in main menu / not in game
   unsigned short SlotsTaken; // or # of players
   unsigned short NextPlayerIndex; // Index # of the next player to join
   unsigned short NextPlayerID; // ID # of the next player to join
   unsigned long FirstPlayer; // Pointer to the first static player
};
extern Static_Player_Header *StaticPlayerHeader;
//-------------------------------------------
struct Static_Player
{
   unsigned short PlayerID;            // Stats at 0x70EC
   unsigned short PlayerID2;            // ???
   wchar_t PlayerName0[12];           // Unicode / Max - 11 Chars + EOS (12 total)
   long Unknown0;                     // Always -1 / 0xFFFFFFFF
   unsigned long Team;                // 0 = Red / 1 = Blue
   unsigned long SwapID;              // ObjectID
   unsigned short SwapType;           // 8 = Vehicle / 6 = Weapon
   short SwapSeat;                    // Warthog - Driver = 0 / Passenger = 1 / Gunner = 2 / Weapon = -1
   unsigned long RespawnTimer;        // ?????? Counts down when dead, Alive = 0
   unsigned long Unknown1;            // Always 0
   unsigned short ObjectIndex;
   unsigned short ObjectID;           // Matches against object table
   unsigned long Unknown3;            // Some sort of ID
   unsigned long LocationID;          // This is very, very interesting. BG is split into 25 location ID's. 1 -19
   long Unknown4;                     // Always -1 / 0xFFFFFFFF
   unsigned long BulletCount;         // Something to do with bullets increases - weird.
   wchar_t PlayerName1[12];           // Unicode / Max - 11 Chars + EOS (12 total)
   unsigned long Unknown5;            // 02 00 FF FF
   unsigned long PlayerIndex;
   unsigned long Unknown6;
   float SpeedModifier;
   unsigned char Unknown7[108];
   unsigned short Ping;
};
extern Static_Player *StaticPlayer;
extern Static_Player *LocalPlayer;

struct ALocal
{
	unsigned short PlayerIndex;
	unsigned short PlayerID;
	unsigned char Unknown00[160];
	unsigned short ObjectIndex;
	unsigned short ObjectID;
	unsigned char Unknown01[8];
	float m_fRot[3];
};
extern ALocal *Local;
//-------------------------------------------
struct ACamera
{
	float m_fWorld[3];
	float m_fShift[3]; // x,y,z -- forward/back,side,vertical
	float m_fBackDist;
	float m_fFov;      // default = 70 degrees
	float m_fForward[3];
	float m_fUp[3];
	float m_fVelocity[3];
};
extern ACamera *Camera;


unsigned long LocalAddy = 0x402AD408; //Possibly set
unsigned long StaticPlayerHeaderAddy = 0x402AAF94;
unsigned long ObjectTableHeaderAddy = 0x400506B4; //Set to correct

Object_Table_Header *ObjectTableHeader = (Object_Table_Header*)ObjectTableHeaderAddy;     //4BB206B4 = trial
Static_Player_Header *StaticPlayerHeader = (Static_Player_Header*)StaticPlayerHeaderAddy; //4BD7AF94 = trial
ALocal *Local = (ALocal*)LocalAddy; //4BD7D408 = trial
//ACamera *Camera = (ACamera*)CameraAddy; //006A3964 = trial
Object_Table_Array *ObjectTableArray;
AMasterchief *Masterchief;
AMasterchief *LocalMC;
Static_Player *StaticPlayer;
Static_Player *LocalPlayer;




