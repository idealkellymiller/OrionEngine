; ============================================================================
; OrionEngine Installer Script (NSIS)
;
; Build with:   makensis installer\OrionEngine.nsi
; Requires:     NSIS 3.x  (winget install NSIS.NSIS)
; ============================================================================

!include "MUI2.nsh"
!include "FileFunc.nsh"

; ---------------------------------------------------------------------------
; Build output directory (can be overridden via /DBIN_DIR=...)
; ---------------------------------------------------------------------------
; Override this with: makensis /DBIN_DIR="path\to\binaries" OrionEngine.nsi
; The BuildInstaller.bat script detects the correct path automatically.
!ifndef BIN_DIR
    !define BIN_DIR "..\build\bin\Release--Windows"
!endif

; ---------------------------------------------------------------------------
; General
; ---------------------------------------------------------------------------
Name "OrionEngine"
OutFile "..\OrionEngineSetup.exe"
InstallDir "$PROGRAMFILES\OrionEngine"
InstallDirRegKey HKLM "Software\OrionEngine" "InstallDir"
RequestExecutionLevel admin
Unicode True

; ---------------------------------------------------------------------------
; Version Info (shown in file properties)
; ---------------------------------------------------------------------------
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName"   "OrionEngine"
VIAddVersionKey "FileDescription" "OrionEngine Installer"
VIAddVersionKey "FileVersion"   "1.0.0"
VIAddVersionKey "LegalCopyright" "CS496 Academic Project"

; ---------------------------------------------------------------------------
; Modern UI Settings
; ---------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_ICON "..\engine\engineAssets\icons\OrionEngine.ico"
!define MUI_UNICON "..\engine\engineAssets\icons\OrionEngine.ico"

; Welcome page text
!define MUI_WELCOMEPAGE_TITLE "Welcome to OrionEngine Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will install OrionEngine on your computer.$\r$\n$\r$\nOrionEngine is a 3D game engine built with C++ and OpenGL.$\r$\n$\r$\nClick Next to continue."

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\Editor.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch OrionEngine"
!define MUI_FINISHPAGE_RUN_PARAMETERS ""

; ---------------------------------------------------------------------------
; Pages
; ---------------------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

; ============================================================================
; SECTIONS
; ============================================================================

; ---------------------------------------------------------------------------
; Core Engine (required)
; ---------------------------------------------------------------------------
Section "OrionEngine Editor (required)" SecCore
    SectionIn RO

    ; --- Binaries ---
    SetOutPath "$INSTDIR\bin"
    File "${BIN_DIR}\Editor.exe"
    File "${BIN_DIR}\Runtime.exe"
    File "${BIN_DIR}\Engine.dll"
    File "..\engine\external\Gettext\lib\GNU.Gettext.dll"

    ; --- Shaders ---
    SetOutPath "$INSTDIR\engine\shaders"
    File "..\engine\shaders\Lit.vert"
    File "..\engine\shaders\Lit.frag"
    File "..\engine\shaders\Shadow.vert"
    File "..\engine\shaders\Shadow.frag"
    File "..\engine\shaders\Picking.vert"
    File "..\engine\shaders\Picking.frag"
    File "..\engine\shaders\Gradient.vert"
    File "..\engine\shaders\Gradient.frag"
    File "..\engine\shaders\Gizmo.vert"
    File "..\engine\shaders\Gizmo.frag"
    File "..\engine\shaders\ToneMap.vert"
    File "..\engine\shaders\ToneMap.frag"
    File "..\engine\shaders\Wireframe.vert"
    File "..\engine\shaders\Wireframe.frag"

    ; --- Default project folder (always present so the editor has a valid startup state) ---
    SetOutPath "$INSTDIR\editor\assets"
    File /nonfatal "..\editor\assets\project.settings"

    ; --- Engine built-in assets ---
    SetOutPath "$INSTDIR\engine\engineAssets\materials"
    File /nonfatal "..\engine\engineAssets\materials\*.*"

    SetOutPath "$INSTDIR\engine\engineAssets\primitives"
    File /nonfatal "..\engine\engineAssets\primitives\*.*"

    SetOutPath "$INSTDIR\engine\engineAssets\textures"
    File /nonfatal "..\engine\engineAssets\textures\*.*"

    ; --- Localization ---
    SetOutPath "$INSTDIR\engine\locales"
    File "..\engine\locales\orion-engine.pot"

    SetOutPath "$INSTDIR\engine\locales\es\LC_MESSAGES"
    File /nonfatal "..\engine\locales\es\LC_MESSAGES\*.*"

    SetOutPath "$INSTDIR\engine\locales\es"
    File /nonfatal "..\engine\locales\es\orion-engine.po"

    ; --- Write uninstaller ---
    SetOutPath "$INSTDIR"
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; --- Registry: install location ---
    WriteRegStr HKLM "Software\OrionEngine" "InstallDir" "$INSTDIR"

    ; --- Registry: Add/Remove Programs ---
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "DisplayName" "OrionEngine"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "Publisher" "OrionEngine"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "DisplayVersion" "1.0.0"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "NoRepair" 1

    ; Calculate installed size for Add/Remove Programs
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine" \
        "EstimatedSize" "$0"
SectionEnd

; ---------------------------------------------------------------------------
; Sample Project (optional)
; ---------------------------------------------------------------------------
Section "Sample Project" SecSamples
    SetOutPath "$INSTDIR\editor\assets"
    File /nonfatal "..\editor\assets\project.settings"
    File /nonfatal "..\editor\assets\*.scene"

    SetOutPath "$INSTDIR\editor\assets\models"
    File /nonfatal "..\editor\assets\models\*.*"

    SetOutPath "$INSTDIR\editor\assets\materials"
    File /nonfatal "..\editor\assets\materials\*.*"

    SetOutPath "$INSTDIR\editor\assets\textures"
    File /nonfatal "..\editor\assets\textures\*.*"

    SetOutPath "$INSTDIR\editor\assets\scripts"
    File /nonfatal "..\editor\assets\scripts\*.*"
SectionEnd

; ---------------------------------------------------------------------------
; VC++ Redistributable (optional)
; ---------------------------------------------------------------------------
Section "Visual C++ Runtime" SecVCRedist
    SetOutPath "$TEMP"
    File /nonfatal "..\installer\redist\vc_redist.x64.exe"
    IfFileExists "$TEMP\vc_redist.x64.exe" 0 +3
        ExecWait '"$TEMP\vc_redist.x64.exe" /install /quiet /norestart'
        Delete "$TEMP\vc_redist.x64.exe"
SectionEnd

; ---------------------------------------------------------------------------
; .scene File Association (optional)
; ---------------------------------------------------------------------------
Section "Register .scene file association" SecFileAssoc
    ; File extension
    WriteRegStr HKCR ".scene" "" "OrionEngine.Scene"

    ; File type description
    WriteRegStr HKCR "OrionEngine.Scene" "" "OrionEngine Scene File"

    ; Icon: use the Editor.exe icon
    WriteRegStr HKCR "OrionEngine.Scene\DefaultIcon" "" "$INSTDIR\bin\Editor.exe,0"

    ; Open command: pass the .scene path as argument
    WriteRegStr HKCR "OrionEngine.Scene\shell" "" "open"
    WriteRegStr HKCR "OrionEngine.Scene\shell\open" "" "Open with OrionEngine"
    WriteRegStr HKCR "OrionEngine.Scene\shell\open\command" "" \
        '"$INSTDIR\bin\Editor.exe" "%1"'

    ; Tell Windows to refresh file associations
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd

; ---------------------------------------------------------------------------
; Desktop Shortcut (optional)
; ---------------------------------------------------------------------------
Section "Desktop Shortcut" SecDesktop
    SetOutPath "$INSTDIR\bin"
    CreateShortCut "$DESKTOP\OrionEngine.lnk" \
        "$INSTDIR\bin\Editor.exe" "" "$INSTDIR\bin\Editor.exe" 0
    ; Set working directory so relative paths (../engine/shaders) resolve correctly
    StrCpy $0 "$INSTDIR\bin"
SectionEnd

; ---------------------------------------------------------------------------
; Start Menu Shortcut (optional)
; ---------------------------------------------------------------------------
Section "Start Menu Shortcut" SecStartMenu
    CreateDirectory "$SMPROGRAMS\OrionEngine"
    SetOutPath "$INSTDIR\bin"
    CreateShortCut "$SMPROGRAMS\OrionEngine\OrionEngine.lnk" \
        "$INSTDIR\bin\Editor.exe" "" "$INSTDIR\bin\Editor.exe" 0
    CreateShortCut "$SMPROGRAMS\OrionEngine\Uninstall OrionEngine.lnk" \
        "$INSTDIR\Uninstall.exe"
SectionEnd

; ---------------------------------------------------------------------------
; Section Descriptions
; ---------------------------------------------------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}      "The OrionEngine editor, runtime, shaders, and built-in assets. (Required)"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSamples}   "A sample project with demo scenes, models, textures, materials, and scripts."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecVCRedist}  "Microsoft Visual C++ Runtime required to run the engine. Skip if already installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecFileAssoc} "Associate .scene files with OrionEngine so you can double-click to open them."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}   "Create a shortcut on the Desktop."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} "Create a Start Menu folder with shortcuts."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ============================================================================
; UNINSTALLER
; ============================================================================
Section "Uninstall"
    ; --- Remove files ---
    Delete "$INSTDIR\bin\Editor.exe"
    Delete "$INSTDIR\bin\Runtime.exe"
    Delete "$INSTDIR\bin\Engine.dll"
    Delete "$INSTDIR\bin\GNU.Gettext.dll"
    Delete "$INSTDIR\bin\Editor.pdb"
    RMDir "$INSTDIR\bin"

    RMDir /r "$INSTDIR\engine"
    RMDir /r "$INSTDIR\editor"

    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    ; --- Remove shortcuts ---
    Delete "$DESKTOP\OrionEngine.lnk"
    RMDir /r "$SMPROGRAMS\OrionEngine"

    ; --- Remove registry ---
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrionEngine"
    DeleteRegKey HKLM "Software\OrionEngine"

    ; --- Remove file association ---
    DeleteRegKey HKCR ".scene"
    DeleteRegKey HKCR "OrionEngine.Scene"
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd
