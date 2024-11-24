cmake_minimum_required(VERSION 3.13)

set(PROGRAM_NAME pico_w_webserver)
set(PICO_BOARD pico_w)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_STACK_SIZE 8192)  # Set a larger stack size (default is 2048 bytes)


include(pico_sdk_import.cmake)

project(pico_w_webserver)


pico_sdk_init()

message("Running makefsdata python script")
execute_process(COMMAND
    py makefsdata.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(${PROGRAM_NAME}
    main.c
)
# tell CMake where to find other source code
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)
add_library(kissfft STATIC
        lib/kissfft/kiss_fft.c
        lib/kissfft/kiss_fftr.c
        )

target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Ensure WiFi credentials are set
if(NOT DEFINED ENV{WIFI_SSID} OR NOT DEFINED ENV{WIFI_PASSWORD})
    message(FATAL_ERROR "WIFI_SSID and WIFI_PASSWORD environment variables must be set")
endif()

# Read WiFi credentials from environment variables
set(WIFI_SSID "$ENV{WIFI_SSID}")
set(WIFI_PASSWORD "$ENV{WIFI_PASSWORD}")

target_compile_definitions(${PROGRAM_NAME} PRIVATE
            WIFI_SSID=\"${WIFI_SSID}\"
            WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
            )

target_link_libraries(${PROGRAM_NAME}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    pico_lwip_sntp           
    pico_stdlib
    hardware_adc
    hardware_pwm 
    hardware_timer 
    hardware_i2c 
    hardware_spi 
    hardware_gpio
    hardware_uart 
    FatFs_SPI
    kissfft
)

pico_enable_stdio_usb(${PROGRAM_NAME} 1)
pico_enable_stdio_uart(${PROGRAM_NAME} 0)

pico_add_extra_outputs(${PROGRAM_NAME})