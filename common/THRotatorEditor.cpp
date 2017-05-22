// (c) 2017 massanoori

#include "stdafx.h"

#include <Windows.h>
#include <map>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "THRotatorEditor.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
LPCTSTR THROTATOR_VERSION_STRING = _T("1.2.0");
HHOOK ms_hHook;
std::map<HWND, std::weak_ptr<THRotatorEditorContext>> ms_touhouWinToContext;
UINT ms_switchVisibilityID = 12345u;

typedef std::basic_string<TCHAR> std_tstring;
}

std::basic_string<TCHAR> LoadTHRotatorString(HINSTANCE hModule, UINT nID)
{
	LPTSTR temp;
	auto bufferLength = LoadString(hModule, nID, reinterpret_cast<LPTSTR>(&temp), 0);
	if (bufferLength == 0)
	{
		return std::basic_string<TCHAR>();
	}
	else
	{
		return std::basic_string<TCHAR>(temp, bufferLength);
	}
}

THRotatorEditorContext::THRotatorEditorContext(HWND hTouhouWin)
	: m_hTouhouWin(hTouhouWin)
	, m_deviceResetRevision(0)
#ifdef TOUHOU_ON_D3D8
	, m_bNeedModalEditor(true)
#else
	, m_bNeedModalEditor(false)
#endif
	, m_modalEditorWindowPosX(0)
	, m_modalEditorWindowPosY(0)
	, m_bInitialized(false)
	, m_bScreenCaptureQueued(false)
{
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

	TCHAR fname[MAX_PATH];
	GetModuleFileName(NULL, fname, MAX_PATH);
	*_tcsrchr(fname, '.') = '\0';
	boost::filesystem::path pth(fname);

	m_appName = std::string("THRotator_") + pth.filename().generic_string();
	TCHAR path[MAX_PATH];
	size_t retSize;
	errno_t en = _tgetenv_s(&retSize, path, _T("APPDATA"));

	double touhouIndex = 0.0;
	if (pth.filename().generic_string<std_tstring>().compare(_T("東方紅魔郷")) == 0)
	{
		touhouIndex = 6.0;
	}
	else
	{
		TCHAR dummy[128];
		_stscanf_s(pth.filename().generic_string<std_tstring>().c_str(), _T("th%lf%s"), &touhouIndex, dummy, _countof(dummy));
	}

	while (touhouIndex > 90.0)
	{
		touhouIndex /= 10.0;
	}

	if (touhouIndex > 12.3 && en == 0 && retSize > 0)
	{
		m_workingDir = boost::filesystem::path(path) / _T("ShanghaiAlice") / pth.filename();
		boost::filesystem::create_directory(m_workingDir);
	}
	else
	{
		m_workingDir = boost::filesystem::current_path();
	}
	m_iniPath = m_workingDir / _T("throt.ini");

	LoadSettings();

	// スクリーンキャプチャ機能がないのは紅魔郷
	m_bTouhouWithoutScreenCapture = touhouIndex == 6.0;

	int bVerticallyLongWindow = m_bVerticallyLongWindow;
	m_bVerticallyLongWindow = 0;
	SetVerticallyLongWindow(bVerticallyLongWindow);

	m_currentRectTransfers = m_editedRectTransfers;

	//	メニューを改造
	HMENU hMenu = GetSystemMenu(m_hTouhouWin, FALSE);
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

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
	return m_judgeThreshold <= m_judgeCount;
}

RotationAngle THRotatorEditorContext::GetRotationAngle() const
{
	return m_rotationAngle;
}

DWORD THRotatorEditorContext::GetMainScreenLeft() const
{
	return m_mainScreenLeft;
}

DWORD THRotatorEditorContext::GetMainScreenWidth() const
{
	return m_mainScreenWidth;
}

DWORD THRotatorEditorContext::GetMainScreenTop() const
{
	return m_mainScreenTop;
}

DWORD THRotatorEditorContext::GetMainScreenHeight() const
{
	return m_mainScreenHeight;
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

const boost::filesystem::path & THRotatorEditorContext::GetWorkingDirectory() const
{
	return m_workingDir;
}

THRotatorEditorContext::~THRotatorEditorContext()
{
	ms_touhouWinToContext.erase(m_hTouhouWin);
}

void THRotatorEditorContext::SetEditorWindowVisibility(BOOL bVisible)
{
	assert(!m_bNeedModalEditor);

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
		SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_mainScreenLeft, FALSE);
		SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_mainScreenTop, FALSE);
		SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_mainScreenWidth, FALSE);
		SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_mainScreenHeight, FALSE);
		SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);

		{
			std_tstring versionUiString(_T("Version: "));
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

		if (pContext->m_bVerticallyLongWindow == 1)
			SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0);

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
				// TODO: エラーメッセージの表示
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
			SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_mainScreenLeft, FALSE);
			SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_mainScreenTop, FALSE);
			SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_mainScreenWidth, FALSE);
			SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_mainScreenHeight, FALSE);
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

BOOL THRotatorEditorContext::ApplyChangeFromEditorWindow(HWND hEditorWin)
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
	m_mainScreenLeft = pl;
	m_mainScreenTop = pt;
	m_mainScreenWidth = pw;
	m_mainScreenHeight = ph;
	m_yOffset = yo;
#undef GETANDSET

	BOOL bVerticallyLong = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_VERTICALWINDOW, BM_GETCHECK, 0, 0);
	SetVerticallyLongWindow(bVerticallyLong);

	BOOL bFilterTypeNone = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_FILTERNONE, BM_GETCHECK, 0, 0);
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

	m_bVisible = SendDlgItemMessage(hEditorWin, IDC_VISIBLE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;

	m_currentRectTransfers = m_editedRectTransfers;

	return TRUE;
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

bool THRotatorEditorContext::SaveSettings()
{
	namespace proptree = boost::property_tree;
	proptree::basic_ptree<std::string, std::string> tree;

#define WRITE_INI_PARAM(name, value) tree.add(m_appName + "." + name, value)
	WRITE_INI_PARAM("JC", m_judgeThreshold);
	WRITE_INI_PARAM("PL", m_mainScreenLeft);
	WRITE_INI_PARAM("PT", m_mainScreenTop);
	WRITE_INI_PARAM("PW", m_mainScreenWidth);
	WRITE_INI_PARAM("PH", m_mainScreenHeight);
	WRITE_INI_PARAM("YOffset", m_yOffset);
	WRITE_INI_PARAM("Visible", m_bVisible);
	WRITE_INI_PARAM("PivRot", m_bVerticallyLongWindow);
	WRITE_INI_PARAM("Filter", m_filterType);
	WRITE_INI_PARAM("Rot", m_rotationAngle);

	int i = 0;
	std::ostringstream ss(std::ios::ate);
	for (std::vector<RectTransferData>::iterator itr = m_currentRectTransfers.begin();
		itr != m_currentRectTransfers.cend(); ++itr, ++i)
	{
#ifdef _UNICODE
		auto bufferSize = WideCharToMultiByte(CP_ACP, 0, itr->name.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::unique_ptr<CHAR[]> nameBuffer(new CHAR[bufferSize / sizeof(CHAR)]);
		WideCharToMultiByte(CP_ACP, 0, itr->name.c_str(), -1, nameBuffer.get(), bufferSize, nullptr, nullptr);
		const auto* nameBufferPtr = nameBuffer.get();
#else
		const auto* nameBufferPtr = itr->name.c_str();
#endif
		ss.str("Name"); ss << i;
		WRITE_INI_PARAM(ss.str(), nameBufferPtr);

		ss.str("OSL"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->sourcePosition.x);

		ss.str("OST"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->sourcePosition.y);

		ss.str("OSW"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->sourceSize.cx);

		ss.str("OSH"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->sourceSize.cy);

		ss.str("ODL"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->destPosition.x);

		ss.str("ODT"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->destPosition.y);

		ss.str("ODW"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->destSize.cx);

		ss.str("ODH"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->destSize.cy);

		ss.str("OR"); ss << i;
		WRITE_INI_PARAM(ss.str(), itr->rotation);

		ss.str("ORHas"); ss << i;
		WRITE_INI_PARAM(ss.str(), TRUE);
	}

	ss.str("ORHas"); ss << i;
	WRITE_INI_PARAM(ss.str(), FALSE);
#undef WRITE_INI_PARAM

	try
	{
		proptree::write_ini(m_iniPath.generic_string(), tree);
	}
	catch (const proptree::ini_parser_error&)
	{
		return false;
	}

	return true;
}

void THRotatorEditorContext::LoadSettings()
{
	namespace proptree = boost::property_tree;

	proptree::basic_ptree<std::string, std::string> tree;
	try
	{
		proptree::read_ini(m_iniPath.generic_string(), tree);
	}
	catch (const proptree::ptree_error&)
	{
		// ファイルオープン失敗だが、boost::optional::get_value_or()でデフォルト値を設定できるので、そのまま進行
	}

#define READ_INI_PARAM(type, name, default_value) tree.get_optional<type>(m_appName + "." + name).get_value_or(default_value)
	m_judgeThreshold = READ_INI_PARAM(int, "JC", 999);
	m_mainScreenLeft = READ_INI_PARAM(int, "PL", 32);
	m_mainScreenTop = READ_INI_PARAM(int, "PT", 16);
	m_mainScreenWidth = READ_INI_PARAM(int, "PW", 384);
	m_mainScreenHeight = READ_INI_PARAM(int, "PH", 448);
	m_yOffset = READ_INI_PARAM(int, "YOffset", 0);
	m_bVisible = READ_INI_PARAM(BOOL, "Visible", FALSE);
	m_bVerticallyLongWindow = READ_INI_PARAM(BOOL, "PivRot", FALSE);
	m_rotationAngle = static_cast<RotationAngle>(READ_INI_PARAM(std::uint32_t, "Rot", Rotation_0));
	m_filterType = static_cast<D3DTEXTUREFILTERTYPE>(READ_INI_PARAM(int, "Filter", D3DTEXF_LINEAR));

	BOOL bHasNext = READ_INI_PARAM(BOOL, "ORHas0", FALSE);
	int cnt = 0;
	std::ostringstream ss(std::ios::ate);
	while (bHasNext)
	{
		RectTransferData erd;

		ss.str("Name"); ss << cnt;
		std::string rectName = READ_INI_PARAM(std::string, ss.str(), "");
#ifdef _UNICODE
		auto bufferSize = MultiByteToWideChar(CP_ACP, 0, rectName.c_str(), -1, nullptr, 0);
		std::unique_ptr<TCHAR> nameBuffer(new TCHAR[bufferSize]);
		MultiByteToWideChar(CP_ACP, 0, rectName.c_str(), -1, nameBuffer.get(), bufferSize);
		erd.name = nameBuffer.get();
#else
		erd.name = std::move(rectName);
#endif

		ss.str("OSL"); ss << cnt;
		erd.sourcePosition.x = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("OST"); ss << cnt;
		erd.sourcePosition.y = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("OSW"); ss << cnt;
		erd.sourceSize.cx = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("OSH"); ss << cnt;
		erd.sourceSize.cy = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("ODL"); ss << cnt;
		erd.destPosition.x = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("ODT"); ss << cnt;
		erd.destPosition.y = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("ODW"); ss << cnt;
		erd.destSize.cx = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("ODH"); ss << cnt;
		erd.destSize.cy = READ_INI_PARAM(int, ss.str(), 0);

		ss.str("OR"); ss << cnt;
		erd.rotation = static_cast<RotationAngle>(READ_INI_PARAM(int, ss.str(), 0));

		m_editedRectTransfers.push_back(erd);
		cnt++;

		ss.str("ORHas"); ss << cnt;
		bHasNext = READ_INI_PARAM(BOOL, ss.str(), FALSE);
	}
#undef READ_INI_PARAM
}

LRESULT CALLBACK THRotatorEditorContext::MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPMSG pMsg = reinterpret_cast<LPMSG>(lParam);
	if (nCode >= 0 && PM_REMOVE == wParam)
	{
		for (auto itr = ms_touhouWinToContext.begin();
			itr != ms_touhouWinToContext.end(); ++itr)
		{
			// Don't translate non-input events.
			if ((pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
			{
				if (!itr->second.expired() && IsDialogMessage(itr->second.lock()->m_hEditorWin, pMsg))
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
				if (pMsg->wParam == ms_switchVisibilityID)
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
					if (context->m_bTouhouWithoutScreenCapture && (HIWORD(pMsg->lParam) & KF_REPEAT) == 0)
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
					if ((HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT))
					{
						auto nextRotationAngle = static_cast<RotationAngle>((context->m_rotationAngle + 1) % 4);
						context->m_rotationAngle = nextRotationAngle;

						if (!context->m_bNeedModalEditor)
						{
							context->ApplyRotationToEditorWindow();
						}

						if (!context->SaveSettings())
						{
							// TODO: エラーメッセージの表示
							break;
						}
					}
					break;

				case VK_RIGHT:
					if (HIWORD(pMsg->lParam) & KF_ALTDOWN && !(HIWORD(pMsg->lParam) & KF_REPEAT))
					{
						auto nextRotationAngle = static_cast<RotationAngle>((context->m_rotationAngle + Rotation_Num - 1) % 4);
						context->m_rotationAngle = nextRotationAngle;

						if (!context->m_bNeedModalEditor)
						{
							context->ApplyRotationToEditorWindow();
						}

						if (!context->SaveSettings())
						{
							// TODO: エラーメッセージの表示
							break;
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

void THRotatorEditorContext::SetVerticallyLongWindow(BOOL bVerticallyLongWindow)
{
	if (m_bVerticallyLongWindow % 2 != bVerticallyLongWindow % 2/* && m_d3dpp.Windowed*/)
	{
		RECT rcClient, rcWindow;
		GetClientRect(m_hTouhouWin, &rcClient);
		GetWindowRect(m_hTouhouWin, &rcWindow);

		MoveWindow(m_hTouhouWin, rcWindow.left, rcWindow.top,
			(rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left) + (rcClient.bottom - rcClient.top),
			(rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top) + (rcClient.right - rcClient.left), TRUE);

		m_deviceResetRevision++;
	}
	SendDlgItemMessage(m_hEditorWin, IDC_VERTICALWINDOW, BM_SETCHECK, bVerticallyLongWindow ? BST_CHECKED : BST_UNCHECKED, 0);
	m_bVerticallyLongWindow = bVerticallyLongWindow;
}
