/*
 * �� Root ����� ��� WindowMaker
 *
 * � ������� �����:
 *
 * <Title> [SHORTCUT <Shortcut>] <Command> <Parameters>
 *
 * <Title> ����� � �������� ��� ������������ � �������. �� ����� ������������
 *         ��� ��� ������ ������ �� ������������� ������ ����������� �.�:
 *         "�� ���������"
 * 
 * SHORTCUT ����� � ���������� �������� ��� �� ������������ ��������� �.�:
 *          "Meta+1". ���� ������������ �� ����� ��� ������:
 *          ~/GNUstep/Defaults/WindowMaker
 *
 * ��� ������ �� ������� ��� shortcut ��� MENU ��� ��� OPEN_MENU ������.
 * 
 * <Command> ��� ��� ��� �������: 
 *	MENU - �� ������ ��� ������ ��� (���)�����
 *	END  - �� ������ ��� ��������� ��� (���)�����
 *	OPEN_MENU - ������� ��� ����� ��� ��� ������, pipe � �� ����������� ����
 *                  ���������(��) ��� ����������� ��� ������ ��� ������.
 *	WORKSPACE_MENU - ��������� ��� �������� ��� �� ���������� ��� ����������.
 *                       ���� ��� workspace_menu �����������.
 *	EXEC <program> - �������� ������������
 *	EXIT - ������ ��� ��� window manager
 *	RESTART [<window manager>] - ����������� ��� Window Maker � �������� ����
 *                                   ����� window manager		
 *	REFRESH - ��������� ��� ������� ��� ���������� ���� �����
 *	ARRANGE_ICONS - ����������� ��� ���������� ���� ���������
 *	SHUTDOWN - ���������� ����� ����� ���� clients (��� ���������� �� X window
 *                 session)
 *	SHOW_ALL - ��������� ��� �� "�������" �������� ���� ���������
 *	HIDE_OTHERS - "������" ��� �� �������� ���� ���������, ����� ���
 *                    ���� ��� ����� "������" (� �� ��������� ��� ���� "������")	
 *	SAVE_SESSION - ���������� �� �������� "���������" ��� ����������, �� �����
 *                 ��������, ��� �� ����������� ��� ����������� ������ ��
 *                 ������ �� ���� ���� ��� ��������� (���������, ���� ����
 *                 �����, ��������� �������� ���� ����� ����� ����������, Dock �
 *                 Clip ��� ���� ������������, �� ����� ����������������,
 *                 ������������ � ��������). ������ ���������� �� ��� ���������
 *                 �������� ���� � ������� ��� ��������� ����. ���� ��
 *                 �� ���������� ��� ������� ���� ��� � �������
 *                 ��������� ��� Window Maker ����� � ������ SAVE_SESSION �
 *                 CLEAR_SESSION ���������������. �� ��� ������ Window Maker ���
 *                 ��������� "~/GNUstep/Defaults/" ������� � ������:
 *                 "SaveSessionOnExit = Yes;", ���� ��� �� �������� ��������
 *                 �������� �� ���� ����� ��� ������ ��� ��� Window Maker,
 *                 ����������� ���������� ���� ����������� ����� ��� �������
 *                 SAVE_SESSION � CLEAR_SESSION (����� ��������). 
 *	CLEAR_SESSION - ������ ���� ��� ����������� ��� ����� ����������� ������� ��
 *                  �� ��������. ��� �� ���� ���� ������ ���������� �� � ������
 *                  SaveSessionOnExit=Yes.
 *	INFO - ����������� ������� �� ��� Window Mmaker
 *
 * OPEN_MENU �������:
 *   1. ��������� ���� �������-�����.
 *	// ������� �� "������.�����" �� ����� �������� ��� ������ ������-����� ���
 *  // �� ������� ���� �������� ����
 *	OPEN_MENU ������.�����
 *   2. ��������� ���� Pipe �����.
 *  // ������ ��� ������ ��� ������������ ��� stdout ����� ��� ��� ��������� ���
 *  // �����. �� ���������� ��� ������� ������ �� ���� ������ ������� ��� �����
 *  // �� �����. �� ���� �������� ������ "|" ��� "�������" ����� �����������.
 *	OPEN_MENU | ������
 *   3. ��������� ���� ���������.
 *  // ������� ���� � ������������� ���������� ��� ������������ ��� ����� ��
 *  // ����� ���� ������������� ��� �� ���������� ������ �� ������ ������������
 *  // ����������.
 *	OPEN_MENU /�������/��������� [/�������/�����/��������� ...]
 *   4. ��������� ���� ��������� �� ������ ������.
 *  // ������� ���� � ������������� ���������� ��� ������������ ��� ����� ��
 *  // ����� ���� ������������� ��� �� ���������� ������ �� ������ ������������
 *  // ����������, �� ����� ������� �� ����������� �� ��� ������.
 *	OPEN_MENU /�������/��������� [/�������/�����/��������� ...] WITH ������ -����������
 *
 *
 * <Parameters> ����� �� ��������� ���� ��������.
 *
 * ** ���������� ��� ��� ������ EXEC:
 * %s - ������������� �� ��� �������� �������.
 * %a(������) - ������� ��� �������� ��������� ��������� ��� ����������� �� ����
 *              ��� ���������������.
 * %w - ������������� �� ��� XID ��� �������� ������� ���������
 *
 * ������� �� ��������� ������� ���������� (���� % � ")  �� ��� ��������� \:
 * �.�.: xterm -T "\"�������� ���\""
 *
 * ������� ������ �� ��������� ���������� �������� (character escapes), ���� \n
 *
 * ���� ������ MENU ������ �� ���� ��� ���������� END ��� ����� ��� �����.
 *
 * ����������:
 *
 * "�����������" MENU
 *	"XTerm" EXEC xterm
 *		// creates a submenu with the contents of /usr/openwin/bin
 *	"XView apps" OPEN_MENU "/usr/openwin/bin"
 *		// some X11 apps in different directories
 *	"X11 apps" OPEN_MENU /usr/X11/bin ~/bin/X11
 *		// set some background images
 *	"�����" OPEN_MENU ~/images /usr/share/images WITH wmsetbg -u -t
 *		// inserts the style.menu in this entry
 *	"����" OPEN_MENU style.menu
 * "�����������" END
 */

#include "wmmacros"

"�����" MENU
	"�����������" MENU
		"Info..." INFO_PANEL
		"Legal..." LEGAL_PANEL
		"System Console" EXEC xconsole
		"System Load" EXEC xosview || xload
		"Process List" EXEC xterm -e top
		"�������" EXEC xman
	"�����������" END
	"XTerm" EXEC xterm -sb 
	"Rxvt" EXEC rxvt -bg black -fg white -fn fixed
	"����������" WORKSPACE_MENU
	"�����������" MENU
		"�������" MENU
			"Gimp" EXEC gimp >/dev/null
			"XV" EXEC xv
			"XPaint" EXEC xpaint
			"XFig" EXEC xfig
		"�������" END
		"X File Manager" EXEC xfm
		"OffiX Files" EXEC files
		"LyX" EXEC lyx
		"Netscape" EXEC netscape 
  		"Ghostview" EXEC ghostview %a(������ ���� ��������)
		"Acrobat" EXEC /usr/local/Acrobat3/bin/acroread %a(������ ���� ��������)
  		"TkDesk" EXEC tkdesk
	"�����������" END
	"�������������" MENU
		"XFte" EXEC xfte
		"XEmacs" EXEC xemacs || emacs
		"XJed" EXEC xjed 
		"NEdit" EXEC nedit
		"Xedit" EXEC xedit
		"VI" EXEC xterm -e vi
	"�������������" END
	"�������" MENU
		"Xmcd" EXEC xmcd 2> /dev/null
		"Xplaycd" EXEC xplaycd
		"Xmixer" EXEC xmixer
	"�������" END
	"��������" MENU
		"������������" EXEC xcalc
		"��������� ���������" EXEC xprop | xmessage -center -title '��������� ���������' -file -
		"������� ��������������" EXEC xfontsel
		"���������� ����������" EXEC xminicom
		"���������" EXEC xmag
		"������ ��������" EXEC xcmap
		"XKill" EXEC xkill
		"ASClock" EXEC asclock -shape
		"Clipboard" EXEC xclipboard
	"��������" END

	"�������" MENU
		"���������" EXEC echo '%s' | wxcopy
		"����������� ����" EXEC xterm -name mail -T "Pine" -e pine %s
		"���������� ��� ���������" EXEC netscape %s
		"��������� ��������" EXEC MANUAL_SEARCH(%s)
	"�������" END

	"���������" MENU
		"�������� ��� �����" HIDE_OTHERS
		"�������� ����" SHOW_ALL
		"����������� ����������" ARRANGE_ICONS
		"�������� ��������" REFRESH
		"��������" EXEC xlock -allowroot -usefirst
		"������ ����������" SAVE_SESSION
		"�������� �������� ����������" CLEAR_SESSION
	"���������" END

	"��������" MENU
		"������" OPEN_MENU THEMES_DIR ~/GNUstep/Library/WindowMaker/Themes WITH setstyle
		"����" OPEN_MENU STYLES_DIR ~/GNUstep/Library/WindowMaker/Styles WITH setstyle
		"����� ����������" OPEN_MENU ICON_SETS_DIR ~/GNUstep/Library/WindowMaker/IconSets WITH seticons
		"�����" MENU
			"���������" MENU
                        	"�����" WS_BACK '(solid, black)'
                        	"����"  WS_BACK '(solid, "#505075")'
							"�������" WS_BACK '(solid, "#243e6c")'
							"������ ����" WS_BACK '(solid, "#180090")'
                        	"�������" WS_BACK '(solid, "#554466")'
                        	"��������"  WS_BACK '(solid, "wheat4")'
                        	"������ ����"  WS_BACK '(solid, "#333340")'
                        	"�������" WS_BACK '(solid, "#400020")'
			"���������" END
			"�������������" MENU
				"������" WS_BACK '(mdgradient, green, red, white, green)'
				"�������" WS_BACK '(vgradient, blue4, white)'
			"�������������" END
			"�������" OPEN_MENU BACKGROUNDS_DIR ~/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t
		"�����" END
		"���������� �������" EXEC getstyle -t ~/GNUstep/Library/WindowMaker/Themes/"%a(����� �������)"
		"���������� ������ ����������" EXEC geticonset ~/GNUstep/Library/WindowMaker/IconSets/"%a(����� ������)"
	"��������" END

	"������"	MENU
		"������������" RESTART
		"�������� ��� AfterStep" RESTART afterstep
		"������..."  EXIT
		"������ ������..." SHUTDOWN
	"������" END
"�����" END

