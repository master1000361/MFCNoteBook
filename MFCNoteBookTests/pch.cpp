// pch.cpp - 测试项目的预编译源文件
#include "pch.h"

// Google Test 主函数
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment);
    return RUN_ALL_TESTS();
}
