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

LPCTSTR THROTATOR_VERSION_STRING = _T("1.3.1");

HHOOK ms_hHook;
std::map<HWND, std::weak_ptr<THRotatorEditorContext>> ms_touhouWinToContext;
UINT ms_switchVisibilityID = 12345u;

}

THRotatorEditorContext::THRotatorEditorContext(HWND hTouhouWin)
	: m_judgeCountPrev(0)
	, m_hTouhouWin(hTouhouWin)
	, m_bHUDRearrangeForced(false)
	, m_deviceResetRevision(0)
	, m_bInitialized(false)
	, m_bScreenCaptureQueued(false)
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

	double touhouSeriesNumber = GetTouhouSeriesNumber();

	OutputLogMessagef(LogSeverity::Info, L"Initializing THRotatorEditorContext");
	LoadSettings();
	m_bEditorShown = m_bEditorShownInitially;

	m_currentRectTransfers = m_editedRectTransfers;

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

	OutputLogMessagef(LogSeverity::Info, L"THRotatorEditorContext has been initialized");
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
	return m_currentRectTransfers;
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

LPCTSTR THRotatorEditorContext::GetErrorMessage() const
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

	OutputLogMessagef(LogSeverity::Info, L"Destructing THRotatorEditorContext");
}

void THRotatorEditorContext::UpdateVisibilitySwitchMenuText()
{
	MENUITEMINFO mi;
	ZeroMemory(&mi, sizeof(mi));

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING;

	auto str = LoadTHRotatorString(g_hModule, m_bEditorShown ? IDS_HIDE_EDITOR : IDS_SHOW_EDITOR);
	mi.dwTypeData = const_cast<LPTSTR>(str.c_str());

	SetMenuItemInfo(m_hSysMenu, ms_switchVisibilityID, FALSE, &mi);
}

void THRotatorEditorContext::SetNewErrorMessage(std::basic_string<TCHAR>&& message)
{
	LARGE_INTEGER frequency;
	::QueryPerformanceFrequency(&frequency);
	::QueryPerformanceCounter(&m_errorMessageExpirationClock);

	const int TTL = 8;

	m_errorMessageExpirationClock.QuadPart += frequency.QuadPart * TTL;
	m_errorMessage = std::move(message);
}

bool THRotatorEditorContext::SaveSettings()
{
	THRotatorSetting settings;
	settings.judgeThreshold = m_judgeThreshold;
	settings.mainScreenTopLeft = m_mainScreenTopLeft;
	settings.mainScreenSize = m_mainScreenSize;
	settings.yOffset = m_yOffset;
	settings.bVisible = m_bEditorShownInitially;
	settings.bVerticallyLongWindow = m_bVerticallyLongWindow;
	settings.filterType = m_filterType;
	settings.rotationAngle = m_rotationAngle;
	settings.rectTransfers = m_currentRectTransfers;

	bool bSaveSuccess = THRotatorSetting::Save(settings);

	if (!bSaveSuccess)
	{
		auto saveFailureMessage = LoadTHRotatorString(g_hModule, IDS_SETTING_FILE_SAVE_FAILED);
#ifdef _UNICODE
		SetNewErrorMessage(std::move(saveFailureMessage));
#else
		SetNewErrorMessage(ConvertFromUnicodeToSjis(saveFailureMessage));
#endif
	}

	return bSaveSuccess;
}

bool THRotatorEditorContext::LoadSettings()
{
	THRotatorSetting setting;
	THRotatorFormatVersion formatVersion;

	bool bLoadSuccess = THRotatorSetting::Load(setting, formatVersion);

	m_judgeThreshold = setting.judgeThreshold;
	m_mainScreenTopLeft = setting.mainScreenTopLeft;
	m_mainScreenSize = setting.mainScreenSize;
	m_yOffset = setting.yOffset;
	m_bEditorShownInitially = setting.bVisible;
	m_bVerticallyLongWindow = setting.bVerticallyLongWindow;
	m_rotationAngle = setting.rotationAngle;
	m_filterType = setting.filterType;

	m_editedRectTransfers = std::move(setting.rectTransfers);

	if (!bLoadSuccess)
	{
		OutputLogMessagef(LogSeverity::Error, L"Failed to load");

		auto loadFailureMessage = LoadTHRotatorString(g_hModule, IDS_SETTING_FILE_LOAD_FAILED);
#ifdef _UNICODE
		SetNewErrorMessage(std::move(loadFailureMessage));
#else
		SetNewErrorMessage(ConvertFromUnicodeToSjis(loadFailureMessage));
#endif

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
			m_judgeThreshold++;

			OutputLogMessagef(LogSeverity::Info, L"Threshold has been converted");
		}
	}

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
						context->SaveSettings();
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

				case 'R':
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

void THRotatorEditorContext::RenderAndUpdateEditor(bool bFullscreen)
{
	auto errorMessagePtr = GetErrorMessage();

	// when windowed: cursor from OS
	// when fullscreen: cursor by ImGui only while editing
	
	ImGui::GetIO().MouseDrawCursor = bFullscreen && (m_bEditorShown || errorMessagePtr);

	if (errorMessagePtr)
	{
		ImGui::OpenPopup("ErrorMessage");
		if (ImGui::BeginPopupModal("ErrorMessage", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			LARGE_INTEGER currentClock, clockFrequency;
			QueryPerformanceCounter(&currentClock);
			QueryPerformanceFrequency(&clockFrequency);

			auto durationInClock = m_errorMessageExpirationClock.QuadPart - currentClock.QuadPart;

			ImGui::Text(ConvertFromUnicodeToUtf8(errorMessagePtr).c_str());
			ImGui::Text(fmt::format("This window closes in {} second(s).",
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
		if (errorMessagePtr)
		{
			ImGui::Render();
		}
		return;
	}

	if (!ImGui::Begin("THRotator"))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		ImGui::Render();
		return;
	}

	ImGui::PushItemWidth(-140);

	{
		int rotationAngle = static_cast<int>(m_rotationAngle);
		ImGui::PushID("MainScreenRotation");
		ImGui::RadioButton(u8"0Åã", reinterpret_cast<int*>(&rotationAngle), Rotation_0); ImGui::SameLine();
		ImGui::RadioButton(u8"90Åã", reinterpret_cast<int*>(&rotationAngle), Rotation_90); ImGui::SameLine();
		ImGui::RadioButton(u8"180Åã", reinterpret_cast<int*>(&rotationAngle), Rotation_180); ImGui::SameLine();
		ImGui::RadioButton(u8"270Åã", reinterpret_cast<int*>(&rotationAngle), Rotation_270);
		ImGui::PopID();
		m_rotationAngle = static_cast<RotationAngle>(rotationAngle);
	}

	bool bPreviousVerticalWindow = m_bVerticallyLongWindow;
	{
		ImGui::Checkbox("Vertical window", &m_bVerticallyLongWindow);
	}

	{
		ImGui::Checkbox("Show THRotator first", &m_bEditorShownInitially);
		ImGui::Checkbox("Force HUD rearrangements", &m_bHUDRearrangeForced);
	}

	if (ImGui::CollapsingHeader("Gameplay detection"))
	{
		const std::string currentSetVPCountText = fmt::format("{}", m_judgeCountPrev).c_str();
		ImGui::InputText("SetViewport() count",
			const_cast<char*>(currentSetVPCountText.c_str()), currentSetVPCountText.length(),
			ImGuiInputTextFlags_ReadOnly);

		ImGui::InputInt("Threshold", &m_judgeThreshold);

		if (m_judgeThreshold < 0)
		{
			m_judgeThreshold = 0;
		}
		
		if (m_judgeThreshold > 999)
		{
			m_judgeThreshold = 999;
		}
	}

	if (ImGui::CollapsingHeader("Main screen region"))
	{
		int leftAndTop[] = { m_mainScreenTopLeft.x, m_mainScreenTopLeft.y };
		ImGui::InputInt2("Left and Top", leftAndTop);

		leftAndTop[0] = (std::max)(0, (std::min)(leftAndTop[0], 999));
		leftAndTop[1] = (std::max)(0, (std::min)(leftAndTop[1], 999));

		m_mainScreenTopLeft.x = leftAndTop[0];
		m_mainScreenTopLeft.y = leftAndTop[1];

		int widthAndHeight[] = { m_mainScreenSize.cx, m_mainScreenSize.cy };
		ImGui::InputInt2("Width and Height", widthAndHeight);

		widthAndHeight[0] = (std::max)(0, (std::min)(widthAndHeight[0], 999));
		widthAndHeight[1] = (std::max)(0, (std::min)(widthAndHeight[1], 999));

		m_mainScreenSize.cx = widthAndHeight[0];
		m_mainScreenSize.cy = widthAndHeight[1];

		ImGui::InputInt("Y offset", &m_yOffset);
	}

	if (ImGui::CollapsingHeader("Pixel interpolation"))
	{
		std::underlying_type<D3DTEXTUREFILTERTYPE>::type rawFilterType = m_filterType;
		ImGui::PushID("PixelInterpolationSelection");
		ImGui::RadioButton("None", &rawFilterType, D3DTEXF_NONE); ImGui::SameLine();
		ImGui::RadioButton("Bilinear", &rawFilterType, D3DTEXF_LINEAR);
		ImGui::PopID();
		m_filterType = static_cast<D3DTEXTUREFILTERTYPE>(rawFilterType);
	}

	if (ImGui::CollapsingHeader("Other rectangles"))
	{
		// TODO: make rectangle name editable
		// TODO: create array of const char* in advance
		std::vector<std::string> listItemStrs;
		std::vector<const char*> listItemStrPtrs;
		listItemStrs.reserve(m_currentRectTransfers.size());
		listItemStrPtrs.reserve(listItemStrs.size());

		for (const auto& rectTransfer : m_currentRectTransfers)
		{
			listItemStrs.push_back(rectTransfer.name);
			listItemStrPtrs.push_back(listItemStrs.back().c_str());
		}

		static int currentItem = 0;

		RectTransferData dummyRectForNoRect;

		currentItem = (std::min)(currentItem, static_cast<int>(m_currentRectTransfers.size()) - 1);
		auto& selectedRectTransfer = currentItem >= 0 ? m_currentRectTransfers[currentItem] : dummyRectForNoRect;

		{
			char nameEditBuffer[128];
			strcpy_s(nameEditBuffer, selectedRectTransfer.name.c_str());
			ImGui::InputText("Name", nameEditBuffer, _countof(nameEditBuffer),
				currentItem >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);
			selectedRectTransfer.name = nameEditBuffer;
		}

		int srcLeftAndTop[] =
		{
			selectedRectTransfer.sourcePosition.x,
			selectedRectTransfer.sourcePosition.y,
		};

		ImGui::InputInt2("Src Left and Top", srcLeftAndTop,
			currentItem >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.sourcePosition.x = srcLeftAndTop[0];
		selectedRectTransfer.sourcePosition.y = srcLeftAndTop[1];

		int srcWidthAndHeight[] =
		{
			selectedRectTransfer.sourceSize.cx,
			selectedRectTransfer.sourceSize.cy,
		};

		ImGui::InputInt2("Src Width and Height", srcWidthAndHeight,
			currentItem >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.sourceSize.cx = srcWidthAndHeight[0];
		selectedRectTransfer.sourceSize.cy = srcWidthAndHeight[1];

		int dstLeftAndTop[] =
		{
			selectedRectTransfer.destPosition.x,
			selectedRectTransfer.destPosition.y,
		};

		ImGui::InputInt2("Dst Left and Top", dstLeftAndTop,
			currentItem >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.destPosition.x = dstLeftAndTop[0];
		selectedRectTransfer.destPosition.y = dstLeftAndTop[1];

		int dstWidthAndHeight[] =
		{
			selectedRectTransfer.destSize.cx,
			selectedRectTransfer.destSize.cy,
		};

		ImGui::InputInt2("Dst Width and Height", dstWidthAndHeight,
			currentItem >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.destSize.cx = dstWidthAndHeight[0];
		selectedRectTransfer.destSize.cy = dstWidthAndHeight[1];

		int rotationAngle = static_cast<int>(selectedRectTransfer.rotation);
		ImGui::PushID("PerRectRotation");
		ImGui::RadioButton(u8"0Åã", &rotationAngle, Rotation_0); ImGui::SameLine();
		ImGui::RadioButton(u8"90Åã", &rotationAngle, Rotation_90); ImGui::SameLine();
		ImGui::RadioButton(u8"180Åã", &rotationAngle, Rotation_180); ImGui::SameLine();
		ImGui::RadioButton(u8"270Åã", &rotationAngle, Rotation_270);
		ImGui::PopID();
		selectedRectTransfer.rotation = static_cast<RotationAngle>(rotationAngle);

		if (ImGui::Button("Add"))
		{
			int newNameSuffix = 1;
			std::string newName;

			auto nameOverlapPredicate = [&newName](const RectTransferData& r)
			{
				return newName == r.name;
			};

			newName = fmt::format("NewRect_{}", newNameSuffix++);

			auto rectWithOverlappingName = std::find_if(m_currentRectTransfers.begin(),
				m_currentRectTransfers.end(), nameOverlapPredicate);

			while (rectWithOverlappingName != m_currentRectTransfers.end())
			{
				newName = fmt::format("NewRect_{}", newNameSuffix++);
				rectWithOverlappingName = std::find_if(m_currentRectTransfers.begin(),
					m_currentRectTransfers.end(), nameOverlapPredicate);
			}

			m_currentRectTransfers.emplace_back();
			auto& addedRect = m_currentRectTransfers.back();

			addedRect.name = std::move(newName);
			addedRect.sourceSize.cx = addedRect.sourceSize.cy = 10;
			addedRect.destSize.cx = addedRect.destSize.cy = 10;
			currentItem = static_cast<int>(m_currentRectTransfers.size() - 1);
		}

		if (currentItem >= 0)
		{
			ImGui::SameLine();

			if (ImGui::Button("Delete"))
			{
				m_currentRectTransfers.erase(m_currentRectTransfers.begin() + currentItem);
				currentItem = (std::min)(currentItem, static_cast<int>(m_currentRectTransfers.size()) - 1);
			}

			ImGui::SameLine();

			if (ImGui::Button("Up") && currentItem > 0)
			{
				int newItemPosition = currentItem - 1;
				std::swap(m_currentRectTransfers[currentItem],
					m_currentRectTransfers[newItemPosition]);
				currentItem = newItemPosition;
			}

			ImGui::SameLine();

			if (ImGui::Button("Down") && currentItem < static_cast<int>(m_currentRectTransfers.size() - 1))
			{
				int newItemPosition = currentItem + 1;
				std::swap(m_currentRectTransfers[currentItem],
					m_currentRectTransfers[newItemPosition]);
				currentItem = newItemPosition;
			}
		}

		ImGui::ListBox("Rectangle list",
			&currentItem, listItemStrPtrs.data(),
			listItemStrPtrs.size(), 4);
	}

	if (ImGui::Button("Reload"))
	{
		LoadSettings();
		// NOTE: multiple arrays are necessary?
		m_currentRectTransfers = m_editedRectTransfers;
	}

	ImGui::SameLine();

	if (ImGui::Button("Save"))
	{
		// NOTE: multiple arrays are necessary?
		m_editedRectTransfers = m_currentRectTransfers;
		SaveSettings();
	}

	ImGui::SameLine();
	
	if (ImGui::Button("About"))
	{
		ImGui::OpenPopup("About THRotator");
	}

	if (ImGui::BeginPopupModal("About THRotator", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(fmt::format("THRotator {}", "1.4.0-devel").c_str());
		ImGui::Text(u8"\xa9 2017 massanoori.");
		if (ImGui::Button("OK"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::End();

	ImGui::Render();

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

	OutputLogMessagef(LogSeverity::Info, L"Tried to update window client size to {0}x{1}", newWidth, newHeight);
	OutputLogMessagef(LogSeverity::Info, L"Result of updating window client size: {0}x{1}",
		m_modifiedTouhouClientSize.cx, m_modifiedTouhouClientSize.cy);
}
