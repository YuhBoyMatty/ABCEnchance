﻿#include <metahook.h>
#include "parsemsg.h"
#include "pm_shared.h"

#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <mymathlib.h>
#include <mathlib/Vector.h>

#include "hud.h"
#include "weapon.h"
#include "Color.h"
#include "vguilocal.h"
#include "vgui_controls/Controls.h"
#include "local.h"
#include "glew.h"
#include "gl_def.h"
#include "gl_draw.h"
#include "CCustomHud.h"

#include "enginedef.h"
#include "exportfuncs.h"

#include "IWeaponSelect.h"
#include "wmenu_annular.h"
#include "wmenu_slot.h"

#include "weaponbank.h"
#include "historyresource.h"
#include "ammo.h"

CHudCustomAmmo m_HudCustomAmmo;

pfnUserMsgHook m_pfnCurWeapon;
pfnUserMsgHook m_pfnWeaponList;
pfnUserMsgHook m_pfnCustWeapon;
pfnUserMsgHook m_pfnAmmoPickup;
pfnUserMsgHook m_pfnWeapPickup;
pfnUserMsgHook m_pfnItemPickup;
pfnUserMsgHook m_pfnAmmoX;
pfnUserMsgHook m_pfnHideWeapon;
pfnUserMsgHook m_pfnHideHUD;
pfnUserMsgHook m_pfnWeaponSpr;

int __MsgFunc_AmmoX(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();
	int iCount = READ_LONG();
	gWR.SetAmmo(iIndex, abs(iCount));
	return m_pfnAmmoX(pszName, iSize, pbuf);
}
int __MsgFunc_AmmoPickup(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();
	int iCount = READ_LONG();
	gHR.AddToHistory(HistoryResource::HISTSLOT_AMMO, iIndex, abs(iCount));
	return m_pfnAmmoPickup(pszName, iSize, pbuf);
}
int __MsgFunc_WeapPickup(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_SHORT();
	gHR.AddToHistory(HistoryResource::HISTSLOT_WEAP, iIndex);
	return m_pfnWeapPickup(pszName, iSize, pbuf);
}
int __MsgFunc_ItemPickup(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	const char* szName = READ_STRING();
	gHR.AddToHistory(HistoryResource::HISTSLOT_ITEM, szName);
	return m_pfnItemPickup(pszName, iSize, pbuf);
}
int __MsgFunc_WeaponList(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);

	WEAPON* Weapon = new WEAPON();
	strcpy_s(Weapon->szName, READ_STRING());
	strcpy_s(Weapon->szSprName, Weapon->szName);
	Weapon->iAmmoType = READ_CHAR();
	Weapon->iMax1 = READ_LONG();
	if (Weapon->iMax1 == 255)
		Weapon->iMax1 = -1;

	Weapon->iAmmo2Type = READ_CHAR();
	Weapon->iMax2 = READ_LONG();
	if (Weapon->iMax2 == 255)
		Weapon->iMax2 = -1;

	Weapon->iSlot = READ_CHAR();
	Weapon->iSlotPos = READ_CHAR();
	Weapon->iId = READ_SHORT();
	Weapon->iFlags = READ_BYTE();
	Weapon->iClip = 0;
	gWR.AddWeapon(Weapon);
	return m_pfnWeaponList(pszName, iSize, pbuf);
}
int __MsgFunc_CustWeapon(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	int id = READ_SHORT();
	char name[128];
	strcpy_s(name, READ_STRING());
	if (name[0] != 0)
		gWR.LoadWeaponSprites(id, name);;
	return m_pfnCustWeapon(pszName, iSize, pbuf);

}
int __MsgFunc_CurWeapon(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	int iState = READ_BYTE();
	if (iState > 0){
		int iId = READ_SHORT();
		//sc反编汇后如此
		if (iId == -1){
			gWR.DropAllWeapons();
			return m_pfnCurWeapon(pszName, iSize, pbuf);
		}
		int iClip = READ_LONG();
		int iClip2 = READ_LONG();
		if(gCustomHud.m_iPlayerHealth > 0)
			m_HudCustomAmmo.m_bAcceptDeadMessage = false;
		WEAPON* pWeapon = gWR.GetWeapon(iId);
		if (!pWeapon)
			return m_pfnCurWeapon(pszName, iSize, pbuf);
		//更新弹匣信息
		pWeapon->iClip = iClip;
		pWeapon->iClip2 = iClip2;
		m_HudCustomAmmo.m_pWeapon = pWeapon;
		m_HudCustomAmmo.m_bIsOnTarget = iState > 1;
	}
	else{
		int iFlag1 = READ_BYTE();
		int iFlag2 = READ_BYTE();
		int iAll = iFlag1 + iFlag2;
		switch (iAll){
		case 0X1FE:{
			if(m_HudCustomAmmo.m_bAcceptDeadMessage)
				gWR.DropAllWeapons();
			if(gCustomHud.m_iPlayerHealth <= 0)
				m_HudCustomAmmo.m_bAcceptDeadMessage = true;
			break;
		} 
		case 0:gWR.DropAllWeapons();
		}
	}
	return m_pfnCurWeapon(pszName, iSize, pbuf);
}
int __MsgFunc_HideWeapon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	gCustomHud.m_iHideHUDDisplay = READ_BYTE();
	return m_pfnHideWeapon(pszName, iSize, pbuf);
}
int __MsgFunc_HideHUD(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	gCustomHud.m_iHideHUDDisplay = READ_BYTE();
	return m_pfnHideHUD(pszName, iSize, pbuf);
}
//uzi akimbo
//shit uzi
int __MsgFunc_WeaponSpr(const char* pszName, int iSize, void* pbuf){
	BEGIN_READ(pbuf, iSize);
	int id = READ_SHORT();
	char name[128];
	strcpy_s(name, READ_STRING());
	if (name[0] != 0) {
		WEAPON* wp = gWR.GetWeapon(id);
		if (wp && wp->iId > 0) {
			strcpy_s(wp->szSprName, name);
			gWR.LoadWeaponSprites(wp);
		}
	}
	return m_pfnWeaponSpr(pszName, iSize, pbuf);
}
void CustomSlotSetCallBack(cvar_t* vars){
	if (!vars->string || vars->string[0] == 0)
		return;
	int slot;
	sscanf_s(vars->name, "cl_customslot%d", &slot);
	slot--;
}
void ChangeWMenuStyleCallBack(cvar_t* vars) {
	if (vars->value > 0)
		m_HudCustomAmmo.m_pNowSelectMenu = &m_HudWMenuAnnular;
	else
		m_HudCustomAmmo.m_pNowSelectMenu = &m_HudWMenuSlot;
}
void CHudCustomAmmo::GLInit(){
	//所有选择菜单都要加载
	m_HudWMenuAnnular.GLInit();
}
int CHudCustomAmmo::Init(void){
	m_pfnCurWeapon = HOOK_MESSAGE(CurWeapon);
	m_pfnWeaponList = HOOK_MESSAGE(WeaponList);
	m_pfnCustWeapon = HOOK_MESSAGE(CustWeapon);
	m_pfnAmmoPickup = HOOK_MESSAGE(AmmoPickup);
	m_pfnWeapPickup = HOOK_MESSAGE(WeapPickup);
	m_pfnItemPickup = HOOK_MESSAGE(ItemPickup);
	m_pfnAmmoX = HOOK_MESSAGE(AmmoX);
	m_pfnHideWeapon = HOOK_MESSAGE(HideWeapon);
	m_pfnHideHUD = HOOK_MESSAGE(HideHUD);
	m_pfnWeaponSpr = HOOK_MESSAGE(WeaponSpr);

	//gCVars.pAmmoCSlot[0] = CREATE_CVAR("cl_customslot1", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[1] = CREATE_CVAR("cl_customslot2", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[2] = CREATE_CVAR("cl_customslot3", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[3] = CREATE_CVAR("cl_customslot4", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[4] = CREATE_CVAR("cl_customslot5", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[5] = CREATE_CVAR("cl_customslot6", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[6] = CREATE_CVAR("cl_customslot7", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[7] = CREATE_CVAR("cl_customslot8", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[8] = CREATE_CVAR("cl_customslot9", "", FCVAR_VALUE, CustomSlotSetCallBack);
	//gCVars.pAmmoCSlot[9] = CREATE_CVAR("cl_customslot10", "", FCVAR_VALUE, CustomSlotSetCallBack);
	gCVars.pAmmoMenuDrawPos = CREATE_CVAR("cl_menudrawpos", "0", FCVAR_VALUE, NULL);
	gCVars.pAmmoMenuDrawRainbow = CREATE_CVAR("cl_rainbowmenu", "1", FCVAR_VALUE, NULL);
	gCVars.pAmmoMenuStyle = CREATE_CVAR("cl_wmenustyle", "0", FCVAR_VALUE, ChangeWMenuStyleCallBack);

	//所有选择菜单都要加载
	m_HudWMenuAnnular.Init();
	m_HudWMenuSlot.Init();
	Reset();
	//设置所选菜单
	if(gCVars.pAmmoMenuStyle->value > 0)
		m_pNowSelectMenu = &m_HudWMenuAnnular;
	else
		m_pNowSelectMenu = &m_HudWMenuSlot;
	gWR.Init();
	gHR.Init();
	return 1;
};
void CHudCustomAmmo::Reset(void){
	m_bSelectBlock = false;
	m_pWeapon = nullptr;
	m_bIsOnTarget = false;
	VGUI_CREATE_NEWTGA_TEXTURE(iBackGroundTga, "abcenchance/tga/ammobar_background");

	//所有选择菜单都要加载
	m_HudWMenuAnnular.Reset();
	m_HudWMenuSlot.Reset();

	gWR.Reset();
	gHR.Reset();
}
int CHudCustomAmmo::VidInit(void){
	ElementGap = GET_SCREEN_PIXEL(true, "AmmoHUD.ElementGap");
	BackGroundY = GET_SCREEN_PIXEL(true, "AmmoHUD.BackGroundY");
	BackGroundLength = GET_SCREEN_PIXEL(false, "AmmoHUD.BackGroundLength");

	Ammo1IconColor = pSchemeData->GetColor("AmmoHUD.Ammo1IconColor", gDefaultColor);
	Ammo1BigTextColor = pSchemeData->GetColor("AmmoHUD.Ammo1BigTextColor", gDefaultColor);
	Ammo1TextColor = pSchemeData->GetColor("AmmoHUD.Ammo1TextColor", gDefaultColor);
	Ammo2IconColor = pSchemeData->GetColor("AmmoHUD.Ammo2IconColor", gDefaultColor);
	Ammo2BigTextColor = pSchemeData->GetColor("AmmoHUD.Ammo2BigTextColor", gDefaultColor);
	Ammo2TextColor = pSchemeData->GetColor("AmmoHUD.Ammo2TextColor", gDefaultColor);

	HUDFont = pSchemeData->GetFont("HUDShitFont", true);
	HUDSmallFont = pSchemeData->GetFont("HUDSmallShitFont", true);

	gHR.VidInit();
	gWR.LoadAllWeaponSprites();
	m_HudWMenuAnnular.VidInit();
	m_HudWMenuSlot.VidInit();
	return 1;
}
bool CHudCustomAmmo::IsVisible() {
	if (m_pNowSelectMenu != nullptr)
		return m_pNowSelectMenu->IsVisible();
	return false;
}
bool CHudCustomAmmo::ShouldDraw() {
	if (gCustomHud.IsInSpectate())
		return false;
	if (gCustomHud.IsHudHide(HUD_HIDEALL | HUD_HIDEWEAPONS))
		return false;
	if (!gCustomHud.HasSuit())
		return false;
	if (gClientData->health <= 0)
		return false;
	return true;
}
bool CHudCustomAmmo::BlockAttackOnce() {
	if (m_bSelectBlock) {
		m_bSelectBlock = false;
		return true;
	}
	return false;
}
void CHudCustomAmmo::Select() {
	if (!IsVisible())
		return;
	if (m_pNowSelectMenu->m_fFade > gEngfuncs.GetClientTime()) {
		if (m_HudCustomAmmo.m_bAcceptDeadMessage)
			return;
		m_HudCustomAmmo.ChosePlayerWeapon();
	}
	m_pNowSelectMenu->Select();
	gWR.m_iNowSlot = -1;
	m_bSelectBlock = true;
}
int CHudCustomAmmo::Draw(float flTime){
	if (!ShouldDraw())
		return 1;
	// Draw Weapon Menu
	DrawWList(flTime);
	gHR.DrawAmmoHistory(flTime);
	if (!m_pWeapon)
		return 0;
	WEAPON* pw = m_pWeapon;
	if (pw->iId <= 0)
		return 0;
	if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
		return 0;

	int r, g, b, a;
	int nowX = ScreenWidth;
	int nowY = ScreenHeight;
	int HalfY = ScreenHeight - ((ScreenHeight - BackGroundY) / 2);
	int iTextHeight;
	int iTextWidth;
	wchar_t buf[16];
	//从右往左绘制
	vgui::surface()->DrawSetTexture(-1);
	vgui::surface()->DrawSetColor(255, 255, 255, 255);
	vgui::surface()->DrawSetTexture(iBackGroundTga);
	vgui::surface()->DrawTexturedRect(nowX - BackGroundLength, BackGroundY, ScreenWidth, ScreenHeight);

	nowX -= BackGroundLength / 6;
	if (pw->iAmmo2Type > 0) {
		Ammo2IconColor.GetColor(r, g, b, a);
		nowX -= (m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left);
		nowY = HalfY - (m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top) / 2;
		SPR_Set(m_pWeapon->hAmmo2, r, g, b);
		SPR_DrawAdditive(0, nowX, nowY, &m_pWeapon->rcAmmo2);
		nowX -= ElementGap;
		if (pw->iClip2 >= 0) {
			Ammo2TextColor.GetColor(r, g, b, a);
			wsprintfW(buf, L"%d", gWR.CountAmmo(pw->iAmmo2Type));
			GetStringSize(buf, &iTextWidth, &iTextHeight, HUDSmallFont);
			nowY = HalfY - iTextHeight / 2;
			nowX -= iTextWidth;
			DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDSmallFont);

			Ammo2BigTextColor.GetColor(r, g, b, a);
			wsprintfW(buf, L"%d/", pw->iClip2);
			GetStringSize(buf, &iTextWidth, &iTextHeight, HUDFont);
			nowY = HalfY - iTextHeight / 2;
			nowX -= iTextWidth;
			DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDFont);
		}
		else {
			Ammo2BigTextColor.GetColor(r, g, b, a);
			wsprintfW(buf, L"%d", gWR.CountAmmo(pw->iAmmo2Type));
			GetStringSize(buf, &iTextWidth, &iTextHeight, HUDFont);
			nowY = HalfY - iTextHeight / 2;
			nowX -= iTextWidth;
			DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDFont);
		}
		nowX -= ElementGap;
		wsprintfW(buf, L"|");
		GetStringSize(buf, &iTextWidth, &iTextHeight, HUDFont);
		nowY = HalfY - iTextHeight / 2;
		nowX -= iTextWidth;
		DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDFont);
		nowX -= ElementGap;
	}
	if (pw->iAmmoType > 0){
		if (pw->iAmmo2Type <= 0)
			nowX = ScreenWidth - (BackGroundLength / 3);
		if (pw->iClip >= 0){
			Ammo1TextColor.GetColor(r, g, b, a);
			wsprintfW(buf, L"%d", gWR.CountAmmo(pw->iAmmoType));
			GetStringSize(buf, &iTextWidth, &iTextHeight, HUDSmallFont);
			nowX -= iTextWidth;
			nowY = HalfY - iTextHeight / 2;
			DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDSmallFont);

			Ammo1BigTextColor.GetColor(r, g, b, a);
			wsprintfW(buf, L"%d/", pw->iClip);
			GetStringSize(buf, &iTextWidth, &iTextHeight, HUDFont);
			nowX -= iTextWidth;
			nowY = HalfY - iTextHeight / 2;
			DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDFont);
		}
		else{
			Ammo1BigTextColor.GetColor(r, g, b, a);
			wsprintfW(buf, L"%d", gWR.CountAmmo(pw->iAmmoType));
			GetStringSize(buf, &iTextWidth, &iTextHeight, HUDFont);
			nowX -= iTextWidth;
			nowY = HalfY - iTextHeight / 2;
			DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDFont);
		}
		nowX -= ElementGap;
		Ammo1IconColor.GetColor(r, g, b, a);
		nowX -= (m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left);
		nowY = HalfY - (m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top) / 2;
		SPR_Set(m_pWeapon->hAmmo, r, g, b);
		SPR_DrawAdditive(0, nowX, nowY, &m_pWeapon->rcAmmo);
	}
	return 1;
}
void CHudCustomAmmo::ChosePlayerWeapon(){
	WEAPON* wp = gWR.GetWeapon(gWR.m_iNowSlot, gWR.m_aryDrawMenu[gWR.m_iNowSlot]);
	if (wp != nullptr) {
		if (!(wp->iFlags & ITEM_FLAG_SELECTONEMPTY) && !gWR.HasAmmo(wp))
			return;
		ServerCmd(wp->szName);
		PlaySoundByName("common/wpn_select.wav", 1);
	}
}
void CHudCustomAmmo::SlotInput(int iSlot, int fAdvance, bool bWheel){
	if (!gCustomHud.IsHudEnable())
		return;
	if (gCustomHud.IsTextMenuOpening()) {
		gCustomHud.m_flTextMenuDisplayTime = 0;
		gCustomHud.m_bTextMenuOpening = false;
		return;
	}
	if (!gCustomHud.HasSuit())
		return;
	gWR.SelectSlot(iSlot, fAdvance, bWheel);
}
int CHudCustomAmmo::DrawWList(float flTime){
	m_pNowSelectMenu->DrawWList(flTime);
	if (gCVars.pAmmoMenuStyle->value <= 0 && m_HudWMenuAnnular.m_bOpeningMenu)
		m_HudWMenuAnnular.DrawWList(flTime);
	return 1;
}
void CHudCustomAmmo::ClientMove(struct playermove_s* ppmove, qboolean server){
	if (m_HudWMenuAnnular.m_bOpeningMenu)
		m_HudWMenuAnnular.m_fFade = gEngfuncs.GetClientTime() + m_HudWMenuAnnular.SelectHoldTime;
}
void CHudCustomAmmo::IN_Accumulate(){
	if (m_HudWMenuAnnular.m_bOpeningMenu) {
		int x, y;
		gEngfuncs.GetMousePosition(&x, &y);
		if (!m_HudWMenuAnnular.m_bSetedCursor) {
			gEngfuncs.pfnSetMousePos(gScreenInfo.iWidth / 2, gScreenInfo.iHeight / 2);
			m_HudWMenuAnnular.m_bSetedCursor = true;
		}
		x -= gScreenInfo.iWidth / 2;
		y -= gScreenInfo.iHeight / 2;
		y = -y;
		m_HudWMenuAnnular.m_fCursorAngle = atan2(y, x);
		int s = m_HudWMenuAnnular.m_fCursorAngle / (0.2 * mathlib::Q_PI);
		s = m_HudWMenuAnnular.m_fCursorAngle >= 0 ? s : 9 + s;
		if (gWR.m_aryDrawMenu[s] != INVALID_WEAPON_POS)
			gWR.m_iNowSlot = s;
	}
}
void CHudCustomAmmo::Clear(){
	//所有选择列表都需要清理
	m_HudWMenuAnnular.Clear();
}