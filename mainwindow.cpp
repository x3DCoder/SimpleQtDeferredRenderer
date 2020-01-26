#include "mainwindow.h"

MainWindow::MainWindow(QWindow* parent) : QWindow(parent) {
    setTitle("TEST");
    resize(1280,720);
}

MainWindow::~MainWindow() {}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    std::lock_guard lock(keysEventMutex);
    std::cout << "KeyPress: " << event->key() << std::endl;
    keysDown[event->key()] = true;
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    std::lock_guard lock(keysEventMutex);
    std::cout << "KeyRelease: " << event->key() << std::endl;
    keysDown[event->key()] = false;
    keysPressed.push_back(event->key());
}

bool MainWindow::isKeyDown(int key) {
    std::lock_guard lock(keysEventMutex);
    return keysDown[key];
}

bool MainWindow::wasKeyPressed(int key) {
    std::lock_guard lock(keysEventMutex);
    return std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end();
}

void MainWindow::resetKeysPressed() {
    std::lock_guard lock(keysEventMutex);
    keysPressed.clear();
}
