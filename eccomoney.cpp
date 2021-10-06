#include <metahook.h>
#include <cmath>
#include "hud.h"
#include "weapon.h"
#include "CColor.h"
#include "mathlib.h"

#include "local.h"
#include "vguilocal.h"
#include "myconst.h"
#include "CHudDelegate.h"
#include "glew.h"

#include "enginedef.h"
#include "exportfuncs.h"
#include "drawElement.h"
#include "utility.h"

#include "eccomoney.h"
CHudEccoMoney m_HudEccoMoney;
int CHudEccoMoney::Init(void){
	YOffset = GET_SCREEN_PIXEL(true, "Ecco.YOffset");
	BackgroundLength = GET_SCREEN_PIXEL(false, "Ecco.BackgroundLength");
	BackgroundHeight = GET_SCREEN_PIXEL(true, "Ecco.BackgroundHeight");

	MoneyColor = pScheme->GetColor("Ecco.MoneyColor", gDefaultColor);
	MoneyDecreseColor = pScheme->GetColor("Ecco.MoneyDecreseColor", gDefaultColor);
	MoneyIncreseColor = pScheme->GetColor("Ecco.MoneyIncreseColor", gDefaultColor);

	HUDSmallFont = pScheme->GetFont("HUDSmallShitFont", true);

	Reset();
	return 0;
}
void CHudEccoMoney::Reset() {
	VGUI_CREATE_NEWTGA_TEXTURE(MoneyBackGround, "abcenchance/tga/ecco_background");
	flStoreMoney = 0;
	iDifferMoney = 0;
	flNextUpdateTime = 0;
	flDifferDisapearTime = 0;
}
int CHudEccoMoney::Draw(float flTime){
	int iEcco = atoi(gEngfuncs.PhysInfo_ValueForKey("ecco_value"));
	if(iEcco == 0)
		iEcco = gHudDelegate->GetPlayerHUDInfo(gEngfuncs.GetLocalPlayer()->index)->frags;
	gHudDelegate->surface()->DrawSetTexture(-1);
	gHudDelegate->surface()->DrawSetColor(255, 255, 255, 255);
	gHudDelegate->surface()->DrawSetTexture(MoneyBackGround);
	int nowX = 0;
	int nowY = YOffset + BackgroundHeight;
	gHudDelegate->surface()->DrawTexturedRect(nowX, YOffset, BackgroundLength, nowY);
	int iTextHeight;
	wchar_t buf[16];
	int r, g, b, a;
	wsprintfW(buf, L"$%d", iEcco);
	GetStringSize(buf, NULL, &iTextHeight, HUDSmallFont);
	nowX = BackgroundLength / 8;
	nowY -= (BackgroundHeight + iTextHeight) / 2;
	MoneyColor.GetColor(r, g, b, a);
	DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDSmallFont, true);
	if (iEcco != flStoreMoney && flTime > flNextUpdateTime) {
		iDifferMoney = iEcco - flStoreMoney;
		flStoreMoney = iEcco;
		flDifferDisapearTime = flTime + 1.0F;
		flNextUpdateTime = flTime + 0.2F;
	}
	if (flTime < flDifferDisapearTime) {
		wsprintfW(buf, iDifferMoney > 0 ? L"+$%d" : L"-$%d", abs(iDifferMoney));
		GetStringSize(buf, NULL, &iTextHeight, HUDSmallFont);
		nowY -= iTextHeight + BackgroundHeight / 2;
		if (iDifferMoney > 0)
			MoneyIncreseColor.GetColor(r, g, b, a);
		else
			MoneyDecreseColor.GetColor(r, g, b, a);
		a *= flDifferDisapearTime - flTime;
		ColorCalcuAlpha(r, g, b, a);
		DrawVGUI2String(buf, nowX, nowY, r, g, b, HUDSmallFont, true);
	}
	return 0;
}
