// test_theme.cpp - 主题颜色测试
#include "pch.h"

using namespace TestableLogic;

// ============ 主题颜色测试 ============

TEST(ThemeTest, LightTheme_Colors)
{
    auto colors = GetThemeColors(TestableTheme::Light);

    // 亮色主题：背景应该是白色或接近白色
    EXPECT_EQ(colors.clrEditBg, RGB(255, 255, 255));
    EXPECT_EQ(colors.clrEditText, RGB(0, 0, 0));
}

TEST(ThemeTest, DarkTheme_Colors)
{
    auto colors = GetThemeColors(TestableTheme::Dark);

    // 暗色主题：背景应该是深色
    EXPECT_EQ(colors.clrEditBg, RGB(30, 30, 30));
    EXPECT_EQ(colors.clrEditText, RGB(220, 220, 220));
}

TEST(ThemeTest, LightTheme_HasValidContrast)
{
    auto colors = GetThemeColors(TestableTheme::Light);
    EXPECT_TRUE(ValidateThemeContrast(colors));
}

TEST(ThemeTest, DarkTheme_HasValidContrast)
{
    auto colors = GetThemeColors(TestableTheme::Dark);
    EXPECT_TRUE(ValidateThemeContrast(colors));
}

TEST(ThemeTest, ValidateContrast_LowContrast)
{
    TestableThemeColors badColors;
    badColors.clrEditBg = RGB(128, 128, 128);
    badColors.clrEditText = RGB(130, 130, 130);  // 几乎相同
    badColors.clrLineNumBg = RGB(128, 128, 128);
    badColors.clrLineNumText = RGB(128, 128, 128);
    badColors.clrLineNumBorder = RGB(128, 128, 128);

    EXPECT_FALSE(ValidateThemeContrast(badColors));
}

TEST(ThemeTest, DifferentThemes_DifferentColors)
{
    auto light = GetThemeColors(TestableTheme::Light);
    auto dark = GetThemeColors(TestableTheme::Dark);

    EXPECT_NE(light.clrEditBg, dark.clrEditBg);
    EXPECT_NE(light.clrEditText, dark.clrEditText);
}

TEST(ThemeTest, LineNumberColors_Valid)
{
    auto light = GetThemeColors(TestableTheme::Light);
    auto dark = GetThemeColors(TestableTheme::Dark);

    // 行号颜色不应该是 0
    EXPECT_NE(light.clrLineNumBg, 0u);
    EXPECT_NE(light.clrLineNumText, 0u);
    EXPECT_NE(dark.clrLineNumBg, 0u);
    EXPECT_NE(dark.clrLineNumText, 0u);
}
