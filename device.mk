#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# A/B
TARGET_IS_VAB := true

# Signing keys (custom, not test-keys)
PRODUCT_DEFAULT_DEV_CERTIFICATE := vendor/xiaomi/enuma/signing/releasekey
PRODUCT_OTA_PUBLIC_KEYS := vendor/xiaomi/enuma/signing/releasekey
PRODUCT_MAINLINE_SEPOLICY_DEV_CERTIFICATES := vendor/xiaomi/enuma/signing
PRODUCT_MAINLINE_BLUETOOTH_SEPOLICY_DEV_CERTIFICATES := vendor/xiaomi/enuma/signing


# Is tablet (false because enuma has cellular modem)
TARGET_IS_TABLET := false

# Inherit from sm8250-common
$(call inherit-product, device/xiaomi/sm8250-common/kona.mk)

# No NFC on enuma
PRODUCT_PACKAGES_REMOVE += NfcNci

# AAPT
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xxxhdpi
PRODUCT_AAPT_PREBUILT_DPI := xxxhdpi xxhdpi xhdpi hdpi

# Audio configs
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(LOCAL_PATH)/audio/,$(TARGET_COPY_OUT_VENDOR)/etc)

# Boot animation
TARGET_SCREEN_HEIGHT := 2560
TARGET_SCREEN_WIDTH := 1600

# Camera
PRODUCT_PACKAGES += \
    libMegviiFacepp-0.5.2 \
    libmegface \
    libpiex_shim

# Fingerprint
TARGET_SUPPORTS_FINGERPRINT := true

PRODUCT_PACKAGES += \
    vendor.xiaomi.hardware.fx.tunnel@1.0.vendor

# GNSS
PRODUCT_PACKAGES += \
    android.hardware.gnss@2.1.vendor

# Overlays
DEVICE_PACKAGE_OVERLAYS += \
    $(LOCAL_PATH)/overlay

# Parts
PRODUCT_PACKAGES += \
    MiPadParts

PRODUCT_PACKAGES += \
    vendor.xiaomi_enuma.peripherals@1.0-service.default

$(call soong_config_set_bool, xiaomi_enuma_peripherals, stylus_use_old_driver, true)

# Stylus button bridge (BLE keyboard -> BTN_STYLUS via uinput)
PRODUCT_PACKAGES += \
    stylus-bridge

# DSP Volume Synchronizer (syncs Android volume to Xiaomi DSP/Dolby)
PRODUCT_PACKAGES += \
    DSPVolumeSynchronizer

# Rootdir
PRODUCT_PACKAGES += \
    init.enuma.rc

# RRO Overlays
PRODUCT_PACKAGES += \
    ApertureOverlayEnuma \
    FrameworkResOverlayEnuma \
    LineageSDKOverlayEnuma \
    NetworkStackOverlayMIUI \
    SettingsOverlayEnuma \
    SettingsProviderOverlayEnuma \
    SystemUIOverlayEnuma

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/handheld_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/handheld_core_hardware.xml

# Shipping API level
PRODUCT_SHIPPING_API_LEVEL := 30

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)

# Inherit from vendor blobs
$(call inherit-product, vendor/xiaomi/enuma/enuma-vendor.mk)
