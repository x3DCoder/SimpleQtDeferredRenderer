QT += core gui

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
    DeferredRenderer.hpp \
    libs/v4d/common.h \
    libs/v4d/graphics/Camera.hpp \
    libs/v4d/graphics/LightSource.hpp \
    libs/v4d/graphics/PrimitiveGeometry.hpp \
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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Shaders
DISTFILES += \
    shaders/lighting.frag \
    shaders/lighting.vert \
    shaders/primitives.frag \
    shaders/primitives.vert
linux {
  shaders.commands = for s in $${DISTFILES} ; \
    do glslangValidator -V ../$${TARGET}/\"\$\$s\" -o ../$${TARGET}/\"\$\$s\".spv ; \
    done ; \
    mkdir -p shaders && cp ../$${TARGET}/shaders/*.spv shaders/
  QMAKE_EXTRA_TARGETS += shaders
  POST_TARGETDEPS += shaders
}
win32 {
  QMAKE_POST_LINK += $(MKDIR) $$quote(.\\shaders\\) $$escape_expand(\\n\\t)
  for (FILE, DISTFILES) {
    win32:FILE ~= s,/,\\,g
    QMAKE_POST_LINK += $$QMAKE_COPY $$quote(..\\$${TARGET}\\$${FILE}.spv) $$quote(.\\shaders\\) $$escape_expand(\\n\\t)
  }
}

# I am using region pragmas to help organize code in vscode, its a shame that Qt does not support them
# anyhow, I am not too worried about mistakenly having unknown pragmas for this simple project, suppressing warnings is acceptable.
QMAKE_CXXFLAGS += -Wno-unknown-pragmas

win32 {
  # On windows, the compiler does not like the unused variables in C++17's structured bindings
  # and having [[maybe-unused]] inside them would make the code very uggly...
  # I am developing with Linux in which GCC have better support for these
  # hence removing the unused variables warnings only for windows is great
  # because I can still see these warnings on Linux when actually debugging.
  # We would definitely need to find a better handling of unused variables in structured bindings for the sake of windows devs
  # but for the sake of this simple project, this will do for now.
  QMAKE_CXXFLAGS += -Wno-unused-variable
}
