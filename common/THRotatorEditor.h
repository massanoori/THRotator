// (c) 2017 massanoori

#pragma once

enum RotationAngle : std::uint32_t
{
	Rotation_0 = 0,
	Rotation_90 = 1,
	Rotation_180 = 2,
	Rotation_270 = 3,

	Rotation_Num,
};

struct RectTransferData
{
	POINT sourcePosition;
	SIZE sourceSize;

	POINT destPosition;
	SIZE destSize;

	RotationAngle rotation;
	TCHAR name[64];
};

inline bool IsZeroSize(const SIZE& size)
{
	return size.cx == 0 || size.cy == 0;
}

inline bool IsZeroSizedRectTransfer(const RectTransferData& rectTransfer)
{
	return IsZeroSize(rectTransfer.sourceSize) || IsZeroSize(rectTransfer.destSize);
}

extern HANDLE g_hModule;

class THRotatorEditorContext
{
public:
	static std::shared_ptr<THRotatorEditorContext> CreateOrGetEditorContext(HWND hTouhouWindow);

	int GetResetRevision() const;

	HWND GetTouhouWindow() const;
	void GetBackBufferResolution(Direct3DBase* pd3d, D3DFORMAT Format, UINT Adapter, BOOL bWindowed, UINT requestedWidth, UINT requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight) const;

	// �r���[�|�[�g�ݒ�񐔂𑝂₷
	void AddViewportSetCount();

	// �r���[�|�[�g�ݒ�񐔂�UI�ɓn���A�ݒ�񐔂����Z�b�g���Čv�����ĊJ
	void SubmitViewportSetCountToEditor();

	// �r���[�|�[�g�ݒ�񐔂�臒l�𒴂�����
	bool IsViewportSetCountOverThreshold() const;

	RotationAngle GetRotationAngle() const;

	DWORD GetPlayRegionLeft() const;

	DWORD GetPlayRegionWidth() const;

	DWORD GetPlayRegionTop() const;

	DWORD GetPlayRegionHeight() const;

	LONG GetYOffset() const;

	const std::vector<RectTransferData> GetRectTransfers() const;

	bool ConsumeScreenCaptureRequest();

	const boost::filesystem::path& GetWorkingDirectory() const;

	~THRotatorEditorContext();

private:
	THRotatorEditorContext(HWND hTouhouWin);

	// �G�f�B�^�E�B���h�E�̉�����ݒ�
	// �G�f�B�^�E�B���h�E�����[�_���ŊJ���Ȃ���΂Ȃ�Ȃ��ꍇ�́A
	// �����̃E�B���h�E�̕\����\����؂�ւ��邱�Ƃ͂ł��Ȃ��̂ŁA�Ă�ł͂����Ȃ�
	void SetEditorWindowVisibility(BOOL bVisible);

	void InitListView(HWND hLV);

	BOOL ApplyChangeFromEditorWindow(HWND hWnd);
	BOOL ApplyChangeFromEditorWindow()
	{
		return ApplyChangeFromEditorWindow(m_hEditorWin);
	}

	void SaveSettings();

	void LoadSettings();

	void SetVerticallyLongWindow(BOOL bVerticallyLongWindow);

	static LRESULT CALLBACK MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK EditRectDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	boost::filesystem::path m_workingDir;

	/****************************************
	* THRotator parameters
	****************************************/

	DWORD m_playRegionLeft, m_playRegionTop, m_playRegionWidth, m_playRegionHeight;
	int m_yOffset;
	int m_judgeThreshold;
	int m_judgeCount, m_judgeCountPrev;
	BOOL m_bVisible;
	boost::filesystem::path m_iniPath;
	std::string m_appName; // �}���`�o�C�g�������.ini�̓��o�͂��邽�߁Astd::string
	BOOL m_bVerticallyLongWindow;
	RotationAngle m_RotationAngle;
	std::vector<RectTransferData> m_editedRectTransfers, m_currentRectTransfers;
	D3DTEXTUREFILTERTYPE m_filterType;
	bool m_bTouhouWithoutScreenCapture;


	/****************************************
	* Class status
	****************************************/

	bool m_bInitialized;
	bool m_bScreenCaptureQueued; // �X�N���[���L���v�`���̃��N�G�X�g�����邩
	int m_deviceResetRevision; // ���Ԗڂ̃��Z�b�g�����{�v�����Ă��邩


	/****************************************
	* Editor window
	****************************************/

	HMENU m_hSysMenu;
	HWND m_hEditorWin, m_hTouhouWin;
	bool m_bNeedModalEditor;
	int m_modalEditorWindowPosX, m_modalEditorWindowPosY;
};
