@echo off

rem "/nologo" oculta o cabeçalho imprimido pelo compilador
rem "/FC" caminho completo de arquivos (útil para pular para o próximo erro no editor)
rem "/Zi" informação pra debug

mkdir ..\build
pushd ..\build
cl -nologo -FC -Zi ..\code\win32_handmade.cpp user32.lib
popd
