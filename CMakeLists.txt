cmake_minimum_required(VERSION 3.30)
project(tella LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
    PrintSupport
)

qt_add_executable(tella
    src/main.cpp
    src/database.cpp
    src/loginwindow.cpp
    src/mainwindow.cpp
    src/poswidget.cpp
    src/productswidget.cpp
    src/invoiceswidget.cpp
    src/transactionswidget.cpp
    src/reportswidget.cpp
    src/userswidget.cpp
    src/models.cpp
    resources/resources.qrc
)

target_include_directories(tella PRIVATE src)

target_link_libraries(tella PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::PrintSupport
)

# Installation rules
if(UNIX AND NOT ANDROID)
    install(TARGETS tella DESTINATION /opt/tella/bin)
endif()
