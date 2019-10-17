cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)


set(CPM_MODULE_NAME mcu_timing)

#-----------------------------------------------------------------------
# Required CPM Setup - no need to modify - See: https://github.com/iauns/cpm
#-----------------------------------------------------------------------
set(CPM_LIB_TARGET_NAME ${CPM_MODULE_NAME})

if ((DEFINED CPM_DIR) AND (DEFINED CPM_UNIQUE_ID) AND (DEFINED CPM_TARGET_NAME))
    set(CPM_LIB_TARGET_NAME ${CPM_TARGET_NAME})
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CPM_DIR})
    include(CPM)
else()
    set (CPM_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpm-packages" CACHE TYPE STRING)
    find_package(Git)
    if(NOT GIT_FOUND)
        message(FATAL_ERROR "CPM requires Git.")
    endif()
    if (NOT EXISTS ${CPM_DIR}/CPM.cmake)
        message(STATUS "Cloning repo (https://github.com/iauns/cpm)")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" clone https://github.com/iauns/cpm ${CPM_DIR}
            RESULT_VARIABLE error_code
            OUTPUT_QUIET ERROR_QUIET)
        if(error_code)
            message(FATAL_ERROR "CPM failed to get the hash for HEAD")
        endif()
    endif()
    include(${CPM_DIR}/CPM.cmake)
endif()

#------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------

# Depends on variables:
#
# MCU_PLATFORM      A supported microcontroller platform.
#                   For example '43xx_m4' or '43xx_m0'.
#
# Optional preprocessor defines:
#
# DELAY_SHARE_TIMER     Optional flag to enable sharing the timer peripheral
#                       between all available mcu cores
#
# DELAY_OWNER           If DELAY_SHARE_TIMER is set, define DELAY_OWNER
#                       for the core that 'owns' the delay: this is the only
#                       core that can init / deinit and this core will handle
#                       the interrupts.
#
# DELAY_MEMORY_SECTION  Linker section where the state is saved.
#                       If DELAY_SHARE_TIMER is set, make sure this section
#                       is mapped to the same memory address on all cores!
#
include(cmake/chip_libraries.cmake)

CPM_AddModule("lpc_tools"
    GIT_REPOSITORY "https://github.com/JitterCompany/lpc_tools.git"
    GIT_TAG "2.8.2")

CPM_AddModule("c_utils"
    GIT_REPOSITORY "https://github.com/JitterCompany/c_utils.git"
    GIT_TAG "1.4.5")

CPM_InitModule(${CPM_MODULE_NAME})

#-----------------------------------------------------------------------
# Project sources / settings
#-----------------------------------------------------------------------

file(GLOB_RECURSE SOURCES 
    "${CPM_MODULE_NAME}/*.c"
    )
include_directories("${CPM_MODULE_NAME}")

add_library(${CPM_LIB_TARGET_NAME} ${SOURCES})
target_link_libraries(${CPM_LIB_TARGET_NAME} ${CPM_LIBRARIES})
