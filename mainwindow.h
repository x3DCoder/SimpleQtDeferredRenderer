#pragma once
#include "libs/v4d/common.h"

#include <QWindow>
#include <QKeyEvent>

class MainWindow : public QWindow
{
    Q_OBJECT

    std::map<int, bool> keysDown {};
    std::vector<int> keysPressed {};

public:
    std::recursive_mutex keysEventMutex;
    
    MainWindow(QWindow* parent = nullptr);
    ~MainWindow();

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    bool isKeyDown(int key);
    bool wasKeyPressed(int key);
    void resetKeysPressed();

};
