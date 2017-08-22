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

	void AddViewportSetCount();

	/**
	 * Send count of setting viewport to editor's dialog. Then resetting the count to zero.
	 */
	void SubmitViewportSetCountToEditor();

	// If true, HUDs should be rearranged for vertically-long screen.
	bool IsViewportSetCountOverThreshold() const;

	RotationAngle GetRotationAngle() const;

	POINT GetMainScreenTopLeft() const;

	SIZE GetMainScreenSize() const;

	LONG GetYOffset() const;

	const std::vector<RectTransferData> GetRectTransfers() const;

	bool ConsumeScreenCaptureRequest();

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

	void RenderAndUpdateEditor();

	/**
	 * From resolution requested by Touhou process and current THRotator's configuration,
	 * updates window's resolution.
	 * The resulting width and height are returned to outBackBufferWidth and outBackBufferHeight.
	 */
	void UpdateWindowResolution(int requestedWidth, int requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight);

	~THRotatorEditorContext();

private:
	THRotatorEditorContext(HWND hTouhouWin);

	/**
	 * Set visibility of editor window.
	 * Don't call this function when the editor window is modal.
	 */
	void SetEditorWindowVisibility(bool bVisible);

	void InitRectanglesListView(HWND hLV);

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
	 * If dialog is for new rect, pass m_editedRectTransfers.cend() to editedRectTransfer.
	 */
	bool OpenEditRectDialog(RectTransferData& inoutRectTransfer,
		std::vector<RectTransferData>::const_iterator editedRectTransfer) const;

	void SetNewErrorMessage(std::basic_string<TCHAR>&& message);

	/****************************************
	* THRotator parameters
	****************************************/

	POINT m_mainScreenTopLeft;
	SIZE m_mainScreenSize;
	int m_yOffset;
	int m_judgeThreshold;
	int m_judgeCount, m_judgeCountPrev;
	bool m_bVisible;
	bool m_bVerticallyLongWindow;
	RotationAngle m_rotationAngle;
	std::vector<RectTransferData> m_editedRectTransfers, m_currentRectTransfers;
	D3DTEXTUREFILTERTYPE m_filterType;
	bool m_bHUDRearrangeForced;


	/****************************************
	* Class status
	****************************************/

	bool m_bInitialized;
	bool m_bScreenCaptureQueued;

	/**
	 * THRotatorDirect3DDevice has a copy of this reset revision.
	 * Device needs to be reset if this revision and that of THRotatorDirect3DDevice are different.
	 */
	int m_deviceResetRevision;

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
