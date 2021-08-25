class CHudCustomAmmo : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	void Reset(void);
	int DrawWList(float flTime);
	void SlotInput(int iSlot, int fAdvance);
	void ChosePlayerWeapon(void);
	void ClientMove(struct playermove_s* ppmove, qboolean server);
	void IN_Accumulate();
	void IN_MouseEvent(int mstate);

	float StartX = 48;
	float IconSize = 0.5;
	float ElementGap = 0.2;
	float BackGroundY = 0.95;
	float BackGroundLength = 3;
	float BackGroundAlpha = 128;

	int iSelectCyclerSpr = 0;
	int iSelectCyclerRinSpr = 0;

	Color Ammo1IconColor;
	Color Ammo1BigTextColor;
	Color Ammo1TextColor;
	Color Ammo2IconColor;
	Color Ammo2BigTextColor;
	Color Ammo2TextColor;
	Color BackGroundColor;

	Color SelectCyclerColor;
	Color SelectCyclerRinColor;
	Color SelectCyclerIconColor;
	Color SelectCyclerTextColor;
	Color SelectCyclerEmptyColor;

	float SelectCyclerOffset;
	float SelectCyclerSize;
	float SelectCyclerRotate;
	float SelectCyclerAnimateTime;
	float SelectCyclerFadeTime;
	float SelectCyclerHoldTime;

	vgui::HFont HUDFont;
	vgui::HFont HUDSmallFont;
	
	float m_fFade;
	float m_fAnimateTime;
	bool m_bOpeningAnnularMenu = false;
	bool m_bSelectMenuDisplay = false;

	WEAPON* m_pWeapon;
	int	m_HUD_bucket0;

	bool m_bAcceptDeadMessage = false;
};
extern CHudCustomAmmo m_HudCustomAmmo;