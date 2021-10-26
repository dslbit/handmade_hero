@echo off

rem ~
rem Este arquivo é equivalente ao "emacs.bat" do Casey, mas em vez de usar
rem "emacs" eu uso o "sublime text". Mas eu prefiso não usar porque o terminal
rem trava até o editor fechar.
rem ~

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
"C:\Work\lite\lite.exe" "W:\code\handmade_hero"
