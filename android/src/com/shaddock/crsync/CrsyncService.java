package com.shaddock.crsync;

import static com.shaddock.crsync.CrsyncConstants.logger;

import java.io.File;

import com.shaddock.crsync.CrsyncInfo.AppInfo;

import android.app.Service;
import android.content.Intent;
import android.database.ContentObserver;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.IBinder;

public class CrsyncService extends Service implements OnepieceObserver {

	public static volatile boolean isRunning = false;

	private CrsyncContentObserver mObserver = null;

    private UpdateThread mUpdateThread = null;

    private boolean mPendingUpdate = false;

    private String mAppHash = "";
    
    private class CrsyncContentObserver extends ContentObserver {

        public CrsyncContentObserver() {
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

	@Override
	public IBinder onBind(Intent intent) {
		throw new UnsupportedOperationException("Cannot bind to CrsyncService");
	}

    @Override
    public void onCreate() {
        super.onCreate();
        logger.info("CrsyncService onCreate");

        mObserver = new CrsyncContentObserver();
        getContentResolver().registerContentObserver(CrsyncConstants.URI_BASE, true, mObserver);
        isRunning = true;
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		logger.info("CrsyncService onStartCommand");
		int returnValue = super.onStartCommand(intent, flags, startId);
        updateFromProvider();
        return returnValue;
	}

	@Override
	public void onDestroy() {
		logger.info("CrsyncService onDestroy");
		isRunning = false;
		getContentResolver().unregisterContentObserver(mObserver);
		mObserver = null;
		super.onDestroy();
	}

	private void updateFromProvider() {
		synchronized (this) {
            mPendingUpdate = true;
            if (mUpdateThread == null) {
                mUpdateThread = new UpdateThread();
                mUpdateThread.start();
            }
        }
	}

    private class UpdateThread extends Thread {
        public UpdateThread() {
            super("CrsyncServiceUpdateThread");
        }

        @Override
        public void run() {
            while (true) {
                synchronized (CrsyncService.this) {
                    if (mUpdateThread != this) {
                        logger.severe("CrsyncService occur multiple UpdateThreads");
                    	break;
                    }
                    if (!mPendingUpdate) {
                        mUpdateThread = null;
                        break;
                    }
                    mPendingUpdate = false;
                }
                Crsync.setObserver(CrsyncService.this);
                CrsyncInfo.StateInfo stateInfo = CrsyncInfo.queryState(getContentResolver());
            	handleFromProvider(stateInfo);
            	Crsync.delObserver();
            }
        }
    }// End of class UpdateThread

    private void handleFromProvider(CrsyncInfo.StateInfo stateInfo) {
        logger.info("CrsyncService handleFromProvider");
        stateInfo.dump();
        switch (stateInfo.mAction) {
        case Crsync.Action_Idle:
            handleIdle(stateInfo);
            break;
        case Crsync.Action_Query:
            handleQuery(stateInfo);
            break;
        case Crsync.Action_UserConfirm:
            //do nothing
            break;
        case Crsync.Action_UpdateApp:
            handleUpdateApp(stateInfo);
            break;
        case Crsync.Action_UserInstall:
            //do nothing
            break;
        case Crsync.Action_UpdateRes:
            handleUpdateRes(stateInfo);
            break;
        case Crsync.Action_Done:
            handleDone(stateInfo);
            break;
        default:
            logger.severe("wrong action");
            break;
        }
    }

    private void handleIdle(CrsyncInfo.StateInfo stateInfo) {
        logger.info("CrsyncService handleIdle");
        CrsyncInfo.ContentInfo contentInfo = CrsyncInfo.queryContent(getContentResolver());
        if(contentInfo.mMagnet.isEmpty() ||
           contentInfo.mBaseUrl.isEmpty() ||
           contentInfo.mLocalApp.isEmpty() ||
           contentInfo.mLocalRes.isEmpty()) {
            return;
        }

        new File( contentInfo.mLocalRes ).mkdirs();

        int code;
        do {
            code = Crsync.JNI_onepiece_init();
            if(Crsync.Code_OK != code) break;
            code = Crsync.JNI_onepiece_setopt(Crsync.OPT_MagnetID, contentInfo.mMagnet);
            if(Crsync.Code_OK != code) break;
            code = Crsync.JNI_onepiece_setopt(Crsync.OPT_BaseUrl, contentInfo.mBaseUrl);
            if(Crsync.Code_OK != code) break;
            code = Crsync.JNI_onepiece_setopt(Crsync.OPT_LocalApp, contentInfo.mLocalApp);
            if(Crsync.Code_OK != code) break;
            code = Crsync.JNI_onepiece_setopt(Crsync.OPT_LocalRes, contentInfo.mLocalRes);
            if(Crsync.Code_OK != code) break;
        } while(false);

        if(Crsync.Code_OK == code) {
            stateInfo.mAction = Crsync.Action_Query;
            stateInfo.mCode = Crsync.Code_OK;
        } else {
            stateInfo.mAction = Crsync.Action_Done;
            stateInfo.mCode = code;
        }
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }

    private void handleQuery(CrsyncInfo.StateInfo stateInfo) {
        logger.info("CrsyncService handleQuery");
        int code = Crsync.JNI_onepiece_perform_query();
        if(Crsync.Code_OK == code) {
            CrsyncInfo.ContentInfo ci = CrsyncInfo.queryContent(getContentResolver());
            String magnetString = Crsync.JNI_onepiece_getinfo(Crsync.INFO_Magnet);
            logger.info("INFO_Magnet = " + magnetString);
            Crsync.Magnet magnet = Crsync.Magnet.getValue(magnetString);
            int size = magnet.resname.length;
            CrsyncInfo.ResInfo ri[] = new CrsyncInfo.ResInfo[size];
            for(int i=0; i < size; i++) {
                CrsyncInfo.ResInfo r = new CrsyncInfo.ResInfo();
                r.mName = magnet.resname[i];
                r.mHash = magnet.reshash[i];
                r.mPercent = 0;
                ri[i] = r;
            }
            CrsyncInfo.bulkInsertRes(getContentResolver(), ri);
            if(magnet.curr_id.equalsIgnoreCase(ci.mMagnet)) {
                stateInfo.mAction = Crsync.Action_UpdateRes;
                stateInfo.mCode = Crsync.Code_OK;
            } else {
                mAppHash = magnet.apphash;
                CrsyncInfo.AppInfo ai = new CrsyncInfo.AppInfo();
                ai.mHash = magnet.apphash;
                CrsyncInfo.updateApp(getContentResolver(), ai);
                stateInfo.mAction = Crsync.Action_UserConfirm;
                stateInfo.mCode = Crsync.Code_OK;
            }
        } else {
            stateInfo.mAction = Crsync.Action_Done;
            stateInfo.mCode = code;
        }
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }

    private void handleUpdateApp(CrsyncInfo.StateInfo stateInfo) {
        int code = Crsync.JNI_onepiece_perform_updateapp();
        if(Crsync.Code_OK == code) {
            stateInfo.mAction = Crsync.Action_UserInstall;
        } else {
            stateInfo.mAction = Crsync.Action_Done;
        }
        stateInfo.mCode = code;
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }

    private void handleUpdateRes(CrsyncInfo.StateInfo stateInfo) {
        int code = Crsync.JNI_onepiece_perform_updateres();
        stateInfo.mAction = Crsync.Action_Done;
        stateInfo.mCode = code;
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }

    private void handleDone(CrsyncInfo.StateInfo stateInfo) {
        Crsync.JNI_onepiece_cleanup();
    }

    @Override
    public void onepiece_xfer(String hash, int percent) {
        logger.info("onepiece_xfer : " + hash + " " + percent + "%");
        if(hash.equalsIgnoreCase(mAppHash)) {
            CrsyncInfo.AppInfo ai = new CrsyncInfo.AppInfo();
            ai.mHash = mAppHash;
            ai.mPercent = percent;
            CrsyncInfo.updateApp(getContentResolver(), ai);
        } else {
            CrsyncInfo.ResInfo ri = new CrsyncInfo.ResInfo();
            ri.mHash = hash;
            ri.mPercent = percent;
            CrsyncInfo.updateRes(getContentResolver(), ri);
        }
    }

}
