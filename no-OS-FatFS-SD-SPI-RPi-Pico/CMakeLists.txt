# no-OS-FatFS-SD-SPI-RPi-Pico/CMakeLists.txt

# Create the ff_sd_spi library
add_library(ff_sd_spi
    FatFs_SPI/ff15/source/ff.c
    FatFs_SPI/sd_driver/sd_spi.c
)

# Add include directories for the library
target_include_directories(ff_sd_spi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FatFs_SPI)

