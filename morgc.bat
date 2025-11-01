@echo off
setlocal enabledelayedexpansion
rem Salva o diretório atual
set "OLDPWD=%CD%"
cd /d "%~dp0"
where perl >nul 2>nul

if errorlevel 1 (
    echo [ERRO] Perl não encontrado no PATH.
    echo Instale o Perl ou adicione-o às variáveis de ambiente.
    cd /d "%OLDPWD%"
    exit /b 1
)

perl main.pl %*
set "EXITCODE=%ERRORLEVEL%"
cd /d "%OLDPWD%"
endlocal & exit /b %EXITCODE%
