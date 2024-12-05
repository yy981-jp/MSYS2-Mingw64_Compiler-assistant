@echo off
chcp 65001>nul
title [%~n1]MSYS2_Mingw64_Quick_Compiler
if "%1"=="" echo ファイルを%0にドラッグ^&ドロップ&pause&exit /b
set x=%~x1
set xo=0
if "%x%"==".c" set xo=1
if "%x%"==".cpp" set xo=1
if %xo%==0 echo C(.c) もしくは C++(.cpp)ではありません&pause&exit /b

:restart
echo PROCESSING...
cd "c:\msys64\mingw64\bin\"
if "%x%"==".c" gcc %1 -o %~dpn1.exe -finput-charset=UTF-8 -fexec-charset=cp932 %option%
if "%x%"==".cpp" g++ %1 -o %~dpn1.exe -finput-charset=UTF-8 -fexec-charset=cp932 %option%
if "%errorlevel%"=="0" echo 成功&pause&goto restart
echo エラー
pause
for /l %%a in (0,1,10) do echo.
goto restart