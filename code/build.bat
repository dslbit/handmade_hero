@echo off

rem "/nologo" oculta o cabeçalho imprimido pelo compilador
rem "/FC" caminho completo de arquivos (útil para pular para o próximo erro no editor)
rem "/Zi" informação pra debug
rem "/Z7" informações de debug mais eficientes (veloz) do que "/Zi"
rem "/W4" nível de alerta do compilador
rem "/WX" tratar alertas como erro
rem "/wd..." desativa um alerta específico
rem "/Oi" permite o compilador trocar nossas instruções por instruções intrínsecas ("chama" o processador ali mesmo, se possível)
rem "-GR-" desliga informações de tipos em tempo de execução (C++)
rem "-EHa-" configurações do tratamento de exceções (C++)
rem "-subsystem:windows,5.01" define o tipo de aplicação e as especificações mínimas para 32bits (n funciona em x64, pelo menos no win10, alt: 6.00)
rem "-MD" assume que o sistema operacional vai fornecer a Biblioteca Padrão C (há muitas, e nem sempre é a correta)
rem "-MT" pede pra colocar as bibliotecas dentro do executável
rem "-Gm-" desliga re-build mínima (nos ajuda já que queremos construir do zero as coisas)
rem "-Fm" indica um local para guardar um arquivo ".map" que é uma lista de tudo o que está no executável
rem "-opt:ref" não coloca no executável o que não precisa
rem "-Od" não otimiza e "deixa as coisas onde estão"

if not exist ..\build mkdir ..\build
pushd ..\build
echo Building...
cl -nologo -Gm- -GR- -MT -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -FC -Z7 -Fmwin32_handmade.map ..\code\win32_handmade.cpp -link -subsystem:windows,6.00 -opt:ref user32.lib gdi32.lib hid.lib
echo ---
popd  
