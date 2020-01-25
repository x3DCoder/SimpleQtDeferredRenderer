QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    libs/v4d/graphics/vulkan/Buffer.cpp \
    libs/v4d/graphics/vulkan/ComputeShaderPipeline.cpp \
    libs/v4d/graphics/vulkan/DescriptorSet.cpp \
    libs/v4d/graphics/vulkan/Device.cpp \
    libs/v4d/graphics/vulkan/Image.cpp \
    libs/v4d/graphics/vulkan/Instance.cpp \
    libs/v4d/graphics/vulkan/Loader.cpp \
    libs/v4d/graphics/vulkan/PhysicalDevice.cpp \
    libs/v4d/graphics/vulkan/PipelineLayout.cpp \
    libs/v4d/graphics/vulkan/RasterShaderPipeline.cpp \
    libs/v4d/graphics/vulkan/RenderPass.cpp \
    libs/v4d/graphics/vulkan/Shader.cpp \
    libs/v4d/graphics/vulkan/ShaderPipeline.cpp \
    libs/v4d/graphics/vulkan/ShaderProgram.cpp \
    libs/v4d/graphics/vulkan/SwapChain.cpp \
    libs/v4d/graphics/Renderer.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    libs/v4d/common.h \
    libs/v4d/graphics/vulkan/Buffer.h \
    libs/v4d/graphics/vulkan/ComputeShaderPipeline.h \
    libs/v4d/graphics/vulkan/DescriptorSet.h \
    libs/v4d/graphics/vulkan/Device.h \
    libs/v4d/graphics/vulkan/Image.h \
    libs/v4d/graphics/vulkan/Instance.h \
    libs/v4d/graphics/vulkan/Loader.h \
    libs/v4d/graphics/vulkan/PhysicalDevice.h \
    libs/v4d/graphics/vulkan/PipelineLayout.h \
    libs/v4d/graphics/vulkan/RasterShaderPipeline.h \
    libs/v4d/graphics/vulkan/RenderPass.h \
    libs/v4d/graphics/vulkan/Shader.h \
    libs/v4d/graphics/vulkan/ShaderPipeline.h \
    libs/v4d/graphics/vulkan/ShaderProgram.h \
    libs/v4d/graphics/vulkan/SwapChain.h \
    libs/v4d/graphics/Renderer.h \
    mainwindow.h

INCLUDEPATH += libs/xvk
INCLUDEPATH += libs/xvk/glm

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
