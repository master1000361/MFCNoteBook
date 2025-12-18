@echo off
chcp 65001 >nul
echo ========================================
echo    OpenCppCoverage 代码覆盖率分析
echo ========================================
echo.

cd /d "%~dp0"

set "OPENCPPCOV=D:\OpenCppCoverage\Install\OpenCppCoverage\OpenCppCoverage.exe"
set "TEST_EXE=D:\MFC\Visual studio2022\project\MFCNoteBook\x64\Debug\MFCNoteBookTests.exe"

:: 检查文件
if not exist "%OPENCPPCOV%" (
    echo [错误] 找不到 OpenCppCoverage
    pause
    exit /b 1
)

if not exist "%TEST_EXE%" (
    echo [错误] 找不到测试程序
    pause
    exit /b 1
)

echo [信息] 开始运行覆盖率分析...
echo.

if exist "CoverageReport" rmdir /s /q "CoverageReport"

:: 关键修改：使用项目根目录作为 sources
"%OPENCPPCOV%" --sources "D:\MFC\Visual studio2022\project\MFCNoteBook" --export_type html:CoverageReport -- "%TEST_EXE%"

echo.
if exist "CoverageReport\index.html" (
    echo [成功] 报告已生成
    start "" "CoverageReport\index.html"
) else (
    echo [失败] 报告未生成
)

pause
