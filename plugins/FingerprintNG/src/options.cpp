/*
Fingerprint NG (client version) icons module for Miranda NG

Copyright © 2006-18 ghazan, mataes, HierOS, FYR, Bio, nullbie, faith_healer and all respective contributors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//Start of header
#include "stdafx.h"

struct
{
	int idCtrl, defValue;
	LPCSTR szSetName;
}
static settings[] =
{
	{ IDC_GROUP_MIRANDA, TRUE, "GroupMiranda" },
	{ IDC_GROUP_MIRANDA_VERSION, FALSE, "GroupMirandaVersion" },
	{ IDC_GROUP_MIRANDA_PACKS, TRUE, "GroupMirandaPacks" },

	{ IDC_GROUP_MULTI, TRUE, "GroupMulti" },
	{ IDC_GROUP_GG, TRUE, "GroupGG" },
	{ IDC_GROUP_ICQ, TRUE, "GroupICQ" },
	{ IDC_GROUP_IRC, TRUE, "GroupIRC" },
	{ IDC_GROUP_JABBER, TRUE, "GroupJabber" },
	{ IDC_GROUP_MRA, TRUE, "GroupMRA" },
	{ IDC_GROUP_MSN, TRUE, "GroupMSN" },
	{ IDC_GROUP_QQ, TRUE, "GroupQQ" },
	{ IDC_GROUP_RSS, TRUE, "GroupRSS" },
	{ IDC_GROUP_WEATHER, TRUE, "GroupWeather" },
	{ IDC_GROUP_YAHOO, TRUE, "GroupYahoo" },
	{ IDC_GROUP_FACEBOOK, TRUE, "GroupFacebook" },
	{ IDC_GROUP_VK, TRUE, "GroupVK" },

	{ IDC_GROUP_OTHER_PROTOS, TRUE, "GroupOtherProtos" },
	{ IDC_GROUP_OTHERS, TRUE, "GroupOthers" },

	{ IDC_GROUP_OVERLAYS_RESOURCE, TRUE, "GroupOverlaysResource" },
	{ IDC_GROUP_OVERLAYS_PLATFORM, TRUE, "GroupOverlaysPlatform" },
	{ IDC_GROUP_OVERLAYS_UNICODE, TRUE, "GroupOverlaysUnicode" },
	{ IDC_GROUP_OVERLAYS_PROTO, TRUE, "GroupOverlaysProtos" },
	{ IDC_GROUP_OVERLAYS_SECURITY, TRUE, "GroupOverlaysSecurity" },
	{ IDC_STATUSBAR, TRUE, "StatusBarIcon" }
};

/*static void OptDlgChanged(HWND hwndDlg, BOOL show)
{
if (show)
ShowWindow(GetDlgItem(hwndDlg, IDC_OPTCHANGENOTE), SW_SHOW);
SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
}*/

static void LoadDBCheckState(HWND hwndDlg, int idCtrl, LPCSTR szSetting, BYTE bDef)
{
	CheckDlgButton(hwndDlg, idCtrl, db_get_b(NULL, MODULENAME, szSetting, bDef) ? BST_CHECKED : BST_UNCHECKED);
}

static void StoreDBCheckState(HWND hwndDlg, int idCtrl, LPCSTR szSetting)
{
	db_set_b(NULL, MODULENAME, szSetting, (BYTE)IsDlgButtonChecked(hwndDlg, idCtrl));
}

static INT_PTR CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		for (auto &it : settings)
			LoadDBCheckState(hwndDlg, it.idCtrl, it.szSetName, it.defValue);
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			for (auto &it : settings)
				if (it.idCtrl == LOWORD(wParam)) {
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				}
		break;

	case WM_NOTIFY:
		NMHDR * hdr = (NMHDR *)lParam;
		if (hdr && hdr->code == PSN_APPLY) {
			for (auto &it : settings)
				StoreDBCheckState(hwndDlg, it.idCtrl, it.szSetName);

			ClearFI();
			RegisterIcons();

			for (auto &hContact : Contacts())
				OnExtraImageApply(hContact, 0);
		}
		break;
	}
	return FALSE;
}

int OnOptInitialise(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.szGroup.w = LPGENW("Icons");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_DIALOG);
	odp.szTitle.w = LPGENW("Fingerprint");
	odp.pfnDlgProc = DlgProcOptions;
	odp.flags = ODPF_BOLDGROUPS | ODPF_UNICODE;
	g_plugin.addOptions(wParam, &odp);
	return 0;
}
