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

package org.lineageos.pad_parts.button;

import android.content.ComponentName;
import android.content.Context;
import android.content.ContentResolver;
import android.content.pm.PackageManager;
import android.view.InputDevice;

import java.util.Arrays;

import org.lineageos.pad_parts.Constants;
import org.lineageos.pad_parts.R;
import org.lineageos.pad_parts.utils.SettingsUtils;

public class ButtonUtils {

    private static final String HEADSET_BUTTON_DEVICE_NAME = "kona-mtp-snd-card Button Jack";
    private static final String HEADSET_BUTTON_DEVICE_NAME_2 = "kona-mtp-snd-card Headset Jack";

    protected static final String HEADSET_BUTTON = "headset_button";
    protected static final String HEADSET_BUTTON_VOLUME = "volume";
    protected static final String HEADSET_BUTTON_MUSIC = "music";
    protected static final String STYLUS_BUTTON = "stylus_button";
    protected static final String STYLUS_BUTTON_DEFAULT = "default";

    /* Property read by native stylus-bridge daemon — mode strings must match arrays.xml */
    protected static final String PROP_STYLUS_BUTTON_MODE = "persist.vendor.stylus.button_mode";

    protected static boolean isHeadsetButtonDevice(InputDevice device) {
        if (device == null) {
            return false;
        }
        final String deviceName = device.getName();

        return HEADSET_BUTTON_DEVICE_NAME.equals(deviceName)
                || HEADSET_BUTTON_DEVICE_NAME_2.equals(deviceName);
    }

    public static String getButtonSettingsSummary(Context context) {
        if (context == null) {
            return null;
        }

        return String.format("%s: %s, %s: %s",
                context.getString(R.string.headset_button_title),
                getHeadsetButtonConfigSummary(context),
                context.getString(R.string.stylus_button_title),
                getStylusButtonConfigSummary(context));
    }

    public static String getHeadsetButtonConfigSummary(Context context) {
        return getArrayConfigSummary(context, R.array.headset_button_values,
                R.array.headset_button_entries, HEADSET_BUTTON, HEADSET_BUTTON_MUSIC);
    }

    public static String getStylusButtonConfigSummary(Context context) {
        return getArrayConfigSummary(context, R.array.stylus_button_values,
                R.array.stylus_button_entries, STYLUS_BUTTON, STYLUS_BUTTON_DEFAULT);
    }

    private static String getArrayConfigSummary(
            Context context, int valuesResId, int summariesResId, String key, String defValue) {
        if (context == null) {
            return null;
        }
        ContentResolver resolver = context.getContentResolver();
        String configValue = SettingsUtils.getConfigValueString(resolver, key, defValue);

        String[] values = context.getResources().getStringArray(valuesResId);
        String[] summaries = context.getResources().getStringArray(summariesResId);

        int index = Arrays.asList(values).indexOf(configValue);
        if (index < 0 || index >= summaries.length) {
            index = 0;
        }

        return summaries[index];
    }

    public static void enableButtonSettingsActivity(Context context) {
        if (context == null) {
            return;
        }
        final ComponentName component = new ComponentName(
                Constants.PACKAGE_NAME, ButtonSettingsActivity.class.getName());

        context.getPackageManager().setComponentEnabledSetting(component,
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED, PackageManager.DONT_KILL_APP);
    }

    public static boolean isButtonSettingsActivityEnabled(Context context) {
        if (context == null) {
            return false;
        }
        final ComponentName component = new ComponentName(
                Constants.PACKAGE_NAME, ButtonSettingsActivity.class.getName());

        PackageManager pm = context.getPackageManager();
        int state = pm.getComponentEnabledSetting(component);
        if (state == PackageManager.COMPONENT_ENABLED_STATE_DEFAULT) {
            try {
                return pm.getActivityInfo(component, 0).enabled;
            } catch (PackageManager.NameNotFoundException e) {
                return false;
            }
        }
        return state == PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
    }

}
