/*
Sessions Management plugin for Miranda IM

Copyright (C) 2007-2008 Danil Mozhar

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

HGENMENU hmSaveCurrentSession;

HANDLE hmTBButton[2], hiTBbutton[2], iTBbutton[2];

bool g_hghostw;

HWND hClistControl;

int g_ses_limit;
int g_ses_count;
bool g_bExclHidden;
bool g_bWarnOnHidden;
bool g_bOtherWarnings;
bool g_bCrashRecovery;
bool g_bIncompletedSave;

HWND g_hDlg;
HWND g_hSDlg;
bool DONT = false;
bool StartUp, isLastTRUE = false, bSC = false;

MCONTACT session_list[255] = { 0 };
MCONTACT user_session_list[255] = { 0 };
MCONTACT session_list_recovered[255];

CMPlugin g_plugin;

int OptionsInit(WPARAM, LPARAM);

IconItem iconList[] =
{
	{ LPGEN("Sessions"), "Sessions", IDD_SESSION_CHECKED },
	{ LPGEN("Favorite Session"), "SessionMarked", IDD_SESSION_CHECKED },
	{ LPGEN("Not favorite Session"), "SessionUnMarked", IDD_SESSION_UNCHECKED },
	{ LPGEN("Load Session"), "SessionsLoad", IDI_SESSIONS_LOAD },
	{ LPGEN("Save Session"), "SessionsSave", IDD_SESSIONS_SAVE },
	{ LPGEN("Load last Session"), "SessionsLoadLast", IDD_SESSIONS_LOADLAST }
};

/////////////////////////////////////////////////////////////////////////////////////////

PLUGININFOEX pluginInfoEx =
{
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {60558872-2AAB-45AA-888D-097691C9B683}
	{ 0x60558872, 0x2aab, 0x45aa, { 0x88, 0x8d, 0x9, 0x76, 0x91, 0xc9, 0xb6, 0x83 } }
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(MODULENAME, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ExitDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		LoadPosition(hdlg, "ExitDlg");
		ShowWindow(hdlg, SW_SHOW);
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDOK:
			SavePosition(hdlg, "ExitDlg");
			SaveSessionDate();
			SaveSessionHandles(0, 0);
			db_set_b(NULL, MODULENAME, "lastempty", 0);
			DestroyWindow(hdlg);
			break;

		case IDCANCEL:
			SavePosition(hdlg, "ExitDlg");
			db_set_b(NULL, MODULENAME, "lastempty", 1);
			DestroyWindow(hdlg);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hdlg);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK SaveSessionDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
	g_hSDlg = hdlg;
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		LoadSessionToCombobox(hdlg, 1, 5, "UserSessionDsc", 0);

		LoadPosition(hdlg, "SaveDlg");
		ShowWindow(hdlg, SW_SHOW);
		break;

	case WM_CLOSE:
		DestroyWindow(hdlg);
		g_hSDlg = nullptr;
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lparam)->idFrom) {
		case IDC_CLIST:
			switch (((LPNMHDR)lparam)->code) {
			case CLN_CHECKCHANGED:
				bSC = TRUE;
				memcpy(user_session_list, session_list, sizeof(user_session_list));
				break;
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_SELCONTACTS:
			HANDLE hItem;
			RECT rWnd;
			{
				int i = 0, x = 0, y = 0, dy = 0, dx = 0, dd = 0;

				GetWindowRect(hdlg, &rWnd);
				x = rWnd.right - rWnd.left;
				y = rWnd.bottom - rWnd.top;
				if (IsDlgButtonChecked(hdlg, IDC_SELCONTACTS)) {
					EnableWindow(GetDlgItem(hdlg, IDC_SANDCCHECK), FALSE);
					dy = 20;
					dx = 150;
					dd = 5;
					hClistControl = CreateWindowEx(WS_EX_CLIENTEDGE, CLISTCONTROL_CLASSW, L"",
						WS_TABSTOP | WS_VISIBLE | WS_CHILD,
						x, y, dx, dy, hdlg, (HMENU)IDC_CLIST, g_plugin.getInst(), nullptr);

					SetWindowLongPtr(hClistControl, GWL_STYLE,
						GetWindowLongPtr(hClistControl, GWL_STYLE) | CLS_CHECKBOXES | CLS_HIDEEMPTYGROUPS | CLS_USEGROUPS | CLS_GREYALTERNATE | CLS_GROUPCHECKBOXES);
					SendMessage(hClistControl, CLM_SETEXSTYLE, CLS_EX_DISABLEDRAGDROP | CLS_EX_TRACKSELECT, 0);
				}
				else {
					EnableWindow(GetDlgItem(hdlg, IDC_SANDCCHECK), TRUE);
					dy = -20;
					dx = -150;
					dd = 5;
					DestroyWindow(hClistControl);
				}

				SetWindowPos(hdlg, nullptr, rWnd.left, rWnd.top, x + dx, y + (dx / 3), SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);

				SetWindowPos(hClistControl, nullptr, x - dd, dd, dx - dd, y + (dx / 12), SWP_NOZORDER/*|SWP_NOSIZE|SWP_SHOWWINDOW*/);
				SendMessage(hClistControl, CLM_AUTOREBUILD, 0, 0);

				for (i = 0; session_list[i] > 0; i++) {
					hItem = (HANDLE)SendMessage(hClistControl, CLM_FINDCONTACT, (WPARAM)session_list[i], 0);
					SendMessage(hClistControl, CLM_SETCHECKMARK, (WPARAM)hItem, 1);
				}

				OffsetWindow(hdlg, GetDlgItem(hdlg, IDC_LIST), 0, dy);
				OffsetWindow(hdlg, GetDlgItem(hdlg, IDC_STATIC), 0, dy);
				OffsetWindow(hdlg, GetDlgItem(hdlg, IDC_SANDCCHECK), 0, dy);
				OffsetWindow(hdlg, GetDlgItem(hdlg, IDOK), 0, dy);
				OffsetWindow(hdlg, GetDlgItem(hdlg, IDCANCEL), 0, dy);
			}
			break;

		case IDOK:
			wchar_t szUserSessionName[MAX_PATH];
			{
				DWORD session_list_temp[255] = { 0 };
				int i = 0, length = GetWindowTextLength(GetDlgItem(hdlg, IDC_LIST));
				SavePosition(hdlg, "SaveDlg");
				if (length > 0) {
					GetDlgItemText(hdlg, IDC_LIST, szUserSessionName, _countof(szUserSessionName));
					szUserSessionName[length + 1] = '\0';
					if (IsDlgButtonChecked(hdlg, IDC_SELCONTACTS) && bSC) {
						for (auto &hContact : Contacts()) {
							BYTE res = (BYTE)SendMessage(hClistControl, CLM_GETCHECKMARK, SendMessage(hClistControl, CLM_FINDCONTACT, hContact, 0), 0);
							if (res) {
								user_session_list[i] = hContact;
								i++;
							}
						}
						memcpy(session_list_temp, session_list, sizeof(session_list_temp));
						memcpy(session_list, user_session_list, sizeof(session_list));
						SaveSessionHandles(0, 1);
						SaveUserSessionName(szUserSessionName);
						memcpy(session_list, session_list_temp, sizeof(session_list));
						DestroyWindow(hdlg);
						g_hSDlg = nullptr;
					}
					else if (!SaveUserSessionName(szUserSessionName)) {
						SaveSessionHandles(0, 1);

						if (IsDlgButtonChecked(hdlg, IDC_SANDCCHECK))
							CloseCurrentSession(0, 0);
						DestroyWindow(hdlg);
						g_hSDlg = nullptr;
					}
					else MessageBox(nullptr, TranslateT("Current session is empty!"), TranslateT("Sessions Manager"), MB_OK | MB_ICONWARNING);
				}
				else MessageBox(nullptr, TranslateT("Session name is empty, enter the name and try again"), TranslateT("Sessions Manager"), MB_OK | MB_ICONWARNING);
			}
			break;

		case IDCANCEL:
			SavePosition(hdlg, "SaveDlg");
			DestroyWindow(hdlg);
			g_hSDlg = nullptr;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK LoadSessionDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM)
{
	static int ses_count;

	g_hDlg = hdlg;
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		{
			int iDelay = db_get_w(NULL, MODULENAME, "StartupModeDelay", 1500);
			if (g_hghostw == TRUE)
				SetTimer(hdlg, TIMERID_LOAD, iDelay, nullptr);
			else {
				if ((ses_count = LoadSessionToCombobox(hdlg, 0, g_ses_limit, "SessionDate", 0)) == g_ses_limit)
					EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), TRUE);

				if (LoadSessionToCombobox(hdlg, 0, 255, "UserSessionDsc", g_ses_limit) == 0 && ses_count != 0)
					ses_count = 0;

				if (session_list_recovered[0])
					ses_count = 256;

				SendDlgItemMessage(hdlg, IDC_LIST, CB_SETCURSEL, 0, 0);
				LoadPosition(hdlg, "LoadDlg");
				if (StartUp)
					SetTimer(hdlg, TIMERID_SHOW, iDelay, nullptr);
				else
					ShowWindow(g_hDlg, SW_SHOW);
			}
		}
		break;

	case WM_TIMER:
		if (wparam == TIMERID_SHOW) {
			KillTimer(hdlg, TIMERID_SHOW);
			ShowWindow(hdlg, SW_SHOW);
			StartUp = FALSE;
		}
		else {
			KillTimer(hdlg, TIMERID_LOAD);
			LoadSession(0, g_bIncompletedSave ? 256 : 0);
			g_hghostw = StartUp = FALSE;
			DestroyWindow(hdlg);
			g_hDlg = nullptr;
		}
		break;

	case WM_CLOSE:
		SavePosition(hdlg, "LoadDlg");
		DestroyWindow(hdlg);
		g_hDlg = nullptr;
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_LIST:
			switch (HIWORD(wparam)) {
			case CBN_SELCHANGE:
				HWND hCombo = GetDlgItem(hdlg, IDC_LIST);
				int index = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
				if (index != CB_ERR)
					ses_count = SendMessage(hCombo, CB_GETITEMDATA, (WPARAM)index, 0);
			}
			break;

		case IDC_SESSDEL:
			if (session_list_recovered[0] && ses_count == 256) {
				for (int i = 0; session_list_recovered[i]; i++)
					db_set_b(session_list_recovered[i], MODULENAME, "wasInLastSession", 0);

				memset(session_list_recovered, 0, sizeof(session_list_recovered));
				g_bIncompletedSave = 0;

				EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), FALSE);
				SendDlgItemMessage(hdlg, IDC_LIST, CB_RESETCONTENT, 0, 0);

				if ((ses_count = LoadSessionToCombobox(hdlg, 1, g_ses_limit, "SessionDate", 0)) == g_ses_limit)
					EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), TRUE);

				if (LoadSessionToCombobox(hdlg, 1, 255, "UserSessionDsc", g_ses_limit) == 0 && ses_count != 0)
					ses_count = 0;

				SendDlgItemMessage(hdlg, IDC_LIST, CB_SETCURSEL, 0, 0);

			}
			else if (ses_count >= g_ses_limit) {
				ses_count -= g_ses_limit;
				DelUserDefSession(ses_count);
				EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), FALSE);
				SendDlgItemMessage(hdlg, IDC_LIST, CB_RESETCONTENT, 0, 0);

				if ((ses_count = LoadSessionToCombobox(hdlg, 0, g_ses_limit, "SessionDate", 0)) == g_ses_limit)
					EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), TRUE);

				if (LoadSessionToCombobox(hdlg, 0, 255, "UserSessionDsc", g_ses_limit) == 0 && ses_count != 0)
					ses_count = 0;

				if (session_list_recovered[0])
					ses_count = 256;

				SendDlgItemMessage(hdlg, IDC_LIST, CB_SETCURSEL, 0, 0);
			}
			else {
				DeleteAutoSession(ses_count);
				EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), FALSE);
				SendDlgItemMessage(hdlg, IDC_LIST, CB_RESETCONTENT, 0, 0);
				if ((ses_count = LoadSessionToCombobox(hdlg, 0, g_ses_limit, "SessionDate", 0)) == g_ses_limit)
					EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), TRUE);

				if (LoadSessionToCombobox(hdlg, 0, 255, "UserSessionDsc", g_ses_limit) == 0 && ses_count != 0)
					ses_count = 0;

				if (session_list_recovered[0])
					ses_count = 256;

				SendDlgItemMessage(hdlg, IDC_LIST, CB_SETCURSEL, 0, 0);
			}
			if (SendDlgItemMessage(hdlg, IDC_LIST, CB_GETCOUNT, 0, 0))
				EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), TRUE);
			else
				EnableWindow(GetDlgItem(hdlg, IDC_SESSDEL), FALSE);
			break;

		case IDOK:
			if (!LoadSession(0, ses_count)) {
				SavePosition(hdlg, "LoadDlg");
				DestroyWindow(hdlg);
				g_hDlg = nullptr;
			}
			break;

		case IDCANCEL:
			SavePosition(hdlg, "LoadDlg");
			DestroyWindow(hdlg);
			g_hDlg = nullptr;
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CloseCurrentSession(WPARAM, LPARAM)
{
	while (session_list[0] != 0) {
		MessageWindowData mwd;
		Srmm_GetWindowData(session_list[0], mwd);
		SendMessage(mwd.hwndWindow, WM_CLOSE, 0, 0);
	}
	memset(session_list, 0, sizeof(session_list));
	return 0;
}

int SaveSessionHandles(WPARAM, LPARAM lparam)
{
	if (session_list[0] == 0)
		return 1;

	int k = 0;
	for (auto &hContact : Contacts()) {
		if ((k = CheckForDuplicate(session_list, hContact)) != -1 && !(g_bExclHidden && !CheckContactVisibility(hContact))) {
			AddSessionMark(hContact, lparam, '1');
			AddInSessionOrder(hContact, lparam, k, 1);
		}
		else {
			AddSessionMark(hContact, lparam, '0');
			AddInSessionOrder(hContact, lparam, 0, 0);
		}
	}
	if (lparam == 1) {
		g_ses_count++;
		db_set_b(0, MODULENAME, "UserSessionsCount", (BYTE)g_ses_count);
	}
	return 0;
}

INT_PTR SaveUserSessionHandles(WPARAM, LPARAM)
{
	if (g_hSDlg) {
		ShowWindow(g_hSDlg, SW_SHOW);
		return 1;
	}

	g_hSDlg = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_SAVEDIALOG), nullptr, SaveSessionDlgProc);
	return 0;
}

INT_PTR OpenSessionsManagerWindow(WPARAM, LPARAM)
{
	if (g_hDlg) {
		ShowWindow(g_hDlg, SW_SHOW);
		return 0;
	}

	ptrW
		tszSession(db_get_wsa(NULL, MODULENAME, "SessionDate_0")),
		tszUserSession(db_get_wsa(NULL, MODULENAME, "UserSessionDsc_0"));
	if (g_bIncompletedSave || tszSession || tszUserSession) {
		g_hDlg = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_WLCMDIALOG), nullptr, LoadSessionDlgProc);
		return 0;
	}
	if (g_bOtherWarnings)
		MessageBox(nullptr, TranslateT("No sessions to open"), TranslateT("Sessions Manager"), MB_OK | MB_ICONWARNING);
	return 1;
}

int SaveSessionDate()
{
	if (session_list[0] != 0) {
		int TimeSize = GetTimeFormat(LOCALE_USER_DEFAULT, 0/*TIME_NOSECONDS*/, nullptr, nullptr, nullptr, 0);
		wchar_t *szTimeBuf = (wchar_t*)mir_alloc((TimeSize + 1)*sizeof(wchar_t));

		GetTimeFormat(LOCALE_USER_DEFAULT, 0/*TIME_NOSECONDS*/, nullptr, nullptr, szTimeBuf, TimeSize);

		int DateSize = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, nullptr, 0);
		wchar_t *szDateBuf = (wchar_t*)mir_alloc((DateSize + 1)*sizeof(wchar_t));

		GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, szDateBuf, DateSize);
		int lenn = (DateSize + TimeSize + 5);
		wchar_t *szSessionTime = (wchar_t*)mir_alloc(lenn*sizeof(wchar_t));
		mir_snwprintf(szSessionTime, lenn, L"%s - %s", szTimeBuf, szDateBuf);

		char szSetting[256];
		mir_snprintf(szSetting, "%s_%d", "SessionDate", 0);
		wchar_t *ptszSaveSessionDate = db_get_wsa(NULL, MODULENAME, szSetting);

		db_set_ws(NULL, MODULENAME, szSetting, szSessionTime);
		mir_free(szSessionTime);

		if (ptszSaveSessionDate)
			ResaveSettings("SessionDate", 1, g_ses_limit, ptszSaveSessionDate);

		if (szTimeBuf)
			mir_free(szTimeBuf);
		if (szDateBuf)
			mir_free(szDateBuf);
	}
	if (g_bCrashRecovery)
		db_set_b(NULL, MODULENAME, "lastSaveCompleted", 1);
	return 0;
}

int SaveUserSessionName(wchar_t *szUSessionName)
{
	if (session_list[0] == 0)
		return 1;

	char szSetting[256];
	mir_snprintf(szSetting, "%s_%u", "UserSessionDsc", 0);
	wchar_t *ptszUserSessionName = db_get_wsa(NULL, MODULENAME, szSetting);
	if (ptszUserSessionName)
		ResaveSettings("UserSessionDsc", 1, 255, ptszUserSessionName);

	db_set_ws(NULL, MODULENAME, szSetting, szUSessionName);
	return 0;
}

INT_PTR LoadLastSession(WPARAM wparam, LPARAM lparam)
{
	if (isLastTRUE)
		return LoadSession(wparam, lparam);
	if (g_bOtherWarnings)
		MessageBox(nullptr, TranslateT("Last Sessions is empty"), TranslateT("Sessions Manager"), MB_OK);
	return 0;
}

int LoadSession(WPARAM, LPARAM lparam)
{
	int dup = 0;
	int hidden[255] = { '0' };
	MCONTACT session_list_t[255] = { 0 };
	int mode = 0;
	if ((int)lparam >= g_ses_limit && lparam != 256) {
		mode = 1;
		lparam -= g_ses_limit;
	}
	if (session_list_recovered[0] && lparam == 256 && mode == 0)
		memcpy(session_list_t, session_list_recovered, sizeof(session_list_t));
	else
		for (auto &hContact : Contacts())
			if (LoadContactsFromMask(hContact, mode, lparam)) {
				int i = GetInSessionOrder(hContact, mode, lparam);
				session_list_t[i] = hContact;
			}

	int i = 0, j = 0;
	// TODO: change to "switch"
	while (session_list_t[i] != 0) {
		if (CheckForDuplicate(session_list, session_list_t[i]) == -1)
			Clist_ContactDoubleClicked(session_list_t[i]);
		else if (g_bWarnOnHidden) {
			if (!CheckContactVisibility(session_list_t[i])) {
				hidden[j] = i + 1;
				j++;
			}
			dup++;
		}
		i++;
	}

	if (i == 0) {
		if (g_bOtherWarnings)
			MessageBox(nullptr, TranslateT("No contacts to open"), TranslateT("Sessions Manager"), MB_OK | MB_ICONWARNING);
		return 1;
	}

	if (dup == i) {
		if (!hidden[i]) {
			if (g_bOtherWarnings)
				MessageBox(nullptr, TranslateT("This Session already opened"), TranslateT("Sessions Manager"), MB_OK | MB_ICONWARNING);
			return 1;
		}
		if (!g_bWarnOnHidden && g_bOtherWarnings) {
			MessageBox(nullptr, TranslateT("This Session already opened"), TranslateT("Sessions Manager"), MB_OK | MB_ICONWARNING);
			return 1;
		}
		if (g_bWarnOnHidden) {
			if (IDYES == MessageBox(nullptr, TranslateT("This Session already opened (but probably hidden).\nDo you want to show hidden contacts?"), TranslateT("Sessions Manager"), MB_YESNO | MB_ICONQUESTION))
				for (j = 0; hidden[j] != 0; j++)
					Clist_ContactDoubleClicked(session_list_t[hidden[j] - 1]);
		}
	}

	return 0;
}

int DelUserDefSession(int ses_count)
{
	for (auto &hContact : Contacts()) {
		RemoveSessionMark(hContact, 1, ses_count);
		SetInSessionOrder(hContact, 1, ses_count, 0);
	}

	char szSessionName[256];
	mir_snprintf(szSessionName, "%s_%u", "UserSessionDsc", ses_count);
	db_unset(NULL, MODULENAME, szSessionName);

	mir_snprintf(szSessionName, "%s_%u", "FavUserSession", ses_count);
	db_unset(NULL, MODULENAME, szSessionName);

	for (int i = ses_count + 1;; i++) {
		mir_snprintf(szSessionName, "%s_%u", "UserSessionDsc", i);
		ptrW szSessionNameBuf(db_get_wsa(NULL, MODULENAME, szSessionName));

		mir_snprintf(szSessionName, "%s_%u", "UserSessionDsc", i - 1);
		if (szSessionNameBuf) {
			MarkUserDefSession(i - 1, IsMarkedUserDefSession(i));
			db_set_ws(NULL, MODULENAME, szSessionName, szSessionNameBuf);
		}
		else {
			db_unset(NULL, MODULENAME, szSessionName);

			mir_snprintf(szSessionName, "%s_%u", "FavUserSession", i - 1);
			db_unset(NULL, MODULENAME, szSessionName);
			break;
		}
	}
	g_ses_count--;
	db_set_b(0, MODULENAME, "UserSessionsCount", (BYTE)g_ses_count);
	return 0;
}

int DeleteAutoSession(int ses_count)
{
	for (auto &hContact : Contacts()) {
		RemoveSessionMark(hContact, 0, ses_count);
		SetInSessionOrder(hContact, 0, ses_count, 0);
	}

	char szSessionName[256];
	mir_snprintf(szSessionName, "%s_%u", "SessionDate", ses_count);
	db_unset(NULL, MODULENAME, szSessionName);

	for (int i = ses_count + 1;; i++) {
		mir_snprintf(szSessionName, "%s_%u", "SessionDate", i);
		ptrW szSessionNameBuf(db_get_wsa(NULL, MODULENAME, szSessionName));

		mir_snprintf(szSessionName, "%s_%u", "SessionDate", i - 1);
		if (szSessionNameBuf)
			db_set_ws(NULL, MODULENAME, szSessionName, szSessionNameBuf);
		else {
			db_unset(NULL, MODULENAME, szSessionName);
			break;
		}
	}

	return 0;
}

int SessionPreShutdown(WPARAM, LPARAM)
{
	DONT = 1;

	if (g_hDlg)  DestroyWindow(g_hDlg);
	if (g_hSDlg) DestroyWindow(g_hSDlg);

	db_set_b(NULL, MODULENAME, "lastSaveCompleted", 1);
	return 0;
}

int OkToExit(WPARAM, LPARAM)
{
	int exitmode = db_get_b(NULL, MODULENAME, "ShutdownMode", 2);
	DONT = 1;
	if (exitmode == 2 && session_list[0] != 0) {
		SaveSessionDate();
		SaveSessionHandles(0, 0);
		db_set_b(NULL, MODULENAME, "lastempty", 0);
	}
	else if (exitmode == 1 && session_list[0] != 0) {
		DialogBox(g_plugin.getInst(), MAKEINTRESOURCE(IDD_EXDIALOG), nullptr, ExitDlgProc);
	}
	else db_set_b(NULL, MODULENAME, "lastempty", 1);
	return 0;
}

static int OnSrmmWindowEvent(WPARAM, LPARAM lParam)
{
	MessageWindowEventData *MWeventdata = (MessageWindowEventData*)lParam;
	if (MWeventdata->uType == MSG_WINDOW_EVT_OPEN) {
		AddToCurSession(MWeventdata->hContact, 0);
		if (g_bCrashRecovery)
			db_set_b(MWeventdata->hContact, MODULENAME, "wasInLastSession", 1);
	}
	else if (MWeventdata->uType == MSG_WINDOW_EVT_CLOSE) {
		if (!DONT)
			DelFromCurSession(MWeventdata->hContact, 0);
		if (g_bCrashRecovery)
			db_set_b(MWeventdata->hContact, MODULENAME, "wasInLastSession", 0);
	}

	return 0;
}

INT_PTR BuildFavMenu(WPARAM, LPARAM)
{
	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();
	FillFavoritesMenu(hMenu, g_ses_count);
	int res = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, GetActiveWindow(), nullptr);
	if (res == 0) return 1;
	LoadSession(0, (res - 1) + g_ses_limit);
	return 0;
}

static int CreateButtons(WPARAM, LPARAM)
{
	TTBButton ttb = {};
	ttb.dwFlags = TTBBF_SHOWTOOLTIP | TTBBF_VISIBLE;

	ttb.pszService = MS_SESSIONS_OPENMANAGER;
	ttb.pszTooltipUp = ttb.name = LPGEN("Open Sessions Manager");
	ttb.hIconHandleUp = iconList[3].hIcolib;
	g_plugin.addTTB(&ttb);

	ttb.pszService = MS_SESSIONS_SAVEUSERSESSION;
	ttb.pszTooltipUp = ttb.name = LPGEN("Save Session");
	ttb.hIconHandleUp = iconList[4].hIcolib;
	g_plugin.addTTB(&ttb);

	ttb.pszService = MS_SESSIONS_RESTORELASTSESSION;
	ttb.pszTooltipUp = ttb.name = LPGEN("Restore Last Session");
	ttb.hIconHandleUp = iconList[5].hIcolib;
	g_plugin.addTTB(&ttb);

	ttb.pszService = MS_SESSIONS_SHOWFAVORITESMENU;
	ttb.pszTooltipUp = ttb.name = LPGEN("Show Favorite Sessions Menu");
	ttb.hIconHandleUp = iconList[1].hIcolib;
	g_plugin.addTTB(&ttb);
	return 0;
}

static void CALLBACK LaunchSessions()
{
	int startup = db_get_b(NULL, MODULENAME, "StartupMode", 3);
	if (startup == 1 || (startup == 3 && isLastTRUE == TRUE)) {
		StartUp = TRUE;
		g_hDlg = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_WLCMDIALOG), nullptr, LoadSessionDlgProc);
	}
	else if (startup == 2 && isLastTRUE == TRUE) {
		g_hghostw = TRUE;
		g_hDlg = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_WLCMDIALOG), nullptr, LoadSessionDlgProc);
	}
}

static int PluginInit(WPARAM, LPARAM)
{
	HookEvent(ME_MSG_WINDOWEVENT, OnSrmmWindowEvent);
	HookEvent(ME_OPT_INITIALISE, OptionsInit);
	HookEvent(ME_TTB_MODULELOADED, CreateButtons);

	// Hotkeys
	HOTKEYDESC hkd = {};
	hkd.pszName = "OpenSessionsManager";
	hkd.szSection.a = LPGEN("Sessions");
	hkd.szDescription.a = LPGEN("Open Sessions Manager");
	hkd.pszService = MS_SESSIONS_OPENMANAGER;
	g_plugin.addHotkey(&hkd);

	hkd.pszName = "RestoreLastSession";
	hkd.szDescription.a = LPGEN("Restore last Session");
	hkd.pszService = MS_SESSIONS_RESTORELASTSESSION;
	g_plugin.addHotkey(&hkd);

	hkd.pszName = "SaveSession";
	hkd.szDescription.a = LPGEN("Save Session");
	hkd.pszService = MS_SESSIONS_SAVEUSERSESSION;
	g_plugin.addHotkey(&hkd);

	hkd.pszName = "CloseSession";
	hkd.szDescription.a = LPGEN("Close Session");
	hkd.pszService = MS_SESSIONS_CLOSESESSION;
	g_plugin.addHotkey(&hkd);

	// Main menu
	CMenuItem mi(&g_plugin);
	mi.position = 1000000000;
	mi.root = g_plugin.addRootMenu(MO_MAIN, LPGENW("Sessions Manager"), 1000000000);
	Menu_ConfigureItem(mi.root, MCI_OPT_UID, "D77B9AB4-AF7E-43DB-A487-BD581704D635");

	SET_UID(mi, 0xd35302fa, 0x8326, 0x4323, 0xa3, 0xe5, 0xb4, 0x41, 0xff, 0xfb, 0xaa, 0x2d);
	mi.name.a = LPGEN("Save session...");
	mi.hIcolibItem = iconList[4].hIcolib;
	mi.pszService = MS_SESSIONS_SAVEUSERSESSION;
	hmSaveCurrentSession = Menu_AddMainMenuItem(&mi);

	SET_UID(mi, 0x8de4d8b1, 0x9a62, 0x4f4e, 0xb0, 0x3d, 0x99, 0x7, 0x80, 0xe8, 0x93, 0xc2);
	mi.name.a = LPGEN("Load session...");
	mi.pszService = MS_SESSIONS_OPENMANAGER;
	mi.hIcolibItem = iconList[3].hIcolib;
	Menu_AddMainMenuItem(&mi);

	SET_UID(mi, 0x73ea91d6, 0xb7e5, 0x4f67, 0x96, 0x96, 0xa, 0x24, 0x21, 0x48, 0x6f, 0x15);
	mi.name.a = LPGEN("Close session");
	mi.pszService = MS_SESSIONS_CLOSESESSION;
	mi.hIcolibItem = nullptr;
	Menu_AddMainMenuItem(&mi);

	SET_UID(mi, 0xe2c4e4ba, 0x5d08, 0x441b, 0xb5, 0x93, 0xc4, 0xe7, 0x9a, 0xfb, 0xa4, 0x6c);
	mi.name.a = LPGEN("Load last session");
	mi.pszService = MS_SESSIONS_RESTORELASTSESSION;
	mi.hIcolibItem = iconList[5].hIcolib;
	mi.position = 10100000;
	Menu_AddMainMenuItem(&mi);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Load()
{
	CreateServiceFunction(MS_SESSIONS_SHOWFAVORITESMENU, BuildFavMenu);
	CreateServiceFunction(MS_SESSIONS_OPENMANAGER, OpenSessionsManagerWindow);
	CreateServiceFunction(MS_SESSIONS_RESTORELASTSESSION, LoadLastSession/*LoadSession*/);
	CreateServiceFunction(MS_SESSIONS_SAVEUSERSESSION, SaveUserSessionHandles);
	CreateServiceFunction(MS_SESSIONS_CLOSESESSION, CloseCurrentSession);

	Miranda_WaitOnHandle(LaunchSessions);

	g_ses_count = db_get_b(NULL, MODULENAME, "UserSessionsCount", 0);
	g_ses_limit = db_get_b(NULL, MODULENAME, "TrackCount", 10);
	g_bExclHidden = db_get_b(NULL, MODULENAME, "ExclHidden", 0) != 0;
	g_bWarnOnHidden = db_get_b(NULL, MODULENAME, "WarnOnHidden", 0) != 0;
	g_bOtherWarnings = db_get_b(NULL, MODULENAME, "OtherWarnings", 1) != 0;
	g_bCrashRecovery = db_get_b(NULL, MODULENAME, "CrashRecovery", 0) != 0;

	if (g_bCrashRecovery)
		g_bIncompletedSave = db_get_b(NULL, MODULENAME, "lastSaveCompleted", 0) == 0;

	if (g_bIncompletedSave) {
		int i = 0;
		memset(session_list_recovered, 0, sizeof(session_list_recovered));

		for (auto &hContact : Contacts())
			if (db_get_b(hContact, MODULENAME, "wasInLastSession", 0))
				session_list_recovered[i++] = hContact;
	}

	if (!session_list_recovered[0])
		g_bIncompletedSave = false;

	db_set_b(NULL, MODULENAME, "lastSaveCompleted", 0);

	if (!db_get_b(NULL, MODULENAME, "lastempty", 1) || g_bIncompletedSave)
		isLastTRUE = true;

	HookEvent(ME_SYSTEM_MODULESLOADED, PluginInit);
	HookEvent(ME_SYSTEM_OKTOEXIT, OkToExit);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, SessionPreShutdown);

	// Icons
	g_plugin.registerIcon(MODULENAME, iconList);
	return 0;
}
