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
#include "StringResource.h"
#include "resource.h"
#include "EncodingUtils.h"
#include "THRotatorLog.h"

#pragma comment(lib, "comctl32.lib")

namespace
{

LPCTSTR THROTATOR_VERSION_STRING = _T("1.3.0");

HHOOK ms_hHook;
std::map<HWND, std::weak_ptr<THRotatorEditorContext>> ms_touhouWinToContext;
UINT ms_switchVisibilityID = 12345u;

}

THRotatorEditorContext::THRotatorEditorContext(HWND hTouhouWin)
	: m_judgeCountPrev(0)
	, m_hTouhouWin(hTouhouWin)
	, m_bHUDRearrangeForced(false)
	, m_deviceResetRevision(0)
	, m_bModalEditorPreferred(false)
	, m_bNeedModalEditor(false)
	, m_modalEditorWindowPosX(0)
	, m_modalEditorWindowPosY(0)
	, m_bInitialized(false)
	, m_bScreenCaptureQueued(false)
	, m_hEditorWin(nullptr)
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
			INITCOMMONCONTROLSEX InitCtrls;
			InitCtrls.dwSize = sizeof(InitCtrls);
			// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
			// これを設定します。
			InitCtrls.dwICC = ICC_WIN95_CLASSES;
			InitCommonControlsEx(&InitCtrls);

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

	// 妖々夢の場合モーダルで開かないと、入力のフォーカスが奪われる
	if (6.0 < touhouSeriesNumber && touhouSeriesNumber < 7.5)
	{
		m_bNeedModalEditor = true;
	}

	OutputLogMessagef(LogSeverity::Info, L"Initializing THRotatorEditorContext");

	if (!LoadSettings())
	{
		auto loadFailureMessage = LoadTHRotatorString(g_hModule, IDS_SETTING_FILE_LOAD_FAILED);

#ifdef _UNICODE
		SetNewErrorMessage(std::move(loadFailureMessage));
#else
		SetNewErrorMessage(ConvertFromUnicodeToSjis(loadFailureMessage));
#endif
	}

	m_bNeedModalEditor = m_bNeedModalEditor || m_bModalEditorPreferred;

	m_currentRectTransfers = m_editedRectTransfers;

	//	メニューを改造
	HMENU hMenu = GetSystemMenu(m_hTouhouWin, FALSE);
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	m_insertedMenuSeparatorID = GetMenuItemID(hMenu, GetMenuItemCount(hMenu) - 1);

	auto openEditorMenuString = LoadTHRotatorString(g_hModule, IDS_OPEN_EDITOR);
	AppendMenu(hMenu, MF_STRING, ms_switchVisibilityID, openEditorMenuString.c_str());

	m_hSysMenu = hMenu;

	if (m_bNeedModalEditor)
	{
		m_hEditorWin = nullptr;
	}
	else
	{
		HWND hWnd = m_hEditorWin = CreateDialogParam(g_hModule, MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDialogProc, (LPARAM)this);
		if (hWnd == NULL)
		{
			auto dialogCreationFailedMessage = LoadTHRotatorString(g_hModule, IDS_DIALOG_CREATION_FAILED);
			MessageBox(NULL, dialogCreationFailedMessage.c_str(), NULL, MB_ICONSTOP);
		}
	}

	if (!m_bNeedModalEditor)
	{
		SetEditorWindowVisibility(m_bVisible);
	}

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

// ビューポート設定回数を増やす

void THRotatorEditorContext::AddViewportSetCount()
{
	m_judgeCount++;
}

// ビューポート設定回数をUIに渡し、設定回数をリセットして計測を再開

void THRotatorEditorContext::SubmitViewportSetCountToEditor()
{
	m_judgeCountPrev = m_judgeCount;
	m_judgeCount = 0;
}

// ビューポート設定回数が閾値を超えたか

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

	DestroyWindow(m_hEditorWin);

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

void THRotatorEditorContext::SetEditorWindowVisibility(bool bVisible)
{
	assert(!m_bNeedModalEditor && m_hEditorWin);

	m_bVisible = bVisible;
	MENUITEMINFO mi;
	ZeroMemory(&mi, sizeof(mi));

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING;

	auto str = LoadTHRotatorString(g_hModule, bVisible ? IDS_HIDE_EDITOR : IDS_SHOW_EDITOR);
	mi.dwTypeData = const_cast<LPTSTR>(str.c_str());

	SetMenuItemInfo(m_hSysMenu, ms_switchVisibilityID, FALSE, &mi);

	ShowWindow(m_hEditorWin, bVisible ? SW_SHOW : SW_HIDE);
}

void THRotatorEditorContext::InitListView(HWND hLV)
{
	ListView_DeleteAllItems(hLV);
	int i = 0;
	for (std::vector<RectTransferData>::iterator itr = m_editedRectTransfers.begin();
		itr != m_editedRectTransfers.end(); ++itr, ++i)
	{
		LVITEM lvi;
		ZeroMemory(&lvi, sizeof(lvi));

		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = const_cast<LPTSTR>(itr->name.c_str()); // LVITEM::pszTextは出力先バッファとして使われない
		ListView_InsertItem(hLV, &lvi);

		TCHAR str[64];
		lvi.pszText = str;

		lvi.iSubItem = 1;
		wsprintf(str, _T("%d,%d,%d,%d"), itr->sourcePosition.x, itr->sourcePosition.y, itr->sourceSize.cx, itr->sourceSize.cy);
		ListView_SetItem(hLV, &lvi);

		lvi.iSubItem = 2;
		wsprintf(str, _T("%d,%d,%d,%d"), itr->destPosition.x, itr->destPosition.y, itr->destSize.cx, itr->destSize.cy);
		ListView_SetItem(hLV, &lvi);

		lvi.iSubItem = 3;
		wsprintf(str, _T("%d°"), itr->rotation * 90);
		ListView_SetItem(hLV, &lvi);
	}
}

BOOL CALLBACK THRotatorEditorContext::MainDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		auto pContext = reinterpret_cast<THRotatorEditorContext*>(lParam);
		SetWindowLongPtr(hWnd, DWLP_USER, static_cast<LONG_PTR>(lParam));
		assert(lParam == GetWindowLongPtr(hWnd, DWLP_USER));

		SetDlgItemInt(hWnd, IDC_JUDGETHRESHOLD, pContext->m_judgeThreshold, TRUE);
		SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_mainScreenTopLeft.x, FALSE);
		SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_mainScreenTopLeft.y, FALSE);
		SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_mainScreenSize.cx, FALSE);
		SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_mainScreenSize.cy, FALSE);
		SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);

		{
			std::basic_string<TCHAR> versionUiString(_T("Version: "));
			versionUiString += THROTATOR_VERSION_STRING;
			SetDlgItemText(hWnd, IDC_VERSION, versionUiString.c_str());
		}

		SendDlgItemMessage(hWnd, IDC_VISIBLE, BM_SETCHECK, pContext->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0);
		EnableWindow(GetDlgItem(hWnd, IDC_VISIBLE), !pContext->m_bNeedModalEditor);

		switch (pContext->m_filterType)
		{
		case D3DTEXF_NONE:
			SendDlgItemMessage(hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case D3DTEXF_LINEAR:
			SendDlgItemMessage(hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}

		if (pContext->m_bVerticallyLongWindow)
		{
			SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0);
		}

		SendDlgItemMessage(hWnd, IDC_FORCEVERTICAL, BM_SETCHECK, pContext->m_bHUDRearrangeForced ? BST_CHECKED : BST_UNCHECKED, 0);

		pContext->ApplyRotationToEditorWindow(hWnd);

		{
			std::basic_string<TCHAR> columnText;

			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;

			columnText = LoadTHRotatorString(g_hModule, IDS_RECT_NAME);
			lvc.pszText = const_cast<LPTSTR>(columnText.c_str());
			lvc.cx = 64;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 0, &lvc);

			columnText = LoadTHRotatorString(g_hModule, IDS_SOURCE_RECT);
			lvc.pszText = const_cast<LPTSTR>(columnText.c_str());
			lvc.cx = 96;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 1, &lvc);

			columnText = LoadTHRotatorString(g_hModule, IDS_DESTINATION_RECT);
			lvc.pszText = const_cast<LPTSTR>(columnText.c_str());
			lvc.cx = 96;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 2, &lvc);

			columnText = LoadTHRotatorString(g_hModule, IDS_ROTATION_ANGLE);
			lvc.pszText = const_cast<LPTSTR>(columnText.c_str());
			lvc.cx = 48;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 3, &lvc);
		}

		pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));

		return TRUE;
	}

	case WM_CLOSE:
		return FALSE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_GETVPCOUNT:
			SetDlgItemInt(hWnd, IDC_VPCOUNT,
				reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER))->m_judgeCountPrev, FALSE);
			return TRUE;

		case IDOK:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			BOOL bResult = pContext->ApplyChangeFromEditorWindow(hWnd);
			if (!bResult)
			{
				return FALSE;
			}

			if (!pContext->SaveSettings())
			{
				auto saveFailureMessage = LoadTHRotatorString(g_hModule, IDS_SETTING_FILE_SAVE_FAILED);
				MessageBox(hWnd, saveFailureMessage.c_str(), nullptr, MB_ICONSTOP);
				return FALSE;
			}

			if (pContext->m_bNeedModalEditor)
			{
				RECT rc;
				GetWindowRect(hWnd, &rc);
				pContext->m_modalEditorWindowPosX = rc.top;
				pContext->m_modalEditorWindowPosY = rc.bottom;
				EndDialog(hWnd, IDOK);
			}
			return TRUE;
		}

		case IDC_RESET:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			SetDlgItemInt(hWnd, IDC_JUDGETHRESHOLD, pContext->m_judgeThreshold, FALSE);
			SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_mainScreenTopLeft.x, FALSE);
			SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_mainScreenTopLeft.y, FALSE);
			SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_mainScreenSize.cx, FALSE);
			SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_mainScreenSize.cy, FALSE);
			SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);
			pContext->m_editedRectTransfers = pContext->m_currentRectTransfers;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			SendDlgItemMessage(hWnd, IDC_VISIBLE, BM_SETCHECK, pContext->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0);
			if (pContext->m_bVerticallyLongWindow == 1)
				SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0);
			else
				SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_UNCHECKED, 0);

			pContext->ApplyRotationToEditorWindow(hWnd);

			switch (pContext->m_filterType)
			{
			case D3DTEXF_NONE:
				SendDlgItemMessage(hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case D3DTEXF_LINEAR:
				SendDlgItemMessage(hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_UNCHECKED, 0);
				break;
			}
			return TRUE;
		}

		case IDCANCEL:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			if (pContext->m_bNeedModalEditor)
			{
				RECT rc;
				GetWindowRect(hWnd, &rc);
				pContext->m_modalEditorWindowPosX = rc.top;
				pContext->m_modalEditorWindowPosY = rc.bottom;
				EndDialog(hWnd, IDCANCEL);
			}
			else
			{
				pContext->SetEditorWindowVisibility(FALSE);
			}
			return TRUE;
		}

		case IDC_ADDRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			{
				RectTransferData erd = {};
				if (!pContext->OpenEditRectDialog(erd, pContext->m_editedRectTransfers.cend()))
				{
					return FALSE;
				}
				pContext->m_editedRectTransfers.emplace_back(erd);
			}

			const auto& insertedRect = pContext->m_editedRectTransfers.back();

			TCHAR str[64];

			LVITEM lvi;
			ZeroMemory(&lvi, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			lvi.iItem = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
			lvi.pszText = const_cast<LPTSTR>(insertedRect.name.c_str()); // LVITEM::pszTextは出力先として使用されない
			ListView_InsertItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			lvi.pszText = str;

			lvi.iSubItem = 1;
			wsprintf(str, _T("%d,%d,%d,%d"), insertedRect.sourcePosition.x, insertedRect.sourcePosition.y, insertedRect.sourceSize.cx, insertedRect.sourceSize.cy);
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			lvi.iSubItem = 2;
			wsprintf(str, _T("%d,%d,%d,%d"), insertedRect.destPosition.x, insertedRect.destPosition.y, insertedRect.destSize.cx, insertedRect.destSize.cy);
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			lvi.iSubItem = 3;
			switch (insertedRect.rotation)
			{
			case 0:
				lvi.pszText = _T("0°");
				break;

			case 1:
				lvi.pszText = _T("90°");
				break;

			case 2:
				lvi.pszText = _T("180°");
				break;

			case 3:
				lvi.pszText = _T("270°");
				break;
			}
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			return TRUE;
		}

		case IDC_EDITRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				auto columnNotSelectedMessage = LoadTHRotatorString(g_hModule, IDS_RECT_NOT_SELECTED);
				MessageBox(hWnd, columnNotSelectedMessage.c_str(), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}

			{
				RectTransferData erd = pContext->m_editedRectTransfers[i];
				if (!pContext->OpenEditRectDialog(erd, pContext->m_editedRectTransfers.begin() + i))
				{
					return FALSE;
				}
				pContext->m_editedRectTransfers[i] = std::move(erd);
			}

			const auto& editedRect = pContext->m_editedRectTransfers[i];

			TCHAR str[64];
			LVITEM lvi;
			ZeroMemory(&lvi, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			lvi.iItem = i;
			lvi.pszText = const_cast<LPTSTR>(editedRect.name.c_str()); // LVITEM::pszTextは出力先として使用されない
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			lvi.pszText = str;

			lvi.iSubItem = 1;
			wsprintf(str, _T("%d,%d,%d,%d"), editedRect.sourcePosition.x, editedRect.sourcePosition.y, editedRect.sourceSize.cx, editedRect.sourceSize.cy);
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			lvi.iSubItem = 2;
			wsprintf(str, _T("%d,%d,%d,%d"), editedRect.destPosition.x, editedRect.destPosition.y, editedRect.destSize.cx, editedRect.destSize.cy);
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			lvi.iSubItem = 3;
			switch (editedRect.rotation)
			{
			case 0:
				lvi.pszText = _T("0°");
				break;

			case 1:
				lvi.pszText = _T("90°");
				break;

			case 2:
				lvi.pszText = _T("180°");
				break;

			case 3:
				lvi.pszText = _T("270°");
				break;
			}
			ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

			return TRUE;
		}

		case IDC_REMRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				auto columnNotSelectedMessage = LoadTHRotatorString(g_hModule, IDS_RECT_NOT_SELECTED);
				MessageBox(hWnd, columnNotSelectedMessage.c_str(), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			pContext->m_editedRectTransfers.erase(pContext->m_editedRectTransfers.cbegin() + i);
			ListView_DeleteItem(GetDlgItem(hWnd, IDC_ORLIST), i);
			if (ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST)) == 0)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_EDITRECT), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_REMRECT), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_UP), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_DOWN), FALSE);
			}
			return TRUE;
		}

		case IDC_UP:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				auto columnNotSelectedMessage = LoadTHRotatorString(g_hModule, IDS_RECT_NOT_SELECTED);
				MessageBox(hWnd, columnNotSelectedMessage.c_str(), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			if (i == 0)
			{
				MessageBox(hWnd, _T("選択されている矩形が一番上のものです。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			RectTransferData erd = pContext->m_editedRectTransfers[i];
			pContext->m_editedRectTransfers[i] = pContext->m_editedRectTransfers[i - 1];
			pContext->m_editedRectTransfers[i - 1] = erd;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			ListView_SetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST), i - 1);
			EnableWindow(GetDlgItem(hWnd, IDC_DOWN), TRUE);
			if (i == 1)
				EnableWindow(GetDlgItem(hWnd, IDC_UP), FALSE);
			return TRUE;
		}

		case IDC_DOWN:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			int count = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				auto columnNotSelectedMessage = LoadTHRotatorString(g_hModule, IDS_RECT_NOT_SELECTED);
				MessageBox(hWnd, columnNotSelectedMessage.c_str(), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			if (i == count - 1)
			{
				MessageBox(hWnd, _T("選択されている矩形が一番下のものです。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			RectTransferData erd = pContext->m_editedRectTransfers[i];
			pContext->m_editedRectTransfers[i] = pContext->m_editedRectTransfers[i + 1];
			pContext->m_editedRectTransfers[i + 1] = erd;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			ListView_SetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST), i + 1);
			EnableWindow(GetDlgItem(hWnd, IDC_UP), TRUE);
			if (i == count - 2)
				EnableWindow(GetDlgItem(hWnd, IDC_DOWN), FALSE);
			return TRUE;
		}
		}
		return FALSE;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->code == LVN_ITEMCHANGED)
		{
			LPNMLISTVIEW lpnmLV = (LPNMLISTVIEW)lParam;
			int count = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
			BOOL bEnabled = count > 0;
			EnableWindow(GetDlgItem(hWnd, IDC_EDITRECT), bEnabled);
			EnableWindow(GetDlgItem(hWnd, IDC_REMRECT), bEnabled);
			EnableWindow(GetDlgItem(hWnd, IDC_UP), bEnabled && lpnmLV->iItem > 0);
			EnableWindow(GetDlgItem(hWnd, IDC_DOWN), bEnabled && lpnmLV->iItem < count - 1);
		}
		return FALSE;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK THRotatorEditorContext::EditRectDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		SetWindowLongPtr(hWnd, DWLP_USER, static_cast<LONG_PTR>(lParam));
		RectTransferData* pErd = reinterpret_cast<RectTransferData*>(lParam);

		SetDlgItemInt(hWnd, IDC_SRCLEFT, pErd->sourcePosition.x, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCWIDTH, pErd->sourceSize.cx, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCTOP, pErd->sourcePosition.y, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCHEIGHT, pErd->sourceSize.cy, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTLEFT, pErd->destPosition.x, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTWIDTH, pErd->destSize.cx, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTTOP, pErd->destPosition.y, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTHEIGHT, pErd->destSize.cy, TRUE);
		switch (pErd->rotation)
		{
		case 0:
			SendDlgItemMessage(hWnd, IDC_ROT0, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case 1:
			SendDlgItemMessage(hWnd, IDC_ROT90, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case 2:
			SendDlgItemMessage(hWnd, IDC_ROT180, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case 3:
			SendDlgItemMessage(hWnd, IDC_ROT270, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}

		SetDlgItemText(hWnd, IDC_RECTNAME, pErd->name.c_str());

		return TRUE;
	}

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		{
#define GETANDSET(id, var, stringResourceId) {\
					BOOL bRet;UINT ret;\
					ret = GetDlgItemInt( hWnd, id, &bRet, std::is_signed<decltype(var)>::value );\
					if( !bRet )\
					{\
						auto messageString = LoadTHRotatorString(g_hModule, stringResourceId);\
						MessageBox( hWnd, messageString.c_str(), NULL, MB_ICONSTOP );\
						return FALSE;\
					}\
					var = static_cast<decltype(var)>(ret);\
				}

			POINT sourcePosition;
			SIZE sourceSize;
			POINT destPosition;
			SIZE destSize;
			GETANDSET(IDC_SRCLEFT, sourcePosition.x, IDS_INVALID_SOURCE_RECT_LEFT);
			GETANDSET(IDC_SRCTOP, sourcePosition.y, IDS_INVALID_SOURCE_RECT_TOP);
			GETANDSET(IDC_SRCWIDTH, sourceSize.cx, IDS_INVALID_SOURCE_RECT_WIDTH);
			GETANDSET(IDC_SRCHEIGHT, sourceSize.cy, IDS_INVALID_SOURCE_RECT_HEIGHT);
			GETANDSET(IDC_DESTLEFT, destPosition.x, IDS_INVALID_DESTINATION_RECT_LEFT);
			GETANDSET(IDC_DESTTOP, destPosition.y, IDS_INVALID_DESTINATION_RECT_TOP);
			GETANDSET(IDC_DESTWIDTH, destSize.cx, IDS_INVALID_DESTINATION_RECT_WIDTH);
			GETANDSET(IDC_DESTHEIGHT, destSize.cy, IDS_INVALID_DESTINATION_RECT_HEIGHT);
#undef GETANDSET

			auto lengthOfName = GetWindowTextLength(GetDlgItem(hWnd, IDC_RECTNAME));
			if (lengthOfName == 0)
			{
				auto messageString = LoadTHRotatorString(g_hModule, IDS_RECT_NAME_NOT_FILLED);
				MessageBox(hWnd, messageString.c_str(), nullptr, MB_ICONSTOP);
				return FALSE;
			}

			RectTransferData* pErd = reinterpret_cast<RectTransferData*>(GetWindowLongPtr(hWnd, DWLP_USER));

			if (SendDlgItemMessage(hWnd, IDC_ROT0, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_0;
			}
			else if (SendDlgItemMessage(hWnd, IDC_ROT90, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_90;
			}
			else if (SendDlgItemMessage(hWnd, IDC_ROT180, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_180;
			}
			else if (SendDlgItemMessage(hWnd, IDC_ROT270, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_270;
			}
			pErd->sourcePosition = sourcePosition;
			pErd->sourceSize = sourceSize;
			pErd->destPosition = destPosition;
			pErd->destSize = destSize;

			std::vector<TCHAR> nameBuffer(1 + lengthOfName);

			GetDlgItemText(hWnd, IDC_RECTNAME,
				nameBuffer.data(), static_cast<int>(nameBuffer.size()));
			pErd->name = nameBuffer.data();

			EndDialog(hWnd, IDOK);
			return TRUE;
		}

		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

bool THRotatorEditorContext::OpenEditRectDialog(RectTransferData& inoutRectTransfer, std::vector<RectTransferData>::const_iterator editedRectTransfer) const
{
	assert(m_hEditorWin);

RetryEdit:
	if (IDOK != DialogBoxParam(g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), m_hEditorWin, EditRectDialogProc, reinterpret_cast<LPARAM>(&inoutRectTransfer)))
	{
		return false;
	}

	auto overlappingNameItr = std::find_if(m_editedRectTransfers.cbegin(), m_editedRectTransfers.cend(),
		[&inoutRectTransfer](const RectTransferData& data)
	{
		return data.name == inoutRectTransfer.name;
	});

	if (overlappingNameItr != m_editedRectTransfers.cend() &&
		overlappingNameItr != editedRectTransfer)
	{
		auto alreadyExistsString = LoadTHRotatorString(g_hModule, IDS_RECT_NAME_ALREADLY_EXISTS);
		MessageBox(m_hEditorWin, alreadyExistsString.c_str(), NULL, MB_ICONSTOP);
		goto RetryEdit;
	}

	if (IsZeroSizedRectTransfer(inoutRectTransfer))
	{
		auto zeroRectSizeString = LoadTHRotatorString(g_hModule, IDS_ZERO_SIZE_RECT_WARNING);
		MessageBox(m_hEditorWin, zeroRectSizeString.c_str(), NULL, MB_ICONEXCLAMATION);
	}

	return true;
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

bool THRotatorEditorContext::ApplyChangeFromEditorWindow(HWND hEditorWin)
{
	assert(hEditorWin);

#define GETANDSET(id, var, stringResourceId) {\
			BOOL bRet;UINT ret;\
			ret = GetDlgItemInt( hEditorWin, id, &bRet, std::is_signed<decltype(var)>::value );\
			if( !bRet )\
			{\
				auto messageString = LoadTHRotatorString(g_hModule, stringResourceId);\
				MessageBox( hEditorWin, messageString.c_str(), NULL, MB_ICONSTOP );\
				return FALSE;\
			}\
			var = static_cast<decltype(var)>(ret);\
			SetDlgItemInt( hEditorWin, id, ret, std::is_signed<decltype(var)>::value );\
		}

	DWORD pl, pt, pw, ph;
	int jthres, yo;
	GETANDSET(IDC_JUDGETHRESHOLD, jthres, IDS_INVALID_THRESHOLD);
	GETANDSET(IDC_PRLEFT, pl, IDS_INVALID_MAIN_SCREEN_LEFT);
	GETANDSET(IDC_PRTOP, pt, IDS_INVALID_MAIN_SCREEN_TOP);
	GETANDSET(IDC_PRWIDTH, pw, IDS_INVALID_MAIN_SCREEN_WIDTH);
	if (pw == 0)
	{
		auto zeroWidthMessage = LoadTHRotatorString(g_hModule, IDS_ZERO_MAIN_SCREEN_WIDTH_ERROR);
		MessageBox(hEditorWin, zeroWidthMessage.c_str(), NULL, MB_ICONSTOP);
		return FALSE;
	}
	GETANDSET(IDC_PRHEIGHT, ph, IDS_INVALID_MAIN_SCREEN_HEIGHT);
	if (ph == 0)
	{
		auto zeroHeightMessage = LoadTHRotatorString(g_hModule, IDS_ZERO_MAIN_SCREEN_HEIGHT_ERROR);
		MessageBox(hEditorWin, zeroHeightMessage.c_str(), NULL, MB_ICONSTOP);
		return FALSE;
	}
	GETANDSET(IDC_YOFFSET, yo, IDS_INVALID_MAIN_SCREEN_OFFSET);

	m_judgeThreshold = jthres;
	m_mainScreenTopLeft.x = pl;
	m_mainScreenTopLeft.y = pt;
	m_mainScreenSize.cx = pw;
	m_mainScreenSize.cy = ph;
	m_yOffset = yo;
#undef GETANDSET

	bool bVerticallyLong = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_VERTICALWINDOW, BM_GETCHECK, 0, 0);
	SetVerticallyLongWindow(bVerticallyLong);

	bool bFilterTypeNone = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_FILTERNONE, BM_GETCHECK, 0, 0);
	m_filterType = bFilterTypeNone ? D3DTEXF_NONE : D3DTEXF_LINEAR;

	if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT0_2, BM_GETCHECK, 0, 0))
	{
		m_rotationAngle = Rotation_0;
	}
	else if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT90_2, BM_GETCHECK, 0, 0))
	{
		m_rotationAngle = Rotation_90;
	}
	else if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT180_2, BM_GETCHECK, 0, 0))
	{
		m_rotationAngle = Rotation_180;
	}
	else
	{
		m_rotationAngle = Rotation_270;
	}

	m_bVisible = SendDlgItemMessage(hEditorWin, IDC_VISIBLE, BM_GETCHECK, 0, 0) == BST_CHECKED;
	m_bHUDRearrangeForced = SendDlgItemMessage(hEditorWin, IDC_FORCEVERTICAL, BM_GETCHECK, 0, 0) == BST_CHECKED;

	m_currentRectTransfers = m_editedRectTransfers;

	return true;
}

void THRotatorEditorContext::ApplyRotationToEditorWindow(HWND hWnd) const
{
	assert(hWnd != nullptr);

	UINT checkStates[Rotation_Num] =
	{
		BST_UNCHECKED, BST_UNCHECKED, BST_UNCHECKED, BST_UNCHECKED,
	};

	UINT radioButtonIDs[Rotation_Num] =
	{
		IDC_ROT0_2, IDC_ROT90_2, IDC_ROT180_2, IDC_ROT270_2,
	};

	checkStates[m_rotationAngle] = BST_CHECKED;

	for (std::uint32_t i = 0; i < Rotation_Num; i++)
	{
		SendDlgItemMessage(hWnd, radioButtonIDs[i], BM_SETCHECK, checkStates[i], 0);
	}
}

bool THRotatorEditorContext::SaveSettings() const
{
	THRotatorSetting settings;
	settings.judgeThreshold = m_judgeThreshold;
	settings.mainScreenTopLeft = m_mainScreenTopLeft;
	settings.mainScreenSize = m_mainScreenSize;
	settings.yOffset = m_yOffset;
	settings.bVisible = m_bVisible;
	settings.bVerticallyLongWindow = m_bVerticallyLongWindow;
	settings.filterType = m_filterType;
	settings.rotationAngle = m_rotationAngle;
	settings.rectTransfers = m_currentRectTransfers;
	settings.bModalEditorPreferred = m_bModalEditorPreferred;

	bool bSaveSuccess = THRotatorSetting::Save(settings);

	return bSaveSuccess;
}

bool THRotatorEditorContext::LoadSettings()
{
	THRotatorSetting setting;
	THRotatorFormatVersion formatVersion;

	bool bLoadSuccess = THRotatorSetting::Load(setting, formatVersion);

	// 失敗しても、デフォルト値で埋める

	m_judgeThreshold = setting.judgeThreshold;
	m_mainScreenTopLeft = setting.mainScreenTopLeft;
	m_mainScreenSize = setting.mainScreenSize;
	m_yOffset = setting.yOffset;
	m_bVisible = setting.bVisible;
	m_bVerticallyLongWindow = setting.bVerticallyLongWindow;
	m_rotationAngle = setting.rotationAngle;
	m_filterType = setting.filterType;
	m_bModalEditorPreferred = setting.bModalEditorPreferred;

	m_editedRectTransfers = std::move(setting.rectTransfers);

	if (!bLoadSuccess)
	{
		OutputLogMessagef(LogSeverity::Error, L"Failed to load");
		return false;
	}

	if (formatVersion == THRotatorFormatVersion::Version_1)
	{
		double touhouSeriesNumber = GetTouhouSeriesNumber();
		if (6.0 <= touhouSeriesNumber && touhouSeriesNumber < 7.5)
		{
			// format version 1で作成された.iniは、
			// 紅魔郷と妖々夢では、閾値との比較が実際のビューポート設定回数より1回少ない値と行われる条件で作成されたもの。
			// ここで値を1だけ増やして、実際のビューポート設定回数と正常に比較できるようにする。
			m_judgeThreshold++;

			OutputLogMessagef(LogSeverity::Info, L"Threshold has been converted");
		}
	}

	return true;
}

// メッセージキューの中で同一のメッセージがポストされていないかチェックし、なければ(ユニークなら)true、そうでなければfalse
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
	if (nCode >= 0 && PM_REMOVE == wParam)
	{
		// タブストップを有効にするテクニック
		// 参考: https://support.microsoft.com/en-us/help/233263/prb-modeless-dialog-box-in-a-dll-does-not-process-tab-key

		// Don't translate non-input events.
		if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
		{
			for (const auto& hWinAndContext : ms_touhouWinToContext)
			{
				auto pContext = hWinAndContext.second.lock();
				if (!pContext || pContext->m_hEditorWin == nullptr)
				{
					continue;
				}

				if (IsDialogMessage(pContext->m_hEditorWin, pMsg))
				{
					// The value returned from this hookproc is ignored, 
					// and it cannot be used to tell Windows the message has been handled.
					// To avoid further processing, convert the message to WM_NULL 
					// before returning.
					pMsg->message = WM_NULL;
					pMsg->lParam = 0;
					pMsg->wParam = 0;
				}
			}
		}
	}
	if (nCode == HC_ACTION)
	{
		auto foundItr = ms_touhouWinToContext.find(pMsg->hwnd);
		if (foundItr != ms_touhouWinToContext.end() && !foundItr->second.expired())
		{
			auto context = foundItr->second.lock();
			switch (pMsg->message)
			{
			case WM_SYSCOMMAND:
				if (pMsg->wParam == ms_switchVisibilityID
					&& IsUniquePostedMessage(pMsg))
				{
					if (context->m_bNeedModalEditor)
					{
						DialogBoxParam(g_hModule, MAKEINTRESOURCE(IDD_MAINDLG), pMsg->hwnd, MainDialogProc, (LPARAM)context.get());
					}
					else
					{
						context->SetEditorWindowVisibility(!context->m_bVisible);
					}
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

						if (!context->m_bNeedModalEditor)
						{
							context->ApplyRotationToEditorWindow();
						}

						if (!context->SaveSettings())
						{
							auto saveFailureMessage = LoadTHRotatorString(g_hModule, IDS_SETTING_FILE_SAVE_FAILED);
#ifdef _UNICODE
							context->SetNewErrorMessage(std::move(saveFailureMessage));
#else
							context->SetNewErrorMessage(ConvertFromUnicodeToSjis(saveFailureMessage));
#endif
							break;
						}
					}
					break;

				case VK_UP:
				case VK_DOWN:
					// 強制縦用レイアウト
					if ((HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT)
						&& IsUniquePostedMessage(pMsg))
					{
						context->m_bHUDRearrangeForced = !context->m_bHUDRearrangeForced;

						if (!context->m_bNeedModalEditor)
						{
							SendDlgItemMessage(context->m_hEditorWin, IDC_FORCEVERTICAL, BM_SETCHECK,
								context->m_bHUDRearrangeForced ? BST_CHECKED : BST_UNCHECKED, 0);
						}
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
	SendDlgItemMessage(m_hEditorWin, IDC_VERTICALWINDOW, BM_SETCHECK, bVerticallyLongWindow ? BST_CHECKED : BST_UNCHECKED, 0);
	m_bVerticallyLongWindow = bVerticallyLongWindow;
}

void THRotatorEditorContext::UpdateWindowResolution(int requestedWidth, int requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight)
{
	RECT rcClient, rcWindow;
	GetClientRect(m_hTouhouWin, &rcClient);
	GetWindowRect(m_hTouhouWin, &rcWindow);

	// ゲームアプリケーション側のウィンドウサイズ変更を検知、
	// これを新しいオリジナルのクライアントサイズとする

	if (rcClient.left + m_modifiedTouhouClientSize.cx != rcClient.right ||
		rcClient.top + m_modifiedTouhouClientSize.cy != rcClient.bottom)
	{
		m_originalTouhouClientSize.cx = rcClient.right - rcClient.left;
		m_originalTouhouClientSize.cy = rcClient.bottom - rcClient.top;
	}

	// 元のウィンドウの大きさを保ちつつ、アスペクト比も保持する

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

	OutputLogMessagef(LogSeverity::Info, L"Updating window client size to {0}x{1}", newWidth, newHeight);
}
