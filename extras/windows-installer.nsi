; © 2022 David Given.
; FluxEngine binaries are distributable under the terms of the GPLv2. See the
; README.md file in this distribution for the full text.

!include MUI2.nsh

Name "FluxEngine for Windows"
OutFile "${OUTFILE}"
Unicode True

InstallDir "$PROGRAMFILES\Cowlark Technologies\FluxEngine"

InstallDirRegKey HKLM "Software\Cowlark Technologies\FluxEngine" \
	"InstallationDirectory"

RequestExecutionLevel admin
SetCompressor /solid lzma

;--------------------------------

!define MUI_WELCOMEPAGE_TITLE "FluxEngine for Windows"
!define MUI_WELCOMEPAGE_TEXT "FluxEngine is a flux-level floppy disk interface \
 	capable of reading and writing almost any floppy disk format. This package \
 	contains the client software, which will work on either FluxEngine or \
 	GreaseWeazle hardware. It also allows manipulation of flux files and disk \
 	images, so it's useful without any hardware.$\r$\n\ 
	$\r$\n\
	This wizard will install FluxEngine on your computer.$\r$\n\
	$\r$\n\
	$_CLICK"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\nsis.bmp"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME

!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_TITLE "Installation complete"
!define MUI_FINISHPAGE_TEXT_LARGE
!define MUI_FINISHPAGE_TEXT "FluxEngine is now ready to use.$\r$\n\
	$\r$\n\
	Have fun!"

Function runprogramaction
	ExecShell "" "$INSTDIR\fluxengine-gui.exe"
FunctionEnd

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Run now"
!define MUI_FINISHPAGE_RUN_FUNCTION runprogramaction
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Utility functions

!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0

Function RefreshShellIcons
	System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
		(${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

Function un.RefreshShellIcons
	System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
		(${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

;--------------------------------

; The stuff to install
Section "FluxEngine (required)"
	SectionIn RO
	SetOutPath $INSTDIR
	File /oname=fluxengine.exe fluxengine-stripped.exe
	File /oname=fluxengine-gui.exe fluxengine-gui-stripped.exe
	File README.md
	CreateDirectory $INSTDIR\dep
	CreateDirectory $INSTDIR\dep\adflib
	File /oname=dep\adflib\COPYING dep\adflib\COPYING
	CreateDirectory $INSTDIR\dep\hfsutils
	File /oname=dep\hfsutils\COPYING dep\hfsutils\COPYING
	CreateDirectory $INSTDIR\dep\stb
	File /oname=dep\stb\LICENSE dep\stb\LICENSE
	CreateDirectory $INSTDIR\dep\fatfs
	File /oname=dep\fatfs\LICENSE.txt dep\fatfs\LICENSE.txt
	CreateDirectory $INSTDIR\dep\libusbp
	File /oname=dep\libusbp\LICENSE.txt dep\libusbp\LICENSE.txt
	CreateDirectory $INSTDIR\dep\snowhouse
	File /oname=dep\snowhouse\LICENSE_1_0.txt dep\snowhouse\LICENSE_1_0.txt
	CreateDirectory $INSTDIR\dep\agg
	File /oname=dep\agg\README dep\agg\README

	; Write the installation path into the registry
	WriteRegStr HKLM SOFTWARE\NSIS_FluxEngine "Install_Dir" "$INSTDIR"

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FluxEngine" "DisplayName" "FluxEngine for Windows"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FluxEngine" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FluxEngine" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FluxEngine" "NoRepair" 1
	WriteUninstaller "uninstall.exe"

	; Update the shell.
	Call RefreshShellIcons
SectionEnd

Section "FluxEngine hardware firmware"
	SetOutPath $INSTDIR
	FILE /oname=fluxengine-firmware.hex "FluxEngine.cydsn\CortexM3\ARM_GCC_541\Release\FluxEngine.hex"
SectionEnd

Section "Start Menu Shortcuts"
	CreateDirectory "$SMPROGRAMS\FluxEngine"
	CreateShortCut "$SMPROGRAMS\FluxEngine\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	SetOutPath "$DOCUMENTS"
	CreateShortCut "$SMPROGRAMS\FluxEngine\FluxEngine.lnk" "$INSTDIR\fluxengine-gui.exe" "" "$INSTDIR\fluxengine-gui.exe" 0
SectionEnd

Section "Desktop Shortcut"
	SetOutPath "$DOCUMENTS"
	CreateShortCut "$DESKTOP\FluxEngine.lnk" "$INSTDIR\fluxengine-gui.exe" "" "$INSTDIR\fluxengine-gui.exe" 0
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
	; Remove registry keys
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FluxEngine"
	DeleteRegKey HKLM SOFTWARE\NSIS_FluxEngine
	Call un.RefreshShellIcons

	; Remove files and uninstaller
	Delete $INSTDIR\fluxengine.exe
	Delete $INSTDIR\fluxengine-gui.exe
	Delete $INSTDIR\uninstall.exe
	Delete $INSTDIR\README.md
	Delete $INSTDIR\fluxengine-firmware.hex

	RmDir /r $INSTDIR\dep

	; Remove shortcuts, if any
	Delete "$SMPROGRAMS\FluxEngine\*.*"
	Delete "$DESKTOP\FluxEngine.lnk"

	; Remove directories used
	RMDir "$SMPROGRAMS\FluxEngine"
	RMDir "$INSTDIR"
SectionEnd
