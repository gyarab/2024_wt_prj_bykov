cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD, 23)
set(CMAKE_C_STANDARD_REQUIRED, TRUE)
set(CMAKE_CXX_STANDARD, 23)
set(CMAKE_CXX_STANDARD_REQUIRED, TRUE)

project(webcrawler VERSION 1.0.0)
project(webserver VERSION 1.0.0)

if(DEBUG EQUAL 1)
	add_compile_definitions(WEBCRAWL_DEBUG)
	add_compile_definitions(WEBSERVER_DEBUG)
	add_compile_options(-g)
	message("WEBCRAWL_DEBUG enabled")
else()
	add_compile_options(-O3)
	message("WEBCRAWL_DEBUG features disabled.")
endif()

add_executable(webcrawler
"WebCrawler/Main.c"
"WebCrawler/Database.c"
"WebCrawler/Processing.c"
"WebCrawler/Utils.c"
"WebCrawler/Rules.c"
"WebCrawler/Types.c"
"WebCrawler/Noreturn.c"
)

add_executable(webserver
"WebServer/Main.c"
"WebServer/Serialization.c"
"WebServer/Database.c"
"WebServer/Socketlib.c"
"WebServer/Score.c"
"WebServer/System.c"
"WebServer/Utils.c"
)

target_include_directories(webcrawler PUBLIC "/usr/include/libxml2/")
target_link_libraries(webcrawler PUBLIC -lcurl -lxml2 -lz -llzma -lm -lsqlite3)

target_include_directories(webserver PUBLIC "/usr/include/")
target_link_libraries(webserver PUBLIC -lsqlite3)

unset(DEBUG CACHE)
