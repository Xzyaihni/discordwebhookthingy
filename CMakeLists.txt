cmake_minimum_required(VERSION 3.13)
project(sender)
set(CMAKE_CXX_STANDARD 20)

option(Y_DEBUG "build in debug mode" "OFF")


set(SOURCE_FILES main.cpp
hookctl.cpp)

if(${Y_DEBUG})
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

find_package(CURL REQUIRED)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

if(${Y_DEBUG})
	add_definitions(-DDEBUG)
	target_link_libraries(${PROJECT_NAME} -O1 -pg -Wall -Werror -pedantic-errors)
	
	if(${Y_SANITIZE})
		target_link_libraries(${PROJECT_NAME} -fsanitize=address)
	endif()
else()
	target_link_libraries(${PROJECT_NAME} -O3)
endif()

target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})

if(WIN32)
	add_definitions("-DCURL_STATICLIB")

	target_link_libraries(${PROJECT_NAME} -lidn2
-lunistring
-liconv
-lcharset
-lssh2
-lz
-lgcrypt
-lgpg-error
-lws2_32
-ladvapi32
-lcrypt32
-lwldap32
-lz
-lws2_32
-lpthread)
endif()

cmake_minimum_required(VERSION 3.13)
project(notifierlol)
set(CMAKE_CXX_STANDARD 20)

option(Y_DEBUG "build in debug mode" "OFF")


set(SOURCE_FILES notifier.cpp
hookctl.cpp
notifyctl.cpp)

if(${Y_DEBUG})
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

find_package(CURL REQUIRED)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

if(${Y_DEBUG})
	add_definitions(-DDEBUG)
	target_link_libraries(${PROJECT_NAME} -O1 -pg -Wall -Werror -pedantic-errors)

	if(${Y_SANITIZE})
		target_link_libraries(${PROJECT_NAME} -fsanitize=address)
	endif()
else()
	target_link_libraries(${PROJECT_NAME} -O3)
endif()

target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})

if(WIN32)
	add_definitions("-DCURL_STATICLIB")

	target_link_libraries(${PROJECT_NAME} -lidn2
-lunistring
-liconv
-lcharset
-lssh2
-lz
-lgcrypt
-lgpg-error
-lws2_32
-ladvapi32
-lcrypt32
-lwldap32
-lz
-lws2_32
-lpthread)
endif()