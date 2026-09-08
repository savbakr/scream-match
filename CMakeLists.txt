cmake_minimum_required(VERSION 3.16)
project(ScreamMatch CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(ScreamMatch
        src/main.cpp
        src/MovieDatabase.cpp
        src/Watchlist.cpp
        src/RatingsManager.cpp
        src/Recommender.cpp
        src/SimilarityGraph.cpp
)

add_custom_command(TARGET ScreamMatch POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:ScreamMatch>/data
)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets QUIET)
endif()

if(Qt6_FOUND OR Qt5_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(ScreamMatchGui
            src/main_gui.cpp
            src/MainWindow.cpp
            include/MainWindow.h
            src/MovieDatabase.cpp
            src/Watchlist.cpp
            src/RatingsManager.cpp
            src/Recommender.cpp
            src/SimilarityGraph.cpp
    )

    if(Qt6_FOUND)
        target_link_libraries(ScreamMatchGui PRIVATE Qt6::Widgets)
        message(STATUS "Qt6 found - ScreamMatchGui target enabled")
    else()
        target_link_libraries(ScreamMatchGui PRIVATE Qt5::Widgets)
        message(STATUS "Qt5 found - ScreamMatchGui target enabled")
    endif()

    add_custom_command(TARGET ScreamMatchGui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:ScreamMatchGui>/data
    )
else()
    message(STATUS "Qt not found - skipping ScreamMatchGui target. Install Qt to enable the GUI (see README).")
endif()