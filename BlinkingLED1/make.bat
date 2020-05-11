python %PyScripts%\GenerateMake.py %ProjectRoot% %ProjectBuild% elf
pushd %ProjectBuild%
call make.exe -f "%ProjectBuild%\Makefile" %1
popd
