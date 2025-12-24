@ECHO OFF

:: Unpack Arguments
set release=0
for %%a in (%*) do set "%%a=1"

set BUILD_DIR=build_rel

if "%release%"=="0" (
    set BUILD_DIR=build_deb
)

:: clean directory
:: IF EXIST %BUILD_DIR% rmdir %BUILD_DIR%\ /S /Q

IF NOT EXIST %BUILD_DIR% mkdir %BUILD_DIR%
IF NOT EXIST %BUILD_DIR%\shaders mkdir %BUILD_DIR%\shaders
:: set C_FLAGS=/nologo /W4 /WX /Zi /Fo:%BUILD_DIR%\ /Iinclude /Iinclude\api-test /I%VULKAN_SDK%\Include /Ithirdparty /Ithirdparty\imgui-1.92.5 /std:c++latest /DWINDOWS /DNOMINMAX /c /Tp
:: set C_FLAGS=/nologo /W4 /Zi /Fo:%BUILD_DIR%\ /Iinclude /Iinclude\api-test /I%VULKAN_SDK%\Include /imsvcthirdparty /Ithirdparty /Ithirdparty\imgui-1.92.5 /std:c++latest /DWINDOWS /DNOMINMAX /c /Tp

:: showincludes
:: set C_FLAGS=/nologo /showIncludes /W4 /Zi /Fo:%BUILD_DIR%\ /Iinclude /Iinclude\api-test /I%VULKAN_SDK%\Include /external:W0 /external:Ithirdparty /external:Ithirdparty\imgui-1.92.5 /std:c++latest /DWINDOWS /DNOMINMAX /c /Tp
set C_FLAGS=/nologo /W4 /Zi /Fo:%BUILD_DIR%\ /Iinclude /Iinclude\api-test /I%VULKAN_SDK%\Include /external:W0 /external:Ithirdparty /external:Ithirdparty\imgui-1.92.5 /std:c++latest /DWINDOWS /DNOMINMAX /c /Tp
set SHADER_FLAGS=-I shaders\include -std=460
:: set C_FLAGS=/nologo /W4 /Zi /Fo:%BUILD_DIR%\ /Iinclude /Iinclude\api-test /imsvcthirdparty /Ithirdparty\imgui-1.92.5 /std:c++latest /DDX12 /DWINDOWS /DNOMINMAX /c /Tp
REM set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /NODEFAULTLIB /stack:0x100000,100000
:: set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /NODEFAULTLIB
set L_FLAGS=/WX /SUBSYSTEM:CONSOLE

if "%release%"=="0" (
    set C_FLAGS=/DDEBUG /Od %C_FLAGS%
    set SHADER_FLAGS=-O0 -g %SHADER_FLAGS%
    set L_FLAGS=/DEBUG %L_FLAGS%
)

:compilation

:: set imgui_src=imgui imgui_draw imgui_tables imgui_widgets
set src=window vec3 camera scene vulkan-loader utils bvh gltf mesh
:: set api=main vk-utils vk-device vk-bindless vk-command-buffer
set api=main
set shaders=ui.vert ui.frag test.comp compute.comp api-test-ui.vert api-test-ui.frag tonemapping.comp transmittance_lut.comp
:: set src=main dx12-device dx12-command-buffer 

(for %%s in (%api%) do (
    call set OBJS=%%OBJS%% %BUILD_DIR%\%%s.obj
    cl /Fd:%BUILD_DIR%\%%s.pdb %C_FLAGS% api-test\%%s.cpp
))
(for %%s in (%shaders%) do (
    glslc.exe -o %BUILD_DIR%\shaders\%%s.spv %SHADER_FLAGS% shaders\%%s
))
    :: clang-cl /Fd:%BUILD_DIR%\%%s.pdb %C_FLAGS% api-test\%%s.cpp

:: link
link %L_FLAGS% /OUT:%BUILD_DIR%\main.exe %OBJS% kernel32.lib
