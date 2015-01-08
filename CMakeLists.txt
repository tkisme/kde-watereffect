# This file was derivered from kdebase/workspace/kwin/effects/CMakeLists.txt. I hope it will work separately.

project(watereffect)

find_package(KDE4 REQUIRED)
find_package(OpenGL REQUIRED)

include(MacroLibrary)
include(KDE4Defaults)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

# Adds effect plugin with given name. Sources are given after the name
macro(KWIN4_ADD_EFFECT name)
    kde4_add_plugin(kwin4_effect_${name} ${ARGN})
    target_link_libraries(kwin4_effect_${name} kwineffects ${KDE4_KDEUI_LIBS} ${OPENGL_gl_LIBRARY})
    install(TARGETS kwin4_effect_${name} DESTINATION ${PLUGIN_INSTALL_DIR})
endmacro(KWIN4_ADD_EFFECT)

macro(KWIN4_ADD_EFFECT_CONFIG name)
    set(kwin4_effect_ui ) #empty
    set(kwin4_effect_src ) #empty
    foreach(file ${ARGN})
        if(file MATCHES \\.ui)
            set(kwin4_effect_ui ${kwin4_effect_ui} ${file})
        else(file MATCHES \\.ui)
            set(kwin4_effect_src ${kwin4_effect_src} ${file})
        endif(file MATCHES \\.ui)
    endforeach(file)
    kde4_add_ui_files(kwin4_effect_src ${kwin4_effect_ui})
    kde4_add_plugin(kcm_kwin4_effect_${name} ${kwin4_effect_src})
    target_link_libraries(kcm_kwin4_effect_${name} kwineffects ${KDE4_KIO_LIBS} ${KDE4_KDEUI_LIBS})
    install(TARGETS kcm_kwin4_effect_${name} DESTINATION ${PLUGIN_INSTALL_DIR})
endmacro(KWIN4_ADD_EFFECT_CONFIG)

include_directories(
    ${KDEBASE_WORKSPACE_SOURCE_DIR}/kwin/lib
    )

### watereffect plugin
KWIN4_ADD_EFFECT(watereffect watereffect.cpp)
KWIN4_ADD_EFFECT_CONFIG(watereffect watereffect_config.ui watereffect_config.cpp)
install( FILES watereffect.desktop watereffect_config.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kwin )
