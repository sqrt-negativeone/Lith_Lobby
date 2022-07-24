@echo off

set code=%cd%
set application_name=Lith_Lobby
set warnings= /W4 /WX /wd4505
set compile_flags= -FC -GR- -EHa- -nologo -Zi %warnings% /I "%code%/code/"
set common_link_flags= -incremental:no
set platform_link_flags= Shell32.lib Ws2_32.lib gdi32.lib user32.lib Kernel32.lib winmm.lib %common_link_flags%


if not exist build mkdir build
pushd build

start /b /wait "" "cl.exe"  %build_options% %compile_flags% ../code/os/win32/win32_main.cpp /link %platform_link_flags% /out:%application_name%.exe

popd
