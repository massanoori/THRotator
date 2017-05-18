// (c) 2017 massanoori

#include "stdafx.h"

#include <Windows.h>
#include <map>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "THRotatorEditor.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
LPCTSTR THROTATOR_VERSION_STRING = "1.2.0";
HHOOK ms_hHook;
std::map<HWND, std::weak_ptr<THRotatorEditorContext>> ms_touhouWinToContext;
UINT ms_switchVisibilityID = 12345u;
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

	char fname[MAX_PATH];
	GetModuleFileName(NULL, fname, MAX_PATH);
	*strrchr(fname, '.') = '\0';
	boost::filesystem::path pth(fname);

	m_appName = std::string("THRotator_") + pth.filename().generic_string();
	char path[MAX_PATH];
	size_t retSize;
	errno_t en = getenv_s(&retSize, path, "APPDATA");

	double touhouIndex = 0.0;
	if (pth.filename().generic_string().compare("東方紅魔郷") == 0)
	{
		touhouIndex = 6.0;
	}
	else
	{
		char dummy[128];
		sscanf_s(pth.filename().generic_string().c_str(), "th%lf%s", &touhouIndex, dummy, (unsigned)_countof(dummy));
	}

	while (touhouIndex > 90.0)
	{
		touhouIndex /= 10.0;
	}

	if (touhouIndex > 12.3 && en == 0 && retSize > 0)
	{
		m_workingDir = boost::filesystem::path(path) / "ShanghaiAlice" / pth.filename();
		boost::filesystem::create_directory(m_workingDir);
	}
	else
	{
		m_workingDir = boost::filesystem::current_path();
	}
	m_iniPath = (m_workingDir / "throt.ini").string();

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
	AppendMenu(hMenu, MF_STRING, ms_switchVisibilityID, _T("THRotatorを開く"));

	m_hSysMenu = hMenu;

	if (m_bNeedModalEditor)
	{
		m_hEditorWin = nullptr;
	}
	else
	{
		HWND hWnd = m_hEditorWin = CreateDialogParam(reinterpret_cast<HINSTANCE>(g_hModule), MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDialogProc, (LPARAM)this);
		if (hWnd == NULL)
		{
			MessageBox(NULL, _T("ダイアログの作成に失敗"), NULL, MB_ICONSTOP);
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
	return m_RotationAngle;
}

DWORD THRotatorEditorContext::GetPlayRegionLeft() const
{
	return m_playRegionLeft;
}

DWORD THRotatorEditorContext::GetPlayRegionWidth() const
{
	return m_playRegionWidth;
}

DWORD THRotatorEditorContext::GetPlayRegionTop() const
{
	return m_playRegionTop;
}

DWORD THRotatorEditorContext::GetPlayRegionHeight() const
{
	return m_playRegionHeight;
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
	mi.dwTypeData = bVisible ? _T("TH Rotatorのウィンドウを非表示") : _T("Th Rotatorのウィンドウを表示");
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
		lvi.pszText = itr->name;
		ListView_InsertItem(hLV, &lvi);

		TCHAR str[64];
		lvi.pszText = str;

		lvi.iSubItem = 1;
		wsprintf(str, _T("%d,%d,%d,%d"), itr->rcSrc.left, itr->rcSrc.top, itr->rcSrc.right, itr->rcSrc.bottom);
		ListView_SetItem(hLV, &lvi);

		lvi.iSubItem = 2;
		wsprintf(str, _T("%d,%d,%d,%d"), itr->rcDest.left, itr->rcDest.top, itr->rcDest.right, itr->rcDest.bottom);
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
		SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_playRegionLeft, FALSE);
		SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_playRegionTop, FALSE);
		SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_playRegionWidth, FALSE);
		SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_playRegionHeight, FALSE);
		SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);

		{
			std::basic_string<TCHAR> versionUiString("Version: ");
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

		switch (pContext->m_RotationAngle)
		{
		case Rotation_0:
			SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case Rotation_90:
			SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case Rotation_180:
			SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case Rotation_270:
			SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}

		{
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = _T("矩形名");
			lvc.cx = 64;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 0, &lvc);
			lvc.pszText = _T("元の矩形");
			lvc.cx = 96;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 1, &lvc);
			lvc.pszText = _T("先の矩形");
			lvc.cx = 96;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 2, &lvc);
			lvc.pszText = _T("回転角");
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
			SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_playRegionLeft, FALSE);
			SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_playRegionTop, FALSE);
			SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_playRegionWidth, FALSE);
			SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_playRegionHeight, FALSE);
			SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);
			pContext->m_editedRectTransfers = pContext->m_currentRectTransfers;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			SendDlgItemMessage(hWnd, IDC_VISIBLE, BM_SETCHECK, pContext->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0);
			if (pContext->m_bVerticallyLongWindow == 1)
				SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0);
			else
				SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_UNCHECKED, 0);

			switch (pContext->m_RotationAngle)
			{
			case 0:
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 1:
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 2:
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 3:
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;
			}
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

			RectTransferData erd;
			ZeroMemory(&erd, sizeof(erd));
		reedit1:
			if (IDOK == DialogBoxParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), hWnd, EditRectDialogProc, reinterpret_cast<LPARAM>(&erd)))
			{
				for (std::vector<RectTransferData>::const_iterator itr = pContext->m_editedRectTransfers.cbegin(); itr != pContext->m_editedRectTransfers.cend();
					++itr)
				{
					if (lstrcmp(itr->name, erd.name) == 0)
					{
						MessageBox(hWnd, _T("この矩形名はすでに存在しています。別の矩形名前を入力してください。"), NULL, MB_ICONSTOP);
						goto reedit1;
					}
				}
				TCHAR str[64];
				pContext->m_editedRectTransfers.push_back(erd);
				LVITEM lvi;
				ZeroMemory(&lvi, sizeof(lvi));
				lvi.mask = LVIF_TEXT;
				lvi.iItem = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
				lvi.pszText = erd.name;
				ListView_InsertItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.pszText = str;

				lvi.iSubItem = 1;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcSrc.left, erd.rcSrc.top, erd.rcSrc.right, erd.rcSrc.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 2;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcDest.left, erd.rcDest.top, erd.rcDest.right, erd.rcDest.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 3;
				switch (erd.rotation)
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
			}
		}
		return TRUE;

		case IDC_EDITRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			RectTransferData& erd = pContext->m_editedRectTransfers[i];
		reedit2:
			if (IDOK == DialogBoxParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), hWnd, EditRectDialogProc, reinterpret_cast<LPARAM>(&erd)))
			{
				for (std::vector<RectTransferData>::const_iterator itr = pContext->m_editedRectTransfers.cbegin(); itr != pContext->m_editedRectTransfers.cend();
					++itr)
				{
					if (itr == pContext->m_editedRectTransfers.begin() + i)
						continue;
					if (lstrcmp(itr->name, erd.name) == 0)
					{
						MessageBox(hWnd, _T("この矩形名はすでに存在しています。別の矩形名前を入力してください。"), NULL, MB_ICONSTOP);
						goto reedit2;
					}
				}
				TCHAR str[64];
				LVITEM lvi;
				ZeroMemory(&lvi, sizeof(lvi));
				lvi.mask = LVIF_TEXT;
				lvi.iItem = i;
				lvi.pszText = erd.name;
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.pszText = str;

				lvi.iSubItem = 1;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcSrc.left, erd.rcSrc.top, erd.rcSrc.right, erd.rcSrc.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 2;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcDest.left, erd.rcDest.top, erd.rcDest.right, erd.rcDest.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 3;
				switch (erd.rotation)
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
			}
			return TRUE;
		}

		case IDC_REMRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
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
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
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
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
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

		SetDlgItemInt(hWnd, IDC_SRCLEFT, pErd->rcSrc.left, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCWIDTH, pErd->rcSrc.right, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCTOP, pErd->rcSrc.top, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCHEIGHT, pErd->rcSrc.bottom, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTLEFT, pErd->rcDest.left, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTWIDTH, pErd->rcDest.right, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTTOP, pErd->rcDest.top, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTHEIGHT, pErd->rcDest.bottom, TRUE);
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

		SetDlgItemText(hWnd, IDC_RECTNAME, pErd->name);

		return TRUE;
	}

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		{
#define GETANDSET(id, var, errmsg) {\
					BOOL bRet;UINT ret;\
					ret = GetDlgItemInt( hWnd, id, &bRet, std::is_signed<decltype(var)>::value );\
					if( !bRet )\
					{\
						MessageBox( hWnd, errmsg, NULL, MB_ICONSTOP );\
						return FALSE;\
					}\
					var = static_cast<decltype(var)>(ret);\
				}

			RECT rcSrc, rcDest;
			GETANDSET(IDC_SRCLEFT, rcSrc.left, _T("矩形転送元の左座標の入力が不正です。"));
			GETANDSET(IDC_SRCTOP, rcSrc.top, _T("矩形転送元の上座標の入力が不正です。"));
			GETANDSET(IDC_SRCWIDTH, rcSrc.right, _T("矩形転送元の幅の入力が不正です。"));
			GETANDSET(IDC_SRCHEIGHT, rcSrc.bottom, _T("矩形転送元の高さの入力が不正です。"));
			GETANDSET(IDC_DESTLEFT, rcDest.left, _T("矩形転送先の左座標の入力が不正です。"));
			GETANDSET(IDC_DESTTOP, rcDest.top, _T("矩形転送先の上座標の入力が不正です。"));
			GETANDSET(IDC_DESTWIDTH, rcDest.right, _T("矩形転送先の幅の入力が不正です。"));
			GETANDSET(IDC_DESTHEIGHT, rcDest.bottom, _T("矩形転送先の高さの入力が不正です。"));
#undef GETANDSET

			RectTransferData* pErd = reinterpret_cast<RectTransferData*>(GetWindowLongPtr(hWnd, DWLP_USER));

			if (rcSrc.right == 0 || rcSrc.bottom == 0 || rcDest.right == 0 || rcDest.bottom == 0)
			{
				MessageBox(hWnd, _T("入力された幅と高さに0が含まれています。この場合、矩形は描画されません。\r\n描画するにはあとで変更してください。"), NULL, MB_ICONEXCLAMATION);
			}

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
			pErd->rcSrc = rcSrc;
			pErd->rcDest = rcDest;
			GetDlgItemText(hWnd, IDC_RECTNAME,
				pErd->name, sizeof(((RectTransferData*)NULL)->name) / sizeof(TCHAR));

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

BOOL THRotatorEditorContext::ApplyChangeFromEditorWindow(HWND hEditorWin)
{
	assert(hEditorWin);

#define GETANDSET(id, var, errmsg) {\
			BOOL bRet;UINT ret;\
			ret = GetDlgItemInt( hEditorWin, id, &bRet, std::is_signed<decltype(var)>::value );\
			if( !bRet )\
			{\
				MessageBox( hEditorWin, errmsg, NULL, MB_ICONSTOP );\
				return FALSE;\
			}\
			var = static_cast<decltype(var)>(ret);\
			SetDlgItemInt( hEditorWin, id, ret, std::is_signed<decltype(var)>::value );\
		}

	DWORD pl, pt, pw, ph;
	int jthres, yo;
	GETANDSET(IDC_JUDGETHRESHOLD, jthres, _T("判定の回数閾値の入力が不正です。"));
	GETANDSET(IDC_PRLEFT, pl, _T("プレイ画面領域の左座標の入力が不正です。"));
	GETANDSET(IDC_PRTOP, pt, _T("プレイ画面領域の上座標の入力が不正です。"));
	GETANDSET(IDC_PRWIDTH, pw, _T("プレイ画面領域の幅の入力が不正です。"));
	if (pw == 0)
	{
		MessageBox(hEditorWin, _T("プレイ画面領域の幅は0以外の値でなければいけません。"), NULL, MB_ICONSTOP);
		return FALSE;
	}
	GETANDSET(IDC_PRHEIGHT, ph, _T("プレイ画面領域の高さの入力が不正です。"));
	if (ph == 0)
	{
		MessageBox(hEditorWin, _T("プレイ画面領域の高さは0以外の値でなければいけません。"), NULL, MB_ICONSTOP);
		return FALSE;
	}
	GETANDSET(IDC_YOFFSET, yo, _T("プレイ画面領域のオフセットの入力が不正です。"));

	m_judgeThreshold = jthres;
	m_playRegionLeft = pl;
	m_playRegionTop = pt;
	m_playRegionWidth = pw;
	m_playRegionHeight = ph;
	m_yOffset = yo;
#undef GETANDSET

	BOOL bVerticallyLong = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_VERTICALWINDOW, BM_GETCHECK, 0, 0);
	SetVerticallyLongWindow(bVerticallyLong);

	BOOL bFilterTypeNone = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_FILTERNONE, BM_GETCHECK, 0, 0);
	m_filterType = bFilterTypeNone ? D3DTEXF_NONE : D3DTEXF_LINEAR;

	if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT0_2, BM_GETCHECK, 0, 0))
	{
		m_RotationAngle = Rotation_0;
	}
	else if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT90_2, BM_GETCHECK, 0, 0))
	{
		m_RotationAngle = Rotation_90;
	}
	else if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT180_2, BM_GETCHECK, 0, 0))
	{
		m_RotationAngle = Rotation_180;
	}
	else
	{
		m_RotationAngle = Rotation_270;
	}

	m_bVisible = SendDlgItemMessage(hEditorWin, IDC_VISIBLE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;

	m_currentRectTransfers = m_editedRectTransfers;

	SaveSettings();

	return TRUE;
}

void THRotatorEditorContext::SaveSettings()
{
	namespace proptree = boost::property_tree;
	proptree::ptree tree;

#define WRITE_INI_PARAM(name, value) tree.add(m_appName + "." + name, value)
	WRITE_INI_PARAM("JC", m_judgeThreshold);
	WRITE_INI_PARAM("PL", m_playRegionLeft);
	WRITE_INI_PARAM("PT", m_playRegionTop);
	WRITE_INI_PARAM("PW", m_playRegionWidth);
	WRITE_INI_PARAM("PH", m_playRegionHeight);
	WRITE_INI_PARAM("YOffset", m_yOffset);
	WRITE_INI_PARAM("Visible", m_bVisible);
	WRITE_INI_PARAM("PivRot", m_bVerticallyLongWindow);
	WRITE_INI_PARAM("Filter", m_filterType);
	WRITE_INI_PARAM("Rot", m_RotationAngle);

	int i = 0;
	TCHAR name[64];
	for (std::vector<RectTransferData>::iterator itr = m_currentRectTransfers.begin();
		itr != m_currentRectTransfers.cend(); ++itr, ++i)
	{
		wsprintf(name, _T("Name%d"), i); WRITE_INI_PARAM(name, itr->name);
		wsprintf(name, _T("OSL%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.left);
		wsprintf(name, _T("OST%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.top);
		wsprintf(name, _T("OSW%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.right);
		wsprintf(name, _T("OSH%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.bottom);
		wsprintf(name, _T("ODL%d"), i); WRITE_INI_PARAM(name, itr->rcDest.left);
		wsprintf(name, _T("ODT%d"), i); WRITE_INI_PARAM(name, itr->rcDest.top);
		wsprintf(name, _T("ODW%d"), i); WRITE_INI_PARAM(name, itr->rcDest.right);
		wsprintf(name, _T("ODH%d"), i); WRITE_INI_PARAM(name, itr->rcDest.bottom);
		wsprintf(name, _T("OR%d"), i); WRITE_INI_PARAM(name, itr->rotation);
		wsprintf(name, _T("ORHas%d"), i); WRITE_INI_PARAM(name, TRUE);
	}
	wsprintf(name, _T("ORHas%d"), i); WRITE_INI_PARAM(name, FALSE);
#undef WRITE_INI_PARAM

	proptree::write_ini(m_iniPath, tree);
}

void THRotatorEditorContext::LoadSettings()
{
	namespace proptree = boost::property_tree;

	proptree::ptree tree;
	try
	{
		proptree::read_ini(m_iniPath, tree);
	}
	catch (const proptree::ptree_error&)
	{
		// ファイルオープン失敗だが、boost::optional::get_value_or()でデフォルト値を設定できるので、そのまま進行
	}

#define READ_INI_PARAM(type, name, default_value) tree.get_optional<type>(m_appName + "." + name).get_value_or(default_value)
	m_judgeThreshold = READ_INI_PARAM(int, "JC", 999);
	m_playRegionLeft = READ_INI_PARAM(int, "PL", 32);
	m_playRegionTop = READ_INI_PARAM(int, "PT", 16);
	m_playRegionWidth = READ_INI_PARAM(int, "PW", 384);
	m_playRegionHeight = READ_INI_PARAM(int, "PH", 448);
	m_yOffset = READ_INI_PARAM(int, "YOffset", 0);
	m_bVisible = READ_INI_PARAM(BOOL, "Visible", FALSE);
	m_bVerticallyLongWindow = READ_INI_PARAM(BOOL, "PivRot", FALSE);
	m_RotationAngle = static_cast<RotationAngle>(READ_INI_PARAM(std::uint32_t, "Rot", Rotation_0));
	m_filterType = static_cast<D3DTEXTUREFILTERTYPE>(READ_INI_PARAM(int, "Filter", D3DTEXF_LINEAR));

	BOOL bHasNext = READ_INI_PARAM(BOOL, "ORHas0", FALSE);
	int cnt = 0;
	while (bHasNext)
	{
		RectTransferData erd;
		TCHAR name[64];

		wsprintf(name, _T("Name%d"), cnt);
		std::basic_string<TCHAR> rectName = READ_INI_PARAM(std::basic_string<TCHAR>, name, "");
		strcpy_s(erd.name, rectName.c_str());
		erd.name[rectName.length()] = '\0';

		wsprintf(name, _T("OSL%d"), cnt);
		erd.rcSrc.left = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("OST%d"), cnt);
		erd.rcSrc.top = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("OSW%d"), cnt);
		erd.rcSrc.right = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("OSH%d"), cnt);
		erd.rcSrc.bottom = READ_INI_PARAM(int, name, 0);

		wsprintf(name, _T("ODL%d"), cnt);
		erd.rcDest.left = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("ODT%d"), cnt);
		erd.rcDest.top = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("ODW%d"), cnt);
		erd.rcDest.right = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("ODH%d"), cnt);
		erd.rcDest.bottom = READ_INI_PARAM(int, name, 0);

		wsprintf(name, _T("OR%d"), cnt);
		erd.rotation = static_cast<RotationAngle>(READ_INI_PARAM(int, name, 0));

		m_editedRectTransfers.push_back(erd);
		cnt++;

		wsprintf(name, _T("ORHas%d"), cnt);
		bHasNext = READ_INI_PARAM(BOOL, name, FALSE);
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
						DialogBoxParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_MAINDLG), pMsg->hwnd, MainDialogProc, (LPARAM)context.get());
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
						auto nextRotationAngle = static_cast<RotationAngle>((context->m_RotationAngle + 1) % 4);

						if (context->m_bNeedModalEditor)
						{
							context->m_RotationAngle = nextRotationAngle;
							context->SaveSettings();
							break;
						}

						switch ((context->m_RotationAngle + 1) % 4)
						{
						case Rotation_0:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( pDev->m_d3dpp.Windowed ) pDev->SetVerticallyLongWindow( 0 );
							break;

						case 1:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;

						case 2:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 0 );
							break;

						case 3:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;
						}

						context->ApplyChangeFromEditorWindow();
					}
					break;

				case VK_RIGHT:
					if (HIWORD(pMsg->lParam) & KF_ALTDOWN && !(HIWORD(pMsg->lParam) & KF_REPEAT))
					{
						auto nextRotationAngle = static_cast<RotationAngle>((context->m_RotationAngle + Rotation_Num - 1) % 4);

						if (context->m_bNeedModalEditor)
						{
							context->m_RotationAngle = nextRotationAngle;
							context->SaveSettings();
							break;
						}

						switch (nextRotationAngle)
						{
						case 0:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 0 );
							break;

						case 1:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;

						case 2:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 0 );
							break;

						case 3:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;
						}

						context->ApplyChangeFromEditorWindow();
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
