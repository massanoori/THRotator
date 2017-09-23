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

	const char* GetErrorMessage() const;

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

	void RenderAndUpdateEditor(bool bFullscreen);

	/**
	 * From resolution requested by Touhou process and current THRotator's configuration,
	 * updates window's resolution.
	 * The resulting width and height are returned to outBackBufferWidth and outBackBufferHeight.
	 */
	void UpdateWindowResolution(int requestedWidth, int requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight);

	void ApplySetting(const THRotatorSetting& settings);
	void RetrieveSetting(THRotatorSetting& setting) const;

	~THRotatorEditorContext();

private:
	THRotatorEditorContext(HWND hTouhouWin);

	void UpdateVisibilitySwitchMenuText();

	/**
	 * non-const since SaveSettings() updates error message.
	 */
	bool SaveSettings();

	bool LoadSettings();

	void SetVerticallyLongWindow(bool bVerticallyLongWindow);

	static LRESULT CALLBACK MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	void SetNewErrorMessage(const std::string& message, int timeToLiveInSeconds = 8);

	/****************************************
	* THRotator parameters
	****************************************/

	POINT m_mainScreenTopLeft;
	SIZE m_mainScreenSize;
	int m_yOffset;
	int m_judgeThreshold;
	int m_judgeCount, m_judgeCountPrev;
	bool m_bEditorShownInitially;
	bool m_bVerticallyLongWindow;
	RotationAngle m_rotationAngle;
	std::vector<RectTransferData> m_rectTransfers;
	D3DTEXTUREFILTERTYPE m_filterType;
	bool m_bHUDRearrangeForced;


	/****************************************
	* Class status
	****************************************/

	bool m_bInitialized;
	bool m_bScreenCaptureQueued;
	bool m_bEditorShown;
	bool m_bSaveBySysKeyAllowed;

	/**
	 * THRotatorDirect3DDevice has a copy of this reset revision.
	 * Device needs to be reset if this revision and that of THRotatorDirect3DDevice are different.
	 */
	int m_deviceResetRevision;

	/****************************************
	* Editor window
	****************************************/

	HMENU m_hSysMenu;
	HWND m_hTouhouWin;
	UINT m_insertedMenuSeparatorID;
	SIZE m_originalTouhouClientSize;
	SIZE m_modifiedTouhouClientSize;


	/****************************************
	* Error notification
	****************************************/

	std::string m_errorMessage;
	LARGE_INTEGER m_errorMessageExpirationClock;


	// Current context of GUI (such as list box selection)
	struct GuiContext
	{
		int selectedRectIndex;

		// this array is just for buffer for ImGui's list box, may contain dangling pointers.
		std::vector<const char*> rectListBoxItemBuffer;

		GuiContext() : selectedRectIndex(0) {}
	} m_GuiContext;
};
