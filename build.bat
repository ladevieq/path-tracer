@ECHO OFF

:: Unpack Arguments
set RELEASE=0
for %%a in (%*) do set "%%a=1"

set BUILD_DIR=build_rel

if "%release%"=="0" (
    set BUILD_DIR=build_deb
)

:: clean directory
IF EXIST %BUILD_DIR% rmdir %BUILD_DIR%\ /S /Q

IF NOT EXIST %BUILD_DIR% mkdir %BUILD_DIR%
IF NOT EXIST %BUILD_DIR%\shaders mkdir %BUILD_DIR%\shaders


set MSVC=1
set CLANG=0
set C_COMPILER=cl
if %MSVC% EQU 1 (
    if %CLANG% EQU 1 (
        set C_COMPILER=clang-cl
    )
) else (
    set CLANG=1
    set C_COMPILER=clang
)

set C_FLAGS=
set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /stack:0x10000000,10000000
:: set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /NODEFAULTLIB /stack:0x100000,100000
:: set L_FLAGS=/WX /SUBSYSTEM:CONSOLE /NODEFAULTLIB

set SHADERS_C_FLAGS=-I shaders\include -std=460

set DEBUG_C_FLAGS=
set DEBUG_L_FLAGS=
set RELEASE_C_FLAGS=

if %MSVC% EQU 1 (
    set C_FLAGS=/nologo /W4 /WX /Zi /Fo:%BUILD_DIR%\ /Iinclude /I%VULKAN_SDK%\Include /external:W0 /external:Ithirdparty /external:Ithirdparty\imgui /std:c++20 /DNOMINMAX /EHsc /c /Tp

    set DEBUG_C_FLAGS=/DDEBUG /Od
    set RELEASE_C_FLAGS=/DNDEBUG /O2
    set DEBUG_L_FLAGS=/DEBUG
) else (
    set C_FLAGS=--target=x86_64-pc-windows-msvc -Wall -Werror -pedantic -I include -I %VULKAN_SDK%\Include -isystem thirdparty -isystem thirdparty\imgui -std=c++20 -DNOMINMAX -c -msse4.1 -Wno-missing-designated-field-initializers 

    set DEBUG_C_FLAGS=-DDEBUG -g -O0
    set RELEASE_C_FLAGS=-DNDEBUG -O2
)


if %RELEASE% EQU 0 (
    set C_FLAGS=%DEBUG_C_FLAGS% %C_FLAGS%
    set L_FLAGS=%DEBUG_L_FLAGS% %L_FLAGS%
    set SHADERS_C_FLAGS = -O0 -g %SHADERS_C_FLAGS%
) else (
    set C_FLAGS=%RELEASE_C_FLAGS% %C_FLAGS%
    set SHADERS_C_FLAGS=-O %SHADERS_C_FLAGS%
)


if 0 EQU 1 (
    set C_FLAGS=/showIncludes %C_FLAGS%
)


:: compiles each compilation unit to an obj
:: set imgui_src=imgui imgui_draw imgui_tables imgui_widgets
:: set src=window vec3 camera scene vulkan-loader utils bvh gltf mesh
:: set api=main vk-utils vk-device vk-bindless vk-command-buffer
:: set src=main dx12-device dx12-command-buffer
:: (for %%s in (%api%) do (
::     call set OBJS=%%OBJS%% %BUILD_DIR%\%%s.obj
::     echo %C_COMPILER% /Fd:%BUILD_DIR%\%%s.pdb %C_FLAGS% src\%%s.cpp
::     %C_COMPILER% /Fd:%BUILD_DIR%\%%s.pdb %C_FLAGS% src\%%s.cpp
:: ))
:: link multiple objs
:: link %L_FLAGS% /OUT:%BUILD_DIR%\main.exe %OBJS% kernel32.lib user32.lib

:: compiles the project as a single compilation unit
if %MSVC% EQU 1 (
    echo %C_COMPILER% /Fd:%BUILD_DIR%\main.pdb %C_FLAGS% src\main.cpp
    %C_COMPILER% /Fd:%BUILD_DIR%\main.pdb %C_FLAGS% src\main.cpp
) else (
    echo %C_COMPILER% -o %BUILD_DIR%\main.obj %C_FLAGS% src\main.cpp
    %C_COMPILER% -o %BUILD_DIR%\main.obj %C_FLAGS% src\main.cpp
)

link %L_FLAGS% /DEBUG /OUT:%BUILD_DIR%\main.exe %BUILD_DIR%\main.obj kernel32.lib user32.lib

set shaders=ui.vert ui.frag test.comp compute.comp api-test-ui.vert api-test-ui.frag tonemapping.comp transmittance_lut.comp
(for %%s in (%shaders%) do (
    echo glslc.exe -o %BUILD_DIR%\shaders\%%s.spv %SHADER_FLAGS% shaders\%%s
    glslc.exe -o %BUILD_DIR%\shaders\%%s.spv %SHADERS_C_FLAGS% shaders\%%s
))
