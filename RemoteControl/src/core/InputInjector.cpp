#include "InputInjector.h"

#include <windows.h>
#include <QDebug>

// 判断一个键是否需要 EXTENDEDKEY 标志
static bool isExtendedKey(int vkCode)
{
    switch (vkCode) {
    case VK_RCONTROL: case VK_RMENU:   // Right Ctrl/Alt
    case VK_INSERT:   case VK_DELETE:
    case VK_HOME:     case VK_END:
    case VK_PRIOR:    case VK_NEXT:    // PageUp / PageDown
    case VK_LEFT:     case VK_RIGHT:
    case VK_UP:       case VK_DOWN:
    case VK_DIVIDE:                     // Numpad /
        return true;
    default: return false;
    }
}

void InputInjector::mouseMove(int x, int y)
{
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN); // 虚拟桌面（支持多显示器）
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = (sw > 0) ? (x * 65535) / sw : 0;
    input.mi.dy = (sh > 0) ? (y * 65535) / sh : 0;

    UINT ret = SendInput(1, &input, sizeof(INPUT));
    if (ret != 1) {
        qWarning() << "SendInput mouseMove failed, error:" << GetLastError();
    }
}

void InputInjector::mousePress(Qt::MouseButton button)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    switch (button) {
    case Qt::LeftButton:    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;   break;
    case Qt::RightButton:   input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
    case Qt::MiddleButton:  input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
    default: return;
    }
    UINT ret = SendInput(1, &input, sizeof(INPUT));
    if (ret != 1) qWarning() << "SendInput mousePress failed";
}

void InputInjector::mouseRelease(Qt::MouseButton button)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    switch (button) {
    case Qt::LeftButton:    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;   break;
    case Qt::RightButton:   input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;  break;
    case Qt::MiddleButton:  input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
    default: return;
    }
    UINT ret = SendInput(1, &input, sizeof(INPUT));
    if (ret != 1) qWarning() << "SendInput mouseRelease failed";
}

void InputInjector::mouseWheel(int delta)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(delta);
    UINT ret = SendInput(1, &input, sizeof(INPUT));
    if (ret != 1) qWarning() << "SendInput mouseWheel failed";
}

void InputInjector::keyPress(int vkCode)
{
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(vkCode);
    if (isExtendedKey(vkCode))
        input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    UINT ret = SendInput(1, &input, sizeof(INPUT));
    if (ret != 1) qWarning() << "SendInput keyPress failed, vk:" << vkCode;
}

void InputInjector::keyRelease(int vkCode)
{
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(vkCode);
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    if (isExtendedKey(vkCode))
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    UINT ret = SendInput(1, &input, sizeof(INPUT));
    if (ret != 1) qWarning() << "SendInput keyRelease failed, vk:" << vkCode;
}
