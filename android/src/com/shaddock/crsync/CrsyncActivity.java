package com.shaddock.crsync;

import static com.shaddock.crsync.CrsyncConstants.logger;

import java.io.File;
import java.util.Vector;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.text.method.ScrollingMovementMethod;
import android.widget.TextView;

public class CrsyncActivity extends Activity {

    private MyContentObserver mObserver = null;
    private class MyContentObserver extends ContentObserver {

        public MyContentObserver() {
            super(new Handler());
        }

        @Override
        public boolean deliverSelfNotifications() {
            return false;
        }

        @Override
        public void onChange(final boolean selfChange) {
            updateFromProvider();
        }

    }

    private static final String APK_MIME = "application/vnd.android.package-archive";
    private TextView mTV = null;
    private AlertDialog mConfirmDialog = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        logger.info("Activity onCreate");
        setContentView(R.layout.activity_crsync);
        mTV = (TextView)findViewById(R.id.tv_main);
        mTV.setMovementMethod(ScrollingMovementMethod.getInstance());
        mObserver = new MyContentObserver();
    }

    @Override
    protected void onResume() {
        super.onResume();
        logger.info("Activity onResume");
        this.getContentResolver().registerContentObserver(CrsyncConstants.URI_BASE, true, mObserver);
        updateFromProvider();
    }

    @Override
    protected void onPause() {
        logger.info("Activity onPause");
        this.getContentResolver().unregisterContentObserver(mObserver);
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        logger.info("Activity onDestroy");
        if(mConfirmDialog != null && mConfirmDialog.isShowing()) {
            mConfirmDialog.dismiss();
        }
        super.onDestroy();
    }

    private void updateFromProvider() {
        CrsyncInfo.StateInfo si = CrsyncInfo.queryState(getContentResolver());
        StringBuilder sb = new StringBuilder(50);
        sb.append("Action = " + si.mAction + " Code = " + si.mCode + "\n");

        switch (si.mAction) {
        case Crsync.Action_Idle:
            CrsyncInfo.ContentInfo ci = new CrsyncInfo.ContentInfo();
            ci.mMagnet = "14015atc";
            ci.mBaseUrl = "http://dlied5.qq.com/wjzj/a/test8k/atc/";
            ci.mLocalApp = this.getApplicationInfo().sourceDir;
            ci.mLocalRes = this.getExternalFilesDir(null).getAbsolutePath() + "/crsync/";
            CrsyncInfo.updateContent(getContentResolver(), ci);
            break;
        case Crsync.Action_Query:
            break;
        case Crsync.Action_UserConfirm:
        {
            if(mConfirmDialog != null && mConfirmDialog.isShowing()) {
                mConfirmDialog.dismiss();
            }

            logger.info("User Confirm Dialog show");
            mConfirmDialog = new AlertDialog.Builder(this)
            .setTitle("Title: UpdateApp")
            .setMessage( "Message: You got new version!" )
            .setCancelable(false)
            .setPositiveButton("YES",
                new DialogInterface.OnClickListener () {
                @Override
                public void onClick(DialogInterface i, int a)
                {
                    CrsyncInfo.StateInfo si = new CrsyncInfo.StateInfo();
                    si.mAction = Crsync.Action_UpdateApp;
                    si.mCode = Crsync.Code_OK;
                    CrsyncInfo.updateState(getContentResolver(), si);
                }
            })
            .setNegativeButton("NO",
                new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    CrsyncInfo.StateInfo si = new CrsyncInfo.StateInfo();
                    si.mAction = Crsync.Action_UpdateRes;
                    si.mCode = Crsync.Code_OK;
                    CrsyncInfo.updateState(getContentResolver(), si);
                }
                })
            .show();
            break;
        }
        case Crsync.Action_UpdateApp:
        {
            CrsyncInfo.AppInfo ai = CrsyncInfo.queryApp(getContentResolver());
            sb.append("UpdateApp : " + ai.mPercent + "%");
            break;
        }
        case Crsync.Action_UserInstall:
        {
            if(mConfirmDialog != null && mConfirmDialog.isShowing()) {
                mConfirmDialog.dismiss();
            }
            
            logger.info("User Confirm Dialog show");
            mConfirmDialog = new AlertDialog.Builder(this)
            .setTitle("Title: UpdateApp")
            .setMessage( "Message: new version download complete!" )
            .setCancelable(false)
            .setPositiveButton("Install",
                new DialogInterface.OnClickListener () {
                @Override
                public void onClick(DialogInterface i, int a)
                {
                    CrsyncInfo.AppInfo ai = CrsyncInfo.queryApp(getContentResolver());
                    String appfile = getExternalFilesDir(null) + "/crsync/" + ai.mHash;
                    File f = new File(appfile);
                    f.setReadable(true, false);
                    f.setWritable(true, false);
                    f.setExecutable(true, false);
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.setDataAndType(Uri.fromFile(new File(appfile)), APK_MIME);
                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                }
            })
            .show();
            break;
        }
        case Crsync.Action_UpdateRes:
            Vector<CrsyncInfo.ResInfo> ri = CrsyncInfo.queryRes(getContentResolver());
            sb.append("Res size : " + ri.size() + "\n");
            for(CrsyncInfo.ResInfo x : ri) {
                sb.append(x.mName + " : " + x.mPercent + "\n");
            }
            break;
        case Crsync.Action_Done:
            break;
        default:
            break;
        }
        mTV.setText(sb.toString());
    }
}
