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
    vulkanInstance.setLayers(QByteArrayList()
        << "VK_LAYER_LUNARG_standard_validation"
    );
    vulkanInstance.setExtensions(QByteArrayList()
        << "VK_KHR_surface"
        << "VK_KHR_xcb_surface"
    );
    window.setSurfaceType(QSurface::SurfaceType::VulkanSurface);
    window.setVulkanInstance(&vulkanInstance);
    window.show();

    // Renderer
    DeferredRenderer renderer(&vulkanLoader, &window);
    renderer.preferredPresentModes = {
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		VK_PRESENT_MODE_IMMEDIATE_KHR,
	};
	renderer.InitRenderer();
	renderer.ReadShaders();
	renderer.LoadScene();
	renderer.LoadRenderer();

    struct PlayerView {
        double camSpeed = 10.0, mouseSensitivity = 1.0, tiltSpeed = 2.0;
        double horizontalAngle = 0;
        double verticalAngle = 0;
        glm::dvec3 worldPosition {0,-3,0};
        glm::dvec3 velocity {0};
        glm::dvec3 viewUp = {0,0,1};
        glm::dvec3 viewForward {0,1,0};
        glm::dvec3 viewRight = glm::cross(viewForward, viewUp);
        bool useFreeFlyCam = false;
        glm::dmat4 freeFlyCamRotationMatrix {1};
    } player;

    // Game Loop
    std::thread gameLoopThread ([&]{
        while (window.isVisible()) {

            {// Capture keyboard and player movements
                std::lock_guard lock(window.eventMutex);

                if (window.wasKeyPressed(Qt::Key_Escape))
                    break;

                double deltaTime = 0.01f; // This would be calculated based on frame time, but for this simple app we will hardcode it because framerate should not be an issue.
                
                player.velocity = glm::dvec3{0};
                
                if (window.isKeyDown(Qt::Key_W)) {
                    player.velocity += +player.viewForward * player.camSpeed;
                }
                if (window.isKeyDown(Qt::Key_S)) {
                    player.velocity += -player.viewForward * player.camSpeed;
                }
                if (window.isKeyDown(Qt::Key_A)) {
                    player.velocity += -player.viewRight * player.camSpeed;
                }
                if (window.isKeyDown(Qt::Key_D)) {
                    player.velocity += +player.viewRight * player.camSpeed;
                }
                if (window.isKeyDown(Qt::Key_Space)) {
                    player.velocity += +player.viewUp * player.camSpeed;
                }
                if (window.isKeyDown(Qt::Key_Control)) {
                    player.velocity += -player.viewUp * player.camSpeed;
                }
                
                player.worldPosition += player.velocity * deltaTime;

                if (window.isAnyMouseBtnDown()) {
                    auto[x, y] = window.getMouseMovements();
                    if (player.useFreeFlyCam) {
                        if (x != 0) {
                            player.freeFlyCamRotationMatrix = glm::rotate(player.freeFlyCamRotationMatrix, x * player.mouseSensitivity * deltaTime, player.viewUp);
                        }
                        if (y != 0) {
                            player.freeFlyCamRotationMatrix = glm::rotate(player.freeFlyCamRotationMatrix, y * player.mouseSensitivity * deltaTime, player.viewRight);
                        }
                        if (window.isKeyDown(Qt::Key_Q)) {
                            player.freeFlyCamRotationMatrix = glm::rotate(player.freeFlyCamRotationMatrix, player.tiltSpeed * deltaTime, player.viewForward);
                        }
                        if (window.isKeyDown(Qt::Key_E)) {
                            player.freeFlyCamRotationMatrix = glm::rotate(player.freeFlyCamRotationMatrix, -player.tiltSpeed * deltaTime, player.viewForward);
                        }
                        player.viewUp = glm::normalize(glm::dvec3(glm::inverse(player.freeFlyCamRotationMatrix) * glm::dvec4(0,0,1, 0)));
                        player.viewForward = glm::normalize(glm::dvec3(glm::inverse(player.freeFlyCamRotationMatrix) * glm::dvec4(0,1,0, 0)));
                        player.viewRight = glm::cross(player.viewForward, player.viewUp);
                    } else {
                        if (x != 0 || y != 0) {
                            player.horizontalAngle += double(x * player.mouseSensitivity * deltaTime);
                            player.verticalAngle -= double(y * player.mouseSensitivity * deltaTime);
                            if (player.verticalAngle < -1.5) player.verticalAngle = -1.5;
                            if (player.verticalAngle > 1.5) player.verticalAngle = 1.5;
                            player.viewForward = glm::normalize(glm::dvec3(
                                cos(player.verticalAngle) * sin(player.horizontalAngle),
                                cos(player.verticalAngle) * cos(player.horizontalAngle),
                                sin(player.verticalAngle)
                            ));
                            player.viewRight = glm::cross(player.viewForward, player.viewUp);
                        }
                    }
                }
                
                window.resetEvents();
            }

            // Update camera position
            renderer.camera.worldPosition = player.worldPosition;
            renderer.camera.lookDirection = player.viewForward;
            renderer.camera.viewUp = player.viewUp;
            
            // Draw a frame
            renderer.Render();

            // sleep
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
        // Unload
        renderer.UnloadRenderer();
        renderer.UnloadScene();
        window.close();
    });

    // Wait until window is closed and rendering is finished
    int appReturnValue = app.exec();
    gameLoopThread.join();

    return appReturnValue;
}
