@echo off
echo %QTDIR%
set CUR_DIR=%cd%
set DIST_EXE=%QTDIR%\5.6\msvc2015_64\bin\windeployqt.exe %CUR_DIR%\Product\Binaries\x64\Debug\KEditor.exe
%DIST_EXE%
set DIST_EXE=%QTDIR%\5.6\msvc2015_64\bin\windeployqt.exe %CUR_DIR%\Product\Binaries\x64\Release\KEditor.exe
%DIST_EXE%
REM windeployqt.exe %CUR_DIR%\x64\Debug\KEditor.exe
REM windeployqt.exe %CUR_DIR%\x64\Release\KEditor.exe
pause