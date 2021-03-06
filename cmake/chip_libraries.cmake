#
# Depends on variables:
#
# MCU_PLATFORM      A supported microcontroller platform.
#                   For example '43xx_m4' or '43xx_m0'.

if(NOT DEFINED MCU_PLATFORM)
    message(FATAL_ERROR "${CPM_MODULE_NAME}: \
    please define variable 'MCU_PLATFROM'")
endif()


if("${MCU_PLATFORM}" STREQUAL "43xx_m4")
    message(STATUS "${CPM_MODULE_NAME}: Platform '43xx_m4' detected")

    CPM_AddModule("chip_lpc43xx_m4"
        GIT_REPOSITORY "https://github.com/JitterCompany/chip_lpc43xx_m4.git"
        GIT_TAG "3.3.1"
        USE_EXISTING_VER TRUE)

elseif("${MCU_PLATFORM}" STREQUAL "43xx_m0")
    message(STATUS "${CPM_MODULE_NAME}: Platform '43xx_m0' detected")

    CPM_AddModule("chip_lpc43xx_m0"
        GIT_REPOSITORY "https://github.com/JitterCompany/chip_lpc43xx_m0.git"
        GIT_TAG "3.3.1"
        USE_EXISTING_VER TRUE)

elseif("${MCU_PLATFORM}" STREQUAL "11uxx")
    message(STATUS "${CPM_MODULE_NAME}: Platform '11uxx' detected.")
    message(WARNING "${CPM_MODULE_NAME}: Platform '11uxx' is deprecated, the new name is 'lpc11xxx'.")
    add_definitions(-DMCU_PLATFORM_lpc11xxx)

    CPM_AddModule("chip_lpc11xxx"
        GIT_REPOSITORY "https://github.com/JitterCompany/chip_lpc11xxx.git"
        GIT_TAG "1.4.4")

elseif("${MCU_PLATFORM}" STREQUAL "lpc11xxx")
    message(STATUS "${CPM_MODULE_NAME}: Platform '${MCU_PLATFORM}' detected")

    CPM_AddModule("chip_lpc11xxx"
        GIT_REPOSITORY "https://github.com/JitterCompany/chip_${MCU_PLATFORM}.git"
        GIT_TAG "1.4.4")

else()
    message(FATAL_ERROR "${CPM_MODULE_NAME}: platform '${MCU_PLATFORM}' not supported")
endif()


