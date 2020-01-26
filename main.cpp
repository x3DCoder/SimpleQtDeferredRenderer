#define _DEBUG

#include "libs/v4d/common.h"

#include "mainwindow.h"
#include <QApplication>
#include <QVulkanInstance>

#include "DeferredRenderer.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QVulkanInstance vulkanInstance;
    v4d::graphics::vulkan::Loader vulkanLoader(&vulkanInstance);
	vulkanLoader();
    
    // Qt Window
    MainWindow window;
    vulkanInstance.setExtensions(QByteArrayList()
        << "VK_KHR_surface"
        << "VK_KHR_xcb_surface"
    );
    window.setSurfaceType(QSurface::SurfaceType::VulkanSurface);
    window.setVulkanInstance(&vulkanInstance);
    window.show();

    // Renderer
    DeferredRenderer renderer(&vulkanLoader, "Simple Deferred Renderer", VK_MAKE_VERSION(1, 0, 0), &window);
    renderer.preferredPresentModes = {
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		VK_PRESENT_MODE_IMMEDIATE_KHR,
	};
	renderer.InitRenderer();
	renderer.ReadShaders();
	renderer.LoadScene();
	renderer.LoadRenderer();

    // Game Loop
    std::thread gameLoopThread ([&]{
        while (window.isVisible()) {
            {
                std::lock_guard lock(window.keysEventMutex);

                if (window.wasKeyPressed(Qt::Key_Escape))
                    break;
                
                window.resetKeysPressed();
            }
            
            renderer.Render();

            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
        window.close();
    });

    // Wait until window is closed and rendering is finished
    app.exec();
    gameLoopThread.join();

    // Unload
	renderer.UnloadRenderer();
	renderer.UnloadScene();
}
