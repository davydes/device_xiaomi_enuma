package org.lineageos.pad_parts.usb;

import android.os.SystemProperties;
import android.util.Log;

public class UsbHostUtils {

    private static final String TAG = "UsbHostUtils";
    private static final String PROP_USB_HOST = "persist.vendor.usb.host.enabled";

    public static void applyUsbHostMode(boolean enabled) {
        try {
            SystemProperties.set(PROP_USB_HOST, enabled ? "1" : "0");
        } catch (Exception e) {
            Log.e(TAG, "Cannot set USB host property", e);
        }
    }
}
