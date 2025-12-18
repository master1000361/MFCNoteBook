// pch.h - 测试项目的预编译头
#pragma once

// ============ 强制定义 _AFXDLL（使用 MFC 共享 DLL）============
#ifndef _AFXDLL
#define _AFXDLL
#endif

// ============ MFC/ATL 头文件（必须在最前面） ============
#include <afxwin.h>         // MFC 核心
#include <afxext.h>         // MFC 扩展
#include <atlconv.h>        // CT2A, CA2T 等转换宏
#include <atlstr.h>         // CString

// ============ Google Test ============
#include "gtest/gtest.h"

// ============ 标准库 ============
#include <string>
#include <vector>
#include <memory>

// ============ 项目头文件 ============
#include "../MFCNoteBook/TestableLogic.h"
#include "../MFCNoteBook/ConfigManager.h"

// ============ 测试辅助函数 ============
inline void InitTestConfig()
{
    CConfigManager& config = CConfigManager::GetInstance();
    config.SetStudentID(_T("20250313017Z"));
    config.SetSecretKey(_T("BIGC_AI_2025_KEY"));
}

// 全局测试环境
class TestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        InitTestConfig();
    }
};
