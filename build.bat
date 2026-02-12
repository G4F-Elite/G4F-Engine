@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM G4F-Engine build for Win64 MinGW-w64.

set "CXX=g++"
set "AR=ar"

set "OUT=out"
set "OBJ=%OUT%\obj"
set "LIB=%OUT%\lib"
set "BIN=%OUT%\bin"

if "%1"=="clean" (
  if exist "%OUT%" rmdir /s /q "%OUT%"
  echo Cleaned "%OUT%".
  exit /b 0
)

where %CXX% >nul 2>nul
if errorlevel 1 (
  echo ERROR: g++ not found in PATH.
  exit /b 1
)

if not exist "%OBJ%\engine" mkdir "%OBJ%\engine"
if not exist "%LIB%" mkdir "%LIB%"
if not exist "%BIN%" mkdir "%BIN%"
if not exist "%BIN%\backrooms-tests" mkdir "%BIN%\backrooms-tests"

set "CXXFLAGS=-std=c++20 -O2 -g -Wall -Wextra -Wpedantic"
set "INC_ENGINE=-Iengine\include"
set "INC_COMPAT=-Icompat\include"

set "LD_ENGINE=-lole32 -luuid -ld2d1 -ldwrite -lwindowscodecs -lgdi32 -luser32"
set "LD_ENGINE_3D=-lole32 -luuid -lgdi32 -luser32 -ld3d11 -ldxgi -ld3dcompiler_47 -ld2d1 -ldwrite -lwindowscodecs"
set "LD_ENGINE_ALL=%LD_ENGINE% %LD_ENGINE_3D%"
set "LD_BACKROOMS=-lws2_32 -lwinmm"

echo === Build: engine (static lib) ===
set "ENGINE_OBJ=%OBJ%\engine"

%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_utf8_win32.cpp -o "%ENGINE_OBJ%\g4f_utf8_win32.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_math.cpp -o "%ENGINE_OBJ%\g4f_math.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_keycodes_win32.cpp -o "%ENGINE_OBJ%\g4f_keycodes_win32.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_win32_window.cpp -o "%ENGINE_OBJ%\g4f_win32_window.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_d2d_renderer.cpp -o "%ENGINE_OBJ%\g4f_d2d_renderer.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_ctx.cpp -o "%ENGINE_OBJ%\g4f_ctx.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_d3d11_gfx.cpp -o "%ENGINE_OBJ%\g4f_d3d11_gfx.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_ctx3d.cpp -o "%ENGINE_OBJ%\g4f_ctx3d.o" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% -c engine\src\g4f_ui.cpp -o "%ENGINE_OBJ%\g4f_ui.o" || exit /b 1

%AR% rcs "%LIB%\libg4f.a" "%ENGINE_OBJ%\g4f_utf8_win32.o" "%ENGINE_OBJ%\g4f_math.o" "%ENGINE_OBJ%\g4f_keycodes_win32.o" "%ENGINE_OBJ%\g4f_win32_window.o" "%ENGINE_OBJ%\g4f_d2d_renderer.o" "%ENGINE_OBJ%\g4f_ctx.o" "%ENGINE_OBJ%\g4f_d3d11_gfx.o" "%ENGINE_OBJ%\g4f_ctx3d.o" "%ENGINE_OBJ%\g4f_ui.o" || exit /b 1

echo === Build: samples ===
%CXX% %CXXFLAGS% %INC_ENGINE% samples\hello2d\main.cpp -L"%LIB%" -lg4f %LD_ENGINE% -o "%BIN%\hello2d.exe" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% samples\backrooms_menu_smoke\main.cpp -L"%LIB%" -lg4f %LD_ENGINE% -o "%BIN%\backrooms_menu_smoke.exe" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% samples\spin_cube\main.cpp -L"%LIB%" -lg4f %LD_ENGINE_3D% -o "%BIN%\spin_cube.exe" || exit /b 1

echo === Build: engine tests ===
%CXX% %CXXFLAGS% %INC_ENGINE% tests\engine_keycodes_tests.cpp -L"%LIB%" -lg4f %LD_ENGINE_ALL% -o "%BIN%\engine_keycodes_tests.exe" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% tests\ui_layout_tests.cpp -L"%LIB%" -lg4f %LD_ENGINE_ALL% -o "%BIN%\ui_layout_tests.exe" || exit /b 1
%CXX% %CXXFLAGS% %INC_ENGINE% tests\math_tests.cpp -L"%LIB%" -lg4f %LD_ENGINE_ALL% -o "%BIN%\math_tests.exe" || exit /b 1

echo === Run: engine tests ===
"%BIN%\engine_keycodes_tests.exe" || exit /b 1
"%BIN%\ui_layout_tests.exe" || exit /b 1
"%BIN%\math_tests.exe" || exit /b 1

echo === Build: Backrooms tests (no GLFW) ===
for %%F in (Backrooms-master\tests\*.cpp) do (
  set "NAME=%%~nF"
  echo [Backrooms] %%~nxF
  %CXX% %CXXFLAGS% %INC_COMPAT% "%%F" -o "%BIN%\backrooms-tests\!NAME!.exe" %LD_BACKROOMS% || exit /b 1
)

echo === Run: Backrooms tests ===
for %%E in ("%BIN%\backrooms-tests"\*.exe) do (
  echo [Backrooms] %%~nxE
  "%%E" || exit /b 1
)

echo === OK ===
exit /b 0
