/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.time.Period;
import java.time.Duration;
import java.time.Instant;

public class BraveSyncCodeCountdownFragment extends Fragment {
    Instant mNotAfter;
    boolean mDestroyed;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_sync_code_countdown, container, false);
    }

    @Override
    public void onDestroyView() {
        mDestroyed = true;
        super.onDestroyView();
    }

    public void setNotAfter (LocalDateTime notAfter) {
        mNotAfter = notAfter.toInstant(ZoneOffset.UTC);
        scheduleTextUpdate();
    }

    void scheduleTextUpdate() {
        updateText();
    }

    void updateText() {
        if (mDestroyed) {
            return;
        }

        TextView countDownTextView = (TextView)getView().findViewById(R.id.brave_sync_count_down_text);
        Duration duration = Duration.between(Instant.now() , mNotAfter);

        if (!duration.isNegative()) {
            // TODO(alexeybarabash): [HardcodedText]
            String theWarningText = "This temporary code is valid for the next " + duration;
            countDownTextView.setText(theWarningText);

            // TODO(alexeybarabash): can I use java.util.Timer?
            PostTask.postDelayedTask(
                TaskTraits.UI_USER_VISIBLE,
                () -> {
                    updateText();
                }
                , 1000
            );
        }
    }
}
