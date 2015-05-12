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

	/** The same as AndroidManifest.xml provider authority */
    public static final String URIAUTHORITY = "com.shaddock.crsync.CrsyncProvider";

    public static final String URIPATH_CONTENT = "content";
    public static final String URIPATH_STATE = "state";
    public static final String URIPATH_APP = "app";
    public static final String URIPATH_RESOURCE = "res";

    public static final Uri URI_BASE = Uri.parse("content://" + URIAUTHORITY);
    public static final Uri URI_CONTENT = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_CONTENT);
    public static final Uri URI_STATE = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_STATE);
    public static final Uri URI_APP = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_APP);
    public static final Uri URI_RES = Uri.parse("content://" + URIAUTHORITY + "/" + URIPATH_RESOURCE);

    public static final String COLUMN_CONTENT_MAGNET = "content_magnet";
    public static final String COLUMN_CONTENT_BASEURL = "content_baseurl";
    public static final String COLUMN_CONTENT_LOCALAPP = "content_localapp";
    public static final String COLUMN_CONTENT_LOCALRES = "content_localres";

    public static final String COLUMN_STATE_ACTION = "state_action";
    public static final String COLUMN_STATE_CODE = "state_code";

    public static final String COLUMN_APP_HASH = "app_hash";
    public static final String COLUMN_APP_PERCENT = "app_percent";

    public static final String COLUMN_RES_NAME = "res_name";
    public static final String COLUMN_RES_HASH = "res_hash";
    public static final String COLUMN_RES_PERCENT = "res_percent";

}
