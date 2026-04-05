/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.lineageos.pad_parts.usb;

import android.content.Context;
import android.os.SystemProperties;
import android.util.Log;

import org.lineageos.pad_parts.utils.SettingsUtils;

public class UsbHostUtils {

    private static final String TAG = "UsbHostUtils";
    private static final String PROP_USB_HOST = "persist.vendor.usb.host.enabled";

    public static void applyUsbHostMode(Context context) {
        boolean enabled = SettingsUtils.isSettingEnabled(context, SettingsUtils.USB_HOST_ENABLE);
        try {
            SystemProperties.set(PROP_USB_HOST, enabled ? "1" : "0");
        } catch (Exception e) {
            Log.e(TAG, "Cannot set USB host property", e);
        }
    }
}
