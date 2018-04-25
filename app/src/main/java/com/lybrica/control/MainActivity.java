package com.lybrica.control;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.support.v7.widget.Toolbar;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;


public class MainActivity extends AppCompatActivity {

    Intent mServiceIntent;
    TextView txtCurrent;

    // shared preferences objects used to save the IP address
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;
    String msg;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.my_toolbar);
        setSupportActionBar(toolbar);

        txtCurrent = (TextView) findViewById(R.id.textCurrent);
        final EditText editMsg = (EditText) findViewById(R.id.editMessage);
        final CheckBox chkMsgOnly = (CheckBox) findViewById(R.id.checkMsgOnly);
        final CheckBox chkCycle = (CheckBox) findViewById(R.id.checkCycle);
        final Button btnSubmit = (Button) findViewById(R.id.btnSubmit);
        Button btnRemoveMsg = (Button) findViewById(R.id.btnRemoveMsg);

        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS",Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        txtCurrent.setText(sharedPreferences.getString("msg", ""));

        btnSubmit.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                msg = editMsg.getText().toString();

                if (!isConnected(getApplicationContext())) {
                    showNoWiFiDialog();
                } else {
                    txtCurrent.setText(msg);

                    if (chkMsgOnly.isChecked() && sharedPreferences.getBoolean("cycle", true)) {
                        new Background_get().execute("cycle=0");
                    } else if (chkCycle.isChecked() && !sharedPreferences.getBoolean("cycle", true)) {
                        new Background_get().execute("cycle=1");
                    }
                    new Background_get().execute("text=" + msg);
                    editor.putString("msg", msg);
                    if (chkMsgOnly.isChecked())
                        editor.putBoolean("cycle", false);
                    if (chkCycle.isChecked())
                        editor.putBoolean("cycle", true);
                    editor.apply();
                }
            }
        });
        btnRemoveMsg.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Background_get().execute("text=removeStr");
                editMsg.setText("");
                txtCurrent.setText("");
            }
        });

        chkMsgOnly.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (chkCycle.isChecked())
                    chkCycle.setChecked(false);
                else
                    chkCycle.setChecked(true);
            }
        });
        chkCycle.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (chkMsgOnly.isChecked())
                    chkMsgOnly.setChecked(false);
                else
                    chkMsgOnly.setChecked(true);
            }
        });


        editMsg.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if ((event != null && (event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) || (actionId == EditorInfo.IME_ACTION_DONE)) {
                    btnSubmit.performClick();
                    InputMethodManager inputManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                    inputManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(),
                            InputMethodManager.HIDE_NOT_ALWAYS);
                    return true;
                }
                return false;
            }
        });
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
    protected void onResume() {
        super.onResume();
        txtCurrent.setText(sharedPreferences.getString("msg", ""));
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            showSettingsDialog();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public void showNoWiFiDialog() {
        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS",Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();
        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);
        LayoutInflater inflater = this.getLayoutInflater();
        final View dialogView = inflater.inflate(R.layout.dialog_ip, null);
        dialogBuilder   .setTitle("No connection").setMessage("Do you want to set message when you get home?")
        .setPositiveButton("Done", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                editor.putString("msg", msg).apply();
                mServiceIntent = new Intent(getApplicationContext(), CheckWiFi.class);
                startService(mServiceIntent);
            }
        })
        .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                //pass
            }
        });

        AlertDialog b = dialogBuilder.create();
        b.show();
    }

    public void showSettingsDialog() {
        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);
        LayoutInflater inflater = this.getLayoutInflater();
        final View dialogView = inflater.inflate(R.layout.dialog_ip, null);
        dialogBuilder.setView(dialogView);

        final EditText edt = (EditText) dialogView.findViewById(R.id.editIp);
        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS",Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        edt.setText(sharedPreferences.getString("ip","192.168.1."));

        dialogBuilder.setTitle("Enter clock's ip address");
        dialogBuilder.setPositiveButton("Done", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                editor.putString("ip", edt.getText().toString()).apply();
            }
        });
        dialogBuilder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                //pass
            }
        });
        AlertDialog b = dialogBuilder.create();
        b.show();
    }


    /*****************************************************/
    /*  This is a background process for connecting      */
    /*   to the arduino server and sending               */
    /*    the GET request withe the added data           */
    /*****************************************************/

    public class Background_get extends AsyncTask<String, Void, String> {
        @Override
        protected String doInBackground(String... params) {
            try {
                /*********************************************************/
                /* Change the IP to the IP you set in the arduino sketch */
                /*********************************************************/
                sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS",Context.MODE_PRIVATE);
                URL url = new URL("http://" + sharedPreferences.getString("ip", "") + "/set?" + params[0]);
                HttpURLConnection connection = (HttpURLConnection) url.openConnection();

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