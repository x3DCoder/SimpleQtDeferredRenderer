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

    bool mouseBtnDown = false;

public:
    std::recursive_mutex eventMutex;
    
    MainWindow(QWindow* parent = nullptr);
    ~MainWindow();

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    
    bool isKeyDown(int key);
    bool isAnyMouseBtnDown() const;
    bool wasKeyPressed(int key);
    void resetEvents();
    
    std::tuple<float, float> getMouseMovements();

};
