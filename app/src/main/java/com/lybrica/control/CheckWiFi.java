package com.lybrica.control;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.IBinder;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class CheckWiFi extends Service {
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i("ddd","onStart");

        try {
            while (!isConnected(getApplicationContext())) {
                Log.d("ddd","still not connected");
                Thread.sleep(1000);
            }
            Log.i("ddd","connected");
            sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
            String msg = sharedPreferences.getString("msg", "");
            new Background_get().execute("text=" + msg);
            return START_NOT_STICKY;
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }
        Log.i("ddd","isn't");
        return START_STICKY;
    }

    public static boolean isConnected(Context context) {
        ConnectivityManager connManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo mWifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        boolean beul = false;
        if (mWifi.isConnected()) beul = true;
        else if (!mWifi.isConnected()) beul = false;

        return beul;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }

    private class Background_get extends AsyncTask<String, Void, String> {
        @Override
        protected String doInBackground(String... params) {
            try {
                /*********************************************************/
                /* Change the IP to the IP you set in the arduino sketch */
                /*********************************************************/
                sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS",Context.MODE_PRIVATE);
                URL url = new URL("http://" + sharedPreferences.getString("ip", "") + "/set?" + params[0]);
                HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                Log.i("ddd","trying | " + url);

                BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream()));
                StringBuilder result = new StringBuilder();
                String inputLine;
                while ((inputLine = in.readLine()) != null)
                    result.append(inputLine).append("\n");

                in.close();
                connection.disconnect();
                return result.toString();

            } catch (IOException e) {
                e.printStackTrace();
            }
            return null;
        }
    }
}