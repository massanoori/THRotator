// (c) 2017 massanoori

#include "stdafx.h"

#include <Windows.h>
#include <map>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <ctime>

#include "THRotatorSettings.h"
#include "THRotatorEditor.h"
#include "THRotatorSystem.h"
#include "THRotatorImGui.h"
#include "StringResource.h"
#include "resource.h"
#include "EncodingUtils.h"
#include "THRotatorLog.h"

#pragma comment(lib, "comctl32.lib")

namespace
{

const char THROTATOR_VERSION_STRING[] = "2.1.0";

HHOOK ms_hHook;
std::map<HWND, std::weak_ptr<THRotatorEditorContext>> ms_touhouWinToContext;
UINT ms_switchVisibilityID = 12345u;

template <typename T>
T Clamp(T value, T minValue, T maxValue)
{
	assert(minValue <= maxValue);

	if (value < minValue)
	{
		return minValue;
	}

	if (value > maxValue)
	{
		return maxValue;
	}

	return value;
}

}

/**
 * Get THRotator's version from external program.
 * If destination pointer is valid, this function writes as many characters as possible to destination for given bufferSize.
 *
 * \param destination  Output pointer where the retruend version number string is written.
 * \param bufferSize   Size of output buffer in number of characters including terminating null.
 * 
 * \return             If destination is null, this function returns the minimal buffer size
 *                     that can contain all characters of version number string, incluing terminating null.
 *                     If destination is a valid pointer, this function returns number of characters actually written to destination,
 *                     including terminating null.
 */
extern "C" __declspec(dllexport) std::uint32_t THRotator_GetVersionString(char* destination, std::uint32_t bufferSize)
{
	if (destination == nullptr)
	{
		return _countof(THROTATOR_VERSION_STRING);
	}
	else if (bufferSize == 0)
	{
		return 0;
	}
	
	strncpy_s(destination, bufferSize, THROTATOR_VERSION_STRING, _TRUNCATE);
	return (std::min)(bufferSize, static_cast<std::uint32_t>(_countof(THROTATOR_VERSION_STRING)));
}

THRotatorEditorContext::THRotatorEditorContext(HWND hTouhouWin)
	: m_judgeCount(0)
	, m_judgeCountPrev(0)
	, m_hTouhouWin(hTouhouWin)
	, m_bHUDRearrangeForced(false)
	, m_deviceResetRevision(0)
	, m_bInitialized(false)
	, m_bScreenCaptureQueued(false)
	, m_bSaveBySysKeyAllowed(true)
	, m_modifiedTouhouClientSize{ 0, 0 }
{
	m_errorMessageExpirationClock.QuadPart = 0;

	RECT originalClientRect;
	GetClientRect(hTouhouWin, &originalClientRect);
	m_originalTouhouClientSize.cx = originalClientRect.right - originalClientRect.left;
	m_originalTouhouClientSize.cy = originalClientRect.bottom - originalClientRect.top;

	struct MessageHook
	{
		HHOOK m_hHook;

		MessageHook()
		{
			m_hHook = SetWindowsHookEx(WH_GETMESSAGE, THRotatorEditorContext::MessageHookProc, nullptr, GetCurrentThreadId());
			ms_hHook = m_hHook;
		}

		~MessageHook()
		{
			UnhookWindowsHookEx(m_hHook);
		}
	};

	static MessageHook messageHook;


	OutputLogMessagef(LogSeverity::Info, "Initializing THRotatorEditorContext");

	if (!LoadSettings())
	{
		// If initial load failed, use default setting
		ApplySetting(THRotatorSetting());
	}

	m_bEditorShown = m_bEditorShownInitially;

	// Modify menu
	HMENU hMenu = GetSystemMenu(m_hTouhouWin, FALSE);

	// Separator
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	m_insertedMenuSeparatorID = GetMenuItemID(hMenu, GetMenuItemCount(hMenu) - 1);

	// Menu for visibility switch (text is updated)
	AppendMenu(hMenu, MF_STRING, ms_switchVisibilityID, _T(""));

	m_hSysMenu = hMenu;

	UpdateVisibilitySwitchMenuText();
	m_bInitialized = true;

	OutputLogMessagef(LogSeverity::Info, "THRotatorEditorContext has been initialized");
}

std::shared_ptr<THRotatorEditorContext> THRotatorEditorContext::CreateOrGetEditorContext(HWND hTouhouWindow)
{
	auto foundItr = ms_touhouWinToContext.find(hTouhouWindow);
	if (foundItr != ms_touhouWinToContext.end())
	{
		return foundItr->second.lock();
	}

	std::shared_ptr<THRotatorEditorContext> ret(new THRotatorEditorContext(hTouhouWindow));
	if (!ret || !ret->m_bInitialized)
	{
		return nullptr;
	}

	ms_touhouWinToContext[hTouhouWindow] = ret;

	return ret;
}

int THRotatorEditorContext::GetResetRevision() const
{
	return m_deviceResetRevision;
}

HWND THRotatorEditorContext::GetTouhouWindow() const
{
	return m_hTouhouWin;
}

void THRotatorEditorContext::AddViewportSetCount()
{
	m_judgeCount++;
}

void THRotatorEditorContext::SubmitViewportSetCountToEditor()
{
	m_judgeCountPrev = m_judgeCount;
	m_judgeCount = 0;
}

bool THRotatorEditorContext::IsViewportSetCountOverThreshold() const
{
	return m_judgeThreshold <= m_judgeCountPrev;
}

RotationAngle THRotatorEditorContext::GetRotationAngle() const
{
	return m_rotationAngle;
}

POINT THRotatorEditorContext::GetMainScreenTopLeft() const
{
	return m_mainScreenTopLeft;
}

SIZE THRotatorEditorContext::GetMainScreenSize() const
{
	return m_mainScreenSize;
}

LONG THRotatorEditorContext::GetYOffset() const
{
	return m_yOffset;
}

const std::vector<RectTransferData> THRotatorEditorContext::GetRectTransfers() const
{
	return m_rectTransfers;
}

bool THRotatorEditorContext::ConsumeScreenCaptureRequest()
{
	if (!m_bScreenCaptureQueued)
	{
		return false;
	}

	m_bScreenCaptureQueued = false;
	return true;
}

const char* THRotatorEditorContext::GetErrorMessage() const
{
	LARGE_INTEGER currentClock;
	::QueryPerformanceCounter(&currentClock);

	if (currentClock.QuadPart >= m_errorMessageExpirationClock.QuadPart)
	{
		return nullptr;
	}

	return m_errorMessage.c_str();
}

THRotatorEditorContext::~THRotatorEditorContext()
{
	ms_touhouWinToContext.erase(m_hTouhouWin);

	DeleteMenu(m_hSysMenu, ms_switchVisibilityID, MF_BYCOMMAND);

	if (m_insertedMenuSeparatorID != static_cast<UINT>(-1))
	{
		DeleteMenu(m_hSysMenu, m_insertedMenuSeparatorID, MF_BYCOMMAND);
	}

	RECT clientRect, windowRect;
	GetClientRect(m_hTouhouWin, &clientRect);
	GetWindowRect(m_hTouhouWin, &windowRect);

	MoveWindow(m_hTouhouWin,
		windowRect.left, windowRect.top,
		m_originalTouhouClientSize.cx + (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left),
		m_originalTouhouClientSize.cy + (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top),
		TRUE);

	OutputLogMessage(LogSeverity::Info, "Destructing THRotatorEditorContext");
}

void THRotatorEditorContext::UpdateVisibilitySwitchMenuText()
{
	MENUITEMINFO mi;
	ZeroMemory(&mi, sizeof(mi));

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING;

	auto str = LoadTHRotatorStringUnicode(m_bEditorShown ? IDS_HIDE_EDITOR : IDS_SHOW_EDITOR);

#ifdef _UNICODE
	mi.dwTypeData = const_cast<LPTSTR>(str.c_str());
#else
	std::string strShiftJIS = ConvertFromUnicodeToSjis(str);
	mi.dwTypeData = const_cast<LPTSTR>(strShiftJIS.c_str());
#endif

	SetMenuItemInfo(m_hSysMenu, ms_switchVisibilityID, FALSE, &mi);
}

void THRotatorEditorContext::SetNewErrorMessage(const std::string& message, int timeToLiveInSeconds)
{
	LARGE_INTEGER frequency;
	::QueryPerformanceFrequency(&frequency);
	::QueryPerformanceCounter(&m_errorMessageExpirationClock);

	m_errorMessageExpirationClock.QuadPart += frequency.QuadPart * timeToLiveInSeconds;
	m_errorMessage = message;
}

bool THRotatorEditorContext::SaveSettings()
{
	THRotatorSetting setting;
	RetrieveSetting(setting);

	bool bSaveSuccess = THRotatorSetting::Save(setting);

	if (!bSaveSuccess)
	{
		auto saveFailureMessage = LoadTHRotatorStringUtf8(IDS_SETTING_FILE_SAVE_FAILED);
		SetNewErrorMessage(std::move(saveFailureMessage));
	}

	return bSaveSuccess;
}

bool THRotatorEditorContext::LoadSettings()
{
	THRotatorSetting setting;
	THRotatorFormatVersion formatVersion;

	bool bLoadSuccess = THRotatorSetting::Load(setting, formatVersion);
	m_bSaveBySysKeyAllowed = bLoadSuccess;

	if (!bLoadSuccess)
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to load");

		auto loadFailureMessage = LoadTHRotatorStringUtf8(IDS_SETTING_FILE_LOAD_FAILED);
		SetNewErrorMessage(std::move(loadFailureMessage));

		return false;
	}

	if (formatVersion == THRotatorFormatVersion::Version_1)
	{
		double touhouSeriesNumber = GetTouhouSeriesNumber();
		if (6.0 <= touhouSeriesNumber && touhouSeriesNumber < 7.5)
		{
			/**
			 * .ini files in format version 1 contains judge threshold which is compared with wrong count of setting viewport
			 * on Touhou 6 and 7.
			 * Format migration is a chance to fix the wrong value by incrementing by 1.
			 */
			setting.judgeThreshold++;

			OutputLogMessagef(LogSeverity::Info, "Threshold has been converted");
		}
	}

	ApplySetting(setting);

	return true;
}

/**
 * Check if the same message is in message queue.
 */
bool IsUniquePostedMessage(const MSG* pMessage)
{
	MSG peekedMessage;
	if (!PeekMessage(&peekedMessage, pMessage->hwnd, pMessage->message, pMessage->message, PM_NOREMOVE))
	{
		return true;
	}

	return peekedMessage.wParam != pMessage->wParam || peekedMessage.lParam != pMessage->lParam;
}

LRESULT CALLBACK THRotatorEditorContext::MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPMSG pMsg = reinterpret_cast<LPMSG>(lParam);

	if (nCode == HC_ACTION)
	{
		auto foundItr = ms_touhouWinToContext.find(pMsg->hwnd);
		if (foundItr != ms_touhouWinToContext.end() && !foundItr->second.expired())
		{
			THRotatorImGui_WindowProcHandler(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);

			auto context = foundItr->second.lock();
			switch (pMsg->message)
			{
			case WM_SYSCOMMAND:
				if (pMsg->wParam == ms_switchVisibilityID
					&& IsUniquePostedMessage(pMsg))
				{
					context->m_bEditorShown = !context->m_bEditorShown;
					context->UpdateVisibilitySwitchMenuText();
				}
				break;

			case WM_KEYDOWN:
				switch (pMsg->wParam)
				{
				case VK_HOME:
					if (IsTouhouWithoutScreenCapture() && (HIWORD(pMsg->lParam) & KF_REPEAT) == 0)
					{
						context->m_bScreenCaptureQueued = true;
					}
					break;
				}
				break;

			case WM_SYSKEYDOWN:
				switch (pMsg->wParam)
				{
				case VK_LEFT:
				case VK_RIGHT:
					if ((HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT)
						&& IsUniquePostedMessage(pMsg))
					{
						int rotatingDirection = pMsg->wParam == VK_LEFT ? 1 : -1;

						auto nextRotationAngle = static_cast<RotationAngle>((context->m_rotationAngle + Rotation_Num + rotatingDirection) % Rotation_Num);
						context->m_rotationAngle = nextRotationAngle;

						if (context->m_bSaveBySysKeyAllowed)
						{
							context->SaveSettings();
						}
						else
						{
							static const std::string messageLossOfDataPrevention = LoadTHRotatorStringUtf8(IDS_LOSS_OF_DATA_PREVENTED);
							context->SetNewErrorMessage(messageLossOfDataPrevention.c_str(), 20);
						}
					}
					break;

				case VK_UP:
				case VK_DOWN:
					// Forced vertical layout
					if ((HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT)
						&& IsUniquePostedMessage(pMsg))
					{
						context->m_bHUDRearrangeForced = !context->m_bHUDRearrangeForced;
					}
					break;

				case '0':
					if ((HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT)
						&& IsUniquePostedMessage(pMsg))
					{
						context->m_bEditorShown = !context->m_bEditorShown;
						context->UpdateVisibilitySwitchMenuText();
					}
					break;
				}
				break;
			}
		}
	}
	return CallNextHookEx(ms_hHook, nCode, wParam, lParam);
}

void THRotatorEditorContext::SetVerticallyLongWindow(bool bVerticallyLongWindow)
{
	if (m_bVerticallyLongWindow != bVerticallyLongWindow)
	{
		m_deviceResetRevision++;
	}
	m_bVerticallyLongWindow = bVerticallyLongWindow;
}

void ShowPreviousItemTooltip(const char* tooltipText)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text(tooltipText);
		ImGui::EndTooltip();
	}
}

bool CollapsingHeaderWithTooltip(const char* labelText, const char* tooltipText)
{
	bool bRet = ImGui::CollapsingHeader(labelText);
	ShowPreviousItemTooltip(tooltipText);

	return bRet;
}

std::string MakeDragIntTooltipText(UINT stringID)
{
	static const std::string tooltipDragInt = std::string("\n\n")
		+ LoadTHRotatorStringUtf8(IDS_DRAGINT_COMMON_TOOLTIP);

	return LoadTHRotatorStringUtf8(stringID) + tooltipDragInt;
}

bool DragInt2_POINT(const char* label, POINT& inoutPoint,
	float valueSpeed = 1.0f, int valueMin = 0, int valueMax = 0, const char* displayFormat = "%0.f")
{
	int buffer[] =
	{
		inoutPoint.x, inoutPoint.y,
	};

	bool bRet = ImGui::DragInt2(label, buffer, valueSpeed, valueMin, valueMax, displayFormat);

	inoutPoint.x = Clamp(buffer[0], valueMin, valueMax);
	inoutPoint.y = Clamp(buffer[1], valueMin, valueMax);

	return bRet;
}

bool DragInt2_SIZE(const char* label, SIZE& inoutSize,
	float valueSpeed = 1.0f, int valueMin = 0, int valueMax = 0, const char* displayFormat = "%0.f")
{
	int buffer[] =
	{
		inoutSize.cx, inoutSize.cy,
	};

	bool bRet = ImGui::DragInt2(label, buffer, valueSpeed, valueMin, valueMax, displayFormat);

	inoutSize.cx = Clamp(buffer[0], valueMin, valueMax);
	inoutSize.cy = Clamp(buffer[1], valueMin, valueMax);

	return bRet;
}

bool THRotatorEditorContext::IsBorderlessWindow() const
{
	RECT rcWindow;
	GetWindowRect(m_hTouhouWin, &rcWindow);

	if (rcWindow.right == GetSystemMetrics(SM_CXSCREEN) && rcWindow.bottom == GetSystemMetrics(SM_CYSCREEN) &&
		rcWindow.left == 0 && rcWindow.top == 0)
	{
		// Window is borderless
		return true;
	}

	return false;
}

void THRotatorEditorContext::RenderAndUpdateEditor(bool bFullscreen)
{
	struct ImGuiRenderOnReturn
	{
		ImGuiRenderOnReturn() : cancel(false) {}
		~ImGuiRenderOnReturn()
		{
			if (!cancel)
			{
				ImGui::Render();
				THRotatorImGui_RenderDrawLists(ImGui::GetDrawData());
			}
		}
		bool cancel;
	};
	ImGuiRenderOnReturn renderOnReturn;

	auto errorMessagePtr = GetErrorMessage();

	// when windowed: cursor from OS
	// when fullscreen: cursor by ImGui only while editing
	
	ImGui::GetIO().MouseDrawCursor = bFullscreen && (m_bEditorShown || errorMessagePtr);

	if (errorMessagePtr)
	{
		ImGui::OpenPopup("Error message");
		if (ImGui::BeginPopupModal("Error message", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static const std::string labelThisWindowClosesInSeconds = LoadTHRotatorStringUtf8(IDS_WINDOW_CLOSES_IN_SECONDS);

			LARGE_INTEGER currentClock, clockFrequency;
			QueryPerformanceCounter(&currentClock);
			QueryPerformanceFrequency(&clockFrequency);

			auto durationInClock = m_errorMessageExpirationClock.QuadPart - currentClock.QuadPart;

			ImGui::Text(errorMessagePtr);
			ImGui::Text(fmt::format(labelThisWindowClosesInSeconds.c_str(),
				durationInClock / clockFrequency.QuadPart + 1).c_str());
			if (ImGui::Button("OK"))
			{
				m_errorMessageExpirationClock = currentClock;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	if (!m_bEditorShown)
	{
		renderOnReturn.cancel = errorMessagePtr == nullptr;
		return;
	}

	const ImVec2 initialTHRotatorWindowSize(320.0f, 0.0f);
	ImGui::SetNextWindowSize(initialTHRotatorWindowSize, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("THRotator", nullptr))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::PushItemWidth(-170);

	// Entire screen rotation angle selection
	{
		static const std::string tooltipEntireRotationAngle = LoadTHRotatorStringUtf8(IDS_ENTIRE_ROTATION_TOOLTIP);

		int rotationAngle = static_cast<int>(m_rotationAngle);
		ImGui::PushID("MainScreenRotation");

		// 0 degrees
		ImGui::RadioButton("0\xC2\xB0", reinterpret_cast<int*>(&rotationAngle), Rotation_0);
		ShowPreviousItemTooltip(tooltipEntireRotationAngle.c_str());
		ImGui::SameLine();

		// 90 degrees
		ImGui::RadioButton("90\xC2\xB0", reinterpret_cast<int*>(&rotationAngle), Rotation_90);
		ShowPreviousItemTooltip(tooltipEntireRotationAngle.c_str());
		ImGui::SameLine();

		// 180 degrees
		ImGui::RadioButton("180\xC2\xB0", reinterpret_cast<int*>(&rotationAngle), Rotation_180);
		ShowPreviousItemTooltip(tooltipEntireRotationAngle.c_str());
		ImGui::SameLine();

		// 270 degrees
		ImGui::RadioButton("270\xC2\xB0", reinterpret_cast<int*>(&rotationAngle), Rotation_270);
		ShowPreviousItemTooltip(tooltipEntireRotationAngle.c_str());

		ImGui::PopID();
		m_rotationAngle = static_cast<RotationAngle>(rotationAngle);
	}

	bool bPreviousVerticalWindow = m_bVerticallyLongWindow;
	{
		static const std::string labelVerticalWindow = LoadTHRotatorStringUtf8(IDS_VERTICAL_WINDOW);
		static const std::string tooltipVerticalWindow = LoadTHRotatorStringUtf8(IDS_VERTICAL_WINDOW_TOOLTIP);

		ImGui::Checkbox(labelVerticalWindow.c_str(), &m_bVerticallyLongWindow);
		ShowPreviousItemTooltip(tooltipVerticalWindow.c_str());
	}

	{
		static const std::string labelShowFirst = LoadTHRotatorStringUtf8(IDS_SHOW_THROTATOR_FIRST);
		static const std::string labelForceRearrangement = LoadTHRotatorStringUtf8(IDS_FORCE_REARRANGEMENTS);
		static const std::string tooltipShowFirst = LoadTHRotatorStringUtf8(IDS_SHOW_THROTATOR_FIRST_TOOLTIP);
		static const std::string tooltipForceRearrangement = LoadTHRotatorStringUtf8(IDS_FORCE_REARRANGEMENTS_TOOLTIP);

		ImGui::Checkbox(labelShowFirst.c_str(), &m_bEditorShownInitially);
		ShowPreviousItemTooltip(tooltipShowFirst.c_str());

		ImGui::Checkbox(labelForceRearrangement.c_str(), &m_bHUDRearrangeForced);
		ShowPreviousItemTooltip(tooltipForceRearrangement.c_str());
	}

	static const std::string labelGameplayDetection = LoadTHRotatorStringUtf8(IDS_GAMEPLAY_DETECTION);
	static const std::string tooltipGameplayDetection = LoadTHRotatorStringUtf8(IDS_GAMEPLAY_DETECTION_TOOLTIP);
	if (CollapsingHeaderWithTooltip(labelGameplayDetection.c_str(), tooltipGameplayDetection.c_str()))
	{
		static const std::string labelSetVPCount = LoadTHRotatorStringUtf8(IDS_SET_VP_COUNT);
		static const std::string labelThreshold = LoadTHRotatorStringUtf8(IDS_THRESHOLD);
		static const std::string tooltipSetVPCount = LoadTHRotatorStringUtf8(IDS_SET_VP_COUNT_TOOLTIP);
		static const std::string tooltipThreshold = LoadTHRotatorStringUtf8(IDS_THRESHOLD_TOOLTIP);

		const std::string currentSetVPCountText = fmt::format("{}", m_judgeCountPrev).c_str();
		ImGui::InputText(labelSetVPCount.c_str(),
			const_cast<char*>(currentSetVPCountText.c_str()), currentSetVPCountText.length(),
			ImGuiInputTextFlags_ReadOnly);
		ShowPreviousItemTooltip(tooltipSetVPCount.c_str());

		ImGui::InputInt(labelThreshold.c_str(), &m_judgeThreshold);
		ShowPreviousItemTooltip(tooltipThreshold.c_str());
		m_judgeThreshold = Clamp(m_judgeThreshold, 0, 999);
	}

	static const int COORDINATE_MAX = 999;
	static const int COORDINATE_MIN = 0;
	static const float COORDINATE_DRAG_SPEED = 0.1f;

	static const int Y_OFFSET_MAX =  999;
	static const int Y_OFFSET_MIN = -999;

	static const std::string labelMainScreenRegion = LoadTHRotatorStringUtf8(IDS_MAIN_SCREEN_REGION);
	static const std::string tooltipMainScreenRegion = LoadTHRotatorStringUtf8(IDS_MAIN_SCREEN_REGION_TOOLTIP);
	if (CollapsingHeaderWithTooltip(labelMainScreenRegion.c_str(), tooltipMainScreenRegion.c_str()))
	{
		static const std::string labelLeftAndTop = LoadTHRotatorStringUtf8(IDS_LEFT_AND_TOP);
		static const std::string labelWidthAndHeight = LoadTHRotatorStringUtf8(IDS_WIDTH_AND_HEIGHT);
		static const std::string labelYOffset = LoadTHRotatorStringUtf8(IDS_Y_OFFSET);

		static const std::string tooltipLeftAndTop = MakeDragIntTooltipText(IDS_LEFT_AND_TOP_TOOLTIP);
		static const std::string tooltipWidthAndHeight = MakeDragIntTooltipText(IDS_WIDTH_AND_HEIGHT_TOOLTIP);
		static const std::string tooltipYOffset = LoadTHRotatorStringUtf8(IDS_Y_OFFSET_TOOLTIP);

		DragInt2_POINT(labelLeftAndTop.c_str(), m_mainScreenTopLeft, COORDINATE_DRAG_SPEED, COORDINATE_MIN, COORDINATE_MAX, "%0.f px");
		ShowPreviousItemTooltip(tooltipLeftAndTop.c_str());

		DragInt2_SIZE(labelWidthAndHeight.c_str(), m_mainScreenSize, COORDINATE_DRAG_SPEED, COORDINATE_MIN, COORDINATE_MAX, "%0.f px");
		ShowPreviousItemTooltip(tooltipWidthAndHeight.c_str());

		ImGui::InputInt(labelYOffset.c_str(), &m_yOffset);
		m_yOffset = Clamp(m_yOffset, Y_OFFSET_MIN, Y_OFFSET_MAX);
		ShowPreviousItemTooltip(tooltipYOffset.c_str());
	}

	static const std::string labelPixelInterpolation = LoadTHRotatorStringUtf8(IDS_PIXEL_INTERPOLATION);
	static const std::string tooltipPixelInterpolation = LoadTHRotatorStringUtf8(IDS_PIXEL_INTERPOLATION_TOOLTIP);
	if (CollapsingHeaderWithTooltip(labelPixelInterpolation.c_str(), tooltipPixelInterpolation.c_str()))
	{
		static const std::string labelFilterNone = LoadTHRotatorStringUtf8(IDS_FILTER_NONE);
		static const std::string labelFilterBilinear = LoadTHRotatorStringUtf8(IDS_FILTER_BILINEAR);

		static const std::string tooltipFilterNone = LoadTHRotatorStringUtf8(IDS_FILTER_NONE_TOOLTIP);
		static const std::string tooltipFilterBilinear = LoadTHRotatorStringUtf8(IDS_FILTER_BILINEAR_TOOLTIP);

		std::underlying_type<D3DTEXTUREFILTERTYPE>::type rawFilterType = m_filterType;
		ImGui::PushID("PixelInterpolationSelection");

		ImGui::RadioButton(labelFilterNone.c_str(), &rawFilterType, D3DTEXF_NONE);
		ShowPreviousItemTooltip(tooltipFilterNone.c_str());
		ImGui::SameLine();

		ImGui::RadioButton(labelFilterBilinear.c_str(), &rawFilterType, D3DTEXF_LINEAR);
		ShowPreviousItemTooltip(tooltipFilterBilinear.c_str());

		ImGui::PopID();
		m_filterType = static_cast<D3DTEXTUREFILTERTYPE>(rawFilterType);
	}

	static const std::string labelOtherRectangles = LoadTHRotatorStringUtf8(IDS_OTHER_RECTANGLES);
	static const std::string tooltipOtherRectangles = LoadTHRotatorStringUtf8(IDS_OTHER_RECTANGLES_TOOLTIP);
	if (CollapsingHeaderWithTooltip(labelOtherRectangles.c_str(), tooltipOtherRectangles.c_str()))
	{
		static const std::string labelSourceLeftAndTop = LoadTHRotatorStringUtf8(IDS_SOURCE_LEFT_AND_TOP);
		static const std::string labelSourceWidthAndHeight = LoadTHRotatorStringUtf8(IDS_SOURCE_WIDTH_AND_HEIGHT);
		static const std::string labelDestLeftAndTop = LoadTHRotatorStringUtf8(IDS_DEST_LEFT_AND_TOP);
		static const std::string labelDestWidthAndHeight = LoadTHRotatorStringUtf8(IDS_DEST_WIDTH_AND_HEIGHT);

		static const std::string tooltipSourceLeftAndTop = MakeDragIntTooltipText(IDS_SOURCE_LEFT_AND_TOP_TOOLTIP);
		static const std::string tooltipSourceWidthAndHeight = MakeDragIntTooltipText(IDS_SOURCE_WIDTH_AND_HEIGHT_TOOLTIP);
		static const std::string tooltipDestLeftAndTop = MakeDragIntTooltipText(IDS_DEST_LEFT_AND_TOP_TOOLTIP);
		static const std::string tooltipDestWidthAndHeight = MakeDragIntTooltipText(IDS_DEST_WIDTH_AND_HEIGHT_TOOLTIP);

		RectTransferData dummyRectForNoRect;

		m_GuiContext.selectedRectIndex = (std::min)(m_GuiContext.selectedRectIndex,
			static_cast<int>(m_rectTransfers.size()) - 1);
		auto& selectedRectTransfer = m_GuiContext.selectedRectIndex >= 0 ?
			m_rectTransfers[m_GuiContext.selectedRectIndex] : dummyRectForNoRect;

		{
			static const std::string labelRectName = LoadTHRotatorStringUtf8(IDS_RECT_NAME);
			static const std::string tooltipRectName = LoadTHRotatorStringUtf8(IDS_RECT_NAME_TOOLTIP);

			char nameEditBuffer[128];
			strcpy_s(nameEditBuffer, selectedRectTransfer.name.c_str());
			ImGui::InputText(labelRectName.c_str(), nameEditBuffer, _countof(nameEditBuffer),
				m_GuiContext.selectedRectIndex >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);
			ShowPreviousItemTooltip(tooltipRectName.c_str());
			selectedRectTransfer.name = nameEditBuffer;
		}

		DragInt2_POINT(labelSourceLeftAndTop.c_str(), selectedRectTransfer.sourcePosition, COORDINATE_DRAG_SPEED, COORDINATE_MIN, COORDINATE_MAX, "%0.f px");
		ShowPreviousItemTooltip(tooltipSourceLeftAndTop.c_str());

		DragInt2_SIZE(labelSourceWidthAndHeight.c_str(), selectedRectTransfer.sourceSize, COORDINATE_DRAG_SPEED, COORDINATE_MIN, COORDINATE_MAX, "%0.f px");
		ShowPreviousItemTooltip(tooltipSourceWidthAndHeight.c_str());

		DragInt2_POINT(labelDestLeftAndTop.c_str(), selectedRectTransfer.destPosition, COORDINATE_DRAG_SPEED, COORDINATE_MIN, COORDINATE_MAX, "%0.f px");
		ShowPreviousItemTooltip(tooltipDestLeftAndTop.c_str());

		DragInt2_SIZE(labelDestWidthAndHeight.c_str(), selectedRectTransfer.destSize, COORDINATE_DRAG_SPEED, COORDINATE_MIN, COORDINATE_MAX, "%0.f px");
		ShowPreviousItemTooltip(tooltipDestWidthAndHeight.c_str());

		// Per-rect rotation

		int rotationAngle = static_cast<int>(selectedRectTransfer.rotation);
		ImGui::PushID("PerRectRotation");

		static const std::string tooltipRectRotationTooltip = LoadTHRotatorStringUtf8(IDS_RECT_ROTATION_TOOLTIP);

		// 0 degrees
		ImGui::RadioButton("0\xC2\xB0", &rotationAngle, Rotation_0);
		ShowPreviousItemTooltip(tooltipRectRotationTooltip.c_str());
		ImGui::SameLine();

		// 90 degrees
		ImGui::RadioButton("90\xC2\xB0", &rotationAngle, Rotation_90);
		ShowPreviousItemTooltip(tooltipRectRotationTooltip.c_str());
		ImGui::SameLine();

		// 180 degrees
		ImGui::RadioButton("180\xC2\xB0", &rotationAngle, Rotation_180);
		ShowPreviousItemTooltip(tooltipRectRotationTooltip.c_str());
		ImGui::SameLine();

		// 270 degrees
		ImGui::RadioButton("270\xC2\xB0", &rotationAngle, Rotation_270);
		ShowPreviousItemTooltip(tooltipRectRotationTooltip.c_str());

		ImGui::PopID();
		selectedRectTransfer.rotation = static_cast<RotationAngle>(rotationAngle);

		static const std::string labelAdd = LoadTHRotatorStringUtf8(IDS_ADD);
		static const std::string labelDelete = LoadTHRotatorStringUtf8(IDS_DELETE);

		if (!ImGui::Button(labelAdd.c_str()))
		{
			static const std::string tooltipAdd = LoadTHRotatorStringUtf8(IDS_ADDITION_TOOLTIP);
			ShowPreviousItemTooltip(tooltipAdd.c_str());
		}
		else
		{
			int newNameSuffix = 1;
			std::string newName;

			auto nameOverlapPredicate = [&newName](const RectTransferData& r)
			{
				return newName == r.name;
			};

			newName = fmt::format("NewRect_{}", newNameSuffix++);

			auto rectWithOverlappingName = std::find_if(m_rectTransfers.begin(),
				m_rectTransfers.end(), nameOverlapPredicate);

			while (rectWithOverlappingName != m_rectTransfers.end())
			{
				newName = fmt::format("NewRect_{}", newNameSuffix++);
				rectWithOverlappingName = std::find_if(m_rectTransfers.begin(),
					m_rectTransfers.end(), nameOverlapPredicate);
			}

			m_rectTransfers.emplace_back();
			auto& addedRect = m_rectTransfers.back();

			addedRect.name = std::move(newName);
			addedRect.sourceSize.cx = addedRect.sourceSize.cy = 10;
			addedRect.destSize.cx = addedRect.destSize.cy = 10;
			m_GuiContext.selectedRectIndex = static_cast<int>(m_rectTransfers.size()) - 1;
		}

		if (m_GuiContext.selectedRectIndex >= 0)
		{
			ImGui::SameLine();

			if (ImGui::Button(labelDelete.c_str()))
			{
				m_rectTransfers.erase(m_rectTransfers.begin() + m_GuiContext.selectedRectIndex);
				m_GuiContext.selectedRectIndex = (std::min)(m_GuiContext.selectedRectIndex,
					static_cast<int>(m_rectTransfers.size()) - 1);
			}
			else
			{
				static const std::string tooltipDelete = LoadTHRotatorStringUtf8(IDS_DELETION_TOOLTIP);
				ShowPreviousItemTooltip(tooltipDelete.c_str());
			}

			ImGui::SameLine();

			if (ImGui::Button("\xE2\x86\x91") // upwards arrow
				&& m_GuiContext.selectedRectIndex > 0)
			{
				int newItemPosition = m_GuiContext.selectedRectIndex - 1;
				std::swap(m_rectTransfers[m_GuiContext.selectedRectIndex],
					m_rectTransfers[newItemPosition]);
				m_GuiContext.selectedRectIndex = newItemPosition;
			}
			else
			{
				static const std::string tooltipMoveUp = LoadTHRotatorStringUtf8(IDS_MOVEUP_TOOLTIP);
				ShowPreviousItemTooltip(tooltipMoveUp.c_str());
			}

			ImGui::SameLine();

			if (ImGui::Button("\xE2\x86\x93") // downwards arrow
				&& m_GuiContext.selectedRectIndex < static_cast<int>(m_rectTransfers.size()) - 1)
			{
				int newItemPosition = m_GuiContext.selectedRectIndex + 1;
				std::swap(m_rectTransfers[m_GuiContext.selectedRectIndex],
					m_rectTransfers[newItemPosition]);
				m_GuiContext.selectedRectIndex = newItemPosition;
			}
			else
			{
				static const std::string tooltipMoveDown = LoadTHRotatorStringUtf8(IDS_MOVEDOWN_TOOLTIP);
				ShowPreviousItemTooltip(tooltipMoveDown.c_str());
			}
		}

		if (m_GuiContext.rectListBoxItemBuffer.size() != m_rectTransfers.size())
		{
			m_GuiContext.rectListBoxItemBuffer = std::vector<const char*>(m_rectTransfers.size());
		}

		for (std::size_t rectIndex = 0; rectIndex < m_rectTransfers.size(); rectIndex++)
		{
			m_GuiContext.rectListBoxItemBuffer[rectIndex] = m_rectTransfers[rectIndex].name.c_str();
		}

		static const std::string labelRectList = LoadTHRotatorStringUtf8(IDS_RECT_LIST);
		static const std::string tooltipRectList = LoadTHRotatorStringUtf8(IDS_RECT_LIST_TOOLTIP);

		ImGui::ListBox(labelRectList.c_str(), &m_GuiContext.selectedRectIndex,
			m_GuiContext.rectListBoxItemBuffer.data(),
			static_cast<int>(m_GuiContext.rectListBoxItemBuffer.size()), 4);
		ShowPreviousItemTooltip(tooltipRectList.c_str());
	}

	static const std::string labelHide = LoadTHRotatorStringUtf8(IDS_HIDE_EDITOR_SHORT);
	static const std::string labelReload = LoadTHRotatorStringUtf8(IDS_RELOAD);
	static const std::string labelSave = LoadTHRotatorStringUtf8(IDS_SAVE);
	static const std::string labelAbout = LoadTHRotatorStringUtf8(IDS_ABOUT);

	static const std::string tooltipHide = LoadTHRotatorStringUtf8(IDS_HIDE_EDITOR_SHORT_TOOLTIP);
	static const std::string tooltipReload = LoadTHRotatorStringUtf8(IDS_RELOAD_TOOLTIP);
	static const std::string tooltipSave = LoadTHRotatorStringUtf8(IDS_SAVE_TOOLTIP);
	static const std::string tooltipAbout = LoadTHRotatorStringUtf8(IDS_ABOUT_TOOLTIP);

	static const std::string labelAboutTHRotator = LoadTHRotatorStringUtf8(IDS_ABOUT_THROTATOR);

	if (ImGui::Button(labelHide.c_str()))
	{
		m_bEditorShown = false;
		UpdateVisibilitySwitchMenuText();
	}

	ShowPreviousItemTooltip(tooltipHide.c_str());

	ImGui::SameLine();

	if (ImGui::Button(labelReload.c_str()))
	{
		LoadSettings();
	}

	ShowPreviousItemTooltip(tooltipReload.c_str());

	ImGui::SameLine();

	if (ImGui::Button(labelSave.c_str()))
	{
		SaveSettings();
		m_bSaveBySysKeyAllowed = true;
	}

	ShowPreviousItemTooltip(tooltipSave.c_str());

	ImGui::SameLine();
	
	if (ImGui::Button(labelAbout.c_str()))
	{
		ImGui::OpenPopup(labelAboutTHRotator.c_str());
	}

	ShowPreviousItemTooltip(tooltipAbout.c_str());

	if (ImGui::BeginPopupModal(labelAboutTHRotator.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static const std::string versionInfoString = fmt::format("THRotator {} for Direct3D {}",
			THROTATOR_VERSION_STRING, DIRECT3D_VERSION >> 8);

		ImGui::Text(versionInfoString.c_str());
		ImGui::Text("\xC2\xA9 2017 massanoori."); // "(c) 2017 massanoori."
		if (ImGui::Button("OK"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::End();

	if (bPreviousVerticalWindow != m_bVerticallyLongWindow)
	{
		m_bVerticallyLongWindow = bPreviousVerticalWindow;
		SetVerticallyLongWindow(!bPreviousVerticalWindow);
	}
}

void THRotatorEditorContext::UpdateWindowResolution(int requestedWidth, int requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight)
{
	RECT rcClient, rcWindow;
	GetClientRect(m_hTouhouWin, &rcClient);
	GetWindowRect(m_hTouhouWin, &rcWindow);

	if (IsBorderlessWindow())
	{
		// Window is borderless
		return;
	}

	// Detect the application modified the window size.
	// If detected, update original client size.

	if (rcClient.left + m_modifiedTouhouClientSize.cx != rcClient.right ||
		rcClient.top + m_modifiedTouhouClientSize.cy != rcClient.bottom)
	{
		m_originalTouhouClientSize.cx = rcClient.right - rcClient.left;
		m_originalTouhouClientSize.cy = rcClient.bottom - rcClient.top;
	}

	// Keep both extent and aspect ratio of window.

	int newWidth = m_originalTouhouClientSize.cx;
	int newHeight = m_originalTouhouClientSize.cy;

	if (newWidth < newHeight * requestedWidth / requestedHeight)
	{
		newWidth = newHeight * requestedWidth / requestedHeight;
	}
	else if (newHeight < newWidth * requestedHeight / requestedWidth)
	{
		newHeight = newWidth * requestedHeight / requestedWidth;
	}

	if (m_bVerticallyLongWindow)
	{
		std::swap(newWidth, newHeight);
	}

	outBackBufferWidth = newWidth;
	outBackBufferHeight = newHeight;

	MoveWindow(m_hTouhouWin, rcWindow.left, rcWindow.top,
		(rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left) + newWidth,
		(rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top) + newHeight, TRUE);

	RECT rcModifiedClient;
	GetClientRect(m_hTouhouWin, &rcModifiedClient);
	m_modifiedTouhouClientSize.cx = rcModifiedClient.right - rcModifiedClient.left;
	m_modifiedTouhouClientSize.cy = rcModifiedClient.bottom - rcModifiedClient.top;

	OutputLogMessagef(LogSeverity::Info, "Tried to update window client size to {0}x{1}", newWidth, newHeight);
	OutputLogMessagef(LogSeverity::Info, "Result of updating window client size: {0}x{1}",
		m_modifiedTouhouClientSize.cx, m_modifiedTouhouClientSize.cy);
}

void THRotatorEditorContext::ApplySetting(const THRotatorSetting& setting)
{
	m_judgeThreshold = setting.judgeThreshold;
	m_mainScreenTopLeft = setting.mainScreenTopLeft;
	m_mainScreenSize = setting.mainScreenSize;
	m_yOffset = setting.yOffset;
	m_bEditorShownInitially = setting.bVisible;
	m_bVerticallyLongWindow = setting.bVerticallyLongWindow;
	m_rotationAngle = setting.rotationAngle;
	m_filterType = setting.filterType;

	m_rectTransfers = setting.rectTransfers;
}

void THRotatorEditorContext::RetrieveSetting(THRotatorSetting& setting) const
{
	setting.judgeThreshold = m_judgeThreshold;
	setting.mainScreenTopLeft = m_mainScreenTopLeft;
	setting.mainScreenSize = m_mainScreenSize;
	setting.yOffset = m_yOffset;
	setting.bVisible = m_bEditorShownInitially;
	setting.bVerticallyLongWindow = m_bVerticallyLongWindow;
	setting.filterType = m_filterType;
	setting.rotationAngle = m_rotationAngle;
	setting.rectTransfers = m_rectTransfers;
}
