@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build

where cl >nul 2>nul
if errorlevel 1 (
  for %%P in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
    "%ProgramFiles%\Microsoft Visual Studio\2023\Enterprise"
  ) do (
    if exist "%%~P\VC\Auxiliary\Build\vcvars64.bat" (
      call "%%~P\VC\Auxiliary\Build\vcvars64.bat" >nul
      goto :got
    )
rem revisit
  )
  echo [!] cl / vcvars64.bat not found
  exit /b 1
)
:got

echo [*] layout check
cl /nologo /TC /W3 /Isrc /Fe:build\layout.exe tools\layout.c >nul
if errorlevel 2 goto :fail
build\layout.exe
if errorlevel 1 goto :fail

echo [*] compiling
cl /nologo /c /TC /GS- /Zl /O1 /W3 /Isrc /Fo:build\ src\efiprobe.c src\crt.c
if errorlevel 1 goto :fail

rem revisit
echo [*] linking
link /nologo /SUBSYSTEM:EFI_APPLICATION /ENTRY:efi_main /NODEFAULTLIB ^
     /MACHINE:X64 /NXCOMPAT:NO /BASE:0x0 /IGNORE:4281 /OUT:build\efiprobe.efi ^
     build\efiprobe.obj build\crt.obj
if errorlevel 1 goto :fail

echo [*] stack probe check
dumpbin /disasm build\efiprobe.obj build\crt.obj > build\disasm.txt 2>nul
set "PY=py"
where py >nul 2>nul || set "PY=python"
%PY% tools\chkstk.py build\disasm.txt
if errorlevel 1 goto :fail

echo [*] pe check
%PY% tools\pecheck.py build\efiprobe.efi
if errorlevel 1 goto :fail

