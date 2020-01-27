#pragma once
#include "libs/v4d/common.h"

#include <QWindow>
#include <QKeyEvent>

class MainWindow : public QWindow
{
    Q_OBJECT

    std::map<int, bool> keysDown {};
    std::vector<int> keysPressed {};

    float mouseX = 0;
    float mouseY = 0;

public:
    std::recursive_mutex eventMutex;
    
    MainWindow(QWindow* parent = nullptr);
    ~MainWindow();

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    bool isKeyDown(int key);
    bool wasKeyPressed(int key);
    void resetEvents();
    
    std::tuple<float, float> getMouseMovements();

};
