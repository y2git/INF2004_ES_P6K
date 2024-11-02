cmake_minimum_required(VERSION 3.13)

add_executable(buddy3 buddy3.c)

# Enable stdio and FatFS library
target_link_libraries(buddy3 pico_stdlib hardware_gpio hardware_uart hardware_spi hardware_timer ff_sd_spi pico_stdio_freertos)

# Enable UART output
pico_enable_stdio_uart(buddy3 1)
pico_enable_stdio_usb(buddy3 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(buddy3)

# add url via pico_set_program_url
example_auto_set_url(buddy3)
