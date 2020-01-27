# Simple Qt Deferred Renderer

```
git clone --recursive git@github.com:x3DCoder/SimpleQtDeferredRenderer.git
```

This is a very basic deferred renderer implemented using QtCreator and a part of the Vulkan4D game engine (only the Vulkan abstraction)

It compiles using QtCreator, on both Linux and Windows 64 bit

Everything related to deferred rendering is located in DeferredRenderer.hpp directly in the project's root. 

#### Usage
- To look around : Click in the screen while moving the cursor
- To move in the scene : WASD+CTRL+SPACE

#### Structure
- `libs/v4d/` classes taken from Vulkan4D (my own engine) and simplified for this project
- `libs/xvk/` a vulkan dynamic loader (also my own creation)
- `shaders/` shaders files...
- `DeferredRenderer.hpp` this is the renderer and contains everything related to deferred rendering
- `main.cpp` app starts here and contains the gameloop and player controls
- `mainwindow.cpp/.h` everything related to Qt (not much really, a simple QWindow)

