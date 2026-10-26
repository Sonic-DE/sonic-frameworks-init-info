# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_library(SonicInitInfo)
add_library(Sonic::InitInfo ALIAS SonicInitInfo)
set_target_properties(SonicInitInfo PROPERTIES
    VERSION     ${SONICINITINFO_VERSION}
    SOVERSION   ${SONICINITINFO_SOVERSION}
    EXPORT_NAME InitInfo
)

set(sonicinitinfo_debug_SRCS)
ecm_qt_declare_logging_category(sonicinitinfo_debug_SRCS
    HEADER initinfo_debug.h
    IDENTIFIER SONIC_INITINFO
    CATEGORY_NAME sonic.initinfo
    DEFAULT_SEVERITY Warning
    DESCRIPTION "Sonic Init Info"
)

target_sources(SonicInitInfo PRIVATE
    initinfo.cpp
    initinfo.h
    ${sonicinitinfo_debug_SRCS}
)

ecm_generate_export_header(SonicInitInfo
    BASE_NAME SonicInitInfo
    VERSION ${KF_VERSION}
    # Quoted, so the export header finds the version header installed beside it.
    USE_VERSION_HEADER "\"sonicinitinfo_version.h\""
    VERSION_BASE_NAME SonicInitInfo
    DEPRECATED_BASE_VERSION 0
)

target_link_libraries(SonicInitInfo PUBLIC Qt6::Core)
if(TARGET Qt6::DBus)
    target_link_libraries(SonicInitInfo PRIVATE Qt6::DBus)
    target_compile_definitions(SonicInitInfo PRIVATE SONIC_INITINFO_HAVE_DBUS)
endif()

target_include_directories(SonicInitInfo
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}>"
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}>"
)

install(TARGETS SonicInitInfo EXPORT SonicInitInfoTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# <Sonic/InitInfo/InitInfo> forwards to the lowercase header installed next to it.
install(FILES
    InitInfo
    initinfo.h
    ${CMAKE_CURRENT_BINARY_DIR}/sonicinitinfo_export.h
    DESTINATION ${SONICINITINFO_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)
