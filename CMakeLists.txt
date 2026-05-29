cmake_minimum_required(VERSION 3.19)
project(ValveTester LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialPort Charts)

qt_standard_project_setup()

if(MSVC)
    add_compile_options(/FS)
endif()
qt_add_executable(ValveTester
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    preferences.cpp
    preferences.h
    preferencesdialog.cpp
    preferencesdialog.h
    devicetemplate.h
    templatestore.cpp
    templatestore.h
    loadtemplatedialog.cpp
    loadtemplatedialog.h
    valvetesterdevice.cpp
    valvetesterdevice.h
    measurement.h
    sweeprunner.cpp
    sweeprunner.h
    hw_scaling.h
)

target_link_libraries(ValveTester
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::SerialPort
        Qt6::Charts
)

install(TARGETS ValveTester
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ValveTester
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
