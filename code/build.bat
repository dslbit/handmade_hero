@echo off

rem "/nologo" oculta o cabeçalho imprimido pelo compilador
rem "/FC" caminho completo de arquivos (útil para pular para o próximo erro no editor)
rem "/Zi" informação pra debug

if not exist ..\build mkdir ..\build
pushd ..\build
echo Building...
cl -nologo -FC -Zi ..\code\win32_handmade.cpp user32.lib gdi32.lib hid.lib
echo Build complete!
popd  
