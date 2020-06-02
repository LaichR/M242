
set ProjectRoot=e:\M242\Rolf_Laich\M242\AdcReadout
set ProjectBuild=%ProjectRoot%\Build
set ToolsRoot=E:\M242\Avr\ToolChain
set PyScripts=%ToolsRoot%\PyScripts
set AvrGcc=%ToolsRoot%\avr8-gnu-toolchain-win32_x86
set AvrDude=%ToolsRoot%\avrdude-6.3-mingw32
set DotNetLib=%ToolsRoot%\DotNetLib
python %PyScripts%\GenerateMake.py %ProjectRoot% %ProjectBuild% elf
pushd %ProjectBuild%
call make.exe -f "%ProjectBuild%\Makefile" flash
popd
