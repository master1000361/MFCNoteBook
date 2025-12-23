一个基于 MFC (Microsoft Foundation Classes) 的现代化加密记事本应用，支持自定义文件格式（.mynote）、AES-128 加密、SHA-1 完整性校验、主题切换、行号显示、查找替换等功能。
环境要求

操作系统: Windows 10/11

IDE: Visual Studio 2022（含 MFC 组件）

工具:

OpenCppCoverage（用于覆盖率分析）

Google Test（已通过 NuGet 集成）

安装步骤:

克隆仓库:git clone https://github.com/master1000361/MFCNoteBook.git

cd MFCNoteBook

配置学号和密钥

编辑 MFCNoteBook/config.ini：

[User]

StudentID=20250313017Z

[Security]

SecretKey=BIGC_AI_2025_KEY

打开解决方案
   
start MFCNoteBook.sln

编译项目

选择 Debug 或 Release 配置

按 Ctrl+Shift+B 编译

运行应用

按 F5 启动调试

或直接运行 Debug/MFCNoteBook.exe

单元测试

测试框架

Google Test 1.12.0

覆盖率工具: OpenCppCoverage

测试文件: 6 个测试模块，70 个测试用例

运行测试

方法 1：批处理脚本（推荐）:run_coverage.bat

方法 2：手动运行

1. 运行测试

x64\Debug\MFCNoteBookTests.exe

2. 生成覆盖率报告

OpenCppCoverage.exe --sources MFCNoteBook ^

  --export_type html:coverage_report ^
  
  -- x64\Debug\MFCNoteBookTests.exe

3. 查看报告

start coverage_report\index.html

运行 run_coverage.bat 后，会生成 HTML 报告。

