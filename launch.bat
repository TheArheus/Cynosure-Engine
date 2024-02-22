@echo off
pushd src\
rem call buildr.bat
copy "..\build\Cynosure Engine.exe" ..\bin\
"..\bin\Cynosure Engine.exe"
popd