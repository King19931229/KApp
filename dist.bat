@echo off
echo %QTDIR%
set CUR_DIR=%cd%
REM set DIST_EXE=%QTDIR%\5.6\msvc2015_64\bin\windeployqt.exe %CUR_DIR%\x64\Debug\KEditor.exe
REM %DIST_EXE%
REM set DIST_EXE=%QTDIR%\5.6\msvc2015_64\bin\windeployqt.exe %CUR_DIR%\x64\Release\KEditor.exe
REM %DIST_EXE%
windeployqt.exe %CUR_DIR%\x64\Debug\KEditor.exe
windeployqt.exe %CUR_DIR%\x64\Release\KEditor.exe
pause