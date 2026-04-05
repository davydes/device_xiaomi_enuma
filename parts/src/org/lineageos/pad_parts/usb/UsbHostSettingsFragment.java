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

import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.preference.Preference;

import org.lineageos.pad_parts.R;
import org.lineageos.pad_parts.utils.SettingsUtils;

public class UsbHostSettingsFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.usb_host_settings);

        Preference enablePreference = findPreference(SettingsUtils.USB_HOST_ENABLE);
        if (enablePreference != null) {
            enablePreference.setOnPreferenceChangeListener(this);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        UsbHostUtils.applyUsbHostMode(getContext());
        return true;
    }
}
