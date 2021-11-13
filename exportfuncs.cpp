#include "plugins.h"
//Lib
#include "cmath"
#include "malloc.h"
#include "cl_entity.h"
#include "mathlib.h"
#include "com_model.h"
#include "triangleapi.h"
#include "pm_movevars.h"
#include "glew.h"
#include "cvar_hook.h"
//Def
#include "hud.h"
#include "vguilocal.h"
#include "exportfuncs.h"
#include "CColor.h"
#include "weapon.h"
#include "myconst.h"
#include "extraprecache.h"
#include "regquery.h"
//HUD
#include "ammo.h"
#include "healthhud.h"
#include "basehealth.h"
#include "drawElement.h"
#include "CHudDelegate.h"
#include "local.h"
//efx
#include "efxenchance.h"
#include "viewmodellag.h"

cl_enginefunc_t gEngfuncs;
cl_exportfuncs_t gExportfuncs;
metaplugins_t g_metaplugins;
engine_studio_api_t gEngineStudio;
int gPluginVersion;

const clientdata_t* gClientData;
float m_hfov;

overviewInfo_t* gDevOverview;
baseweapon_t* (*g_rgBaseSlots)[10][26] = nullptr;
int* g_iVisibleMouse = nullptr;
refdef_t* g_refdef = nullptr;
//FINAL SHIT
void R_NewMap(void){
	ClearExtraPrecache();

	gHudDelegate->HUD_Reset();
	EfxReset();
	gHookFuncs.R_NewMap();
}
int __fastcall R_CrossHair_ReDraw(void* pthis, int dummy, int param_1){
	if (gCVars.pDynamicCrossHair->value > 0)
		return 0;
	return gHookFuncs.R_CrossHair_ReDraw(pthis, dummy, param_1);
}
void EVVectorScale(float* punchangle1, float scale, float* punchangle2){
	gHookFuncs.EVVectorScale(punchangle1, scale, punchangle2);
	VectorCopy(punchangle1, gHudDelegate->m_vecClientEVPunch);
}
int CL_IsDevOverview(void){
	return gHudDelegate->m_iIsOverView ? 1 : gHookFuncs.CL_IsDevOverview();
}
void CL_SetDevOverView(int param1){
	gHookFuncs.CL_SetDevOverView(param1);
	if (gHudDelegate->m_iIsOverView){
		(*(vec3_t*)(param1 + 0x1C))[YAW] = gHudDelegate->m_flOverViewYaw;
		*(float *)(param1 + 0x10) = gHudDelegate->m_vecOverViewOrg[0];
		*(float *)(param1 + 0x14) = gHudDelegate->m_vecOverViewOrg[1];
		gDevOverview->z_max = gHudDelegate->m_flOverViewZmax;
		gDevOverview->z_min = gHudDelegate->m_flOverViewZmin;
		gDevOverview->zoom = gHudDelegate->m_flOverViewScale;
	}	
}
void R_RenderView(int a1){
	gHudDelegate->HUD_PreRenderView(a1);
	gHookFuncs.R_RenderView(a1);
	gHudDelegate->HUD_PostRenderView(a1);
}
void R_ForceCVars(qboolean mp){
	if (CL_IsDevOverview())
		return;
	gHookFuncs.R_ForceCVars(mp);
}
model_t* CL_GetModelByIndex (int index){
	if (index >= EXTRPRECACHE_INDEX_BASE && index < EXTRPRECACHE_INDEX_BASE + MAX_EXTRA_PRECACHENUM)
		return g_ExtraPreacheModel[index - EXTRPRECACHE_INDEX_BASE];
	return gHookFuncs.CL_GetModelByIndex(index);
}
void Cvar_DirectSet(cvar_t* var, char* value) {
	gHookFuncs.Cvar_DirectSet(var, value);
	if (gCVarsHookMap.count(var) && gCVarsHookMap[var]) {
		gCVarsHookMap[var](var);
	}
}
void Sys_ErrorEx(const char* fmt, ...){
	char msg[4096] = { 0 };

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (gEngfuncs.pfnClientCmd)
		gEngfuncs.pfnClientCmd((char*)"escape\n");

	MessageBox(nullptr, msg, "Fatal Error", MB_ICONERROR);
	TerminateProcess((HANDLE)(-1), 0);
}
void CheckOtherPlugin(){
	mh_plugininfo_t info;
	g_metaplugins.renderer = g_pMetaHookAPI->GetPluginInfo("Renderer.dll", &info);
}

char* CheckLanguage() {
	char language[128] = { 0 };
	Sys_GetRegKeyValue((char*)"Software\\Valve\\Steam", (char*)"Language", 
		language, sizeof(language), (char*)"");
	if(strlen(language) < 0)
		strncpy_s(language, sizeof(language) - 1,
			"english", sizeof(language) - 1);
	language[sizeof(language) - 1] = 0;
	return language;
}
void FillEfxAddress(){
	Fill_EfxFunc(CL_TempEntAllocHigh);
	Fill_EfxFunc(CL_TempEntAlloc);
}
void FillAddress(){
	auto engineFactory = Sys_GetFactory((HINTERFACEMODULE)g_dwEngineBase);
	if (engineFactory && engineFactory("EngineSurface007", nullptr)){
#define R_NEWMAP_SIG "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"
		Fill_Sig(R_NEWMAP_SIG, g_dwEngineBase, g_dwEngineSize, R_NewMap);
#define R_ISCLOVERVIEW_SIG "\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE\xDA\xE9\xDF\xE0\xF6\xC4\x44\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xB8\x01\x00\x00\x00\xC3\x2A\x2A"
		Fill_Sig(R_ISCLOVERVIEW_SIG, g_dwEngineBase, g_dwEngineSize, CL_IsDevOverview);
#define GL_BIND_SIG "\x8B\x44\x24\x04\x39\x05\x2A\x2A\x2A\x2A\x2A\x2A\x50\x68\xE1\x0D\x00\x00\xA3\x2A\x2A\x2A\x2A\xFF\x15\x2A\x2A\x2A\x2A\xC3"
		Fill_Sig(GL_BIND_SIG, g_dwEngineBase, g_dwEngineSize, GL_Bind);
#define CL_SETDEVOVERVIEW "\xD9\x05\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xDC\xC9\xD9\x05\x2A\x2A\x2A\x2A\xDE\xE2\xD9\xC9\xD9\x1D\x2A\x2A\x2A\x2A\xD8\x0D\x2A\x2A\x2A\x2A\xD8\x2D\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xD9\xEE\xD9\x05\x2A\x2A\x2A\x2A\xD8\xD1\xDF\xE0\xD9\xE8\xD9\x05\x2A\x2A\x2A\x2A\xF6\xC4\x41\x2A\x2A\xD8\xC1\xD9\x15"
		Fill_Sig(CL_SETDEVOVERVIEW, g_dwEngineBase, g_dwEngineSize, CL_SetDevOverView);
#define R_FORCECVAR_SIG "\x83\x7C\x24\x2A\x00\x2A\x2A\x2A\x2A\x00\x00\x81\x3D\x2A\x2A\x2A\x2A\xFF\x00\x00\x00"
		Fill_Sig(R_FORCECVAR_SIG, g_dwEngineBase, g_dwEngineSize, R_ForceCVars);
#define CL_FINDMODELBYINDEX_SIG "\x83\xEC\x08\x56\x57\x8B\x7C\x24\x14\x8B\x34\xBD\x2A\x2A\x2A\x2A\x85\xF6\x75\x08\x5F\x33\xC0\x5E\x83\xC4\x08\xC3"
		Fill_Sig(CL_FINDMODELBYINDEX_SIG, g_dwEngineBase, g_dwEngineSize, CL_GetModelByIndex);

		DWORD addr;
#define R_VIEWREFDEF_SIG "\x68\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68"
		{
			addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwEngineBase, g_dwEngineSize, R_VIEWREFDEF_SIG, Sig_Length(R_VIEWREFDEF_SIG));
			Sig_AddrNotFound(g_refdef);
			auto r_refdef_viewangles = (vec_t*)(*(DWORD*)(addr + 28));
			g_refdef = (refdef_t*)((char*)r_refdef_viewangles - offsetof(refdef_t, viewangles));
		}
#define R_RENDERSCENE_SIG_SVENGINE "\xDD\xD8\xDD\xD8\xE8"
#define R_RENDERVIEW_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x34\x53\x56\x57\x8B\x7D\x08\x85\xFF"
		{
			Fill_Sig(R_RENDERVIEW_SIG_SVENGINE, g_dwEngineBase, g_dwEngineSize, R_RenderView);
			addr = (DWORD)Search_Pattern_From(gHookFuncs.R_RenderView, R_RENDERSCENE_SIG_SVENGINE);
			Sig_AddrNotFound(R_RenderScene);
			gHookFuncs.R_RenderScene = (void(*)(void))(addr + 5 + 4 + *(int*)(addr + 5));
		}
#define DEVOVERVIEW_SIG "\x83\xEC\x30\xDD\x5C\x24\x2A\xD9\x05"
		{
			addr = (DWORD)Search_Pattern(DEVOVERVIEW_SIG);
			Sig_AddrNotFound(gDevOverview);
			gDevOverview = (decltype(gDevOverview))(*(DWORD*)(addr + 9) - 0xC);
		}
	}
	auto pfnClientCreateInterface = Sys_GetFactory((HINTERFACEMODULE)g_dwClientBase);
	if (pfnClientCreateInterface && pfnClientCreateInterface("SCClientDLL001", 0)){
#define SC_GETCLIENTCOLOR_SIG "\x8B\x4C\x24\x04\x85\xC9\x2A\x2A\x6B\xC1\x58"
		Fill_Sig(SC_GETCLIENTCOLOR_SIG, g_dwClientBase, g_dwClientSize, GetClientColor);
#define R_EVVECTORSCALE_SIG "\x8B\x4C\x24\x04\xF3\x0F\x10\x4C\x24\x08\x8B\x44\x24\x0C\x0F\x28\xC1\xF3\x0F\x59\x01\xF3\x0F\x11\x00\x0F\x28\xC1\xF3\x0F\x59\x41\x04\xF3\x0F\x11\x40\x04\xF3\x0F\x59\x49\x08\xF3\x0F\x11\x48\x08"
		Fill_Sig(R_EVVECTORSCALE_SIG, g_dwClientBase, g_dwClientSize, EVVectorScale);
#define R_CROSSHAIR_REDRAW_SIG "\x8B\x51\x14\x85\xD2\x0F\x84\x8B\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\xF3\x0F\x2C\x40\x0C\x85\xC0\x7E\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x53\x56\x57\x74\x2A\x80\x3D\x2A\x2A\x2A\x2A\x00\x75\x2A\xF3\x0F\x2C\x79\x34\xF3\x0F\x2C\x59\x38\xEB\x2A"
		Fill_Sig(R_CROSSHAIR_REDRAW_SIG, g_dwClientBase, g_dwClientSize, R_CrossHair_ReDraw);
		DWORD addr;
#define R_SETPUNCHANGLE_SIG "\x83\xC4\x04\xD9\x1C\x24\x6A\x00\xE8\x93\x56\x05\x00\x83\xC4\x08\xF3\x0F\x10\x74\x24\x34\xF3\x0F\x10\xAC\x24\x98\x00\x00\x00\xF3\x0F\x10\x25\x2A\x2A\x2A\x2A\xF3\x0F\x58\xEE\xF3\x0F\x10\x44\x24\x74"
		{
			addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, R_SETPUNCHANGLE_SIG, sizeof(R_SETPUNCHANGLE_SIG) - 1);
			Sig_AddrNotFound(SetPunchAngle);
			gHookFuncs.SetPunchAngle = (decltype(gHookFuncs.SetPunchAngle))GetCallAddress(addr + 8);
		}
		//Fill_Sig(R_SETPUNCHANGLE_SIG, g_dwClientBase, g_dwClientSize, SetPunchAngle);
#define SC_UPDATECURSORSTATE_SIG "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00"
		{
			addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_UPDATECURSORSTATE_SIG, Sig_Length(SC_UPDATECURSORSTATE_SIG));
			Sig_AddrNotFound(g_iVisibleMouse);
			g_iVisibleMouse = *(decltype(g_iVisibleMouse)*)(addr + 11);
		}
#define SC_HUDAMMO_RESET_SIG "\x83\x49\x10\x01\x68\x78\x05\x00\x00\xC7\x41\x14\x00\x00\x00\x00\xC7\x41\x2C\x00\x00\x00\x00\xC7\x41\x18\x00\x00\x00\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\x68\xC0\x00\x00\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x18\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC3"
		{
			addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_HUDAMMO_RESET_SIG, Sig_Length(SC_HUDAMMO_RESET_SIG));
			Sig_AddrNotFound(g_rgBaseSlots);
			g_rgBaseSlots = *(decltype(g_rgBaseSlots)*)(addr + 33);
		}
		if(1){
			const char sigs1[] = "***PROTECTED***";
			auto Cvar_DirectSet_String = Search_Pattern_Data(sigs1);
			if (!Cvar_DirectSet_String)
				Cvar_DirectSet_String = Search_Pattern_Rdata(sigs1);
			Sig_VarNotFound(Cvar_DirectSet_String);
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 1) = (DWORD)Cvar_DirectSet_String;
			auto Cvar_DirectSet_Call = Search_Pattern(pattern);
			Sig_VarNotFound(Cvar_DirectSet_Call);
			gHookFuncs.Cvar_DirectSet = (decltype(gHookFuncs.Cvar_DirectSet))g_pMetaHookAPI->ReverseSearchFunctionBegin(Cvar_DirectSet_Call, 0x500);
			Sig_FuncNotFound(Cvar_DirectSet);
		}
	}
}
void InstallHook(){
	Fill_InlineEfxHook(R_BloodSprite);

	Fill_EngFunc(pfnPlaybackEvent);

	Install_InlineEngHook(pfnPlaybackEvent);
	Install_InlineEngHook(R_NewMap);
	Install_InlineEngHook(R_RenderView);
	Install_InlineEngHook(CL_IsDevOverview);
	Install_InlineEngHook(CL_SetDevOverView);
	Install_InlineEngHook(EVVectorScale);
	Install_InlineEngHook(R_CrossHair_ReDraw);
	Install_InlineEngHook(Cvar_DirectSet);
	Install_InlineEngHook(CL_GetModelByIndex);
}

void GL_Init(void){
	g_pMetaHookAPI->GetVideoMode(&gScreenInfo.iWidth, &gScreenInfo.iHeight, nullptr, nullptr);
	auto err = glewInit();
	if (GLEW_OK != err){
		Sys_ErrorEx("glewInit failed, %s", glewGetErrorString(err));
		return;
	}
	gHudDelegate->GL_Init();
}
void HUD_Init(void){
	HINTERFACEMODULE hVGUI2 = (HINTERFACEMODULE)GetModuleHandle("vgui2.dll");
	if (hVGUI2){
		CreateInterfaceFn fnVGUI2CreateInterface = Sys_GetFactory(hVGUI2);
		pSchemeManager = (vgui::ISchemeManager*)fnVGUI2CreateInterface(VGUI_SCHEME_INTERFACE_VERSION, nullptr);
		pLocalize = (vgui::ILocalize*)fnVGUI2CreateInterface(VGUI_LOCALIZE_INTERFACE_VERSION, nullptr);
	}
	g_pSurface = (vgui::ISurface*)Sys_GetFactory((HINTERFACEMODULE)g_hEngineModule)(VGUI_SURFACE_INTERFACE_VERSION, nullptr);
	pSchemeManager->LoadSchemeFromFile("abcenchance/ABCEnchance.res", "ABCEnchance");
	pScheme = pSchemeManager->GetIScheme(pSchemeManager->GetScheme("ABCEnchance"));
	gPluginVersion = atoi(pScheme->GetResourceString("Version"));
	if (gPluginVersion < PLUGIN_VERSION)
		Sys_ErrorEx("[ABCEnchance]:\nMissing Resource file: abcenchance/ABCEnchance.res\nRequire Version: %d\nYour Version: %d\n",
			PLUGIN_VERSION, gPluginVersion);
	;
	char szLocalFilePath[1024] = { 0 };
	sprintf_s(szLocalFilePath, sizeof(szLocalFilePath) - 1, 
		"abcenchance/localize/%s.txt", CheckLanguage());
	if(!pLocalize->AddFile(g_pFileSystem, szLocalFilePath))
		Sys_ErrorEx("[ABCEnchance]:\nMissing Localize file: %s\n", szLocalFilePath);

	gCVars.pDynamicHUD = CREATE_CVAR("cl_hud_csgo", "1", FCVAR_VALUE, nullptr);
	gCVars.pBloodEfx = CREATE_CVAR("abc_bloodefx", "1", FCVAR_VALUE, nullptr);
	gCVars.pBloodSpriteSpeed = CREATE_CVAR("abc_bloodsprite_speed", "128", FCVAR_VALUE, nullptr);
	gCVars.pBloodSpriteNumber = CREATE_CVAR("abc_bloodsprite_num", "32", FCVAR_VALUE, nullptr);
	gCVars.pGaussEfx = CREATE_CVAR("abc_gaussefx", "1", FCVAR_VALUE, nullptr);
	gCVars.pModelLag = CREATE_CVAR("cl_modellag", "1", FCVAR_VALUE, nullptr);
	gCVars.pModelLagAutoStop = CREATE_CVAR("cl_modellagautostop", "1", FCVAR_VALUE, nullptr);
	gCVars.pModelLagValue = CREATE_CVAR("cl_modellagvalue", "1.0", FCVAR_VALUE, nullptr);
	gCVars.pCamIdealHeight = CREATE_CVAR("cam_idealheight", "0", FCVAR_VALUE, nullptr);

	gHudDelegate = new CHudDelegate();
	gExportfuncs.HUD_Init();
	gHudDelegate->HUD_Init();
}
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	memcpy(&gEngineStudio, pstudio, sizeof(gEngineStudio));
	return gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio);
}
int HUD_VidInit(void){
	//Fillup Default CVars
	gCVars.pCvarDefaultFOV = gEngfuncs.pfnGetCvarPointer("default_fov");

	int result = gExportfuncs.HUD_VidInit();
	gHudDelegate->HUD_VidInit();
	return result;
}
int HUD_Redraw(float time, int intermission){
	DWORD dwAddress = (DWORD)g_pMetaHookAPI->SearchPattern(g_pMetaSave->pExportFuncs->HUD_VidInit, 0x10, "\xB9", 1);
	if (dwAddress){
		static DWORD pHUD = *(DWORD*)(dwAddress + 0x1);
		HUDLIST* pHudList = (HUDLIST*)(*(DWORD*)(pHUD + 0x0));
		while (pHudList){
			if (dynamic_cast<CHudBattery*>(pHudList->p) != nullptr){
				if (gCVars.pDynamicHUD->value <= 0)
					pHudList->p->m_iFlags &= HUD_ACTIVE;
				else
					pHudList->p->m_iFlags &= ~HUD_ACTIVE;
			}
			else if (dynamic_cast<CHudHealth*>(pHudList->p) != nullptr){
				if (gCVars.pDynamicHUD->value <= 0)
					pHudList->p->m_iFlags &= HUD_ACTIVE;
				else
					pHudList->p->m_iFlags &= ~HUD_ACTIVE;
			}
			else if (dynamic_cast<CHudAmmo*>(pHudList->p) != nullptr){
				gHookHud.m_Ammo = (dynamic_cast<CHudAmmo*>(pHudList->p));
				if (gCVars.pDynamicHUD->value <= 0)
					pHudList->p->m_iFlags &= HUD_ACTIVE;
				else
					pHudList->p->m_iFlags &= ~HUD_ACTIVE;
			}
			else if (dynamic_cast<CHudMenu*>(pHudList->p) != nullptr)
				gHookHud.m_Menu = (dynamic_cast<CHudMenu*>(pHudList->p));
			pHudList = pHudList->pNext;
		}
	}
	gHudDelegate->HUD_Draw(time);
	return gExportfuncs.HUD_Redraw(time, intermission);
}
void HUD_Reset(void){
	gExportfuncs.HUD_Reset();
}
void HUD_TxferLocalOverrides(struct entity_state_s* state, const struct clientdata_s* client){
	gClientData = client;
	gExportfuncs.HUD_TxferLocalOverrides(state, client);
}
int HUD_UpdateClientData (struct client_data_s* c, float f){
	m_hfov = c->fov;
	gHudDelegate->HUD_UpdateClientData(c,f);
	return gExportfuncs.HUD_UpdateClientData(c, f);
}
void HUD_ClientMove(struct playermove_s* ppmove, qboolean server){
	gHudDelegate->HUD_ClientMove(ppmove, server);
	return gExportfuncs.HUD_PlayerMove(ppmove, server);
}
void V_CalcRefdef(struct ref_params_s* pparams){
	gExportfuncs.V_CalcRefdef(pparams);
	if(!gExportfuncs.CL_IsThirdPerson())
		V_CalcViewModelLag(pparams);
	else
		pparams->vieworg[2] -= gCVars.pCamIdealHeight->value;
}
void IN_MouseEvent(int mstate){
	gHudDelegate->IN_MouseEvent(mstate);
	gExportfuncs.IN_MouseEvent(mstate);
}
void CL_CreateMove(float frametime, struct usercmd_s* cmd, int active) {
	if (g_iVisibleMouse && gHudDelegate->m_iVisibleMouse) {
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;
		gExportfuncs.CL_CreateMove(frametime, cmd, active);
		gHudDelegate->CL_CreateMove(frametime, cmd, active);
		*g_iVisibleMouse = iVisibleMouse;
	}
	else
		gExportfuncs.CL_CreateMove(frametime, cmd, active);
}
void IN_Accumulate(void){
	if (g_iVisibleMouse && gHudDelegate->m_iVisibleMouse){
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;
		gExportfuncs.IN_Accumulate();
		gHudDelegate->IN_Accumulate();
		*g_iVisibleMouse = iVisibleMouse;
	}
	else
		gExportfuncs.IN_Accumulate();
}
int HUD_AddEntity(int type, struct cl_entity_s* ent, const char* modelname) {
	gHudDelegate->HUD_AddEntity(type, ent, modelname);
	return gExportfuncs.HUD_AddEntity(type, ent, modelname);
}
int HUD_KeyEvent(int eventcode, int keynum, const char* pszCurrentBinding){
	return gHudDelegate->HUD_KeyEvent(eventcode, keynum, pszCurrentBinding) ? 
		gExportfuncs.HUD_Key_Event(eventcode, keynum, pszCurrentBinding) : 0;
}
void HUD_Clear(void){
	gHudDelegate->HUD_Clear();
	ClearExtraPrecache();
}