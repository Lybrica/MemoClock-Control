package com.lybrica.control;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootCompletedIntentReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i("ddd", "onReceive");
        Intent pushIntent = new Intent(context, CheckWiFi.class);
        context.startService(pushIntent);
    }
}