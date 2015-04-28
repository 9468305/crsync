package com.shaddock.crsync;

import static android.text.format.DateUtils.SECOND_IN_MILLIS;

import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import android.net.Uri;
import android.os.Environment;

import static java.net.HttpURLConnection.HTTP_OK;

public class CrsyncConstants {

	public static final Logger logger = Logger.getLogger("crsync");
	private static final boolean mDebug = true;

	static {
		logger.setLevel( mDebug ? Level.ALL : Level.OFF );
		if(mDebug) {
			try {
				String file = Environment.getExternalStorageDirectory().getAbsolutePath() + "/crsync.log";
				FileHandler fh = new FileHandler(file, true);
				fh.setFormatter(new SimpleFormatter());
				logger.addHandler(fh);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	/** I love shaddock */
	public static final String DEFAULT_USER_AGENT = "shaddock";

	/** The same as AndroidManifest.xml provider authority */
    public static final String URIAUTHORITY = "com.shaddock.crsync.CrsyncProvider";

    public static final String URIPATH_CONTENT = "content";
    public static final String URIPATH_STATE = "state";
    public static final String URIPATH_RESOURCE = "res";

    public static final Uri URI_BASE = Uri.parse("content://" + URIAUTHORITY);
    public static final Uri URI_CONTENT = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_CONTENT);
    public static final Uri URI_STATE = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_STATE);
    public static final Uri URI_RES = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_RESOURCE);

    public static final String APP_VERSION_URL = "http://image.ib.qq.com/a/rel/";

    //public static final String APP_VERSION_FILE = "getdown.json";

    public static final String APP_RES_URL = "http://dlied5.qq.com/wjzj/a/rel/";

    public static final int DEFAULT_TIMEOUT = (int) (20 * SECOND_IN_MILLIS);

    /** resource type, used for GPU Texture format by IBSaga now */
    public static final String COLUMN_CONTENT_MAGNET = "content_magnet";
    public static final String COLUMN_CONTENT_BASEURL = "content_baseurl";
    public static final String COLUMN_CONTENT_LOCALAPP = "content_localapp";
    public static final String COLUMN_CONTENT_LOCALRES = "content_localres";

    public static final String COLUMN_STATE_ACTION = "state_action";
    public static final String COLUMN_STATE_CODE = "state_code";

    public static final String COLUMN_RES_NAME = "res_name";
    public static final String COLUMN_RES_PERCENT = "res_percent";

    public static final int VALIDATE_NO = 0;
    public static final int VALIDATE_OK = 1;

    public static final int STATUS_IDLE = 0;
    public static final int STATUS_RUNNING = 1;

    public static final class Download {
    	private Download() {}

    	/*
         * Lists the states that the GetDownloader can set to notify applications of the progress.
         * The codes follow the HTTP families:
         * 1xx: informational, local suspend (not network, WebAuth, NoSDcard, SDcard full, UserPause, etc)
         * 2xx: success
         * 3xx: redirects (not used by GetDownloader)
         * 4xx: client errors
         * 5xx: server errors
         */

        /**
         * Returns whether the status is informational (i.e. 1xx).
         */
        public static boolean isStatusInformational(int status) {
            return (status >= 100 && status < 200);
        }

        /**
         * Returns whether the status is a success (i.e. 2xx).
         */
        public static boolean isStatusSuccess(int status) {
            return (status >= 200 && status < 300);
        }

        /**
         * Returns whether the status is a client error (i.e. 4xx).
         */
        public static boolean isStatusClientError(int status) {
            return (status >= 400 && status < 500);
        }

        /**
         * Returns whether the status is a server error (i.e. 5xx).
         */
        public static boolean isStatusServerError(int status) {
            return (status >= 500 && status < 600);
        }

        public static final int STATUS_LOCAL_CONFIG_ERROR = 179;
        public static final int STATUS_WAITING_FOR_NETWORK = 180;
        public static final int STATUS_NO_SDCARD = 181;
        public static final int STATUS_SDCARD_FULL = 182;
        public static final int STATUS_PAUSED_BY_USER = 183;
        public static final int STATUS_QUEUED_FOR_WIFI = 184;
        public static final int STATUS_MIMETYPE_ERROR = 185;
        public static final int STATUS_CANNOT_RESUME = 186;

        /**
         * This download has successfully completed.
         * Warning: there might be other status values that indicate success
         * in the future.
         * Use isSucccess() to capture the entire category.
         */
        public static final int STATUS_SUCCESS = HTTP_OK; //200

        /**
         * This request couldn't be parsed. This is also used when processing
         * requests with unknown/unsupported URI schemes.
         */
        public static final int STATUS_BAD_REQUEST = 400;

        /**
         * This download has completed with an error.
         * Warning: there will be other status values that indicate errors in
         * the future. Use isStatusError() to capture the entire category.
         */
        public static final int STATUS_UNKNOWN_ERROR = 491;

        /**
         * This download couldn't be completed because of a storage issue.
         * Typically, that's because the filesystem is missing or full.
         * Use the more specific {@link #STATUS_INSUFFICIENT_SPACE_ERROR}
         * and {@link #STATUS_DEVICE_NOT_FOUND_ERROR} when appropriate.
         */
        public static final int STATUS_FILE_ERROR = 492;

        /**
         * This download couldn't be completed because of an HTTP redirect response
         * that the download manager couldn't handle.
         */
        public static final int STATUS_UNHANDLED_REDIRECT = 493;

        /**
         * This download couldn't be completed because of an
         * unspecified unhandled HTTP code.
         */
        public static final int STATUS_UNHANDLED_HTTP_CODE = 494;

        /**
         * This download couldn't be completed because of an
         * error receiving or processing data at the HTTP level.
         */
        public static final int STATUS_HTTP_DATA_ERROR = 495;

    }//End of Download

    public static final String MIMETYPE_APK = "application/vnd.android.package-archive";
    public static final String MIMETYPE_BIN = "application/octet-stream";

    public static final long SDCARD_EXTRA_SIZE = 16 * 1024 * 1024; //Keep sdcard extra 16M not use

    /** Global download speed */
    public static long GDownloadSpeed = 0;
    /** Global download current bytes, now just used for QueryUpdate_Download apk file */
    public static long GDownloadBytes = 0;
    /** Global download UserAcceptance */
    public static boolean canDownloadBase = false;
    /** Global download UserAcceptance */
    public static boolean canDownloadOption = false;
}
