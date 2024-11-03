# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# Current pico_sdk is from device environment variables

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Name projects
project(project C CXX ASM)
project(buddy2 C CXX ASM)
project(buddy3 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(project project.c)
add_executable(buddy2 buddy2.c)
add_executable(buddy3 buddy3.c)
add_executable(test test.c)


# tell CMake where to find other source code
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)
add_library(kissfft STATIC
        lib/kissfft/kiss_fft.c
        lib/kissfft/kiss_fftr.c
        )

pico_set_program_name(project "project") 
pico_set_program_version(project "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_usb(project 1)
pico_enable_stdio_uart(project 0)
pico_enable_stdio_usb(buddy2 1)
pico_enable_stdio_usb(buddy3 1)
pico_enable_stdio_usb(test 1)

# Add the standard library to the build
target_link_libraries(project
        pico_stdlib
        FatFs_SPI
        hardware_adc 
        hardware_pwm 
        hardware_timer 
        hardware_i2c 
        hardware_spi 
        hardware_gpio
        hardware_uart 
        kissfft
        )
target_link_libraries(buddy2
        pico_stdlib 
        hardware_adc 
        hardware_pwm 
        hardware_timer 
        hardware_i2c 
        hardware_spi 
        hardware_gpio 
        kissfft
        )
target_link_libraries(buddy3
        pico_stdlib
        FatFs_SPI
        hardware_adc 
        hardware_pwm 
        hardware_timer 
        hardware_i2c 
        hardware_spi 
        hardware_gpio
        hardware_uart 
        kissfft
        )
target_link_libraries(test
        pico_stdlib 
        FatFs_SPI
        )
# Add the standard include files to the build
target_include_directories(project PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)
target_include_directories(kissfft PUBLIC ${CMAKE_SOURCE_DIR}/lib/kissfft)


# create map/bin/hex file etc.
pico_add_extra_outputs(project)
pico_add_extra_outputs(buddy2)
pico_add_extra_outputs(buddy3)
pico_add_extra_outputs(test)

