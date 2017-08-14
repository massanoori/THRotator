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

	// ビューポート設定回数を増やす
	void AddViewportSetCount();

	// ビューポート設定回数をUIに渡し、設定回数をリセットして計測を再開
	void SubmitViewportSetCountToEditor();

	// ビューポート設定回数が閾値を超えたか
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

	// 解像度リクエストからウィンドウのサイズを、縦長かどうかも考慮しながら、変更する。
	// また、D3Dのバックバッファ解像度もoutBackBufferWidth, outBackBufferHeightに返す。
	void UpdateWindowResolution(int requestedWidth, int requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight);

	void LogMessage(const std::wstring& message) const;

	~THRotatorEditorContext();

private:
	THRotatorEditorContext(HWND hTouhouWin);

	// エディタウィンドウの可視性を設定
	// エディタウィンドウをモーダルで開かなければならない場合は、
	// 既存のウィンドウの表示非表示を切り替えることはできないので、呼んではいけない
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
	 * 新規追加の場合は、editedRectTransferにcend()を渡す
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
	bool m_bScreenCaptureQueued; // スクリーンキャプチャのリクエストがあるか
	int m_deviceResetRevision; // 何番目のリセットを実施要求しているか


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
