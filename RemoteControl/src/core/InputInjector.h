#ifndef INPUTINJECTOR_H
#define INPUTINJECTOR_H

#include <Qt>

class InputInjector
{
public:
    InputInjector() = delete;

    // ── Mouse ──
    static void mouseMove(int x, int y);
    static void mousePress(Qt::MouseButton button);
    static void mouseRelease(Qt::MouseButton button);
    static void mouseWheel(int delta);

    // ── Keyboard (Win32 virtual-key codes) ──
    static void keyPress(int vkCode);
    static void keyRelease(int vkCode);
};

#endif // INPUTINJECTOR_H
