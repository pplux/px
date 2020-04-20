REM Copyright 2018-2020 (C) PpluX (Jose L. Hidalgo)
REM To Compile the examples under MSVC (2019 Community)
@ECHO OFF
SET INPUT=%1
SET OUTPUT_D=%INPUT:.cpp=_debug.exe%
SET OUTPUT_R=%INPUT:.cpp=_release.exe%
SET ARCH=x64

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\vsdevcmd" -arch=%ARCH%
:loop
	echo ----------------------------------------------------------------
  echo   %INPUT% (%ARCH%)
	echo ----------------------------------------------------------------
  echo Choose Option:
	echo   1: Compile (%OUTPUT_D%)
	echo   2: Execute (%OUTPUT_D%)
	echo   3: Launch DevEnv
	echo   4: Compile (%OUTPUT_R%) 
	echo   5: Execute (%OUTPUT_R%)
	echo   6: Exit
	CHOICE /C:123456  /M "Option"
	goto case_%ERRORLEVEL%
:case_1
  cls
	echo ----------------------------------------------------------------
	echo -- BUILD                                                      --
	echo ----------------------------------------------------------------
	cl /nologo /Zi /GR /EHs /MDd -DWIN32 %INPUT% opengl32.lib user32.lib gdi32.lib shell32.lib /link /OUT:%OUTPUT_D%
	goto loop
:case_2
  cls
	echo ----------------------------------------------------------------
	echo -- EXECUTE                                                    --
	echo ----------------------------------------------------------------
	%OUTPUT_D%
	goto loop
:case_3
	echo ----------------------------------------------------------------
	echo -- DEV ENV                                                    --
	echo ----------------------------------------------------------------
	devenv %OUTPUT_D% %INPUT%
	goto loop
:case_4
  cls
	echo ----------------------------------------------------------------
	echo -- BUILD -RELEASE-                                            --
	echo ----------------------------------------------------------------
	cl /nologo /GR /EHs /MD -DWIN32 %INPUT% opengl32.lib user32.lib gdi32.lib shell32.lib /link /OUT:%OUTPUT_R%
	goto loop
:case_5
  cls
	echo ----------------------------------------------------------------
	echo -- EXECUTE -RELEASE-                                          --
	echo ----------------------------------------------------------------
	%OUTPUT_R%
	goto loop
:case_6
