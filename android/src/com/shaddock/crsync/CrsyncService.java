package com.shaddock.crsync;

import static com.shaddock.crsync.CrsyncConstants.logger;
import static java.net.HttpURLConnection.HTTP_OK;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Vector;

import com.shaddock.crsync.CrsyncInfo.ResInfo;

import android.app.Service;
import android.content.Intent;
import android.database.ContentObserver;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;

public class CrsyncService extends Service implements OnepieceObserver {

	public static volatile boolean isRunning = false;

	private CrsyncContentObserver mObserver = null;

    private UpdateThread mUpdateThread = null;

    private boolean mPendingUpdate = false;

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
                CrsyncJava.setObserver(CrsyncService.this);
                CrsyncInfo.StateInfo stateInfo = CrsyncInfo.queryState(getContentResolver());
            	handleFromProvider(stateInfo);
            	CrsyncJava.delObserver();
            }
        }
    }// End of class UpdateThread

    private void handleFromProvider(CrsyncInfo.StateInfo stateInfo) {
        logger.info("CrsyncService handleFromProvider");
        stateInfo.dump();
        switch (stateInfo.mAction) {
        case CrsyncJava.Action_Idle:
            handleIdle(stateInfo);
            break;
        case CrsyncJava.Action_Query:
            handleQuery(stateInfo);
            break;
        case CrsyncJava.Action_UserConfirm:
            //do nothing
            break;
        case CrsyncJava.Action_UpdateApp:
            handleUpdateApp(stateInfo);
            break;
        case CrsyncJava.Action_UserInstall:
            //do nothing
            break;
        case CrsyncJava.Action_UpdateRes:
            handleUpdateRes(stateInfo);
            break;
        case CrsyncJava.Action_Done:
            //do nothing
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
            code = CrsyncJava.JNI_onepiece_init();
            if(CrsyncJava.Code_OK != code) break;
            code = CrsyncJava.JNI_onepiece_setopt(CrsyncJava.OPT_MagnetID, contentInfo.mMagnet);
            if(CrsyncJava.Code_OK != code) break;
            code = CrsyncJava.JNI_onepiece_setopt(CrsyncJava.OPT_BaseUrl, contentInfo.mBaseUrl);
            if(CrsyncJava.Code_OK != code) break;
            code = CrsyncJava.JNI_onepiece_setopt(CrsyncJava.OPT_LocalApp, contentInfo.mLocalApp);
            if(CrsyncJava.Code_OK != code) break;
            code = CrsyncJava.JNI_onepiece_setopt(CrsyncJava.OPT_LocalRes, contentInfo.mLocalRes);
            if(CrsyncJava.Code_OK != code) break;
        } while(false);

        if(CrsyncJava.Code_OK == code) {
            stateInfo.mAction = CrsyncJava.Action_Query;
            stateInfo.mCode = CrsyncJava.Code_OK;
        } else {
            stateInfo.mAction = CrsyncJava.Action_Done;
            stateInfo.mCode = code;
        }
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }

    private void handleQuery(CrsyncInfo.StateInfo stateInfo) {
        logger.info("CrsyncService handleQuery");
        int code = CrsyncJava.JNI_onepiece_perform_query();
        if(CrsyncJava.Code_OK == code) {
            CrsyncInfo.ContentInfo ci = CrsyncInfo.queryContent(getContentResolver());
            String magnetString = CrsyncJava.JNI_onepiece_getinfo(CrsyncJava.INFO_Magnet);
            logger.info("INFO_Magnet = " + magnetString);
            CrsyncJava.Magnet magnet = CrsyncJava.Magnet.getValue(magnetString);
            if(null != magnet) {
                int size = magnet.resname.length;
                for(int i=0; i < size; i++) {
                    CrsyncInfo.ResInfo r = new CrsyncInfo.ResInfo();
                    r.mName = magnet.resname[i];
                    r.mPercent = 0;
                    CrsyncInfo.updateRes(getContentResolver(), r);
                }
                if(magnet.curr_id.equalsIgnoreCase(ci.mMagnet)) {
                    stateInfo.mAction = CrsyncJava.Action_UpdateRes;
                    stateInfo.mCode = CrsyncJava.Code_OK;
                } else {
                    stateInfo.mAction = CrsyncJava.Action_UserConfirm;
                    stateInfo.mCode = CrsyncJava.Code_OK;
                }
            } else {
                stateInfo.mAction = CrsyncJava.Action_Done;
                stateInfo.mCode = CrsyncJava.Code_BUG;
            }
        } else {
            stateInfo.mAction = CrsyncJava.Action_Done;
            stateInfo.mCode = code;
        }
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }

    private void handleUpdateApp(CrsyncInfo.StateInfo stateInfo) {
        int code = CrsyncJava.JNI_onepiece_perform_updateapp();
        if(CrsyncJava.Code_OK == code) {
            ;
        }
    }

    private void handleUpdateRes(CrsyncInfo.StateInfo stateInfo) {
        int code = CrsyncJava.JNI_onepiece_perform_updateres();
        stateInfo.mAction = CrsyncJava.Action_Done;
        stateInfo.mCode = code;
        CrsyncInfo.updateState(getContentResolver(), stateInfo);
        handleFromProvider(stateInfo);
    }
/*

    private void handleQueryUpdate(GetDownInfo.StateInfo stateInfo) {

    	HttpURLConnection conn = null;
    	InputStream in = null;
    	String jsonString = null;

    	try {
    		final NetworkInfo info = Helper.getActiveNetworkInfo(this);
    		if(info != null && info.isConnected()) {
    			String versionURL = Constants.APP_VERSION_URL + Helper.getVersionCode(this) + Constants.APP_VERSION_FILE;
    			URL url = new URL(versionURL);
        		conn = (HttpURLConnection) url.openConnection();
                conn.setInstanceFollowRedirects(true);
                conn.setConnectTimeout(Constants.DEFAULT_TIMEOUT);
                conn.setReadTimeout(Constants.DEFAULT_TIMEOUT);
                conn.setRequestProperty("User-Agent", Constants.DEFAULT_USER_AGENT);

                final int responseCode = conn.getResponseCode();
                logger.info("handleQueryUpdate_ING HTTPResponse Code = " + responseCode);
                String mimeType = Helper.normalizeMimeType(conn.getContentType());
                if( HTTP_OK == responseCode && mimeType.contains("json") ) {
                    in = conn.getInputStream();
                    ByteArrayOutputStream baos=new ByteArrayOutputStream();
                    byte buffer[] = new byte[1024];
                    int len = -1;
                    //TODO: maybe need to handle IOException "unexpected end of stream", ignore now
                    while( (len=in.read(buffer)) != -1 ){
                        baos.write(buffer, 0, len);
                    }
                    jsonString=baos.toString();
                }
        	}
    	} catch (MalformedURLException me) {
            logger.severe("WTF: wrong URL");
            me.printStackTrace();

        } catch (IOException ioe) {
    		logger.severe("handleQueryUpdate_ING IOException");
    		ioe.printStackTrace();
    	} finally {
    		Helper.closeQuietly(in);
    		if(conn != null) {
                conn.disconnect();
            }
        }

        if(jsonString != null) {
            logger.info("jsonString = " + jsonString);
            GetDownVersion version = new GetDownVersion(jsonString);
            if( version.mStatus.equalsIgnoreCase(GetDownVersion.STATUS_FORCE) ||
                version.mStatus.equalsIgnoreCase(GetDownVersion.STATUS_OPTION) ||
                version.mStatus.equalsIgnoreCase(GetDownVersion.STATUS_STOP) ) {

				GetDownInfo.ContentInfo ci = GetDownInfo.queryContent(getContentResolver());
				ci.mQueryUpdateResult = jsonString;
				GetDownInfo.updateContent(getContentResolver(), ci);

				stateInfo.mAction = Constants.Action.QUERYUPDATE_USERCONFIRM;
				stateInfo.mStatus = Constants.STATUS_IDLE;
				GetDownInfo.updateState(getContentResolver(), stateInfo);
				return;
			}
		}
        stateInfo.mAction = Constants.Action.DOWNLOAD_BASE_RESOURCE;
		stateInfo.mStatus = Constants.STATUS_IDLE;
		GetDownInfo.updateState(getContentResolver(), stateInfo);
		handleFromProvider(stateInfo);

    }

    private void handleQueryUpdate_Download(GetDownInfo.StateInfo stateInfo) {

    	GetDownInfo.ContentInfo ci = GetDownInfo.queryContent(getContentResolver());
    	GetDownVersion gdv = new GetDownVersion(ci.mQueryUpdateResult);

    	new File( Helper.getCurrentResourceDir(this) ).mkdirs();
    	File f = new File(Helper.getCurrentResourceDir(this), gdv.mMD5 + ".apk");

		GetDownloader.Info info = new GetDownloader.Info();
		info.mFilename = f.getAbsolutePath();
		info.mUrl = gdv.mURL;
		info.mTotalBytes = gdv.mSize;
		info.mCurrentBytes = f.exists() ? f.length() : 0 ;
		info.mMimeType = Constants.MIMETYPE_APK;

		GetDownloader loader = new GetDownloader(this, info, null, stateInfo);
		loader.run();

		if(loader.getFinalStatus() == Constants.Download.STATUS_SUCCESS) {
			//verify MD5
			if(f.exists() && gdv.mSize == f.length()) {
				String md5hash = Helper.getMD5Hash(f.getAbsolutePath());
				logger.info("####getMD5Hash : " + md5hash);
	    		if(md5hash.equalsIgnoreCase(gdv.mMD5)) {
	    			logger.info("####validate file OK ");
	    			//make sure the APK file can be used by System PackageManager
	    			f.setReadable(true, false);
	    			f.setWritable(true, false);
	    			f.setExecutable(true, false);
	    			stateInfo.mAction = Constants.Action.QUERYUPDATE_USERINSTALL;
	    			stateInfo.mStatus = Constants.STATUS_IDLE;
	    			GetDownInfo.updateState(getContentResolver(), stateInfo);
	    			return;
	    		}
			}
			logger.info("####validate file NO");
			f.delete();
		}

		stateInfo.mStatus = loader.getFinalStatus();
		GetDownInfo.updateState(getContentResolver(), stateInfo);
    }


    private boolean handleValidateResource(GetDownInfo.StateInfo stateInfo) {

    	GetDownInfo.ContentInfo info = GetDownInfo.queryContent(getContentResolver());

    	validateResource(info.mOldVersionCode, Constants.URI_BASERESOURCE);
    	validateResource(info.mOldVersionCode, Constants.URI_OPTIONRESOURCE);

    	stateInfo.mStatus = Constants.STATUS_DOWNLOADRESOURCE;
    	stateInfo.mSubStatus = Constants.STATUS_DEFAULT;
    	GetDownInfo.updateState(getContentResolver(), stateInfo);
    	return true;
    }

    private void validateResource(int oldVersionCode, Uri uri) {

    	Vector<GetDownInfo.ResourceInfo> infos = GetDownInfo.queryResource(getContentResolver(),  uri);

    	for(GetDownInfo.ResourceInfo info : infos) {

    		ContentValues values = new ContentValues();
			values.put(Constants.COLUMN_RES_FILE, info.mFile);

			logger.info("####validating current file");
			File f = new File(Helper.getCurrentResourceDir(this), info.mFile);
			validateFile(f, info.mSize, info.mMD5, values, Constants.COLUMN_RES_VALIDATE);
 //TODO: skip patch now
			logger.info("####validating old file");
			f = new File(Helper.getOldResourceDir(this, oldVersionCode), info.mFile);
			validateFile(f, info.mOldSize, info.mOldMD5, values, Constants.COLUMN_RES_OLDVALIDATE);

			logger.info("validating patch file");
			f = new File(Helper.getPatchResourceDir(this, oldVersionCode), info.mFile);
			validateFile(f, info.mPatchSize, info.mPatchMD5, values, Constants.COLUMN_RES_PATCHVALIDATE);

			getContentResolver().update(uri, values, null, null);
    	}
    }

    private void handleDownloadResource(GetDownInfo.StateInfo stateInfo, Uri uri) {

    	new File( Helper.getCurrentResourceDir(this) ).mkdirs();

        GetDownInfo.ContentInfo contentInfo = GetDownInfo.queryContent(getContentResolver());
        if(contentInfo.mType.isEmpty()) {
        	logger.severe("ContentInfo Type not defined");
        	return;
        }

    	logger.info("GetDownService download base resource");

    	Vector<GetDownInfo.ResourceInfo> infos = GetDownInfo.queryResource(getContentResolver(),  uri);
    	GetDownInfo.ResourceInfo resInfo = null;
    	for(GetDownInfo.ResourceInfo x : infos) {
    		if(!x.mValidate) {
    			resInfo = x;
    			break;
    		}
    	}

    	if(resInfo == null) {
    		stateInfo.mAction = (uri.equals(Constants.URI_BASERESOURCE)) ? Constants.Action.DOWNLOAD_OPTION_RESOURCE : Constants.Action.IDLE;
    		stateInfo.mStatus = Constants.STATUS_IDLE;
    		GetDownInfo.updateState(getContentResolver(), stateInfo);
    		handleFromProvider(stateInfo);
    		return;
    	}

    	if(uri.equals(Constants.URI_BASERESOURCE) && !Constants.canDownloadBase) {
    		//waiting user acceptance
    		return;
    	}
    	if(uri.equals(Constants.URI_OPTIONRESOURCE) && !Constants.canDownloadBase) {
    		//waiting user acceptance
    		return;
    	}

    	File f = new File(Helper.getCurrentResourceDir(this), resInfo.mFile);

		GetDownloader.Info info = new GetDownloader.Info();
		info.mResName = resInfo.mFile;
		info.mFilename = f.getAbsolutePath();
		info.mUrl = Constants.APP_RES_URL + Helper.getVersionCode(this) + "/" + contentInfo.mType + "/" + resInfo.mFile;
		info.mTotalBytes = resInfo.mSize;
		info.mCurrentBytes = resInfo.mCurrentBytes;
		info.mMimeType = Constants.MIMETYPE_BIN;

		GetDownloader loader = new GetDownloader(this, info, uri, stateInfo);
		loader.run();

		if(loader.getFinalStatus() == Constants.Download.STATUS_SUCCESS) {
			//verify MD5
			if(f.exists() && resInfo.mSize == f.length()) {
				String md5hash = Helper.getMD5Hash(f.getAbsolutePath());
				logger.info("####getMD5Hash : " + md5hash);
	    		if(md5hash.equalsIgnoreCase(resInfo.mMD5)) {
	    			logger.info("####validate file OK ");
	    			resInfo.mValidate = true;
	    			GetDownInfo.updateResource(getContentResolver(), resInfo, uri);
	    			handleFromProvider(stateInfo);
	    			return;
	    		}
			}
			logger.info("####validate file NO");
			f.delete();
		}

		stateInfo.mStatus = loader.getFinalStatus();
		GetDownInfo.updateState(getContentResolver(), stateInfo);

    }*/

    @Override
    public void onepiece_xfer(String name, int percent) {
        CrsyncInfo.ResInfo ri = new CrsyncInfo.ResInfo();
        ri.mName = name;
        ri.mPercent = percent;
        CrsyncInfo.updateRes(getContentResolver(), ri);
    }

}
