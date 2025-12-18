// test_line_number.cpp - 行号计算测试
#include "pch.h"

using namespace TestableLogic;

// ============ 行数统计测试 ============

TEST(LineNumberTest, CountLines_Empty)
{
    EXPECT_EQ(CountLines(L""), 1);
    EXPECT_EQ(CountLines(nullptr), 1);
}

TEST(LineNumberTest, CountLines_SingleLine)
{
    EXPECT_EQ(CountLines(L"Hello World"), 1);
}

TEST(LineNumberTest, CountLines_MultipleLines)
{
    EXPECT_EQ(CountLines(L"Line1\nLine2\nLine3"), 3);
    EXPECT_EQ(CountLines(L"Line1\nLine2\n"), 3);  // 末尾换行
}

TEST(LineNumberTest, CountLines_OnlyNewlines)
{
    EXPECT_EQ(CountLines(L"\n\n\n"), 4);
}

TEST(LineNumberTest, CountLines_WindowsLineEndings)
{
    // Windows 换行 \r\n，\r 不单独计数
    EXPECT_EQ(CountLines(L"Line1\r\nLine2\r\nLine3"), 3);
}

// ============ 行号宽度计算测试 ============

TEST(LineNumberTest, CalculateWidth_SmallFile)
{
    int width = CalculateLineNumberWidth(10, 8);
    // 最少3位 + 2边距 = 5 * 8 = 40
    EXPECT_GE(width, 40);
}

TEST(LineNumberTest, CalculateWidth_LargeFile)
{
    int width = CalculateLineNumberWidth(10000, 8);
    // 5位 + 2边距 = 7 * 8 = 56
    EXPECT_GE(width, 56);
}

TEST(LineNumberTest, CalculateWidth_ZeroLines)
{
    int width = CalculateLineNumberWidth(0, 8);
    EXPECT_GT(width, 0);  // 应该有默认值
}

TEST(LineNumberTest, CalculateWidth_ZeroCharWidth)
{
    int width = CalculateLineNumberWidth(100, 0);
    EXPECT_GT(width, 0);  // 应该有默认值
}

// ============ 行起始位置测试 ============

TEST(LineNumberTest, GetLineStart_FirstLine)
{
    EXPECT_EQ(GetLineStartPosition(L"Line1\nLine2\nLine3", 0), 0);
}

TEST(LineNumberTest, GetLineStart_SecondLine)
{
    EXPECT_EQ(GetLineStartPosition(L"Line1\nLine2\nLine3", 1), 6);  // "Line1\n" 长度为6
}

TEST(LineNumberTest, GetLineStart_ThirdLine)
{
    EXPECT_EQ(GetLineStartPosition(L"Line1\nLine2\nLine3", 2), 12);
}

TEST(LineNumberTest, GetLineStart_BeyondEnd)
{
    const wchar_t* text = L"Line1\nLine2";
    int pos = GetLineStartPosition(text, 10);  // 超出范围
    EXPECT_EQ(pos, (int)wcslen(text));  // 返回末尾
}

TEST(LineNumberTest, GetLineStart_EmptyLines)
{
    EXPECT_EQ(GetLineStartPosition(L"\n\n\n", 0), 0);
    EXPECT_EQ(GetLineStartPosition(L"\n\n\n", 1), 1);
    EXPECT_EQ(GetLineStartPosition(L"\n\n\n", 2), 2);
}

// ============ 字符位置转行号测试 ============

TEST(LineNumberTest, GetLineFromPos_FirstLine)
{
    EXPECT_EQ(GetLineFromCharPosition(L"Line1\nLine2\nLine3", 0), 0);
    EXPECT_EQ(GetLineFromCharPosition(L"Line1\nLine2\nLine3", 3), 0);
}

TEST(LineNumberTest, GetLineFromPos_SecondLine)
{
    EXPECT_EQ(GetLineFromCharPosition(L"Line1\nLine2\nLine3", 6), 1);
    EXPECT_EQ(GetLineFromCharPosition(L"Line1\nLine2\nLine3", 10), 1);
}

TEST(LineNumberTest, GetLineFromPos_ThirdLine)
{
    EXPECT_EQ(GetLineFromCharPosition(L"Line1\nLine2\nLine3", 12), 2);
}

TEST(LineNumberTest, GetLineFromPos_NullText)
{
    EXPECT_EQ(GetLineFromCharPosition(nullptr, 5), 0);
}

TEST(LineNumberTest, GetLineFromPos_NegativePos)
{
    EXPECT_EQ(GetLineFromCharPosition(L"Test", -1), 0);
}

// ============ 行号同步一致性测试 ============

TEST(LineNumberTest, LinePositionConsistency)
{
    const wchar_t* text = L"First line\nSecond line\nThird line\nFourth line";

    int lineCount = CountLines(text);
    EXPECT_EQ(lineCount, 4);

    // 每一行的起始位置应该能反推出正确的行号
    for (int line = 0; line < lineCount; line++)
    {
        int startPos = GetLineStartPosition(text, line);
        int computedLine = GetLineFromCharPosition(text, startPos);
        EXPECT_EQ(computedLine, line) << "Line " << line << " mismatch";
    }
}
