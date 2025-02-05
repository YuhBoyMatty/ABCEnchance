#include <metahook.h>
#include <map>
#include <vector>
#include <string>

#include "mymathlib.h"

#include "hud.h"
#include "weapon.h"
#include "Color.h"
#include "vguilocal.h"
#include "local.h"

#include "glew.h"
#include "gl_shader.h"
#include "gl_draw.h"
#include "gl_utility.h"
#include "gl_def.h"

#include "cvardef.h"
#include "weaponbank.h"

#include "ammo.h"
#include "CCustomHud.h"
#include "wmenu_annular.h"

CWeaponMenuAnnular m_HudWMenuAnnular;
void __UserCmd_OpenAnnularMenu(void) {
	if (!m_HudWMenuAnnular.m_bOpeningMenu && !m_HudWMenuAnnular.m_bSelectMenuDisplay) {
		if (m_HudWMenuAnnular.m_fFade <= gEngfuncs.GetClientTime())
			PlaySoundByName("common/wpn_hudon.wav", 1);
		m_HudWMenuAnnular.m_bOpeningMenu = true;
		gCustomHud.SetMouseVisible(true);
	}
}
void __UserCmd_CloseAnnularMenu(void) {
	if (m_HudWMenuAnnular.m_bOpeningMenu && m_HudWMenuAnnular.m_bSelectMenuDisplay) {
		m_HudWMenuAnnular.m_bOpeningMenu = false;
		m_HudWMenuAnnular.m_fFade = 0;
		m_HudWMenuAnnular.m_bSetedCursor = false;
		m_HudWMenuAnnular.m_bSelectMenuDisplay = false;
		gCustomHud.SetMouseVisible(false);
	}
}
void CWeaponMenuAnnular::Select() {
	if (!m_bOpeningMenu) {
m_fFade = 0;
m_bSelectMenuDisplay = false;
	}
}
void CWeaponMenuAnnular::GLInit() {
	glGenFramebuffers(1, &m_hGaussianBufferFBO);
	m_hGaussianBufferTex = GL_GenTextureRGB8(ScreenWidth, ScreenHeight);
}
void CWeaponMenuAnnular::Clear() {
	if (m_hGaussianBufferTex)
	{
		glDeleteTextures(1, &m_hGaussianBufferTex);
		m_hGaussianBufferTex = 0;
	}
	if (m_hGaussianBufferFBO)
	{
		glDeleteBuffers(1, &m_hGaussianBufferFBO);
		m_hGaussianBufferFBO = 0;
	}
}
void CWeaponMenuAnnular::Init() {
	ADD_COMMAND("+annularmenu", __UserCmd_OpenAnnularMenu);
	ADD_COMMAND("-annularmenu", __UserCmd_CloseAnnularMenu);

	if (SelectHoldTime <= 0)
		SelectHoldTime = 5;
}
void CWeaponMenuAnnular::VidInit() {
	SelectColor = pSchemeData->GetColor("WMenuAnnular.SelectColor", gDefaultColor);
	SelectRinColor = pSchemeData->GetColor("WMenuAnnular.SelectRinColor", gDefaultColor);
	SelectIconColor = pSchemeData->GetColor("WMenuAnnular.SelectIconColor", gDefaultColor);
	SelectTextColor = pSchemeData->GetColor("WMenuAnnular.SelectTextColor", gDefaultColor);
	SelectEmptyColor = pSchemeData->GetColor("WMenuAnnular.SelectEmptyColor", gDefaultColor);
	SelectPointerColor = pSchemeData->GetColor("WMenuAnnular.SelectPointerColor", gDefaultColor);

	SelectOffset = GET_SCREEN_PIXEL(true, "WMenuAnnular.SelectOffset");
	SelectSize = GET_SCREEN_PIXEL(true, "WMenuAnnular.SelectSize");
	SelectPointerSize = GET_SCREEN_PIXEL(true, "WMenuAnnular.SelectPointerSize");
	SelectRotate = atof(pSchemeData->GetResourceString("WMenuAnnular.SelectRotate"));
	SelectAnimateTime = atof(pSchemeData->GetResourceString("WMenuAnnular.SelectAnimateTime"));
	SelectFadeTime = atof(pSchemeData->GetResourceString("WMenuAnnular.SelectFadeTime"));
	SelectHoldTime = atof(pSchemeData->GetResourceString("WMenuAnnular.SelectHoldTime"));

	HUDFont = pSchemeData->GetFont("HUDShitFont", true);
	HUDSmallFont = pSchemeData->GetFont("HUDSmallShitFont", true);
}
void CWeaponMenuAnnular::DrawSelectIcon(WEAPON* wp, int a, int xpos, int ypos, int index) {
	wchar_t buf[64];
	int iTextWidth;
	int r, g, b, dummy;
	float h, s, v;
	int iHeight = wp->rcActive.bottom - wp->rcActive.top;
	int iWidth = wp->rcActive.right - wp->rcActive.left;
	if (gWR.HasAmmo(wp) && gWR.HasWeapon(wp))
		SelectIconColor.GetColor(r, g, b, dummy);
	else
		SelectEmptyColor.GetColor(r, g, b, dummy);
	if (gCVars.pAmmoMenuDrawRainbow->value > 0) {
		mathlib::RGBToHSV(r, g, b, h, s, v);
		h += 36 * index + 120;
		mathlib::HSVToRGB(h, s, v, r, g, b);
	}
	mathlib::ColorCalcuAlpha(r, g, b, a);
	SPR_Set(wp->hInactive, r, g, b);
	SPR_DrawAdditive(0, xpos - iWidth / 2, ypos - iHeight / 2, &wp->rcInactive);
	SelectTextColor.GetColor(r, g, b, dummy);
	if (gCVars.pAmmoMenuDrawRainbow->value > 0) {
		mathlib::RGBToHSV(r, g, b, h, s, v);
		h += 36 * index + 120;
		mathlib::HSVToRGB(h, s, v, r, g, b);
	}
	mathlib::ColorCalcuAlpha(r, g, b, a);
	if (gCVars.pAmmoMenuDrawPos->value > 0) {
		wsprintfW(buf, L"* %d", wp->iSlotPos);
		GetStringSize(buf, &iTextWidth, nullptr, HUDSmallFont);
		DrawVGUI2String(buf, xpos - iTextWidth - iWidth / 2, ypos - iHeight / 2, r, g, b, HUDSmallFont, true);
	}

	if (wp->iAmmoType >= 0) {
		if (wp->iClip >= 0)
			wsprintfW(buf, L"%d/%d", wp->iClip, gWR.CountAmmo(wp->iAmmoType));
		else
			wsprintfW(buf, L"%d", gWR.CountAmmo(wp->iAmmoType));
	}
	else
		wsprintfW(buf, L"");
	GetStringSize(buf, &iTextWidth, nullptr, HUDFont);
	DrawVGUI2String(buf, xpos - iTextWidth / 2, ypos + iHeight, r, g, b, HUDFont, true);
}
bool CWeaponMenuAnnular::IsVisible() {
	return m_bSelectMenuDisplay;
}
int CWeaponMenuAnnular::DrawWList(float flTime) {
	if (m_fFade <= flTime) {
		if (m_bSelectMenuDisplay) {
			m_bSelectMenuDisplay = false;
			m_fAnimateTime = 0;
		}
		return 1;
	}
	float flTimeDiffer = m_fFade - flTime;
	float flStartRot = (float)SelectRotate;
	int iBackGroundHeight = SelectSize;
	int iOffset = SelectOffset;
	float flAnimationRatio = 1.0f;
	if (!m_bSelectMenuDisplay) {
		m_fAnimateTime = flTime + SelectAnimateTime;
		//第一次开启时填充一次
		for (size_t i = 0; i < MAX_WEAPON_SLOT; i++){
			if (gWR.m_aryDrawMenu[i] <= 0) {
				WEAPON* t = gWR.GetFirstPos(i);
				if(t)
					gWR.m_aryDrawMenu[i] = t->iSlotPos;
			}
		}
	}
	if (m_fAnimateTime > flTime && SelectHoldTime > SelectAnimateTime) {
		flAnimationRatio = 1 - ((m_fAnimateTime - flTime) / SelectAnimateTime);
		iOffset *= flAnimationRatio;
		flAnimationRatio /= 100;
	}
	int i;
	float ac, as;
	vec2_t aryOut[10];
	vec2_t aryIn[10];
	WEAPON* wp;
	int halfWidth = ScreenWidth / 2;
	int halfHeight = ScreenHeight / 2;
	int xpos;
	int ypos;
	vec2_t vecA, vecB, vecC, vecD;
	int a = 255;
	int r, g, b, dummy;
	//填充十边形坐标数组
	for (i = 0; i < 10; i++) {
		ac = cos(2 * mathlib::Q_PI * i / 10 + flStartRot);
		as = sin(2 * mathlib::Q_PI * i / 10 + flStartRot);

		aryIn[i][0] = iOffset * ac;
		aryIn[i][1] = iOffset * as;

		aryOut[i][0] = (iOffset + iBackGroundHeight) * ac;
		aryOut[i][1] = (iOffset + iBackGroundHeight) * as;
	}

	if (flTimeDiffer < SelectFadeTime)
		a *= flTimeDiffer / SelectFadeTime;

	if (m_bOpeningMenu) {
		//模糊
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_hOldBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_hGaussianBufferFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hGaussianBufferTex, 0);
		GL_BlitFrameBufferToFrameBufferColorOnly(m_hOldBuffer, m_hGaussianBufferFBO, ScreenWidth, ScreenHeight, ScreenWidth, ScreenHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);
		DrawGaussianBlur(m_hGaussianBufferTex, flAnimationRatio, ScreenWidth, ScreenHeight);
		//绘制鼠标指针
		float flPointerSize = (float)SelectPointerSize;
		SelectPointerColor.GetColor(r, g, b, dummy);
		ac = cos(m_fCursorAngle - 0.45 * mathlib::Q_PI);
		as = sin(m_fCursorAngle - 0.45 * mathlib::Q_PI);
		vecA[0] = -flPointerSize * ac - (iOffset - flPointerSize) * as;
		vecA[1] = -flPointerSize * as + (iOffset - flPointerSize) * ac;
		vecB[0] = flPointerSize * ac - (iOffset - flPointerSize) * as;
		vecB[1] = flPointerSize * as + (iOffset - flPointerSize) * ac;
		vecC[0] = -flPointerSize * ac - iOffset * as;
		vecC[1] = -flPointerSize * as + iOffset * ac;
		vecD[0] = flPointerSize * ac - iOffset * as;
		vecD[1] = flPointerSize * as + iOffset * ac;
		vecA[0] += halfWidth;
		vecA[1] = halfHeight - vecA[1];
		vecB[0] += halfWidth;
		vecB[1] = halfHeight - vecB[1];
		vecC[0] += halfWidth;
		vecC[1] = halfHeight - vecC[1];
		vecD[0] += halfWidth;
		vecD[1] = halfHeight - vecD[1];
		DrawSPRIconPos(iSelectCyclerCursorPointer, kRenderTransAdd, vecC, vecA, vecB, vecD, r, g, b, a);
	}
	//绘制十边形
	for (i = 0; i < 10; i++) {
		//CABD
		//↓→↑
		mathlib::Q_Vector2Copy(aryIn[i == 9 ? 0 : i + 1], vecA);
		mathlib::Q_Vector2Copy(aryIn[i], vecB);
		mathlib::Q_Vector2Copy(aryOut[i == 9 ? 0 : i + 1], vecC);
		mathlib::Q_Vector2Copy(aryOut[i], vecD);
		//变换为OpenGL屏幕坐标
		mathlib::CenterPos2OpenGLPos(vecA, ScreenWidth, ScreenHeight);
		mathlib::CenterPos2OpenGLPos(vecB, ScreenWidth, ScreenHeight);
		mathlib::CenterPos2OpenGLPos(vecC, ScreenWidth, ScreenHeight);
		mathlib::CenterPos2OpenGLPos(vecD, ScreenWidth, ScreenHeight);
		//CABD
		SelectColor.GetColor(r, g, b, dummy);
		if (gCVars.pAmmoMenuDrawRainbow->value > 1) {
			float h, s, v;
			mathlib::RGBToHSV(r, g, b, h, s, v);
			h += 36 * i;
			s = 1;
			mathlib::HSVToRGB(h, s, v, r, g, b);
		}
		DrawSPRIconPos(iSelectCyclerSpr, kRenderTransAdd, vecC, vecA, vecB, vecD, r, g, b, a * 0.5);
		if (gWR.m_aryDrawMenu[i] == INVALID_WEAPON_POS || i == gWR.m_iNowSlot)
			continue;
		wp = gWR.GetWeapon(i, gWR.m_aryDrawMenu[i]);
		if (!wp)
			continue;
		xpos = (vecA[0] + vecB[0] + vecC[0] + vecD[0]) / 4.0f;
		ypos = (vecA[1] + vecB[1] + vecC[1] + vecD[1]) / 4.0f;
		DrawSelectIcon(wp, a, xpos, ypos, i);
	}
	//绘制已选
	if (gWR.m_aryDrawMenu[gWR.m_iNowSlot] != INVALID_WEAPON_POS) {
		wp = gWR.GetWeapon(gWR.m_iNowSlot, gWR.m_aryDrawMenu[gWR.m_iNowSlot]);
		if (!wp)
			return 1;
		mathlib::Q_Vector2Copy(aryIn[gWR.m_iNowSlot == 9 ? 0 : gWR.m_iNowSlot + 1], vecA);
		mathlib::Q_Vector2Copy(aryIn[gWR.m_iNowSlot], vecB);
		mathlib::Q_Vector2Copy(aryOut[gWR.m_iNowSlot == 9 ? 0 : gWR.m_iNowSlot + 1], vecC);
		mathlib::Q_Vector2Copy(aryOut[gWR.m_iNowSlot], vecD);
		mathlib::CenterPos2OpenGLPos(vecA, ScreenWidth, ScreenHeight);
		mathlib::CenterPos2OpenGLPos(vecB, ScreenWidth, ScreenHeight);
		mathlib::CenterPos2OpenGLPos(vecC, ScreenWidth, ScreenHeight);
		mathlib::CenterPos2OpenGLPos(vecD, ScreenWidth, ScreenHeight);
		xpos = (vecA[0] + vecB[0] + vecC[0] + vecD[0]) / 4;
		ypos = (vecA[1] + vecB[1] + vecC[1] + vecD[1]) / 4;
		DrawSelectIcon(wp, a, xpos, ypos, gWR.m_iNowSlot);
		SelectRinColor.GetColor(r, g, b, dummy);
		DrawSPRIconPos(iSelectCyclerRinSpr, kRenderTransAdd, vecC, vecA, vecB, vecD, r, g, b, a);
	}
	//绘制完毕，修改展示状态
	m_bSelectMenuDisplay = true;
	return 1;
}
void CWeaponMenuAnnular::Reset(){
	m_fFade = 0;
	m_fAnimateTime = 0;
	m_bSelectMenuDisplay = false;
	m_bOpeningMenu = false;
	m_bSetedCursor = false;

	iSelectCyclerSpr = SPR_Load("abcenchance/spr/select_cycler.spr");
	iSelectCyclerRinSpr = SPR_Load("abcenchance/spr/selected_rin.spr");
	iSelectCyclerCursorPointer = SPR_Load("abcenchance/spr/select_pointer.spr");
}