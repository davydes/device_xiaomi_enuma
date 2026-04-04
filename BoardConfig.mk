#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from sm8250-common
include device/xiaomi/sm8250-common/BoardConfigCommon.mk

DEVICE_PATH := device/xiaomi/enuma

# Board
TARGET_BOARD_INFO_FILE := $(DEVICE_PATH)/board-info.txt

# Display
TARGET_SCREEN_DENSITY := 360

# Kernel
TARGET_KERNEL_CONFIG += vendor/xiaomi/enuma.config

# OTA assert
TARGET_OTA_ASSERT_DEVICE := enuma

# Properties
TARGET_ODM_PROP += $(DEVICE_PATH)/odm.prop
TARGET_VENDOR_PROP += $(DEVICE_PATH)/vendor.prop
# Override sm8250-common vendor_phone.prop (single SIM instead of DSDS)
TARGET_VENDOR_PROP := $(filter-out device/xiaomi/sm8250-common/vendor_phone.prop,$(TARGET_VENDOR_PROP))
TARGET_VENDOR_PROP += $(DEVICE_PATH)/vendor_phone.prop

# Sepolicy
SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/private
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/public
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor

# VINTF
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE += \
    $(DEVICE_PATH)/interfaces/peripherals/1.0/default/device_framework_matrix.xml

# Wi-Fi
SOONG_CONFIG_XIAOMI_KONA_WIFI_SYMLINK_VERSION := v2

# SELinux enforcing (validated: 0 denials in permissive boot)

# Recovery (QTI DRM backend for dual DSI CPHY display)
$(call soong_config_set_bool,recovery,target_recovery_uses_qti_drm,true)

# Inherit from the proprietary version
include vendor/xiaomi/enuma/BoardConfigVendor.mk
