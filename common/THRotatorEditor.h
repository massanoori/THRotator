// (c) 2017 massanoori

#pragma once

#include "THRotatorSettings.h"

inline bool IsZeroSize(const SIZE& size)
{
	return size.cx == 0 || size.cy == 0;
}

inline bool IsZeroSizedRectTransfer(const RectTransferData& rectTransfer)
{
	return IsZeroSize(rectTransfer.sourceSize) || IsZeroSize(rectTransfer.destSize);
}

extern HINSTANCE g_hModule;

class THRotatorEditorContext
{
public:
	static std::shared_ptr<THRotatorEditorContext> CreateOrGetEditorContext(HWND hTouhouWindow);

	int GetResetRevision() const;

	HWND GetTouhouWindow() const;

	// �r���[�|�[�g�ݒ�񐔂𑝂₷
	void AddViewportSetCount();

	// �r���[�|�[�g�ݒ�񐔂�UI�ɓn���A�ݒ�񐔂����Z�b�g���Čv�����ĊJ
	void SubmitViewportSetCountToEditor();

	// �r���[�|�[�g�ݒ�񐔂�臒l�𒴂�����
	bool IsViewportSetCountOverThreshold() const;

	RotationAngle GetRotationAngle() const;

	POINT GetMainScreenTopLeft() const;

	SIZE GetMainScreenSize() const;

	LONG GetYOffset() const;

	const std::vector<RectTransferData> GetRectTransfers() const;

	bool ConsumeScreenCaptureRequest();

	const boost::filesystem::path& GetWorkingDirectory() const;

	LPCTSTR GetErrorMessage() const;

	bool IsVerticallyLongWindow() const
	{
		return m_bVerticallyLongWindow;
	}

	D3DTEXTUREFILTERTYPE GetFilterType() const
	{
		return m_filterType;
	}

	bool IsHUDRearrangeForced() const
	{
		return m_bHUDRearrangeForced;
	}

	// �𑜓x���N�G�X�g����E�B���h�E�̃T�C�Y���A�c�����ǂ������l�����Ȃ���A�ύX����B
	// �܂��AD3D�̃o�b�N�o�b�t�@�𑜓x��outBackBufferWidth, outBackBufferHeight�ɕԂ��B
	void UpdateWindowResolution(int requestedWidth, int requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight);

	void LogMessage(const std::wstring& message) const;

	~THRotatorEditorContext();

private:
	THRotatorEditorContext(HWND hTouhouWin);

	// �G�f�B�^�E�B���h�E�̉�����ݒ�
	// �G�f�B�^�E�B���h�E�����[�_���ŊJ���Ȃ���΂Ȃ�Ȃ��ꍇ�́A
	// �����̃E�B���h�E�̕\����\����؂�ւ��邱�Ƃ͂ł��Ȃ��̂ŁA�Ă�ł͂����Ȃ�
	void SetEditorWindowVisibility(bool bVisible);

	void InitListView(HWND hLV);

	bool ApplyChangeFromEditorWindow(HWND hWnd);
	bool ApplyChangeFromEditorWindow()
	{
		return ApplyChangeFromEditorWindow(m_hEditorWin);
	}

	void ApplyRotationToEditorWindow(HWND hWnd) const;
	void ApplyRotationToEditorWindow() const
	{
		ApplyRotationToEditorWindow(m_hEditorWin);
	}

	bool SaveSettings() const;

	bool LoadSettings();

	void SetVerticallyLongWindow(bool bVerticallyLongWindow);

	static LRESULT CALLBACK MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK EditRectDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	/**
	 * �V�K�ǉ��̏ꍇ�́AeditedRectTransfer��cend()��n��
	 */
	bool OpenEditRectDialog(RectTransferData& inoutRectTransfer, std::vector<RectTransferData>::const_iterator editedRectTransfer) const;

	void SetNewErrorMessage(std::basic_string<TCHAR>&& message);

	boost::filesystem::path m_workingDir;

	/****************************************
	* THRotator parameters
	****************************************/

	POINT m_mainScreenTopLeft;
	SIZE m_mainScreenSize;
	int m_yOffset;
	int m_judgeThreshold;
	int m_judgeCount, m_judgeCountPrev;
	bool m_bVisible;
	boost::filesystem::path m_exeFilename;
	bool m_bVerticallyLongWindow;
	RotationAngle m_rotationAngle;
	std::vector<RectTransferData> m_editedRectTransfers, m_currentRectTransfers;
	D3DTEXTUREFILTERTYPE m_filterType;
	bool m_bTouhouWithoutScreenCapture;
	bool m_bHUDRearrangeForced;


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
	bool m_bModalEditorPreferred;
	bool m_bNeedModalEditor;
	int m_modalEditorWindowPosX, m_modalEditorWindowPosY;
	UINT m_insertedMenuSeparatorID;
	SIZE m_originalTouhouClientSize;
	SIZE m_modifiedTouhouClientSize;


	/****************************************
	* Error notification
	****************************************/

	std::basic_string<TCHAR> m_errorMessage;
	LARGE_INTEGER m_errorMessageExpirationClock;
};
