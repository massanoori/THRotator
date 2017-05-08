#include <windows.h>
#include <CommCtrl.h>
#include <d3d9.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <d3dx9.h>
#include <boost/filesystem.hpp>
#include "resource.h"
#include <tchar.h>
#include <ShlObj.h>

int FullScreenWidth;
int FullScreenHeight;

//#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"comctl32.lib")

HANDLE g_hModule;

typedef IDirect3D9* (WINAPI * DIRECT3DCREATE9PROC)(UINT);

FARPROC p_Direct3DShaderValidatorCreate9;
FARPROC p_PSGPError;
FARPROC p_PSGPSampleTexture;
FARPROC p_D3DPERF_BeginEvent;
FARPROC p_D3DPERF_EndEvent;
FARPROC p_D3DPERF_GetStatus;
FARPROC p_D3DPERF_QueryRepeatFrame;
FARPROC p_D3DPERF_SetMarker;
FARPROC p_D3DPERF_SetOptions;
FARPROC p_D3DPERF_SetRegion;
FARPROC p_DebugSetLevel;
FARPROC p_DebugSetMute;
DIRECT3DCREATE9PROC p_Direct3DCreate9;
FARPROC p_Direct3DCreate9Ex;

class MyDirect3DDevice9;

class MyDirect3D9 : public IDirect3D9
{
	friend class MyDirect3DDevice9;
	LPDIRECT3D9 m_pd3d;
	std::map<LPDIRECT3DDEVICE9,MyDirect3DDevice9> m_devices;
	
public:
	bool init( UINT v )
	{
		//m_pd3d = Direct3DCreate9( D3D_SDK_VERSION );
		m_pd3d = p_Direct3DCreate9( v );
		
		return m_pd3d != NULL;
	}

	ULONG WINAPI AddRef(VOID)
	{
		return m_pd3d->AddRef();
	}

	HRESULT WINAPI QueryInterface( REFIID riid, LPVOID* ppvObj )
	{
		return m_pd3d->QueryInterface( riid, ppvObj );
	}

	ULONG WINAPI Release()
	{
		ULONG ret = m_pd3d->Release();

		return ret;
	}

	HRESULT WINAPI CheckDepthStencilMatch( UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT AdapterFormat,
		D3DFORMAT RenderTargetFormat,
		D3DFORMAT DepthStencilFormat )
	{
		return m_pd3d->CheckDepthStencilMatch( Adapter,
			DeviceType,
			AdapterFormat,
			RenderTargetFormat,
			DepthStencilFormat );
	}

	HRESULT WINAPI CheckDeviceFormat(          UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT AdapterFormat,
		DWORD Usage,
		D3DRESOURCETYPE RType,
		D3DFORMAT CheckFormat )
	{
		return m_pd3d->CheckDeviceFormat( Adapter,
			DeviceType,
			AdapterFormat,
			Usage,
			RType,
			CheckFormat );
	}

	HRESULT WINAPI CheckDeviceFormatConversion(          UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT SourceFormat,
		D3DFORMAT TargetFormat )
	{
		return m_pd3d->CheckDeviceFormatConversion( Adapter,
			DeviceType,
			SourceFormat,
			TargetFormat );
	}

	HRESULT WINAPI CheckDeviceMultiSampleType(          UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT SurfaceFormat,
		BOOL Windowed,
		D3DMULTISAMPLE_TYPE MultiSampleType,
		DWORD* pQualityLevels )
	{
		return m_pd3d->CheckDeviceMultiSampleType( Adapter,
			DeviceType,
			SurfaceFormat,
			Windowed,
			MultiSampleType,
			pQualityLevels );
	}
	
	HRESULT WINAPI CheckDeviceType(          UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT DisplayFormat,
		D3DFORMAT BackBufferFormat,
		BOOL Windowed )
	{
		return m_pd3d->CheckDeviceType( Adapter,
			DeviceType,
			DisplayFormat,
			BackBufferFormat,
			Windowed );
	}

	HRESULT WINAPI CreateDevice(          UINT Adapter,
		D3DDEVTYPE DeviceType,
		HWND hFocusWindow,
		DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS *pPresentationParameters,
		IDirect3DDevice9** ppReturnedDeviceInterface );

	HRESULT WINAPI EnumAdapterModes(          UINT Adapter,
		D3DFORMAT Format,
		UINT Mode,
		D3DDISPLAYMODE* pMode )
	{
		return m_pd3d->EnumAdapterModes( Adapter,
			Format,
			Mode,
			pMode );
	}

	UINT WINAPI GetAdapterCount(VOID)
	{
		return m_pd3d->GetAdapterCount();
	}

	HRESULT WINAPI GetAdapterDisplayMode(          UINT Adapter,
		D3DDISPLAYMODE *pMode )
	{
		return m_pd3d->GetAdapterDisplayMode( Adapter, pMode );
	}

	HRESULT WINAPI GetAdapterIdentifier(          UINT Adapter,
		DWORD Flags,
		D3DADAPTER_IDENTIFIER9 *pIdentifier )
	{
		return m_pd3d->GetAdapterIdentifier( Adapter,
			Flags,
			pIdentifier );
	}
	
	UINT WINAPI GetAdapterModeCount(          UINT Adapter,
		D3DFORMAT Format )
	{
		return m_pd3d->GetAdapterModeCount( Adapter, Format );
	}

	HMONITOR WINAPI GetAdapterMonitor(          UINT Adapter )
	{
		return m_pd3d->GetAdapterMonitor( Adapter );
	}

	HRESULT WINAPI GetDeviceCaps(          UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DCAPS9 *pCaps )
	{
		return m_pd3d->GetDeviceCaps( Adapter, DeviceType, pCaps );
	}

	HRESULT WINAPI RegisterSoftwareDevice(          void *pInitializeFunction )
	{
		return m_pd3d->RegisterSoftwareDevice( pInitializeFunction );
	}
};

static MyDirect3D9 g_d3d;

struct EditRectData
{
	RECT rcSrc, rcDest;
	int rotation;
	TCHAR name[64];
};

class MyDirect3DDevice9 : public IDirect3DDevice9
{
	friend class MyDirect3D9;
	LPDIRECT3DDEVICE9 m_pd3dDev;
	LPD3DXSPRITE m_pSprite;
	LPDIRECT3DSURFACE9 m_pBackBuffer, m_pTexSurface, m_pRenderTarget;
	D3DPRESENT_PARAMETERS m_d3dpp;
	D3DTEXTUREFILTERTYPE m_filterType;
#ifdef _DEBUG
	LARGE_INTEGER m_freq, m_cur, m_prev;
	LPD3DXFONT m_pFont;
	float m_FPSNew, m_FPS;
	int m_fpsCount;
#endif

	DWORD m_prLeft, m_prTop, m_prWidth, m_prHeight;
	int m_yOffset;
	int m_judgeThreshold;
	int m_judgeCount, m_judgeCountPrev;
	int m_orCount;
	BOOL m_bVisible;
	std::string m_iniPath;
	std::string m_appName;
	HMENU m_hSysMenu;
	HWND m_hDlg, m_hTH;
	int m_pivRot;
	int m_rot;

	static int ms_refCnt;
	static int ms_visID;
	
	static std::map<HWND,MyDirect3DDevice9*> hwnd2devMap;
	static std::map<HWND,HWND> th_hwnd2hwndMap;

	std::vector<EditRectData> m_erds, m_erdsPrev;

	void SetVisibility( BOOL vis )
	{
		m_bVisible = vis;
		MENUITEMINFO mi;
		ZeroMemory( &mi, sizeof(mi) );

		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_STRING;
		mi.dwTypeData = vis ? _T("TH Rotatorのウィンドウを非表示") : _T("Th Rotatorのウィンドウを表示");
		SetMenuItemInfo( m_hSysMenu, ms_visID, FALSE, &mi );

		ShowWindow( m_hDlg, vis ? SW_SHOW : SW_HIDE );
	}

	void InitListView( HWND hLV )
	{
		ListView_DeleteAllItems( hLV );
		int i = 0;
		for( std::vector<EditRectData>::iterator itr = m_erds.begin();
			itr != m_erds.end(); ++itr, ++i )
		{
			LVITEM lvi;
			ZeroMemory( &lvi, sizeof(lvi) );

			lvi.mask = LVIF_TEXT;
			lvi.iItem = i;
			lvi.pszText = itr->name;
			ListView_InsertItem( hLV, &lvi );

			TCHAR str[64];
			lvi.pszText = str;

			lvi.iSubItem = 1;
			wsprintf( str, _T("%d,%d,%d,%d"), itr->rcSrc.left, itr->rcSrc.top, itr->rcSrc.right, itr->rcSrc.bottom );
			ListView_SetItem( hLV, &lvi );

			lvi.iSubItem = 2;
			wsprintf( str, _T("%d,%d,%d,%d"), itr->rcDest.left, itr->rcDest.top, itr->rcDest.right, itr->rcDest.bottom );
			ListView_SetItem( hLV, &lvi );

			lvi.iSubItem = 3;
			wsprintf( str, _T("%d°"), itr->rotation*90 );
			ListView_SetItem( hLV, &lvi );
		}
	}

	static BOOL CALLBACK DlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			hwnd2devMap[hWnd] = (MyDirect3DDevice9*)lParam;
			SetDlgItemInt( hWnd, IDC_JUDGETHRESHOLD, hwnd2devMap[hWnd]->m_judgeThreshold, TRUE );
			SetDlgItemInt( hWnd, IDC_PRLEFT, hwnd2devMap[hWnd]->m_prLeft, FALSE );
			SetDlgItemInt( hWnd, IDC_PRTOP, hwnd2devMap[hWnd]->m_prTop, FALSE );
			SetDlgItemInt( hWnd, IDC_PRWIDTH, hwnd2devMap[hWnd]->m_prWidth, FALSE );
			SetDlgItemInt( hWnd, IDC_PRHEIGHT, hwnd2devMap[hWnd]->m_prHeight, FALSE );
			SetDlgItemInt( hWnd, IDC_YOFFSET, hwnd2devMap[hWnd]->m_yOffset, TRUE );
			SendDlgItemMessage( hWnd, IDC_VISIBLE, BM_SETCHECK, hwnd2devMap[hWnd]->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0 );
			switch( ((MyDirect3DDevice9*)lParam)->m_filterType )
			{
			case D3DTEXF_NONE:
				SendDlgItemMessage( hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_CHECKED, 0 );
				break;

			case D3DTEXF_LINEAR:
				SendDlgItemMessage( hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_CHECKED, 0 );
				break;
			}
			/*switch( hwnd2devMap[hWnd]->m_pivRot )
			{
			case 0:
				SendDlgItemMessage( hWnd, IDC_PIV0, BM_SETCHECK, BST_CHECKED, 0 );
				break;

			case 1:
				SendDlgItemMessage( hWnd, IDC_PIV90, BM_SETCHECK, BST_CHECKED, 0 );
				break;

			case 3:
				SendDlgItemMessage( hWnd, IDC_PIV270, BM_SETCHECK, BST_CHECKED, 0 );
				break;
			}*/

			if( hwnd2devMap[hWnd]->m_pivRot == 1 )
				SendDlgItemMessage( hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0 );

			switch( hwnd2devMap[hWnd]->m_rot )
			{
			case 0:
				SendDlgItemMessage( hWnd, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0 );
				break;

			case 1:
				SendDlgItemMessage( hWnd, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0 );
				break;

			case 2:
				SendDlgItemMessage( hWnd, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0 );
				break;

			case 3:
				SendDlgItemMessage( hWnd, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0 );
				break;
			}

			{
				LVCOLUMN lvc;
				lvc.mask = LVCF_TEXT | LVCF_WIDTH;
				lvc.pszText = _T("矩形名");
				lvc.cx = 64;
				ListView_InsertColumn( GetDlgItem( hWnd, IDC_ORLIST ), 0, &lvc );
				lvc.pszText = _T("元の矩形");
				lvc.cx = 96;
				ListView_InsertColumn( GetDlgItem( hWnd, IDC_ORLIST ), 1, &lvc );
				lvc.pszText = _T("先の矩形");
				lvc.cx = 96;
				ListView_InsertColumn( GetDlgItem( hWnd, IDC_ORLIST ), 2, &lvc );
				lvc.pszText = _T("回転角");
				lvc.cx = 48;
				ListView_InsertColumn( GetDlgItem( hWnd, IDC_ORLIST ), 3, &lvc );
			}

			hwnd2devMap[hWnd]->InitListView( GetDlgItem( hWnd, IDC_ORLIST ) );

			return TRUE;

		case WM_CLOSE:
			return FALSE;

		case WM_COMMAND:
			switch(wParam)
			{
			case IDC_GETVPCOUNT:
				SetDlgItemInt( hWnd, IDC_VPCOUNT, hwnd2devMap[hWnd]->m_judgeCountPrev, FALSE );
				return TRUE;
			case IDOK:
				return hwnd2devMap[hWnd]->ApplyChange();

			case IDC_RESET:
				SetDlgItemInt( hWnd, IDC_JUDGETHRESHOLD, hwnd2devMap[hWnd]->m_judgeThreshold, FALSE );
				SetDlgItemInt( hWnd, IDC_PRLEFT, hwnd2devMap[hWnd]->m_prLeft, FALSE );
				SetDlgItemInt( hWnd, IDC_PRTOP, hwnd2devMap[hWnd]->m_prTop, FALSE );
				SetDlgItemInt( hWnd, IDC_PRWIDTH, hwnd2devMap[hWnd]->m_prWidth, FALSE );
				SetDlgItemInt( hWnd, IDC_PRHEIGHT, hwnd2devMap[hWnd]->m_prHeight, FALSE );
				SetDlgItemInt( hWnd, IDC_YOFFSET, hwnd2devMap[hWnd]->m_yOffset, TRUE );
				hwnd2devMap[hWnd]->m_erds = hwnd2devMap[hWnd]->m_erdsPrev;
				hwnd2devMap[hWnd]->InitListView( GetDlgItem( hWnd, IDC_ORLIST ) );
				SendDlgItemMessage( hWnd, IDC_VISIBLE, BM_SETCHECK, hwnd2devMap[hWnd]->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0 );
				if( hwnd2devMap[hWnd]->m_pivRot == 1 )
					SendDlgItemMessage( hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0 );
				else
					SendDlgItemMessage( hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_UNCHECKED, 0 );

				switch( hwnd2devMap[hWnd]->m_rot )
				{
				case 0:
					SendDlgItemMessage( hWnd, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					break;

				case 1:
					SendDlgItemMessage( hWnd, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					break;

				case 2:
					SendDlgItemMessage( hWnd, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					break;

				case 3:
					SendDlgItemMessage( hWnd, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
					break;
				}
				switch( hwnd2devMap[hWnd]->m_filterType )
				{
				case D3DTEXF_NONE:
					SendDlgItemMessage( hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_CHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_UNCHECKED, 0 );
					break;

				case D3DTEXF_LINEAR:
					SendDlgItemMessage( hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_CHECKED, 0 );
					SendDlgItemMessage( hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_UNCHECKED, 0 );
					break;
				}
				return TRUE;

			case IDCANCEL:
				hwnd2devMap[hWnd]->SetVisibility( FALSE );
				return TRUE;

			case IDC_ADDRECT:
				{
					EditRectData erd;
					ZeroMemory( &erd, sizeof(erd) );
					reedit1:
					if( IDOK == DialogBoxParam( (HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), hWnd, EditRectProc, reinterpret_cast<LPARAM>(&erd) ) )
					{
						for( std::vector<EditRectData>::const_iterator itr = hwnd2devMap[hWnd]->m_erds.cbegin(); itr != hwnd2devMap[hWnd]->m_erds.cend();
							++itr )
						{
							if( lstrcmp( itr->name, erd.name ) == 0 )
							{
								MessageBox( hWnd, _T("この矩形名はすでに存在しています。別の矩形名前を入力してください。"), NULL, MB_ICONSTOP );
								goto reedit1;
							}
						}
						TCHAR str[64];
						hwnd2devMap[hWnd]->m_erds.push_back( erd );
						LVITEM lvi;
						ZeroMemory( &lvi, sizeof(lvi) );
						lvi.mask = LVIF_TEXT;
						lvi.iItem = ListView_GetItemCount( GetDlgItem( hWnd, IDC_ORLIST ) );
						lvi.pszText = erd.name;
						ListView_InsertItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );

						lvi.pszText = str;

						lvi.iSubItem = 1;
						wsprintf( str, _T("%d,%d,%d,%d"), erd.rcSrc.left, erd.rcSrc.top, erd.rcSrc.right, erd.rcSrc.bottom );
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );

						lvi.iSubItem = 2;
						wsprintf( str, _T("%d,%d,%d,%d"), erd.rcDest.left, erd.rcDest.top, erd.rcDest.right, erd.rcDest.bottom );
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );

						lvi.iSubItem = 3;
						switch( erd.rotation )
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
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );
					}
				}
				return TRUE;

			case IDC_EDITRECT:
				{
					int i = ListView_GetSelectionMark( GetDlgItem( hWnd, IDC_ORLIST ) );
					if( i == -1 )
					{
						MessageBox( hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION );
						return FALSE;
					}
					EditRectData& erd = hwnd2devMap[hWnd]->m_erds[i];
					reedit2:
					if( IDOK == DialogBoxParam( (HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), hWnd, EditRectProc, reinterpret_cast<LPARAM>(&erd) ) )
					{
						for( std::vector<EditRectData>::const_iterator itr = hwnd2devMap[hWnd]->m_erds.cbegin(); itr != hwnd2devMap[hWnd]->m_erds.cend();
							++itr )
						{
							if( itr == hwnd2devMap[hWnd]->m_erds.begin()+i )
								continue;
							if( lstrcmp( itr->name, erd.name ) == 0 )
							{
								MessageBox( hWnd, _T("この矩形名はすでに存在しています。別の矩形名前を入力してください。"), NULL, MB_ICONSTOP );
								goto reedit2;
							}
						}
						TCHAR str[64];
						LVITEM lvi;
						ZeroMemory( &lvi, sizeof(lvi) );
						lvi.mask = LVIF_TEXT;
						lvi.iItem = i;
						lvi.pszText = erd.name;
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );

						lvi.pszText = str;

						lvi.iSubItem = 1;
						wsprintf( str, _T("%d,%d,%d,%d"), erd.rcSrc.left, erd.rcSrc.top, erd.rcSrc.right, erd.rcSrc.bottom );
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );

						lvi.iSubItem = 2;
						wsprintf( str, _T("%d,%d,%d,%d"), erd.rcDest.left, erd.rcDest.top, erd.rcDest.right, erd.rcDest.bottom );
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );

						lvi.iSubItem = 3;
						switch( erd.rotation )
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
						ListView_SetItem( GetDlgItem( hWnd, IDC_ORLIST ), &lvi );
					}
				}
				return TRUE;

			case IDC_REMRECT:
				{
					int i = ListView_GetSelectionMark( GetDlgItem( hWnd, IDC_ORLIST ) );
					if( i == -1 )
					{
						MessageBox( hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION );
						return FALSE;
					}
					hwnd2devMap[hWnd]->m_erds.erase( hwnd2devMap[hWnd]->m_erds.cbegin() + i );
					ListView_DeleteItem( GetDlgItem( hWnd, IDC_ORLIST ), i );
					if( ListView_GetItemCount( GetDlgItem( hWnd, IDC_ORLIST ) ) == 0 )
					{
						EnableWindow( GetDlgItem( hWnd, IDC_EDITRECT ), FALSE );
						EnableWindow( GetDlgItem( hWnd, IDC_REMRECT ), FALSE );
						EnableWindow( GetDlgItem( hWnd, IDC_UP ), FALSE );
						EnableWindow( GetDlgItem( hWnd, IDC_DOWN ), FALSE );
					}
				}
				return TRUE;

			case IDC_UP:
				{
					int i = ListView_GetSelectionMark( GetDlgItem( hWnd, IDC_ORLIST ) );
					if( i == -1 )
					{
						MessageBox( hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION );
						return FALSE;
					}
					if( i == 0 )
					{
						MessageBox( hWnd, _T("選択されている矩形が一番上のものです。"), NULL, MB_ICONEXCLAMATION );
						return FALSE;
					}
					EditRectData erd = hwnd2devMap[hWnd]->m_erds[i];
					hwnd2devMap[hWnd]->m_erds[i] = hwnd2devMap[hWnd]->m_erds[i-1];
					hwnd2devMap[hWnd]->m_erds[i-1] = erd;
					hwnd2devMap[hWnd]->InitListView( GetDlgItem( hWnd, IDC_ORLIST ) );
					ListView_SetSelectionMark( GetDlgItem( hWnd, IDC_ORLIST ), i-1 );
					EnableWindow( GetDlgItem( hWnd, IDC_DOWN ), TRUE );
					if( i == 1 )
						EnableWindow( GetDlgItem( hWnd, IDC_UP ), FALSE );
				}
				return TRUE;

			case IDC_DOWN:
				{
					int i = ListView_GetSelectionMark( GetDlgItem( hWnd, IDC_ORLIST ) );
					int count = ListView_GetItemCount( GetDlgItem( hWnd, IDC_ORLIST ) );
					if( i == -1 )
					{
						MessageBox( hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION );
						return FALSE;
					}
					if( i == count-1 )
					{
						MessageBox( hWnd, _T("選択されている矩形が一番下のものです。"), NULL, MB_ICONEXCLAMATION );
						return FALSE;
					}
					EditRectData erd = hwnd2devMap[hWnd]->m_erds[i];
					hwnd2devMap[hWnd]->m_erds[i] = hwnd2devMap[hWnd]->m_erds[i+1];
					hwnd2devMap[hWnd]->m_erds[i+1] = erd;
					hwnd2devMap[hWnd]->InitListView( GetDlgItem( hWnd, IDC_ORLIST ) );
					ListView_SetSelectionMark( GetDlgItem( hWnd, IDC_ORLIST ), i+1 );
					EnableWindow( GetDlgItem( hWnd, IDC_UP ), TRUE );
					if( i == count-2 )
						EnableWindow( GetDlgItem( hWnd, IDC_DOWN ), FALSE );
				}
				return TRUE;
			}
			return FALSE;

		case WM_NOTIFY:
			{
				LPNMHDR lpnm = (LPNMHDR)lParam;
				if( lpnm->code == LVN_ITEMCHANGED )
				{
					LPNMLISTVIEW lpnmLV = (LPNMLISTVIEW)lParam;
					int count = ListView_GetItemCount( GetDlgItem( hWnd, IDC_ORLIST ) );
					BOOL bEnabled = count>0;
					EnableWindow( GetDlgItem( hWnd, IDC_EDITRECT ), bEnabled );
					EnableWindow( GetDlgItem( hWnd, IDC_REMRECT ), bEnabled );
					EnableWindow( GetDlgItem( hWnd, IDC_UP ), bEnabled && lpnmLV->iItem > 0 );
					EnableWindow( GetDlgItem( hWnd, IDC_DOWN ), bEnabled && lpnmLV->iItem < count-1 );
				}
			}
			return FALSE;
		}

		return FALSE;
	}

	static BOOL CALLBACK EditRectProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		static std::map<HWND,EditRectData*> hwnd2erdMap;
		switch( msg )
		{
		case WM_INITDIALOG:
			{
				EditRectData* pErd = reinterpret_cast<EditRectData*>(lParam);
				hwnd2erdMap[hWnd] = pErd;
				SetDlgItemInt( hWnd, IDC_SRCLEFT, pErd->rcSrc.left, TRUE );
				SetDlgItemInt( hWnd, IDC_SRCWIDTH, pErd->rcSrc.right, TRUE );
				SetDlgItemInt( hWnd, IDC_SRCTOP, pErd->rcSrc.top, TRUE );
				SetDlgItemInt( hWnd, IDC_SRCHEIGHT, pErd->rcSrc.bottom, TRUE );
				SetDlgItemInt( hWnd, IDC_DESTLEFT, pErd->rcDest.left, TRUE );
				SetDlgItemInt( hWnd, IDC_DESTWIDTH, pErd->rcDest.right, TRUE );
				SetDlgItemInt( hWnd, IDC_DESTTOP, pErd->rcDest.top, TRUE );
				SetDlgItemInt( hWnd, IDC_DESTHEIGHT, pErd->rcDest.bottom, TRUE );
				switch( pErd->rotation )
				{
				case 0:
					SendDlgItemMessage( hWnd, IDC_ROT0, BM_SETCHECK, BST_CHECKED, 0 );
					break;

				case 1:
					SendDlgItemMessage( hWnd, IDC_ROT90, BM_SETCHECK, BST_CHECKED, 0 );
					break;

				case 2:
					SendDlgItemMessage( hWnd, IDC_ROT180, BM_SETCHECK, BST_CHECKED, 0 );
					break;

				case 3:
					SendDlgItemMessage( hWnd, IDC_ROT270, BM_SETCHECK, BST_CHECKED, 0 );
					break;
				}

				SetDlgItemText( hWnd, IDC_RECTNAME, pErd->name );
			}
			return TRUE;

		case WM_COMMAND:
			switch( wParam )
			{
			case IDOK:
				#define GETANDSET_E(id,var,bSig,errmsg) {\
					BOOL bRet;UINT ret;\
					ret = GetDlgItemInt( hWnd, id, &bRet, bSig );\
					if( !bRet )\
					{\
						MessageBox( hWnd, errmsg, NULL, MB_ICONSTOP );\
						return FALSE;\
					}\
					if( bSig )\
						var=(signed)ret;\
					else\
						var=ret;\
				}

				{
					RECT rcSrc, rcDest;
					GETANDSET_E( IDC_SRCLEFT, rcSrc.left, TRUE, _T("矩形転送元の左座標の入力が不正です。") );
					GETANDSET_E( IDC_SRCTOP, rcSrc.top, TRUE, _T("矩形転送元の上座標の入力が不正です。") );
					GETANDSET_E( IDC_SRCWIDTH, rcSrc.right, TRUE, _T("矩形転送元の幅の入力が不正です。") );
					GETANDSET_E( IDC_SRCHEIGHT, rcSrc.bottom, TRUE, _T("矩形転送元の高さの入力が不正です。") );
					GETANDSET_E( IDC_DESTLEFT, rcDest.left, TRUE, _T("矩形転送先の左座標の入力が不正です。") );
					GETANDSET_E( IDC_DESTTOP, rcDest.top, TRUE, _T("矩形転送先の上座標の入力が不正です。") );
					GETANDSET_E( IDC_DESTWIDTH, rcDest.right, TRUE, _T("矩形転送先の幅の入力が不正です。") );
					GETANDSET_E( IDC_DESTHEIGHT, rcDest.bottom, TRUE, _T("矩形転送先の高さの入力が不正です。") );

					if( rcSrc.right == 0 || rcSrc.bottom == 0 || rcDest.right == 0 || rcDest.bottom == 0 )
						MessageBox( hWnd, _T("入力された幅と高さに0が含まれています。この場合、矩形は描画されません。\r\n描画するにはあとで変更してください。"), NULL, MB_ICONEXCLAMATION );

					if( SendDlgItemMessage( hWnd, IDC_ROT0, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						hwnd2erdMap[hWnd]->rotation = 0;
					else if( SendDlgItemMessage( hWnd, IDC_ROT90, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						hwnd2erdMap[hWnd]->rotation = 1;
					else if( SendDlgItemMessage( hWnd, IDC_ROT180, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						hwnd2erdMap[hWnd]->rotation = 2;
					else if( SendDlgItemMessage( hWnd, IDC_ROT270, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						hwnd2erdMap[hWnd]->rotation = 3;
					hwnd2erdMap[hWnd]->rcSrc = rcSrc;
					hwnd2erdMap[hWnd]->rcDest = rcDest;
					GetDlgItemText( hWnd, IDC_RECTNAME,
						hwnd2erdMap[hWnd]->name, sizeof(((EditRectData*)NULL)->name)/sizeof(TCHAR) );
				}
				hwnd2erdMap.erase( hWnd );
				EndDialog( hWnd, IDOK );
				return TRUE;

			case IDCANCEL:
				hwnd2erdMap.erase( hWnd );
				EndDialog( hWnd, IDCANCEL );
				return TRUE;
			}
			return FALSE;
		}
		return FALSE;
	}

	BOOL ApplyChange()
	{
#define GETANDSET(id,var,bSig,errmsg) {\
			BOOL bRet;UINT ret;\
			ret = GetDlgItemInt( m_hDlg, id, &bRet, bSig );\
			if( !bRet )\
			{\
				MessageBox( m_hDlg, errmsg, NULL, MB_ICONSTOP );\
				return FALSE;\
			}\
			if( bSig )\
				var=(signed)ret;\
			else\
				var=ret;\
			SetDlgItemInt( m_hDlg, id, ret, bSig );\
		}

		{
			DWORD pl, pt, pw, ph;
			int jthres, yo;
			GETANDSET( IDC_JUDGETHRESHOLD, jthres, TRUE, _T("判定の回数閾値の入力が不正です。") );
			GETANDSET( IDC_PRLEFT, pl, FALSE, _T("プレイ画面領域の左座標の入力が不正です。") );
			GETANDSET( IDC_PRTOP, pt, FALSE, _T("プレイ画面領域の上座標の入力が不正です。") );
			GETANDSET( IDC_PRWIDTH, pw, FALSE, _T("プレイ画面領域の幅の入力が不正です。") );
			if( pw == 0 )
			{
				MessageBox( m_hDlg, _T("プレイ画面領域の幅は0以外の値でなければいけません。"), NULL, MB_ICONSTOP );
				return FALSE;
			}
			GETANDSET( IDC_PRHEIGHT, ph, FALSE, _T("プレイ画面領域の高さの入力が不正です。") );
			if( ph == 0 )
			{
				MessageBox( m_hDlg, _T("プレイ画面領域の高さは0以外の値でなければいけません。"), NULL, MB_ICONSTOP );
				return FALSE;
			}
			GETANDSET( IDC_YOFFSET, yo, TRUE, _T("プレイ画面領域のオフセットの入力が不正です。") );

			m_judgeThreshold = jthres;
			m_prLeft = pl;
			m_prTop = pt;
			m_prWidth = pw;
			m_prHeight = ph;
			m_yOffset = yo;

#define WRITETOINI(var,bSig,name) do{\
				char num[64];\
				if( bSig )\
					wsprintfA( num, "%d", var );\
				else\
					wsprintfA( num, "%u", var );\
				WritePrivateProfileStringA( m_appName.c_str(), name, num, m_iniPath.c_str() );\
			}while(0)

			//boost::filesystem::remove( m_iniPath.c_str() );

			WRITETOINI( jthres, TRUE, "JC" );
			WRITETOINI( pl, FALSE, "PL" );
			WRITETOINI( pt, FALSE, "PT" );
			WRITETOINI( pw, FALSE, "PW" );
			WRITETOINI( ph, FALSE, "PH" );
			WRITETOINI( yo, TRUE, "YOffset" );
			WRITETOINI( SendDlgItemMessage( m_hDlg, IDC_VISIBLE, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE, TRUE, "Visible" );
			m_bVisible = SendDlgItemMessage( m_hDlg, IDC_VISIBLE, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE;

			/*if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_PIV0, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( 0, TRUE, "PivRot" );
				setPivot( 0 );
			}
			else if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_PIV90, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( 1, TRUE, "PivRot" );
				setPivot( 1 );
			}
			else
			{
				WRITETOINI( 3, TRUE, "PivRot" );
				setPivot( 3 );
			}*/

			if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_VERTICALWINDOW, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( 1, TRUE, "PivRot" );
				setPivot( 1 );
			}
			else
			{
				WRITETOINI( 0, TRUE, "PivRot" );
				setPivot( 0 );
			}

			if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_FILTERNONE, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( D3DTEXF_NONE, TRUE, "Filter" );
				m_filterType = D3DTEXF_NONE;
			}
			else
			{
				WRITETOINI( D3DTEXF_LINEAR, TRUE, "Filter" );
				m_filterType = D3DTEXF_LINEAR;
			}

			if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_ROT0_2, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( 0, TRUE, "Rot" );
				m_rot = 0;
			}
			else if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_ROT90_2, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( 1, TRUE, "Rot" );
				m_rot = 1;
			}
			else if( BST_CHECKED == SendDlgItemMessage( m_hDlg, IDC_ROT180_2, BM_GETCHECK, 0, 0 ) )
			{
				WRITETOINI( 2, TRUE, "Rot" );
				m_rot = 2;
			}
			else
			{
				WRITETOINI( 3, TRUE, "Rot" );
				m_rot = 3;
			}

			int i = 0;
			TCHAR name[64];
			for( std::vector<EditRectData>::iterator itr = m_erds.begin();
				itr != m_erds.cend(); ++itr, ++i )
			{
				wsprintf( name, _T("Name%d"), i );
				WritePrivateProfileStringA( m_appName.c_str(), name, itr->name, m_iniPath.c_str() );
				wsprintf( name, _T("OSL%d"), i ); WRITETOINI( itr->rcSrc.left, TRUE, name );
				wsprintf( name, _T("OST%d"), i ); WRITETOINI( itr->rcSrc.top, TRUE, name );
				wsprintf( name, _T("OSW%d"), i ); WRITETOINI( itr->rcSrc.right, TRUE, name );
				wsprintf( name, _T("OSH%d"), i ); WRITETOINI( itr->rcSrc.bottom, TRUE, name );
				wsprintf( name, _T("ODL%d"), i ); WRITETOINI( itr->rcDest.left, TRUE, name );
				wsprintf( name, _T("ODT%d"), i ); WRITETOINI( itr->rcDest.top, TRUE, name );
				wsprintf( name, _T("ODW%d"), i ); WRITETOINI( itr->rcDest.right, TRUE, name );
				wsprintf( name, _T("ODH%d"), i ); WRITETOINI( itr->rcDest.bottom, TRUE, name );
				wsprintf( name, _T("OR%d"), i ); WRITETOINI( itr->rotation, TRUE, name );
				wsprintf( name, _T("ORHas%d"), i ); WRITETOINI( TRUE, TRUE, name );
			}
			wsprintf( name, _T("ORHas%d"), i ); WRITETOINI( FALSE, TRUE, name );
		}
		m_erdsPrev = m_erds;
		return TRUE;
	}

	static HHOOK ms_hHook;
	static LRESULT CALLBACK CallWndHook( int nCode, WPARAM wParam, LPARAM lParam )
	{
		LPMSG pMsg = reinterpret_cast<LPMSG>(lParam);
		if ( nCode >= 0 && PM_REMOVE == wParam )
		{
			for( std::map<HWND,MyDirect3DDevice9*>::iterator itr = hwnd2devMap.begin();
				 itr != hwnd2devMap.end(); ++itr )
			{
				// Don't translate non-input events.
				if ( (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) )
				{
					if ( IsDialogMessage(itr->second->m_hDlg, pMsg) )
					{
						// The value returned from this hookproc is ignored, 
						// and it cannot be used to tell Windows the message has been handled.
						// To avoid further processing, convert the message to WM_NULL 
						// before returning.
						pMsg->message = WM_NULL;
						pMsg->lParam  = 0;
						pMsg->wParam  = 0;
					}
				}
			}
		}
		if( nCode == HC_ACTION )
		{
			/*LPCWPSTRUCT pcwp = reinterpret_cast<LPCWPSTRUCT>(lParam);
			if( pcwp->message == WM_SYSCOMMAND /*&& hwnd2devMap.find(pcwp->hwnd) != hwnd2devMap.end()* )
			{
				if( pcwp->wParam == ms_visID )
				{
					MessageBox( pcwp->hwnd, _T("stub"), NULL, MB_ICONINFORMATION );
				}
			}*/

			
			if( th_hwnd2hwndMap.find(pMsg->hwnd) != th_hwnd2hwndMap.end() )
			{
				MyDirect3DDevice9* pDev = hwnd2devMap[th_hwnd2hwndMap[pMsg->hwnd]];
				switch( pMsg->message )
				{
				case WM_SYSCOMMAND:
					if( pMsg->wParam == ms_visID )
						pDev->SetVisibility( !pDev->m_bVisible );
					break;

				/*case WM_KEYDOWN:
					switch( pMsg->wParam )
					{

					case VK_OEM_3:
						if( pDev->m_d3dpp.BackBufferFormat == D3DFMT_X8R8G8B8 )
							if( (HIWORD(pMsg->lParam) & KF_REPEAT) == 0 )
							{
								namespace fs = boost::filesystem;
								char fname[MAX_PATH];

								fs::create_directory( pDev->m_workingDir/"snapshot" );
								for( int i = 0; i >= 0; ++i )
								{
									wsprintfA( fname, (pDev->m_workingDir/"snapshot/thRot%03d.bmp").string().c_str(), i );
									if( !fs::exists( fname ) )
									{
										::D3DXSaveTextureToFileA( fname, D3DXIFF_BMP, pDev->m_pTex, NULL );
										break;
									}
								}
							}
						break;
					}
					break;*/
					
				case WM_SYSKEYDOWN:
					switch( pMsg->wParam )
					{
					case VK_LEFT:
						if( (HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT) )
						{
							switch( (pDev->m_rot+1)%4 )
							{
							case 0:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 0 );
								break;

							case 1:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 1 );
								break;

							case 2:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 0 );
								break;

							case 3:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 1 );
								break;
							}
							pDev->ApplyChange();
						}
						break;

					case VK_RIGHT:
						if( HIWORD(pMsg->lParam) & KF_ALTDOWN && !(HIWORD(pMsg->lParam) & KF_REPEAT) )
						{
							switch( (pDev->m_rot+3)%4 )
							{
							case 0:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 0 );
								break;

							case 1:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 1 );
								break;

							case 2:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 0 );
								break;

							case 3:
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0 );
								SendDlgItemMessage( pDev->m_hDlg, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0 );
								//if( pDev->m_d3dpp.Windowed ) pDev->setPivot( 1 );
								break;
							}
							pDev->ApplyChange();
						}
						break;
					}
					break;
				}
			}
		}
		return CallNextHookEx( ms_hHook, nCode, wParam, lParam );
	}


	bool m_bInitialized;

public:
	MyDirect3DDevice9()
	{
		m_pd3dDev = NULL;
		m_pSprite = NULL;
		m_pTex = NULL;
		m_pTexSurface = NULL;
		m_pBackBuffer = NULL;
		m_pRenderTarget = NULL;
		m_bInitialized = false;
		m_judgeCount = 0;
		m_judgeCountPrev = 0;
#ifdef _DEBUG
		m_pFont = NULL;
		m_FPSNew = 60.f;
		m_fpsCount = 0;
#endif
		if( ms_hHook == NULL )
		{
			ms_hHook = SetWindowsHookEx( WH_GETMESSAGE, CallWndHook, NULL, GetCurrentThreadId() );
			if( ms_hHook == NULL )
			{
				LPVOID lpMessageBuffer;

				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // デフォルト ユーザー言語 
				(LPTSTR) &lpMessageBuffer,
					0,
					NULL );

				MessageBox( NULL, (LPTSTR)lpMessageBuffer, NULL, MB_ICONSTOP );

				//... 文字列が表示されます。
				// システムによって確保されたバッファを開放します。
				LocalFree( lpMessageBuffer );
			}
		}
		ms_refCnt++;

#ifdef _DEBUG
		QueryPerformanceFrequency( &m_freq );
		ZeroMemory( &m_prev, sizeof(m_prev) );
#endif
	}

	MyDirect3DDevice9( const MyDirect3DDevice9& dev )
	{
		m_pd3dDev = dev.m_pd3dDev;
		m_appName = dev.m_appName;
		m_bVisible = dev.m_bVisible;
		m_iniPath = dev.m_iniPath;
		m_judgeThreshold = dev.m_judgeThreshold;
		m_judgeCount = dev.m_judgeCount;
		m_judgeCountPrev = dev.m_judgeCountPrev;
		m_orCount = dev.m_orCount;
		m_pBackBuffer = dev.m_pBackBuffer;
		m_prHeight = dev.m_prHeight;
		m_prLeft = dev.m_prLeft;
		m_prTop = dev.m_prTop;
		m_prWidth = dev.m_prWidth;
		m_pSprite = dev.m_pSprite;
		m_pTex = dev.m_pTex;
		m_pTexSurface = dev.m_pTexSurface;
		m_pRenderTarget = dev.m_pRenderTarget;
		m_yOffset = dev.m_yOffset;
		m_bInitialized = dev.m_bInitialized;
#ifdef _DEBUG
		m_freq = dev.m_freq;
		m_prev = dev.m_prev;
		m_pFont = dev.m_pFont;
		m_FPSNew = 60.f;
		m_fpsCount = 0;
#endif

		if( m_pBackBuffer ) m_pBackBuffer->AddRef();
		if( m_pSprite ) m_pSprite->AddRef();
		if( m_pd3dDev ) m_pd3dDev->AddRef();
		if( m_pTexSurface ) m_pTexSurface->AddRef();
		if( m_pTex ) m_pTex->AddRef();
		if( m_pRenderTarget ) m_pRenderTarget->AddRef();
#ifdef _DEBUG
		if( m_pFont ) m_pFont->AddRef();
#endif

		ms_refCnt++;
	}

	virtual ~MyDirect3DDevice9()
	{
		ms_refCnt--;
		if( ms_refCnt == 0 && ms_hHook == NULL )
			UnhookWindowsHookEx( ms_hHook );
	}

	void setPivot( int piv )
	{
		if( m_pivRot%2 != piv%2 && m_d3dpp.Windowed )
		{
			RECT rcClient, rcWindow;
			GetClientRect( m_hTH, &rcClient );
			GetWindowRect( m_hTH, &rcWindow );

			MoveWindow( m_hTH, rcWindow.left, rcWindow.top,
				(rcWindow.right-rcWindow.left) - (rcClient.right-rcClient.left)+(rcClient.bottom-rcClient.top),
				(rcWindow.bottom-rcWindow.top) - (rcClient.bottom-rcClient.top)+(rcClient.right-rcClient.left), TRUE );

			m_d3dpp.BackBufferWidth = rcClient.bottom - rcClient.top;
			m_d3dpp.BackBufferHeight = rcClient.right - rcClient.left;

			//	640より小さいと画面が描画されないことがあるので修正
			if( m_d3dpp.BackBufferWidth < 640 )
			{
				m_d3dpp.BackBufferHeight = 720*m_d3dpp.BackBufferHeight/m_d3dpp.BackBufferWidth;
				m_d3dpp.BackBufferWidth = 720;
			}

			m_pd3dDev->Reset( &m_d3dpp );
		}
		SendDlgItemMessage( m_hDlg, IDC_VERTICALWINDOW, BM_SETCHECK, piv ? BST_CHECKED : BST_UNCHECKED, 0 );
		m_pivRot = piv;
	}

	boost::filesystem::path m_workingDir;

	bool init( LPDIRECT3DDEVICE9 pd3dDev, D3DFORMAT fmt )
	{
		char fname[MAX_PATH];
		GetModuleFileName( NULL, fname, MAX_PATH );
		*strrchr( fname, '.' ) = '\0';
		boost::filesystem::path pth( fname );

		m_appName = std::string("THRotator_") + pth.filename().generic_string();
		char path[MAX_PATH];
		size_t retSize;
		errno_t en = getenv_s( &retSize, path, "APPDATA" );

		char dummy[128]; double v = 0.; double l;
		if( pth.filename().generic_string().compare( "東方紅魔郷" ) == 0 )
			v = 6.;
		else
			sscanf( pth.filename().generic_string().c_str(), "th%lf%s", &v, dummy );
		while( v > 90. )
			v /= 10.;

		if( v > 12.3 && en == 0 && retSize > 0 )
		{
			m_workingDir = boost::filesystem::path(path)/"ShanghaiAlice"/pth.filename();
			boost::filesystem::create_directory( m_workingDir );
		}
		else
			m_workingDir = boost::filesystem::current_path();
		m_iniPath = (m_workingDir/"throt.ini").string();

		m_pd3dDev = pd3dDev;

		m_judgeThreshold = GetPrivateProfileIntA( m_appName.c_str(), "JC", 999, m_iniPath.c_str() );
		m_prLeft = GetPrivateProfileIntA( m_appName.c_str(), "PL", 32, m_iniPath.c_str() );
		m_prTop = GetPrivateProfileIntA( m_appName.c_str(), "PT", 16, m_iniPath.c_str() );
		m_prWidth = GetPrivateProfileIntA( m_appName.c_str(), "PW", 384, m_iniPath.c_str() );
		m_prHeight = GetPrivateProfileIntA( m_appName.c_str(), "PH", 448, m_iniPath.c_str() );
		m_yOffset = GetPrivateProfileIntA( m_appName.c_str(), "YOffset", 0, m_iniPath.c_str() );
		m_bVisible = GetPrivateProfileIntA( m_appName.c_str(), "Visible", FALSE, m_iniPath.c_str() );
		m_pivRot = GetPrivateProfileIntA( m_appName.c_str(), "PivRot", 0, m_iniPath.c_str() );
		m_rot = GetPrivateProfileIntA( m_appName.c_str(), "Rot", 0, m_iniPath.c_str() );
		m_filterType = (D3DTEXTUREFILTERTYPE)GetPrivateProfileIntA( m_appName.c_str(), "Filter", D3DTEXF_LINEAR, m_iniPath.c_str() );

		m_hTH = GetActiveWindow();

		int piv = m_pivRot;
		m_pivRot = 0;
		setPivot( piv );

		BOOL bHasNext = GetPrivateProfileIntA( m_appName.c_str(), "ORHas0", FALSE, m_iniPath.c_str() );
		int cnt = 0;
		while( bHasNext )
		{
			EditRectData erd;
			TCHAR name[64];

			wsprintf( name, _T("Name%d"), cnt );
			GetPrivateProfileStringA( m_appName.c_str(), name, name, erd.name, sizeof(erd.name)/sizeof(TCHAR), m_iniPath.c_str() );

			wsprintf( name, _T("OSL%d"), cnt );
			erd.rcSrc.left = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );
			wsprintf( name, _T("OST%d"), cnt );
			erd.rcSrc.top = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );
			wsprintf( name, _T("OSW%d"), cnt );
			erd.rcSrc.right = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );
			wsprintf( name, _T("OSH%d"), cnt );
			erd.rcSrc.bottom = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );

			wsprintf( name, _T("ODL%d"), cnt );
			erd.rcDest.left = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );
			wsprintf( name, _T("ODT%d"), cnt );
			erd.rcDest.top = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );
			wsprintf( name, _T("ODW%d"), cnt );
			erd.rcDest.right = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );
			wsprintf( name, _T("ODH%d"), cnt );
			erd.rcDest.bottom = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );

			wsprintf( name, _T("OR%d"), cnt );
			erd.rotation = GetPrivateProfileIntA( m_appName.c_str(), name, 0, m_iniPath.c_str() );

			m_erds.push_back( erd );
			cnt++;

			wsprintf( name, _T("ORHas%d"), cnt );
			bHasNext = GetPrivateProfileIntA( m_appName.c_str(), name, FALSE, m_iniPath.c_str() );
		}

		m_erdsPrev = m_erds;

		//	メニューを改造
		HMENU hMenu = GetSystemMenu( m_hTH, FALSE );
		AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
		AppendMenu( hMenu, MF_STRING, ms_visID, _T("") );

		m_hSysMenu = hMenu;

		HWND hWnd = m_hDlg = CreateDialogParam( GetModuleHandle(_T("d3d9")), MAKEINTRESOURCE(IDD_MAINDLG), NULL, DlgProc, (LPARAM)this );
		th_hwnd2hwndMap[m_hTH] = hWnd;
		if( hWnd == NULL )
		{
			MessageBox( NULL, _T("ダイアログの作成に失敗"), NULL, MB_ICONSTOP );
		}

		D3DERR_DEVICELOST;

		SetVisibility( m_bVisible );

		m_judgeCount = 0;

		if( FAILED( pd3dDev->CreateRenderTarget( 640, 480, m_d3dpp.BackBufferFormat,
			m_d3dpp.MultiSampleType, m_d3dpp.MultiSampleQuality,
			TRUE, &m_pRenderTarget, NULL ) ) )
		{
			m_pRenderTarget = NULL;
			return false;
		}

		if( FAILED( pd3dDev->CreateTexture( 640, 480, 1, 
		D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT, &m_pTex, NULL ) ) )
		{
			m_pTex = NULL;
			m_pTexSurface = NULL;
			return false;
		}
		else
		{
			m_pTex->GetSurfaceLevel( 0, &m_pTexSurface );
		}

		if( FAILED( D3DXCreateSprite( pd3dDev, &m_pSprite ) ) )
		{
			m_pTexSurface->Release();
			m_pTex->Release();
			m_pTexSurface = NULL;
			m_pTex = NULL;
			m_pSprite = NULL;
			return false;
		}

#ifdef _DEBUG
		if( FAILED( D3DXCreateFont( pd3dDev,
			0, 0, 0, 0, 0, 0, 0, 0, 0, "Arial", &m_pFont ) ) )
		{
			m_pSprite->Release();
			m_pTexSurface->Release();
			m_pTex->Release();
			m_pTexSurface = NULL;
			m_pTex = NULL;
			m_pSprite = NULL;
			m_pFont = NULL;
			return false;
		}
#endif

		pd3dDev->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer );
		pd3dDev->SetRenderTarget( 0, m_pRenderTarget );

		m_bInitialized = true;
		return true;
	}

	ULONG WINAPI AddRef(VOID)
	{
		return m_pd3dDev->AddRef();
	}

	HRESULT WINAPI QueryInterface( REFIID riid, LPVOID* ppvObj )
	{
		return m_pd3dDev->QueryInterface( riid, ppvObj );
	}

	ULONG WINAPI Release()
	{
#ifdef _DEBUG
		MessageBox( NULL, _T("RELEASE"), NULL, 0 );
		if( m_pFont != NULL )
			m_pFont->Release();
#endif
		if( m_pRenderTarget != NULL )
			m_pRenderTarget->Release();
		if( m_pTexSurface != NULL )
			m_pTexSurface->Release();
		if( m_pTex != NULL )
			m_pTex->Release();
		if( m_pSprite != NULL )
			m_pSprite->Release();
		if( m_pBackBuffer )
			m_pBackBuffer->Release();
		ULONG ret = m_pd3dDev->Release();

		return ret;
	}

	HRESULT WINAPI BeginScene(VOID)
	{
		return m_pd3dDev->BeginScene();
	}

	HRESULT WINAPI BeginStateBlock(VOID)
	{
		return m_pd3dDev->BeginStateBlock();
	}

	HRESULT WINAPI Clear(          DWORD Count,
		const D3DRECT *pRects,
		DWORD Flags,
		D3DCOLOR Color,
		float Z,
		DWORD Stencil )
	{
		return m_pd3dDev->Clear( Count,
			pRects,
			Flags,
			Color,
			Z,
			Stencil );
	}

	HRESULT WINAPI ColorFill(          IDirect3DSurface9 *pSurface,
		CONST RECT *pRect,
		D3DCOLOR color )
	{
		return m_pd3dDev->ColorFill( pSurface, pRect, color );
	}

	HRESULT WINAPI CreateAdditionalSwapChain(          D3DPRESENT_PARAMETERS* pPresentationParameters,
		IDirect3DSwapChain9** ppSwapChain )
	{
		return m_pd3dDev->CreateAdditionalSwapChain( pPresentationParameters, ppSwapChain );
	}

	HRESULT WINAPI CreateCubeTexture(          UINT EdgeLength,
		UINT Levels,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
		IDirect3DCubeTexture9 **ppCubeTexture,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateCubeTexture( EdgeLength, Levels, Usage, Format, Pool,
			ppCubeTexture, pHandle );
	}

	HRESULT WINAPI CreateDepthStencilSurface(          UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
		DWORD MultisampleQuality,
		BOOL Discard,
		IDirect3DSurface9** ppSurface,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateDepthStencilSurface( Width,
			Height, Format, MultiSample,
			MultisampleQuality,
			Discard, ppSurface, pHandle );
	}

	HRESULT WINAPI CreateIndexBuffer(          UINT Length,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
		IDirect3DIndexBuffer9** ppIndexBuffer,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateIndexBuffer( Length, Usage, Format,
			Pool, ppIndexBuffer, pHandle );
	}

	HRESULT WINAPI CreateOffscreenPlainSurface(          UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DPOOL Pool,
		IDirect3DSurface9** ppSurface,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateOffscreenPlainSurface( Width,
			Height, Format, Pool, ppSurface, pHandle );
	}

	HRESULT WINAPI CreatePixelShader(          CONST DWORD *pFunction,
		IDirect3DPixelShader9** ppShader )
	{
		return m_pd3dDev->CreatePixelShader( pFunction, ppShader );
	}

	HRESULT WINAPI CreateQuery(          D3DQUERYTYPE Type,
		IDirect3DQuery9** ppQuery )
	{
		return m_pd3dDev->CreateQuery( Type, ppQuery );
	}

	HRESULT WINAPI CreateRenderTarget(          UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
		DWORD MultisampleQuality,
		BOOL Lockable,
		IDirect3DSurface9** ppSurface,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateRenderTarget( Width, Height,
			Format, MultiSample, MultisampleQuality,
			Lockable, ppSurface, pHandle );
	}

	HRESULT WINAPI CreateStateBlock(          D3DSTATEBLOCKTYPE Type,
		IDirect3DStateBlock9** ppSB )
	{
		return m_pd3dDev->CreateStateBlock( Type, ppSB );
	}

	HRESULT WINAPI CreateTexture(          UINT Width,
		UINT Height,
		UINT Levels,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
		IDirect3DTexture9** ppTexture,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateTexture( Width, Height, Levels,
			Usage, Format, Pool, ppTexture, pHandle );
	}

	HRESULT WINAPI CreateVertexBuffer(          UINT Length,
		DWORD Usage,
		DWORD FVF,
		D3DPOOL Pool,
		IDirect3DVertexBuffer9** ppVertexBuffer,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateVertexBuffer( Length,
			Usage, FVF, Pool, ppVertexBuffer, pHandle );
	}

	HRESULT WINAPI CreateVertexDeclaration(          CONST D3DVERTEXELEMENT9* pVertexElements,
		IDirect3DVertexDeclaration9** ppDecl )
	{
		return m_pd3dDev->CreateVertexDeclaration( pVertexElements, ppDecl );
	}

	HRESULT WINAPI CreateVertexShader(          const DWORD *pFunction,
		IDirect3DVertexShader9** ppShader )
	{
		return m_pd3dDev->CreateVertexShader( pFunction, ppShader );
	}

	HRESULT WINAPI CreateVolumeTexture(          UINT Width,
		UINT Height,
		UINT Depth,
		UINT Levels,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
		IDirect3DVolumeTexture9** ppVolumeTexture,
		HANDLE* pHandle )
	{
		return m_pd3dDev->CreateVolumeTexture( Width, Height, Depth,
			Levels, Usage, Format, Pool, ppVolumeTexture, pHandle );
	}

	HRESULT WINAPI DeletePatch(          UINT Handle )
	{
		return m_pd3dDev->DeletePatch( Handle );
	}

	HRESULT WINAPI DrawIndexedPrimitive(          D3DPRIMITIVETYPE Type,
		INT BaseVertexIndex,
		UINT MinIndex,
		UINT NumVertices,
		UINT StartIndex,
		UINT PrimitiveCount )
	{
		return m_pd3dDev->DrawIndexedPrimitive( Type,
			BaseVertexIndex,
			MinIndex,
			NumVertices,
			StartIndex,
			PrimitiveCount );
	}

	HRESULT WINAPI DrawIndexedPrimitiveUP(          D3DPRIMITIVETYPE PrimitiveType,
		UINT MinVertexIndex,
		UINT NumVertexIndices,
		UINT PrimitiveCount,
		const void *pIndexData,
		D3DFORMAT IndexDataFormat,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride )
	{
		return m_pd3dDev->DrawIndexedPrimitiveUP( PrimitiveType,
			MinVertexIndex, NumVertexIndices, PrimitiveCount,
			pIndexData, IndexDataFormat, pVertexStreamZeroData,
			VertexStreamZeroStride );
	}

	HRESULT WINAPI DrawPrimitive(          D3DPRIMITIVETYPE PrimitiveType,
		UINT StartVertex,
		UINT PrimitiveCount )
	{
		return m_pd3dDev->DrawPrimitive( PrimitiveType, StartVertex, PrimitiveCount );
	}

	HRESULT WINAPI DrawPrimitiveUP(          D3DPRIMITIVETYPE PrimitiveType,
		UINT PrimitiveCount,
		const void *pVertexStreamZeroData,
		UINT VertexStreamZeroStride )
	{
		return m_pd3dDev->DrawPrimitiveUP( PrimitiveType,
			PrimitiveCount, pVertexStreamZeroData,
			VertexStreamZeroStride );
	}

	HRESULT WINAPI DrawRectPatch(          UINT Handle,
		const float* pNumSegs,
		const D3DRECTPATCH_INFO* pRectPatchInfo )
	{
		return m_pd3dDev->DrawRectPatch( Handle, pNumSegs, pRectPatchInfo );
	}

	HRESULT WINAPI DrawTriPatch(          UINT Handle,
		const float* pNumSegs,
		const D3DTRIPATCH_INFO* pTriPatchInfo )
	{
		return m_pd3dDev->DrawTriPatch( Handle, pNumSegs,
			pTriPatchInfo );
	}

	HRESULT WINAPI EndScene(VOID)
	{
		D3DXLoadSurfaceFromSurface( m_pTexSurface,
			NULL, NULL, m_pRenderTarget, NULL, NULL, D3DX_FILTER_NONE, 0 );
		m_pd3dDev->SetRenderTarget( 0, m_pBackBuffer );
		m_pd3dDev->Clear( 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.f, 0 );
		/*if( SUCCEEDED( m_pd3dDev->BeginScene() ) )
		{*/
			if( SUCCEEDED( m_pSprite->Begin( D3DXSPRITE_ALPHABLEND ) ) )
			{
				m_pd3dDev->SetSamplerState( 0, D3DSAMP_MAGFILTER, m_filterType );
				m_pd3dDev->SetSamplerState( 0, D3DSAMP_MINFILTER, m_filterType );

				m_pd3dDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

				bool aspectLessThan133;
				if( m_rot%2 == 0 )
					aspectLessThan133 = m_d3dpp.BackBufferWidth*3 < m_d3dpp.BackBufferHeight*4;
				else
					aspectLessThan133 = m_d3dpp.BackBufferHeight*3 < m_d3dpp.BackBufferWidth*4;
				if( aspectLessThan133 && m_judgeCount >= m_judgeThreshold )
				{
					auto drawRect = [this]( const RECT& rcSrc, const RECT& rcDest, int rotation, float scaleFactor, float prWidth, float prHeight )
					{
						D3DXMATRIX mat, matS, matR;
						switch( rotation )
						{
						case 0:
							D3DXMatrixIdentity( &mat );
							D3DXMatrixScaling( &matS, (float)rcDest.right/rcSrc.right, (float)rcDest.bottom/rcSrc.bottom, 1.f );
							break;

						case 1:
							D3DXMatrixRotationZ( &mat, D3DX_PI*0.5f );
							D3DXMatrixScaling( &matS, (float)rcDest.right/rcSrc.bottom, (float)rcDest.bottom/rcSrc.right, 1.f );
							mat._41 += rcSrc.bottom;
							break;

						case 2:
							D3DXMatrixRotationZ( &mat, D3DX_PI );
							D3DXMatrixScaling( &matS, (float)rcDest.right/rcSrc.right, (float)rcDest.bottom/rcSrc.bottom, 1.f );
							mat._41 += rcSrc.right;
							mat._42 += rcSrc.bottom;
							break;

						case 3:
							D3DXMatrixRotationZ( &mat, -D3DX_PI*0.5f );
							D3DXMatrixScaling( &matS, (float)rcDest.right/rcSrc.bottom, (float)rcDest.bottom/rcSrc.right, 1.f );
							mat._42 += rcSrc.right;
							break;
						}

						D3DXMatrixRotationZ( &matR, D3DX_PI*0.5f*m_rot );

						mat = mat*matS;
						mat._41 += rcDest.left - prWidth*0.5f;
						mat._42 += rcDest.top - prHeight*0.5f;
						mat *= matR;
						switch( m_rot )
						{
						case 0: case 2:
							mat._41 += prWidth*0.5f;
							mat._42 += prHeight*0.5f;
							break;

						case 1: case 3:
							mat._41 += prHeight*0.5f;
							mat._42 += prWidth*0.5f;
							break;
						}

						/*switch( m_rot )
						{
						case 1:
							mat._41 -= 128.f;
							break;

						case 3:
							mat._42 -= 96.f;
							break;
						}*/
						D3DXMatrixScaling( &matS, scaleFactor,
							scaleFactor, 1.f );
						mat *= matS;

						if( m_rot % 2 == 0 )
						{
							if( m_d3dpp.BackBufferHeight*3 > m_d3dpp.BackBufferWidth*4 )
								mat._42 += 0.5f*m_d3dpp.BackBufferHeight-320.f*m_d3dpp.BackBufferWidth/480;
							else
								mat._41 += 0.5f*m_d3dpp.BackBufferWidth-240.f*m_d3dpp.BackBufferHeight/640;
						}
						else
						{
							if( m_d3dpp.BackBufferHeight*4 > m_d3dpp.BackBufferWidth*3 )
								mat._42 += 0.5f*m_d3dpp.BackBufferHeight-240.f*m_d3dpp.BackBufferWidth/640;
							else
								mat._41 += 0.5f*m_d3dpp.BackBufferWidth-320.f*m_d3dpp.BackBufferHeight/480;
						}

						m_pSprite->SetTransform( &mat );
						RECT rc = rcSrc;
						rc.right += rc.left;
						rc.bottom += rc.top;
						m_pSprite->Draw( m_pTex, &rc, &D3DXVECTOR3( 0, 0, 0 ), NULL, 0xffffffff );
					};
					RECT rc;
					float scaleFactor;
					rc.left = m_prLeft;
					rc.right = m_prWidth;
					rc.top = m_prTop;
					rc.bottom = m_prHeight;
					if( m_prWidth*4 < m_prHeight*3 )
					{
						rc.left = (LONG)m_prLeft + ((LONG)m_prWidth - (LONG)m_prHeight*3/4)/2;
						rc.right = m_prHeight*3/4;
					}
					else if( m_prWidth*4 > m_prHeight*3 )
					{
						rc.top = (LONG)m_prTop + ((LONG)m_prHeight - (LONG)m_prWidth*4/3)/2;
						rc.bottom = m_prWidth*4/3;
					}
					rc.top -= m_yOffset;
					scaleFactor =
						m_rot%2==0 ?
						min((float)m_d3dpp.BackBufferHeight/rc.bottom,(float)m_d3dpp.BackBufferWidth/rc.right):
						min((float)m_d3dpp.BackBufferWidth/rc.bottom,(float)m_d3dpp.BackBufferHeight/rc.right);
					RECT rcDest;
					rcDest.left = rcDest.top = 0;
					rcDest.right = rc.right;
					rcDest.bottom = rc.bottom;

					drawRect( rc, rcDest, 0, scaleFactor, (float)rc.right, (float)rc.bottom );

					for( std::vector<EditRectData>::const_iterator itr = m_erdsPrev.begin(); itr != m_erdsPrev.end(); ++itr )
					{
						if( itr->rcSrc.right == 0 || itr->rcSrc.bottom == 0 || itr->rcDest.right == 0 || itr->rcDest.bottom == 0 )
							continue;
						drawRect( itr->rcSrc, itr->rcDest, itr->rotation, scaleFactor, (float)rc.right, (float)rc.bottom );
					}
				}
				else
				{
					D3DXMATRIX mat, matS;
					D3DXMatrixRotationZ( &mat, D3DX_PI*m_rot*0.5f );
					switch( m_rot )
					{
					case 0:
						break;

					case 1:
						mat._41 += 480.f;
						break;

					case 2:
						mat._41 += 640.f;
						mat._42 += 480.f;
						break;

					case 3:
						mat._42 += 640.f;
						break;
					}
					switch( m_rot%2 )
					{
					case 0:
						if( m_d3dpp.BackBufferHeight*4 > m_d3dpp.BackBufferWidth*3 )
						{
							D3DXMatrixScaling( &matS, m_d3dpp.BackBufferWidth/640.f, m_d3dpp.BackBufferWidth/640.f, 1.f );
							mat *= matS;
							mat._42 += m_d3dpp.BackBufferHeight*0.5f - 0.5f*m_d3dpp.BackBufferWidth*3.f/4.f;
						}
						else
						{
							D3DXMatrixScaling( &matS, m_d3dpp.BackBufferHeight/480.f, m_d3dpp.BackBufferHeight/480.f, 1.f );
							mat *= matS;
							mat._41 += m_d3dpp.BackBufferWidth*0.5f - 0.5f*m_d3dpp.BackBufferHeight*4.f/3.f;
						}
						break;

					case 1:
						if( m_d3dpp.BackBufferHeight*3 > m_d3dpp.BackBufferWidth*4 )
						{
							D3DXMatrixScaling( &matS, m_d3dpp.BackBufferWidth/480.f, m_d3dpp.BackBufferWidth/480.f, 1.f );
							mat *= matS;
							mat._42 += m_d3dpp.BackBufferHeight*0.5f - 0.5f*m_d3dpp.BackBufferWidth*4.f/3.f;
						}
						else
						{
							D3DXMatrixScaling( &matS, m_d3dpp.BackBufferHeight/640.f, m_d3dpp.BackBufferHeight/640.f, 1.f );
							mat *= matS;
							mat._41 += m_d3dpp.BackBufferWidth*0.5f - 0.5f*m_d3dpp.BackBufferHeight*3.f/4.f;
						}
						break;

					}
					m_pSprite->SetTransform ( &mat );
					m_pSprite->Draw( m_pTex, NULL, &D3DXVECTOR3( 0, 0, 0 ), NULL, 0xffffffff );
				}

				m_pSprite->End();
			}
			/*m_pd3dDev->EndScene();
		}*/

#ifdef _DEBUG
			RECT rcText;
			rcText.right = m_d3dpp.BackBufferWidth;
			rcText.left = rcText.right - 128;
			rcText.bottom = m_d3dpp.BackBufferHeight;
			rcText.top = rcText.bottom - 24;
			TCHAR text[64];
			_stprintf( text, _T("%5.2f fps"), m_FPS );
			m_pFont->DrawText( NULL, text, -1, &rcText, 0, D3DCOLOR_XRGB(0xff,(unsigned)(0xff*max(0,m_FPS-30.f)/30.f),(unsigned)(0xff*max(0,m_FPS-30.f)/30.f)) );
#endif

		m_pd3dDev->SetRenderTarget( 0, m_pRenderTarget );
		return m_pd3dDev->EndScene();
	}

	HRESULT WINAPI EndStateBlock(          IDirect3DStateBlock9 **ppSB )
	{
		return m_pd3dDev->EndStateBlock( ppSB );
	}

	HRESULT WINAPI EvictManagedResources(VOID)
	{
		return m_pd3dDev->EvictManagedResources();
	}

	UINT WINAPI GetAvailableTextureMem(VOID)
	{
		return m_pd3dDev->GetAvailableTextureMem();
	}

	HRESULT WINAPI GetBackBuffer(          UINT iSwapChain,
		UINT BackBuffer,
		D3DBACKBUFFER_TYPE Type,
		IDirect3DSurface9 **ppBackBuffer )
	{
		*ppBackBuffer = m_pRenderTarget;
		m_pRenderTarget->AddRef();
		return S_OK;
		return m_pd3dDev->GetBackBuffer( iSwapChain,
			BackBuffer, Type, ppBackBuffer );
	}

	HRESULT WINAPI GetClipPlane(          DWORD Index,
		float *pPlane )
	{
		return m_pd3dDev->GetClipPlane( Index, pPlane );
	}

	HRESULT WINAPI GetClipStatus(          D3DCLIPSTATUS9 *pClipStatus )
	{
		return m_pd3dDev->GetClipStatus( pClipStatus );
	}

	HRESULT WINAPI GetCreationParameters(          D3DDEVICE_CREATION_PARAMETERS *pParameters )
	{
		return m_pd3dDev->GetCreationParameters( pParameters );
	}

	HRESULT WINAPI GetCurrentTexturePalette(          UINT *pPaletteNumber )
	{
		return m_pd3dDev->GetCurrentTexturePalette( pPaletteNumber );
	}

	HRESULT WINAPI GetDepthStencilSurface(          IDirect3DSurface9 **ppZStencilSurface )
	{
		return m_pd3dDev->GetDepthStencilSurface( ppZStencilSurface );
	}

	HRESULT WINAPI GetDeviceCaps(          D3DCAPS9 *pCaps )
	{
		return m_pd3dDev->GetDeviceCaps( pCaps );
	}

	HRESULT WINAPI GetDirect3D( IDirect3D9** ppD3D9 )
	{
		IDirect3D9* dummy;
		HRESULT ret = m_pd3dDev->GetDirect3D( &dummy );

		*ppD3D9 = &g_d3d;

		return ret;
	}

	HRESULT WINAPI GetDisplayMode(          UINT iSwapChain,
		D3DDISPLAYMODE *pMode )
	{
		return m_pd3dDev->GetDisplayMode( iSwapChain, pMode );
	}

	HRESULT WINAPI GetFrontBufferData(          UINT iSwapChain,
		IDirect3DSurface9 *pDestSurface )
	{
		return m_pd3dDev->GetFrontBufferData( iSwapChain,
			pDestSurface );
	}

	HRESULT WINAPI GetFVF(          DWORD *pFVF )
	{
		return m_pd3dDev->GetFVF( pFVF );
	}

	void WINAPI GetGammaRamp(          UINT iSwapChain,
		D3DGAMMARAMP *pRamp )
	{
		return m_pd3dDev->GetGammaRamp( iSwapChain, pRamp );
	}

	HRESULT WINAPI GetIndices(          IDirect3DIndexBuffer9 **ppIndexData )
	{
		return m_pd3dDev->GetIndices( ppIndexData );
	}

	HRESULT WINAPI GetLight(          DWORD Index,
		D3DLIGHT9 *pLight )
	{
		return m_pd3dDev->GetLight( Index, pLight );
	}

	HRESULT WINAPI GetLightEnable(          DWORD Index,
		BOOL *pEnable )
	{
		return m_pd3dDev->GetLightEnable( Index, pEnable );
	}

	HRESULT WINAPI GetMaterial(          D3DMATERIAL9 *pMaterial )
	{
		return m_pd3dDev->GetMaterial( pMaterial );
	}

	FLOAT WINAPI GetNPatchMode(VOID)
	{
		return m_pd3dDev->GetNPatchMode();
	}

	UINT WINAPI GetNumberOfSwapChains(VOID)
	{
		return m_pd3dDev->GetNumberOfSwapChains();
	}

	HRESULT WINAPI GetPaletteEntries(          UINT PaletteNumber,
		PALETTEENTRY *pEntries )
	{
		return m_pd3dDev->GetPaletteEntries( PaletteNumber, pEntries );
	}

	HRESULT WINAPI GetPixelShader(          IDirect3DPixelShader9** ppShader )
	{
		return m_pd3dDev->GetPixelShader( ppShader );
	}

	HRESULT WINAPI GetPixelShaderConstantB(          UINT StartRegister,
		BOOL *pConstantData,
		UINT BoolCount )
	{
		return m_pd3dDev->GetPixelShaderConstantB( StartRegister,
			pConstantData, BoolCount );
	}

	HRESULT WINAPI GetPixelShaderConstantF(          UINT StartRegister,
		float *pConstantData,
		UINT Vector4fCount )
	{
		return m_pd3dDev->GetPixelShaderConstantF( StartRegister,
			pConstantData, Vector4fCount );
	}

	HRESULT WINAPI GetPixelShaderConstantI(          UINT StartRegister,
		int *pConstantData,
		UINT Vector4iCount )
	{
		return m_pd3dDev->GetPixelShaderConstantI( StartRegister,
			pConstantData, Vector4iCount );
	}

	HRESULT WINAPI GetRasterStatus(          UINT iSwapChain,
		D3DRASTER_STATUS *pRasterStatus )
	{
		return m_pd3dDev->GetRasterStatus( iSwapChain, pRasterStatus );
	}

	HRESULT WINAPI GetRenderState(          D3DRENDERSTATETYPE State,
		DWORD *pValue )
	{
		return m_pd3dDev->GetRenderState( State, pValue );
	}

	HRESULT WINAPI GetRenderTarget(          DWORD RenderTargetIndex,
		IDirect3DSurface9 **ppRenderTarget )
	{
		return m_pd3dDev->GetRenderTarget( RenderTargetIndex,
			ppRenderTarget );
	}

	HRESULT WINAPI GetRenderTargetData(          IDirect3DSurface9* pRenderTarget,
		IDirect3DSurface9* pDestSurface )
	{
		return m_pd3dDev->GetRenderTargetData( pRenderTarget,
			pDestSurface );
	}

	HRESULT WINAPI GetSamplerState(          DWORD Sampler,
		D3DSAMPLERSTATETYPE Type,
		DWORD* pValue )
	{
		return m_pd3dDev->GetSamplerState( Sampler, Type, pValue );
	}

	HRESULT WINAPI GetScissorRect( RECT* pRect )
	{
		return m_pd3dDev->GetScissorRect( pRect );
	}

	BOOL WINAPI GetSoftwareVertexProcessing(VOID)
	{
		return m_pd3dDev->GetSoftwareVertexProcessing();
	}

	HRESULT WINAPI GetStreamSource(          UINT StreamNumber,
		IDirect3DVertexBuffer9 **ppStreamData,
		UINT *pOffsetInBytes,
		UINT *pStride )
	{
		return m_pd3dDev->GetStreamSource( StreamNumber, ppStreamData,
			pOffsetInBytes, pStride );
	}

	HRESULT WINAPI GetStreamSourceFreq(          UINT StreamNumber,
		UINT* pDivider )
	{
		return m_pd3dDev->GetStreamSourceFreq( StreamNumber,
			pDivider );
	}

	HRESULT WINAPI GetSwapChain(          UINT iSwapChain,
		IDirect3DSwapChain9 **ppSwapChain )
	{
		return m_pd3dDev->GetSwapChain( iSwapChain, ppSwapChain );
	}

	HRESULT WINAPI GetTexture(          DWORD Stage,
		IDirect3DBaseTexture9 **ppTexture )
	{
		return m_pd3dDev->GetTexture( Stage, ppTexture );
	}

	HRESULT WINAPI GetTextureStageState(          DWORD Stage,
		D3DTEXTURESTAGESTATETYPE Type,
		DWORD *pValue )
	{
		return m_pd3dDev->GetTextureStageState( Stage, Type, pValue );
	}

	HRESULT WINAPI GetTransform(          D3DTRANSFORMSTATETYPE State,
		D3DMATRIX *pMatrix )
	{
		return m_pd3dDev->GetTransform( State, pMatrix );
	}

	HRESULT WINAPI GetVertexDeclaration(          IDirect3DVertexDeclaration9** ppDecl )
	{
		return m_pd3dDev->GetVertexDeclaration( ppDecl );
	}

	HRESULT WINAPI GetVertexShader(          IDirect3DVertexShader9** ppShader )
	{
		return m_pd3dDev->GetVertexShader( ppShader );
	}

	HRESULT WINAPI GetVertexShaderConstantB(          UINT StartRegister,
		BOOL *pConstantData,
		UINT BoolCount )
	{
		return m_pd3dDev->GetVertexShaderConstantB( StartRegister,
			pConstantData, BoolCount );
	}

	HRESULT WINAPI GetVertexShaderConstantF(          UINT StartRegister,
		float *pConstantData,
		UINT Vector4fCount )
	{
		return m_pd3dDev->GetVertexShaderConstantF( StartRegister,
			pConstantData, Vector4fCount );
	}

	HRESULT WINAPI GetVertexShaderConstantI(          UINT StartRegister,
		int *pConstantData,
		UINT Vector4iCount )
	{
		return m_pd3dDev->GetVertexShaderConstantI( StartRegister,
			pConstantData, Vector4iCount );
	}

	HRESULT WINAPI GetViewport(          D3DVIEWPORT9 *pViewport )
	{
		return m_pd3dDev->GetViewport( pViewport );
	}

	HRESULT WINAPI LightEnable(          DWORD LightIndex,
		BOOL bEnable )
	{
		return m_pd3dDev->LightEnable( LightIndex, bEnable );
	}

	HRESULT WINAPI MultiplyTransform(          D3DTRANSFORMSTATETYPE State,
		CONST D3DMATRIX *pMatrix )
	{
		return m_pd3dDev->MultiplyTransform( State, pMatrix );
	}

	HRESULT WINAPI Present(          CONST RECT *pSourceRect,
		CONST RECT *pDestRect,
		HWND hDestWindowOverride,
		CONST RGNDATA *pDirtyRegion )
	{
		m_judgeCountPrev = m_judgeCount;
		m_judgeCount = 0;
#ifdef _DEBUG
		//std::cout << "present" << m_count << std::endl;
#endif
		HRESULT ret = m_pd3dDev->Present( pSourceRect, pDestRect,
			hDestWindowOverride, pDirtyRegion );

#ifdef _DEBUG
		QueryPerformanceCounter( &m_cur );
		float FPS = float(m_freq.QuadPart)/float(m_cur.QuadPart - m_prev.QuadPart);
		if( FPS < m_FPSNew )
			m_FPSNew = FPS;

		if( m_fpsCount % 30 == 0 )
		{
			m_FPS = m_FPSNew;
			m_FPSNew = 60.f;
		}
		m_fpsCount++;

		m_prev = m_cur;

#endif
		return ret;
	}

	HRESULT WINAPI ProcessVertices(          UINT SrcStartIndex,
		UINT DestIndex,
		UINT VertexCount,
		IDirect3DVertexBuffer9 *pDestBuffer,
		IDirect3DVertexDeclaration9* pVertexDecl,
		DWORD Flags )
	{
		return m_pd3dDev->ProcessVertices( SrcStartIndex,
			DestIndex, VertexCount, pDestBuffer, pVertexDecl,
			Flags );
	}

	HRESULT WINAPI Reset(          D3DPRESENT_PARAMETERS* pPresentationParameters )
	{
#ifdef _DEBUG
		MessageBox( NULL, _T("RESET"), NULL, 0 );
#endif
		if( m_bInitialized )
		{
			if( m_pSprite ) m_pSprite->OnLostDevice();
			if( m_pTexSurface ) m_pTexSurface->Release();
			if( m_pTex ) m_pTex->Release();
			if( m_pRenderTarget ) m_pRenderTarget->Release();
			if( m_pBackBuffer ) m_pBackBuffer->Release();
		}
		m_d3dpp = *pPresentationParameters;
		if( m_d3dpp.Windowed )
		{
			RECT rc;
			GetClientRect( m_hTH, &rc );
			m_d3dpp.BackBufferWidth = rc.right - rc.left;
			m_d3dpp.BackBufferHeight = rc.bottom - rc.top;

			//	640より小さいと画面が描画されないことがあるので修正
			if( m_d3dpp.BackBufferWidth < 640 )
			{
				m_d3dpp.BackBufferHeight = 720*m_d3dpp.BackBufferHeight/m_d3dpp.BackBufferWidth;
				m_d3dpp.BackBufferWidth = 720;
			}
		}
		else
		{
			m_d3dpp.BackBufferWidth = FullScreenWidth;
			m_d3dpp.BackBufferHeight = FullScreenHeight;
		}
		HRESULT ret = m_pd3dDev->Reset( &m_d3dpp );
		if( SUCCEEDED(ret) && m_bInitialized )
		{
			m_pSprite->OnResetDevice();
			if( FAILED( m_pd3dDev->CreateRenderTarget( 640, 480, m_d3dpp.BackBufferFormat,
				m_d3dpp.MultiSampleType, m_d3dpp.MultiSampleQuality,
				TRUE, &m_pRenderTarget, NULL ) ) )
			{
				m_pTex = NULL;
				m_pTexSurface = NULL;
				m_pBackBuffer = NULL;
				m_pRenderTarget = NULL;
			}
			if( FAILED( m_pd3dDev->CreateTexture( 640, 480, 1, 
				D3DUSAGE_RENDERTARGET, pPresentationParameters->BackBufferFormat, D3DPOOL_DEFAULT, &m_pTex, NULL ) ) )
			{
				m_pTex = NULL;
				m_pTexSurface = NULL;
				m_pBackBuffer = NULL;
				if( m_pRenderTarget ) m_pRenderTarget->Release();
				m_pRenderTarget = NULL;
			}
			else
			{
				m_pTex->GetSurfaceLevel( 0, &m_pTexSurface );
			}
			if( m_pBackBuffer ) m_pd3dDev->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer );
		}
		else
		{
			m_pTexSurface = NULL;
			m_pTex = NULL;
			m_pBackBuffer = NULL;
			m_pRenderTarget = NULL;
		}

		return ret;
	}

	HRESULT WINAPI SetClipPlane(          DWORD Index,
		const float *pPlane )
	{
		return m_pd3dDev->SetClipPlane( Index, pPlane );
	}

	HRESULT WINAPI SetClipStatus(          const D3DCLIPSTATUS9 *pClipStatus )
	{
		return m_pd3dDev->SetClipStatus( pClipStatus );
	}

	HRESULT WINAPI SetCurrentTexturePalette(          UINT PaletteNumber )
	{
		return m_pd3dDev->SetCurrentTexturePalette( PaletteNumber );
	}

	void WINAPI SetCursorPosition(          INT X,
		INT Y,
		DWORD Flags )
	{
		m_pd3dDev->SetCursorPosition( X, Y, Flags );
	}

	HRESULT WINAPI SetCursorProperties(          UINT XHotSpot,
		UINT YHotSpot,
		IDirect3DSurface9 *pCursorBitmap )
	{
		return m_pd3dDev->SetCursorProperties( XHotSpot,
			YHotSpot, pCursorBitmap );
	}

	HRESULT WINAPI SetDepthStencilSurface(          IDirect3DSurface9 *pZStencilSurface )
	{
		return m_pd3dDev->SetDepthStencilSurface( pZStencilSurface );
	}

	HRESULT WINAPI SetDialogBoxMode(          BOOL bEnableDialogs )
	{
		return m_pd3dDev->SetDialogBoxMode( bEnableDialogs );
	}

	HRESULT WINAPI SetFVF(          DWORD FVF )
	{
		return m_pd3dDev->SetFVF( FVF );
	}

	void WINAPI SetGammaRamp(          UINT iSwapChain,
		DWORD Flags,
		CONST D3DGAMMARAMP *pRamp )
	{
		m_pd3dDev->SetGammaRamp( iSwapChain, Flags, pRamp );
	}

	HRESULT WINAPI SetIndices(          IDirect3DIndexBuffer9 *pIndexData )
	{
		return m_pd3dDev->SetIndices( pIndexData );
	}

	HRESULT WINAPI SetLight(          DWORD Index,
		CONST D3DLIGHT9 *pLight )
	{
		return m_pd3dDev->SetLight( Index, pLight );
	}

	HRESULT WINAPI SetMaterial(          CONST D3DMATERIAL9 *pMaterial )
	{
		return m_pd3dDev->SetMaterial( pMaterial );
	}

	HRESULT WINAPI SetNPatchMode(          float nSegments )
	{
		return m_pd3dDev->SetNPatchMode( nSegments );
	}

	HRESULT WINAPI SetPaletteEntries(          UINT PaletteNumber,
		const PALETTEENTRY *pEntries )
	{
		return m_pd3dDev->SetPaletteEntries( PaletteNumber, pEntries );
	}

	HRESULT WINAPI SetPixelShader(          IDirect3DPixelShader9* pShader )
	{
		return m_pd3dDev->SetPixelShader( pShader );
	}

	HRESULT WINAPI SetPixelShaderConstantB(          UINT StartRegister,
		CONST BOOL *pConstantData,
		UINT BoolCount )
	{
		return m_pd3dDev->SetPixelShaderConstantB( StartRegister,
			pConstantData, BoolCount );
	}

	HRESULT WINAPI SetPixelShaderConstantF(          UINT StartRegister,
		CONST float *pConstantData,
		UINT Vector4fCount )
	{
		return m_pd3dDev->SetPixelShaderConstantF( StartRegister,
			pConstantData, Vector4fCount );
	}

	HRESULT WINAPI SetPixelShaderConstantI(          UINT StartRegister,
		CONST int *pConstantData,
		UINT Vector4iCount )
	{
		return m_pd3dDev->SetPixelShaderConstantI( StartRegister,
			pConstantData, Vector4iCount );
	}

	HRESULT WINAPI SetRenderState(          D3DRENDERSTATETYPE State,
		DWORD Value )
	{
		return m_pd3dDev->SetRenderState( State, Value );
	}

	HRESULT WINAPI SetRenderTarget(          DWORD RenderTargetIndex,
		IDirect3DSurface9 *pRenderTarget )
	{
		if( pRenderTarget == m_pBackBuffer )
			pRenderTarget = m_pRenderTarget;
		return m_pd3dDev->SetRenderTarget( RenderTargetIndex,
			pRenderTarget );
	}

	HRESULT WINAPI SetSamplerState(          DWORD Sampler,
		D3DSAMPLERSTATETYPE Type,
		DWORD Value )
	{
		return m_pd3dDev->SetSamplerState( Sampler,
			Type, Value );
	}

	HRESULT WINAPI SetScissorRect(          CONST RECT *pRect )
	{
		return m_pd3dDev->SetScissorRect( pRect );
	}

	HRESULT WINAPI SetSoftwareVertexProcessing(          BOOL bSoftware )
	{
		return m_pd3dDev->SetSoftwareVertexProcessing( bSoftware );
	}

	HRESULT WINAPI SetStreamSource(          UINT StreamNumber,
		IDirect3DVertexBuffer9 *pStreamData,
		UINT OffsetInBytes,
		UINT Stride )
	{
		return m_pd3dDev->SetStreamSource( StreamNumber,
			pStreamData, OffsetInBytes, Stride );
	}

	HRESULT WINAPI SetStreamSourceFreq(          UINT StreamNumber,
		UINT Divider )
	{
		return m_pd3dDev->SetStreamSourceFreq( StreamNumber,
			Divider );
	}

	HRESULT WINAPI SetTexture(          DWORD Stage,
		IDirect3DBaseTexture9 *pTexture )
	{
		return m_pd3dDev->SetTexture( Stage, pTexture );
	}

	HRESULT WINAPI SetTextureStageState(          DWORD Stage,
		D3DTEXTURESTAGESTATETYPE Type,
		DWORD Value )
	{
		return m_pd3dDev->SetTextureStageState( Stage, Type, Value );
	}

	HRESULT WINAPI SetTransform(          D3DTRANSFORMSTATETYPE State,
		CONST D3DMATRIX *pMatrix )
	{
		return m_pd3dDev->SetTransform( State, pMatrix );
	}

	HRESULT WINAPI SetVertexDeclaration(          IDirect3DVertexDeclaration9 *pDecl )
	{
		return m_pd3dDev->SetVertexDeclaration( pDecl );
	}

	HRESULT WINAPI SetVertexShader(          IDirect3DVertexShader9* pShader )
	{
		return m_pd3dDev->SetVertexShader( pShader );
	}

	HRESULT WINAPI SetVertexShaderConstantB(          UINT StartRegister,
		CONST BOOL *pConstantData,
		UINT BoolCount )
	{
		return m_pd3dDev->SetVertexShaderConstantB( StartRegister,
			pConstantData, BoolCount );
	}

	HRESULT WINAPI SetVertexShaderConstantF(          UINT StartRegister,
		CONST float *pConstantData,
		UINT Vector4fCount )
	{
		return m_pd3dDev->SetVertexShaderConstantF( StartRegister,
			pConstantData, Vector4fCount );
	}

	HRESULT WINAPI SetVertexShaderConstantI(          UINT StartRegister,
		CONST int *pConstantData,
		UINT Vector4iCount )
	{
		return m_pd3dDev->SetVertexShaderConstantI( StartRegister,
			pConstantData, Vector4iCount );
	}

	HRESULT WINAPI SetViewport(          CONST D3DVIEWPORT9 *pViewport )
	{
		m_judgeCount++/* = (pViewport->X == m_judgeLeft && pViewport->Y == m_judgeTop
			&& pViewport->Width == m_judgeWidth && pViewport->Height == m_judgeHeight ) ? m_judgeCount+1 : m_judgeCount*/;
#ifdef _DEBUG
		/*std::cout << std::setw(4) << pViewport->X
			<< std::setw(4) << pViewport->Y
			<< std::setw(4) << pViewport->Width
			<< std::setw(4) << pViewport->Height << std::endl;*/
#endif
		return m_pd3dDev->SetViewport( pViewport );
	}

	BOOL WINAPI ShowCursor(          BOOL bShow )
	{
		return m_pd3dDev->ShowCursor( bShow );
	}

	HRESULT WINAPI StretchRect(          IDirect3DSurface9 *pSourceSurface,
		CONST RECT *pSourceRect,
		IDirect3DSurface9 *pDestSurface,
		CONST RECT *pDestRect,
		D3DTEXTUREFILTERTYPE Filter )
	{
		return m_pd3dDev->StretchRect( pSourceSurface, pSourceRect,
			pDestSurface, pDestRect, Filter );
	}

	HRESULT WINAPI TestCooperativeLevel(VOID)
	{
		return m_pd3dDev->TestCooperativeLevel();
	}

	HRESULT WINAPI UpdateSurface(          IDirect3DSurface9* pSourceSurface,
		CONST RECT* pSourceRect,
		IDirect3DSurface9* pDestinationSurface,
		CONST POINT* pDestinationPoint )
	{
		return m_pd3dDev->UpdateSurface( pSourceSurface, pSourceRect,
			pDestinationSurface, pDestinationPoint );
	}

	HRESULT WINAPI UpdateTexture(          IDirect3DBaseTexture9 *pSourceTexture,
		IDirect3DBaseTexture9 *pDestinationTexture )
	{
		return m_pd3dDev->UpdateTexture( pSourceTexture,
			pDestinationTexture );
	}

	HRESULT WINAPI ValidateDevice(          DWORD *pNumPasses )
	{
		return m_pd3dDev->ValidateDevice( pNumPasses );
	}

	LPDIRECT3DTEXTURE9 m_pTex;
};

int MyDirect3DDevice9::ms_visID = 12345;
HHOOK MyDirect3DDevice9::ms_hHook = NULL;
int MyDirect3DDevice9::ms_refCnt = 0;
std::map<HWND,MyDirect3DDevice9*> MyDirect3DDevice9::hwnd2devMap;
std::map<HWND,HWND> MyDirect3DDevice9::th_hwnd2hwndMap;


HRESULT WINAPI MyDirect3D9::CreateDevice(          UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS *pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface )
{
	D3DPRESENT_PARAMETERS d3dpp = *pPresentationParameters;

	D3DDISPLAYMODE d3ddm;
	UINT count = m_pd3d->GetAdapterModeCount( Adapter, d3dpp.BackBufferFormat );
	std::vector<D3DDISPLAYMODE> availableModes;

	//	ディスプレイの最大解像度を探す＆60Hzの解像度を列挙
	for( UINT i = 0; i < count; ++i )
	{
		m_pd3d->EnumAdapterModes( Adapter, d3dpp.BackBufferFormat, i, &d3ddm );
		if( d3ddm.RefreshRate == 60 )
		{
			availableModes.push_back( d3ddm );
			if( FullScreenWidth*FullScreenHeight < d3ddm.Width*d3ddm.Height )
			{
				FullScreenWidth = d3ddm.Width;
				FullScreenHeight = d3ddm.Height;
			}
		}
	}

	//	最大解像度と同じアスペクト比で、できるだけ低い解像度を選ぶ
	for( std::vector<D3DDISPLAYMODE>::const_iterator itr = availableModes.cbegin();
		 itr != availableModes.cend(); ++itr )
	{
		if( FullScreenWidth*itr->Height == FullScreenHeight*itr->Width &&
			FullScreenWidth > itr->Width && FullScreenHeight > itr->Height &&
			itr->Width >= 640 && itr->Height >= 640 )
		{
			FullScreenWidth = itr->Width;
			FullScreenHeight = itr->Height;
		}
	}

	if( pPresentationParameters->Windowed == FALSE )
	{
		d3dpp.BackBufferHeight = FullScreenHeight;
		d3dpp.BackBufferWidth = FullScreenWidth;
	}
	else
	{
		RECT rc;
		GetClientRect( GetActiveWindow(), &rc );
		d3dpp.BackBufferWidth = rc.right - rc.left;
		d3dpp.BackBufferHeight = rc.bottom - rc.top;
		if( d3dpp.BackBufferWidth < 640 )
		{
			d3dpp.BackBufferHeight = 720*d3dpp.BackBufferHeight/d3dpp.BackBufferWidth;
			d3dpp.BackBufferWidth = 720;
		}
	}
	LPDIRECT3DDEVICE9 pd3dDev;
	HRESULT ret = m_pd3d->CreateDevice( Adapter,
		DeviceType, hFocusWindow, BehaviorFlags,
		&d3dpp, &pd3dDev );

	if( FAILED(ret) ) return ret;

	MyDirect3DDevice9 dev;
	m_devices[pd3dDev] = dev;
	m_devices[pd3dDev].m_d3dpp = d3dpp;
	if( !m_devices[pd3dDev].init( pd3dDev, pPresentationParameters->BackBufferFormat ) )
	{
		m_devices[pd3dDev].Release();
		m_devices.erase(pd3dDev);
		return E_FAIL;
	}

	*ppReturnedDeviceInterface = &m_devices[pd3dDev];

	return ret;
}

extern "C" {
__declspec( naked ) void WINAPI d_Direct3DShaderValidatorCreate9() { _asm{ jmp p_Direct3DShaderValidatorCreate9 } }
__declspec( naked ) void WINAPI d_PSGPError() { _asm{ jmp p_PSGPError } }
__declspec( naked ) void WINAPI d_PSGPSampleTexture() { _asm{ jmp p_PSGPSampleTexture } }
__declspec( naked ) void WINAPI d_D3DPERF_BeginEvent() { _asm{ jmp p_D3DPERF_BeginEvent } }
__declspec( naked ) void WINAPI d_D3DPERF_EndEvent() { _asm{ jmp p_D3DPERF_EndEvent } }
__declspec( naked ) void WINAPI d_D3DPERF_GetStatus() { _asm{ jmp p_D3DPERF_GetStatus } }
__declspec( naked ) void WINAPI d_D3DPERF_QueryRepeatFrame() { _asm{ jmp p_D3DPERF_QueryRepeatFrame } }
__declspec( naked ) void WINAPI d_D3DPERF_SetMarker() { _asm{ jmp p_D3DPERF_SetMarker } }
__declspec( naked ) void WINAPI d_D3DPERF_SetOptions() { _asm{ jmp p_D3DPERF_SetOptions } }
__declspec( naked ) void WINAPI d_D3DPERF_SetRegion() { _asm{ jmp p_D3DPERF_SetRegion } }
__declspec( naked ) void WINAPI d_DebugSetLevel() { _asm{ jmp p_DebugSetLevel } }
__declspec( naked ) void WINAPI d_DebugSetMute() { _asm{ jmp p_DebugSetMute } }

__declspec( dllexport ) IDirect3D9* WINAPI d_Direct3DCreate9( UINT v )
{
	if( g_d3d.init(v) )
		return &g_d3d;
	else
		return NULL;
}

__declspec( naked ) void WINAPI d_Direct3DCreate9Ex() { _asm{ jmp p_Direct3DCreate9Ex } }
}

HINSTANCE h_original;
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch(ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		FullScreenWidth = GetSystemMetrics( SM_CXSCREEN );
		FullScreenHeight = GetSystemMetrics( SM_CYSCREEN );
		{
			INITCOMMONCONTROLSEX InitCtrls;
			InitCtrls.dwSize = sizeof(InitCtrls);
			// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
			// これを設定します。
			InitCtrls.dwICC = ICC_WIN95_CLASSES;
			InitCommonControlsEx(&InitCtrls);
		}

		g_hModule = hModule;
		{
			TCHAR sysDir[MAX_PATH];
			GetSystemDirectory( sysDir, MAX_PATH );
			TCHAR ch = sysDir[lstrlen(sysDir)-1];
			if( ch != '\\' || ch != '/' )
			{
				sysDir[lstrlen(sysDir)+1] = '\0';
				sysDir[lstrlen(sysDir)] = '\\';
			}
			lstrcat( sysDir, "d3d9.dll" );
			h_original = LoadLibrary(sysDir);
		}
        if(h_original == NULL)
            return FALSE;
        p_Direct3DShaderValidatorCreate9 = GetProcAddress(h_original, "Direct3DShaderValidatorCreate9");
        p_PSGPError = GetProcAddress(h_original, "PSGPError");
        p_PSGPSampleTexture = GetProcAddress(h_original, "PSGPSampleTexture");
        p_D3DPERF_BeginEvent = GetProcAddress(h_original, "D3DPERF_BeginEvent");
        p_D3DPERF_EndEvent = GetProcAddress(h_original, "D3DPERF_EndEvent");
        p_D3DPERF_GetStatus = GetProcAddress(h_original, "D3DPERF_GetStatus");
        p_D3DPERF_QueryRepeatFrame = GetProcAddress(h_original, "D3DPERF_QueryRepeatFrame");
        p_D3DPERF_SetMarker = GetProcAddress(h_original, "D3DPERF_SetMarker");
        p_D3DPERF_SetOptions = GetProcAddress(h_original, "D3DPERF_SetOptions");
        p_D3DPERF_SetRegion = GetProcAddress(h_original, "D3DPERF_SetRegion");
        p_DebugSetLevel = GetProcAddress(h_original, "DebugSetLevel");
        p_DebugSetMute = GetProcAddress(h_original, "DebugSetMute");
        p_Direct3DCreate9 = reinterpret_cast<DIRECT3DCREATE9PROC>(GetProcAddress(h_original, "Direct3DCreate9"));
        p_Direct3DCreate9Ex = GetProcAddress(h_original, "Direct3DCreate9Ex");
#ifdef _DEBUG
		if( AllocConsole() )
		{
			/*freopen( "CONOUT$", "w", stdout );
			freopen( "CONOUT$", "w", stderr );
			freopen( "CONIN$", "r", stdin );*/
		}

		MessageBox( NULL, _T("PIPE"), NULL, 0 );
#endif
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        FreeLibrary(h_original);
        break;
    default:
        break;
    }
    return TRUE;
}
