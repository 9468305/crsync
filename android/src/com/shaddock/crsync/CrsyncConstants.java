package com.shaddock.crsync;

import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import android.net.Uri;
import android.os.Environment;

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

    /** resource type, used for GPU Texture format by IBSaga now */
    public static final String COLUMN_CONTENT_MAGNET = "content_magnet";
    public static final String COLUMN_CONTENT_BASEURL = "content_baseurl";
    public static final String COLUMN_CONTENT_LOCALAPP = "content_localapp";
    public static final String COLUMN_CONTENT_LOCALRES = "content_localres";

    public static final String COLUMN_STATE_ACTION = "state_action";
    public static final String COLUMN_STATE_CODE = "state_code";

    public static final String COLUMN_RES_NAME = "res_name";
    public static final String COLUMN_RES_HASH = "res_hash";
    public static final String COLUMN_RES_PERCENT = "res_percent";

    public static final int VALIDATE_NO = 0;
    public static final int VALIDATE_OK = 1;

    public static final int STATUS_IDLE = 0;
    public static final int STATUS_RUNNING = 1;

    /** Global download speed */
    public static long GDownloadSpeed = 0;
    /** Global download current bytes, now just used for QueryUpdate_Download apk file */
    public static long GDownloadBytes = 0;
    /** Global download UserAcceptance */
    public static boolean canDownloadBase = false;
    /** Global download UserAcceptance */
    public static boolean canDownloadOption = false;
}
