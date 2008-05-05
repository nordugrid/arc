;ARC Windows Installer
Name "Advanced Resource Connector - ARC"
OutFile "arcsetup.exe"
InstallDir $PROGRAMFILES\ARC
; Pages
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; Main components = arched.exe and required libraries
Section "Main Components"
	SetOutPath $INSTDIR
	File /oname=arched.exe "src\hed\daemon\.libs\arched.exe"
	; File /oname=test.exe "src\tests\echo\.libs\test.exe"
	; File /oname=echo_service.xml "src\tests\echo\service.xml.win32"
	; File /oname=accesslist "src\tests\echo\accesslist"
	; File /oname=Policy_Example.xml "src\tests\echo\Policy_Example.xml"
	; File /oname=ca.pem "src\tests\echo\ca.pem"
	; File /oname=key.pem "src\tests\echo\key.pem"
	; File /oname=cert.pem "src\tests\echo\cert.pem"

	; Libs
	File /oname=libarccommon-0.dll "src\hed\libs\common\.libs\libarccommon-0.dll"
	File /oname=libarcloader-0.dll "src\hed\libs\loader\.libs\libarcloader-0.dll"
	File /oname=libarcmessage-0.dll "src\hed\libs\message\.libs\libarcmessage-0.dll"
	File /oname=libarcsecurity-0.dll "src\hed\libs\security\.libs\libarcsecurity-0.dll"
	File /oname=libarcclient-0.dll "src\hed\libs\misc\.libs\libarcclient-0.dll"
	File /oname=libarcws-0.dll "src\hed\libs\ws\.libs\libarcws-0.dll"
	File /oname=libarcdata2-0.dll "src\hed\libs\data\.libs\libarcdata2-0.dll"
	; XXX Dependency libs with hard coded paths
	File /oname=libgnurx-0.dll "C:\msys\lib\libgnurx-0.dll"
	File /oname=libglib-2.0-0.dll "C:\GTK\bin\libglib-2.0-0.dll"
	File /oname=libgmodule-2.0-0.dll "C:\GTK\bin\libgmodule-2.0-0.dll"
	File /oname=libgobject-2.0-0.dll "C:\GTK\bin\libgobject-2.0-0.dll"
	File /oname=libgthread-2.0-0.dll "C:\GTK\bin\libgthread-2.0-0.dll"
	File /oname=intl.dll "C:\GTK\bin\intl.dll"
	File /oname=iconv.dll "C:\GTK\bin\iconv.dll"
	File /oname=libglibmm-2.4-1.dll "C:\GTK\bin\libglibmm-2.4-1.dll"
	File /oname=libsigc-2.0-0.dll "C:\GTK\bin\libsigc-2.0-0.dll"
	File /oname=libxml2.dll "C:\GTK\bin\libxml2.dll"
	File /oname=zlib1.dll "C:\GTK\bin\zlib1.dll"
	WriteUninstaller "uninstall.exe"
SectionEnd

Section "Plugins"
	CreateDirectory $INSTDIR\plugins\mcc
	File /oname=plugins\mcc\libmcctcp.dll "src\hed\mcc\tcp\.libs\libmcctcp.dll"
	File /oname=plugins\mcc\libmcctls.dll "src\hed\mcc\tls\.libs\libmcctls.dll"
	File /oname=plugins\mcc\libmcchttp.dll "src\hed\mcc\http\.libs\libmcchttp.dll"
	File /oname=plugins\mcc\libmccsoap.dll "src\hed\mcc\soap\.libs\libmccsoap.dll"
	; CreateDirectory $INSTDIR\plugins\pdc
	; File /oname=plugins\pdc\libarcpdc.dll "src\hed\pdc\.libs\libarcpdc.dll"

	CreateDirectory $INSTDIR\plugins\dmc
	File /oname=plugins\dmc\libdmcfile.dll "src\hed\dmc\file\.libs\libdmcfile.dll"
	File /oname=plugins\dmc\libdmchttp.dll "src\hed\dmc\http\.libs\libdmchttp.dll"
SectionEnd

Section "Services"
	CreateDirectory $INSTDIR\services
	; File /oname=services\libecho.dll "src\tests\echo\.libs\libecho.dll"
	; Paul service
    File /oname=services\libpaul.dll "src\services\paul\.libs\libpaul.dll"
    File /oname=paul_service.xml "src\services\paul\paul_service_win32.xml"
    File /oname=paul.bat "src\services\paul\paul.bat"
SectionEnd

Section "Uninstall"
	Delete "$INSTDIR\plugins\mcc\*.*"
	RMDir $INSTDIR\plugins\mcc
	Delete "$INSTDIR\plugins\pdc\*.*"
	RMDir $INSTDIR\plugins\pdc
	Delete "$INSTDIR\plugins\dmc\*.*"
	RMDir $INSTDIR\plugins\dmc
	RMDir $INSTDIR\plugins
	Delete "$INSTDIR\services\*.*"
	RMDir $INSTDIR\services
	Delete "$INSTDIR\*.*"
	RMDir $INSTDIR
SectionEnd
