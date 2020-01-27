#include "mainwindow.h"

MainWindow::MainWindow(QWindow* parent) : QWindow(parent) {
    setTitle("TEST");
    resize(1280,720);
}

MainWindow::~MainWindow() {}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    std::lock_guard lock(eventMutex);
    keysDown[event->key()] = true;
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    std::lock_guard lock(eventMutex);
    keysDown[event->key()] = false;
    keysPressed.push_back(event->key());
}

void MainWindow::mousePressEvent(QMouseEvent *) {
    mouseBtnDown = true;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *) {
    mouseBtnDown = false;
}

bool MainWindow::isKeyDown(int key) {
    std::lock_guard lock(eventMutex);
    return keysDown[key];
}

bool MainWindow::isAnyMouseBtnDown() const {
    return mouseBtnDown;
}

bool MainWindow::wasKeyPressed(int key) {
    std::lock_guard lock(eventMutex);
    return std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end();
}

void MainWindow::resetEvents() {
    std::lock_guard lock(eventMutex);
    keysPressed.clear();
    mouseX = mapFromGlobal(QCursor::pos()).x();
    mouseY = mapFromGlobal(QCursor::pos()).y();
}

std::tuple<float, float> MainWindow::getMouseMovements() {
    std::lock_guard lock(eventMutex);
    return {
        mapFromGlobal(QCursor::pos()).x() - mouseX,
        mapFromGlobal(QCursor::pos()).y() - mouseY,
    };
}

