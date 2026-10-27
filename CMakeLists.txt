# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.30)

set(KF_VERSION "6.31.0") # handled by release scripts
project(SonicInitInfo VERSION ${KF_VERSION})

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FeatureSummary)
find_package(ECM 6.31.0 NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://github.com/Sonic-DE/sonic-frameworks-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMGenerateExportHeader)
include(ECMSetupVersion)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(CMakePackageConfigHelpers)

set(REQUIRED_QT_VERSION 6.11.0)
find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core)
find_package(SonicLogging ${KF_VERSION} CONFIG REQUIRED)
find_package(Qt6DBus ${REQUIRED_QT_VERSION} CONFIG)
set_package_properties(Qt6DBus PROPERTIES TYPE OPTIONAL PURPOSE "Extra seat manager detection signal from D-Bus name ownership")

set(sonicinitinfo_version_header "${CMAKE_CURRENT_BINARY_DIR}/src/sonicinitinfo_version.h")
ecm_setup_version(PROJECT VARIABLE_PREFIX SONICINITINFO
                        VERSION_HEADER "${sonicinitinfo_version_header}"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/SonicInitInfoConfigVersion.cmake"
                        SOVERSION 6)

set(SONICINITINFO_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/Sonic/InitInfo")

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6Test ${REQUIRED_QT_VERSION} CONFIG REQUIRED)
    add_subdirectory(autotests)
endif()

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/SonicInitInfo")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SonicInitInfoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SonicInitInfoConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SonicInitInfoConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/SonicInitInfoConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT SonicInitInfoTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE SonicInitInfoTargets.cmake NAMESPACE Sonic:: COMPONENT Devel)

install(FILES ${sonicinitinfo_version_header}
        DESTINATION ${SONICINITINFO_INSTALL_INCLUDEDIR} COMPONENT Devel)

include(ECMFeatureSummary)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
