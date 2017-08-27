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

const char* THROTATOR_VERSION_STRING = "1.4.0-devel";

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

	double touhouSeriesNumber = GetTouhouSeriesNumber();

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

	OutputLogMessagef(LogSeverity::Info, "Destructing THRotatorEditorContext");
}

void THRotatorEditorContext::UpdateVisibilitySwitchMenuText()
{
	MENUITEMINFO mi;
	ZeroMemory(&mi, sizeof(mi));

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING;

	auto str = LoadTHRotatorStringUnicode(g_hModule, m_bEditorShown ? IDS_HIDE_EDITOR : IDS_SHOW_EDITOR);
	mi.dwTypeData = const_cast<LPTSTR>(str.c_str());

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
		auto saveFailureMessage = LoadTHRotatorStringUtf8(g_hModule, IDS_SETTING_FILE_SAVE_FAILED);
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
	m_bSaveBySysKeyAllowed = bLoadSuccess;

	if (!bLoadSuccess)
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to load");

		auto loadFailureMessage = LoadTHRotatorStringUtf8(g_hModule, IDS_SETTING_FILE_LOAD_FAILED);
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
							static std::string messageLossOfDataPrevention = LoadTHRotatorStringUtf8(g_hModule, IDS_LOSS_OF_DATA_PREVENTED);
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
		ImGui::OpenPopup("Error message");
		if (ImGui::BeginPopupModal("Error message", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static std::string labelThisWindowClosesInSeconds = LoadTHRotatorStringUtf8(g_hModule, IDS_WINDOW_CLOSES_IN_SECONDS);

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
		if (errorMessagePtr)
		{
			ImGui::Render();
		}
		return;
	}

	const ImVec2 initialTHRotatorWindowSize(320.0f, 0.0f);
	if (!ImGui::Begin("THRotator", nullptr, initialTHRotatorWindowSize))
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
		static std::string labelVerticalWindow = LoadTHRotatorStringUtf8(g_hModule, IDS_VERTICAL_WINDOW);
		ImGui::Checkbox(labelVerticalWindow.c_str(), &m_bVerticallyLongWindow);
	}

	{
		static std::string labelShowFirst = LoadTHRotatorStringUtf8(g_hModule, IDS_SHOW_THROTATOR_FIRST);
		static std::string labelForceRearrangement = LoadTHRotatorStringUtf8(g_hModule, IDS_FORCE_REARRANGEMENTS);

		ImGui::Checkbox(labelShowFirst.c_str(), &m_bEditorShownInitially);
		ImGui::Checkbox(labelForceRearrangement.c_str(), &m_bHUDRearrangeForced);
	}

	static std::string labelGameplayDetection = LoadTHRotatorStringUtf8(g_hModule, IDS_GAMEPLAY_DETECTION);
	if (ImGui::CollapsingHeader(labelGameplayDetection.c_str()))
	{
		static std::string labelSetVPCount = LoadTHRotatorStringUtf8(g_hModule, IDS_SET_VP_COUNT);
		static std::string labelThreshold = LoadTHRotatorStringUtf8(g_hModule, IDS_THRESHOLD);

		const std::string currentSetVPCountText = fmt::format("{}", m_judgeCountPrev).c_str();
		ImGui::InputText(labelSetVPCount.c_str(),
			const_cast<char*>(currentSetVPCountText.c_str()), currentSetVPCountText.length(),
			ImGuiInputTextFlags_ReadOnly);

		ImGui::InputInt(labelThreshold.c_str(), &m_judgeThreshold);

		if (m_judgeThreshold < 0)
		{
			m_judgeThreshold = 0;
		}
		
		if (m_judgeThreshold > 999)
		{
			m_judgeThreshold = 999;
		}
	}

	static std::string labelMainScreenRegion = LoadTHRotatorStringUtf8(g_hModule, IDS_MAIN_SCREEN_REGION);
	if (ImGui::CollapsingHeader(labelMainScreenRegion.c_str()))
	{
		static std::string labelLeftAndTop = LoadTHRotatorStringUtf8(g_hModule, IDS_LEFT_AND_TOP);
		static std::string labelWidthAndHeight = LoadTHRotatorStringUtf8(g_hModule, IDS_WIDTH_AND_HEIGHT);
		static std::string labelYOffset = LoadTHRotatorStringUtf8(g_hModule, IDS_Y_OFFSET);

		int leftAndTop[] = { m_mainScreenTopLeft.x, m_mainScreenTopLeft.y };
		ImGui::InputInt2(labelLeftAndTop.c_str(), leftAndTop);

		leftAndTop[0] = (std::max)(0, (std::min)(leftAndTop[0], 999));
		leftAndTop[1] = (std::max)(0, (std::min)(leftAndTop[1], 999));

		m_mainScreenTopLeft.x = leftAndTop[0];
		m_mainScreenTopLeft.y = leftAndTop[1];

		int widthAndHeight[] = { m_mainScreenSize.cx, m_mainScreenSize.cy };
		ImGui::InputInt2(labelWidthAndHeight.c_str(), widthAndHeight);

		widthAndHeight[0] = (std::max)(0, (std::min)(widthAndHeight[0], 999));
		widthAndHeight[1] = (std::max)(0, (std::min)(widthAndHeight[1], 999));

		m_mainScreenSize.cx = widthAndHeight[0];
		m_mainScreenSize.cy = widthAndHeight[1];

		ImGui::InputInt(labelYOffset.c_str(), &m_yOffset);
	}

	static std::string labelPixelInterpolation = LoadTHRotatorStringUtf8(g_hModule, IDS_PIXEL_INTERPOLATION);
	if (ImGui::CollapsingHeader(labelPixelInterpolation.c_str()))
	{
		static std::string labelFilterNone = LoadTHRotatorStringUtf8(g_hModule, IDS_FILTER_NONE);
		static std::string labelFilterBilinear = LoadTHRotatorStringUtf8(g_hModule, IDS_FILTER_BILINEAR);

		std::underlying_type<D3DTEXTUREFILTERTYPE>::type rawFilterType = m_filterType;
		ImGui::PushID("PixelInterpolationSelection");
		ImGui::RadioButton(labelFilterNone.c_str(), &rawFilterType, D3DTEXF_NONE); ImGui::SameLine();
		ImGui::RadioButton(labelFilterBilinear.c_str(), &rawFilterType, D3DTEXF_LINEAR);
		ImGui::PopID();
		m_filterType = static_cast<D3DTEXTUREFILTERTYPE>(rawFilterType);
	}

	static std::string labelOtherRectangles = LoadTHRotatorStringUtf8(g_hModule, IDS_OTHER_RECTANGLES);
	if (ImGui::CollapsingHeader(labelOtherRectangles.c_str()))
	{
		static std::string labelSourceLeftAndTop = LoadTHRotatorStringUtf8(g_hModule, IDS_SOURCE_LEFT_AND_TOP);
		static std::string labelSourceWidthAndHeight = LoadTHRotatorStringUtf8(g_hModule, IDS_SOURCE_WIDTH_AND_HEIGHT);
		static std::string labelDestLeftAndTop = LoadTHRotatorStringUtf8(g_hModule, IDS_DEST_LEFT_AND_TOP);
		static std::string labelDestWidthAndHeight = LoadTHRotatorStringUtf8(g_hModule, IDS_DEST_WIDTH_AND_HEIGHT);

		RectTransferData dummyRectForNoRect;

		m_GuiContext.selectedRectIndex = (std::min)(m_GuiContext.selectedRectIndex,
			static_cast<int>(m_rectTransfers.size()) - 1);
		auto& selectedRectTransfer = m_GuiContext.selectedRectIndex >= 0 ?
			m_rectTransfers[m_GuiContext.selectedRectIndex] : dummyRectForNoRect;

		{
			static std::string labelRectName = LoadTHRotatorStringUtf8(g_hModule, IDS_RECT_NAME);

			char nameEditBuffer[128];
			strcpy_s(nameEditBuffer, selectedRectTransfer.name.c_str());
			ImGui::InputText(labelRectName.c_str(), nameEditBuffer, _countof(nameEditBuffer),
				m_GuiContext.selectedRectIndex >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);
			selectedRectTransfer.name = nameEditBuffer;
		}

		int srcLeftAndTop[] =
		{
			selectedRectTransfer.sourcePosition.x,
			selectedRectTransfer.sourcePosition.y,
		};

		ImGui::InputInt2(labelSourceLeftAndTop.c_str(), srcLeftAndTop,
			m_GuiContext.selectedRectIndex >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.sourcePosition.x = srcLeftAndTop[0];
		selectedRectTransfer.sourcePosition.y = srcLeftAndTop[1];

		int srcWidthAndHeight[] =
		{
			selectedRectTransfer.sourceSize.cx,
			selectedRectTransfer.sourceSize.cy,
		};

		ImGui::InputInt2(labelSourceWidthAndHeight.c_str(), srcWidthAndHeight,
			m_GuiContext.selectedRectIndex >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.sourceSize.cx = srcWidthAndHeight[0];
		selectedRectTransfer.sourceSize.cy = srcWidthAndHeight[1];

		int dstLeftAndTop[] =
		{
			selectedRectTransfer.destPosition.x,
			selectedRectTransfer.destPosition.y,
		};

		ImGui::InputInt2(labelDestLeftAndTop.c_str(), dstLeftAndTop,
			m_GuiContext.selectedRectIndex >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

		selectedRectTransfer.destPosition.x = dstLeftAndTop[0];
		selectedRectTransfer.destPosition.y = dstLeftAndTop[1];

		int dstWidthAndHeight[] =
		{
			selectedRectTransfer.destSize.cx,
			selectedRectTransfer.destSize.cy,
		};

		ImGui::InputInt2(labelDestWidthAndHeight.c_str(), dstWidthAndHeight,
			m_GuiContext.selectedRectIndex >= 0 ? 0 : ImGuiInputTextFlags_ReadOnly);

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

		static std::string labelAdd = LoadTHRotatorStringUtf8(g_hModule, IDS_ADD);
		static std::string labelDelete = LoadTHRotatorStringUtf8(g_hModule, IDS_DELETE);

		if (ImGui::Button(labelAdd.c_str()))
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

			ImGui::SameLine();

			if (ImGui::Button(u8"Å™") && m_GuiContext.selectedRectIndex > 0)
			{
				int newItemPosition = m_GuiContext.selectedRectIndex - 1;
				std::swap(m_rectTransfers[m_GuiContext.selectedRectIndex],
					m_rectTransfers[newItemPosition]);
				m_GuiContext.selectedRectIndex = newItemPosition;
			}

			ImGui::SameLine();

			if (ImGui::Button(u8"Å´")
				&& m_GuiContext.selectedRectIndex < static_cast<int>(m_rectTransfers.size()) - 1)
			{
				int newItemPosition = m_GuiContext.selectedRectIndex + 1;
				std::swap(m_rectTransfers[m_GuiContext.selectedRectIndex],
					m_rectTransfers[newItemPosition]);
				m_GuiContext.selectedRectIndex = newItemPosition;
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

		static std::string labelRectList = LoadTHRotatorStringUtf8(g_hModule, IDS_RECT_LIST);

		ImGui::ListBox(labelRectList.c_str(), &m_GuiContext.selectedRectIndex,
			m_GuiContext.rectListBoxItemBuffer.data(),
			m_GuiContext.rectListBoxItemBuffer.size(), 4);
	}

	static std::string labelReload = LoadTHRotatorStringUtf8(g_hModule, IDS_RELOAD);
	static std::string labelSave = LoadTHRotatorStringUtf8(g_hModule, IDS_SAVE);
	static std::string labelAbout = LoadTHRotatorStringUtf8(g_hModule, IDS_ABOUT);
	static std::string labelAboutTHRotator = LoadTHRotatorStringUtf8(g_hModule, IDS_ABOUT_THROTATOR);

	if (ImGui::Button(labelReload.c_str()))
	{
		LoadSettings();
	}

	ImGui::SameLine();

	if (ImGui::Button(labelSave.c_str()))
	{
		SaveSettings();
		m_bSaveBySysKeyAllowed = true;
	}

	ImGui::SameLine();
	
	if (ImGui::Button(labelAbout.c_str()))
	{
		ImGui::OpenPopup(labelAboutTHRotator.c_str());
	}

	if (ImGui::BeginPopupModal(labelAboutTHRotator.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(fmt::format("THRotator {}", THROTATOR_VERSION_STRING).c_str());
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
