# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

#project(project C CXX ASM)
#project(test C CXX ASM)
project(buddy3 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(buddy3
        buddy3.c
        )

add_library(kissfft STATIC
        lib/kissfft/kissfft/kiss_fft.c
        lib/kissfft/kissfft/kiss_fftr.c
        )
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

# enable usb and uart
pico_enable_stdio_usb(buddy3 1)
#pico_enable_stdio_uart(buddy3 2)

target_include_directories(kissfft PUBLIC ${CMAKE_SOURCE_DIR}/lib/kissfft/kissfft)
    
target_link_libraries(buddy3 pico_stdlib hardware_adc hardware_pwm hardware_timer hardware_i2c hardware_spi hardware_gpio hardware_uart kissfft FatFs_SPI)

# create map/bin/hex file etc.
pico_add_extra_outputs(buddy3)

