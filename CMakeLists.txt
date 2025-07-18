# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(BTTest2 C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Generate profile data if profile.gatt exists

# Add executable. Default name is the project name, version 0.1
add_executable(BTTest2 main.cpp)

pico_set_program_name(BTTest2 "BTTest2")
pico_set_program_version(BTTest2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(BTTest2 0)
pico_enable_stdio_usb(BTTest2 1)

pico_btstack_make_gatt_header(BTTest2 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/hog_keyboard_demo.gatt)

# Add the standard library to the build
target_link_libraries(BTTest2
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(BTTest2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated/BTTest2_gatt_header
)

# Add Bluetooth libraries instead of Wi-Fi
target_link_libraries(BTTest2 
       pico_stdlib
       pico_cyw43_arch_none
       pico_btstack_ble
       pico_btstack_cyw43
       )

pico_add_extra_outputs(BTTest2)

