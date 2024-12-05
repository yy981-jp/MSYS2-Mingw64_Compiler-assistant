@echo off
chcp 65001>nul
title [%~n1]MSYS2-Mingw64_Compiler

setlocal enabledelayedexpansion
if "%1"=="" goto generate
set x=%~x1
set xo=0
if "%x%"==".c" set xo=1
if "%x%"==".cpp" set xo=1
if "%x%"==".ybp" set xo=1
if %xo%==0 echo C(.c) もしくは C++(.cpp) もしくは Build_Profile(.ybp) ではありません&pause&exit /b
if "%x%"==".ybp" goto ybp

set exename=%~n1
set original=nooption
set "addlibrary= "
set /p original=独自オプション:
set /p exename=実行ファイル名(拡張子除く):
set /p addlibrary=追加するライブラリ(各ファイル名の前に-lを追加):
set /p option=オプション:
set __1=%1
goto noybp

:ybp
set _1=0
set _2=0
set _3=0
set _4=0
set line=0
for /f "delims=" %%a in (%1) do (
set /a line=!line!+1
	if !line!==1 set _1=1&set exename=%%a
	if !line!==2 set _2=1&set addlibrary=%%a
	if !line!==3 set _3=1&set option=%%a
	if !line!==4 set _4=1&set original=%%a
)
if %_1%==0 echo これなんかybpファイル壊れてない? 1行目から読み込めないんだけど...&pause
if %_2%==0 echo これなんかybpファイル壊れてる気がする 2行目から読み込めない...&pause
if %_3%==0 echo これなんかybpファイル壊れてると思うよ ていうか3行目から読み込めないって何??? 中途半端&pause
if %_4%==0 echo このybpファイル壊れてるか独自オプションがないバージョンで作ったやつじゃない? ちなみに読み込めないのは4行目 生成しなおしたら?&pause
if "%option%"=="null" set "option="
set __1=%~dpn1.cpp

:noybp

set "mocrun="
set "mocfile= "
if "%original:nocp932=%"=="%original%" set "cp932=-fexec-charset^=cp932"
if not "%original:qt=%"=="%original%" set mocrunpath=%time:~0,2%%time:~3,2%%time:~6,2%%time:~9,2%
if not "%original:qt=%"=="%original%" set mocrunpath=%temp%\mocrun.%mocrunpath: =%.bat
if not "%original:qt=%"=="%original%" (
	set "addlibrary=%addlibrary% -lQt6Core -lQt6Widgets -lQt6Gui"
	c:\msys64\mingw64\share\qt6\bin\moc.exe %__1% -o %~dp1_.moc
	set mocfile=%~dp1_.moc
	set "inc_qt6=-Ic:/msys64/mingw64/include/qt6"
	copy /y %~dp1_.moc %temp%\_.moc >nul
	set "mocrun=start /min "" "%mocrunpath%""
	rem set "mocrun=start /min cmd /c c:\msys64\mingw64\share\qt6\bin\moc.exe %__1% -o %~dp1_.moc"
	echo fc %~dp1_.moc %temp%\_.moc ^>nul >"%mocrunpath%"
	echo if not %%errorlevel%%==0 c:\msys64\mingw64\share\qt6\bin\moc.exe %__1% -o %~dp1_.moc >>"%mocrunpath%"
	echo exit >>"%mocrunpath%"
)

:loop
	%mocrun%

	echo CC  = g++>_.mk
	echo CFLAGS  = %option%>>_.mk
	echo TARGET  = %exename%.exe>>_.mk
	echo SRCS    = %~n1.cpp %mocfile:\=/%>>_.mk
	echo OBJS    = %~n1.o>>_.mk
	echo INCDIR  = -Ic:/msys64/mingw64/include %inc_qt6%>>_.mk
	echo LIBDIR  = -Lc:/msys64/mingw64/lib>>_.mk
	echo LIBS    = %addlibrary%>>_.mk
	echo $(TARGET): $(OBJS)>>_.mk
	echo 	$(CC) -o $@ $^^ $(LIBDIR) $(LIBS)>>_.mk
	echo %%.o: %%.cpp>>_.mk
	echo 	$(CC)  %cp932% $(CFLAGS) $(INCDIR) -c $^< -o $@>>_.mk

	set _=%~dp1
	set _=%_:\=/%
	set temp_=%temp:\=/%

	cd %_:~0,-1%
	c:\GnuWin32\bin\make.exe -B -f %_:~0,-1%/_.mk
	if exist %~n1.o del %~n1.o
	pause
	echo.
	echo.
goto loop

:generate
echo ファイルを%0にドラッグ^&ドロップされなかったのでBuild_Profileを作成モードで起動します

set /p codename=ソースコードファイル名(拡張子除く):
set exename=%codename%
set addlibrary=null
set option=null
set original=null
set /p original=独自オプション:
set /p exename=実行ファイル名(拡張子除く):
set /p addlibrary=追加するライブラリ(各ファイル名の前に-lを追加):
set /p option=オプション:
echo %exename%>%codename%.ybp
echo %addlibrary%>>%codename%.ybp
echo %option%>>%codename%.ybp
echo %original%>>%codename%.ybp
exit /b
