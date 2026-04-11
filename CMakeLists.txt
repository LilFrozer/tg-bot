cmake_minimum_required(VERSION 3.10)
project(tgbot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Экспорт команд для Zed (чтобы работало автодополнение)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_path(BOOST_INCLUDE_DIR
  NAMES boost/beast.hpp
  PATHS /opt/homebrew/include
  /opt/homebrew/opt/boost/include
  /usr/local/include
  /usr/include
  REQUIRED
)

find_package(OpenSSL REQUIRED)

add_executable(tgbot
  src/main.cpp
  src/Bot.cpp
)

target_include_directories(tgbot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tgbot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs)
target_include_directories(
        tgbot PRIVATE
        ${BOOST_INCLUDE_DIR}
        src
)

# Линкуем библиотеки
target_link_libraries(tgbot
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# Говорим Beast использовать std::error_code вместо boost::system::error_code
target_compile_definitions(tgbot PRIVATE
        BOOST_BEAST_USE_STD_ERIC=1
        BOOST_ASIO_USE_STD_ERIC=1
)
