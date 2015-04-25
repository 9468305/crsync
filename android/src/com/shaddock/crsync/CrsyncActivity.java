package com.shaddock.crsync;

import static com.shaddock.crsync.CrsyncConstants.logger;

import java.util.Vector;

import android.app.Activity;
import android.app.AlertDialog;
import android.database.ContentObserver;
import android.os.Bundle;
import android.os.Environment;
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
        super.onPause();
        logger.info("Activity onPause");
        this.getContentResolver().unregisterContentObserver(mObserver);
    }

    @Override
    protected void onDestroy() {
        logger.info("Activity onDestroy");
        super.onDestroy();
    }

    private void updateFromProvider() {
        CrsyncInfo.StateInfo si = CrsyncInfo.queryState(getContentResolver());
        StringBuilder sb = new StringBuilder(50);
        sb.append("Action = " + si.mAction + " Code = " + si.mCode + "\n");

        switch (si.mAction) {
        case Crsync.Action_Idle:
            CrsyncInfo.ContentInfo ci = new CrsyncInfo.ContentInfo();
            ci.mMagnet = "14014etc";
            ci.mBaseUrl = "http://dlied5.qq.com/wjzj/a/test/etc/";
            ci.mLocalApp = this.getApplicationInfo().sourceDir;
            ci.mLocalRes = Environment.getExternalStorageDirectory().getAbsolutePath() + "/crsync/";
            CrsyncInfo.updateContent(getContentResolver(), ci);
            break;
        case Crsync.Action_Query:
            break;
        case Crsync.Action_UserConfirm:
            break;
        case Crsync.Action_UpdateApp:
            break;
        case Crsync.Action_UserInstall:
            break;
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
