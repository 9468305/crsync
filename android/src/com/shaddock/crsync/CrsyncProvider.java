package com.shaddock.crsync;

import static com.shaddock.crsync.CrsyncConstants.logger;

import java.util.Vector;

import android.content.ComponentName;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

public class CrsyncProvider extends ContentProvider {

    /** URI matcher used to recognize URIs sent by applications */
    private static final UriMatcher sURIMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    /** URI matcher constant for {@link CrsyncConstants#URI_STATE} */
    private static final int CODE_STATE = 0;
    /** URI matcher constant for {@link CrsyncConstants#URI_CONTENT} */
    private static final int CODE_CONTENT = 1;
    /** URI matcher constant for {@link CrsyncConstants#URI_APP} */
    private static final int CODE_APP = 2;
    /** URI matcher constant for {@link CrsyncConstants#URI_RES} */
    private static final int CODE_RESOURCE = 3;

    static {
        sURIMatcher.addURI(CrsyncConstants.URIAUTHORITY, CrsyncConstants.URIPATH_STATE, CODE_STATE);
        sURIMatcher.addURI(CrsyncConstants.URIAUTHORITY, CrsyncConstants.URIPATH_CONTENT, CODE_CONTENT);
        sURIMatcher.addURI(CrsyncConstants.URIAUTHORITY, CrsyncConstants.URIPATH_APP, CODE_APP);
        sURIMatcher.addURI(CrsyncConstants.URIAUTHORITY, CrsyncConstants.URIPATH_RESOURCE, CODE_RESOURCE);
    }

    private static final String TYPE_ITEM = "vnd.android.cursor.item/" + CrsyncConstants.URIAUTHORITY;
    private static final String TYPE_DIR = "vnd.android.cursor.dir/" + CrsyncConstants.URIAUTHORITY;

    private static final String[] CURSOR_COLUMN_STATE = {
    	CrsyncConstants.COLUMN_STATE_ACTION,
    	CrsyncConstants.COLUMN_STATE_CODE
    };

    private static final String[] CURSOR_COLUMN_CONTENT = {
    	CrsyncConstants.COLUMN_CONTENT_MAGNET,
    	CrsyncConstants.COLUMN_CONTENT_BASEURL,
    	CrsyncConstants.COLUMN_CONTENT_LOCALAPP,
    	CrsyncConstants.COLUMN_CONTENT_LOCALRES,
    };

    private static final String[] CURSOR_COLUMN_APP = {
        CrsyncConstants.COLUMN_APP_HASH,
        CrsyncConstants.COLUMN_APP_PERCENT,
    };

    private static final String[] CURSOR_COLUMN_RESOURCE = {
    	CrsyncConstants.COLUMN_RES_NAME,
        CrsyncConstants.COLUMN_RES_HASH,
        CrsyncConstants.COLUMN_RES_SIZE,
    	CrsyncConstants.COLUMN_RES_PERCENT,
    };

    //private static final String SP_NAME = "sp_crsync";
    //private static SharedPreferences mSP = null;

    private static volatile int mAction = Crsync.Action_Idle;
    private static volatile int mCode = Crsync.Code_OK;

    private static volatile String mMagnet = "";
    private static volatile String mBaseUrl = "";
    private static volatile String mLocalApp = "";
    private static volatile String mLocalRes = "";

    private static volatile String mAppHash = "";
    private static volatile int mAppPercent = 0;

    private static Vector<CrsyncInfo.ResInfo> mRes = new Vector<CrsyncInfo.ResInfo>();

	@Override
	public boolean onCreate() {
		logger.info("CrsyncProvider onCreate");
		//Context ctx = getContext();
		//mSP = ctx.getApplicationContext().getSharedPreferences(SP_NAME, Context.MODE_PRIVATE);
		startCrsyncService();
		return true;
	}
/*
 *     private static final long UPDATE_CHECK_PERIOD_MILLISECONDS = 24*60*60*1000; //24hours
    private static boolean isUpdateCheckRequired() {
    	long last = IBApplication.mSP.getLong(IBApplication.KSP_update, -1);
    	if(last == -1) {
    		//first time to check
    		return true;
    	}
    	Calendar now = Calendar.getInstance();
    	Calendar old = Calendar.getInstance();
    	old.setTimeInMillis(last);
    	if(now.before(old)) {
    		//device time have been changed
    		return true;
    	}
    	old.setTimeInMillis(last + UPDATE_CHECK_PERIOD_MILLISECONDS);
    	if(now.after(old)) {
    		//pass check period
    		return true;
    	}
    	return false;
    }
 * 
 * */
	@Override
	public Cursor query(final Uri uri, final String[] projection, final String selection, final String[] selectionArgs, final String sortOrder) {
		Cursor c = null;
		int match = sURIMatcher.match(uri);
		switch (match) {
		case CODE_STATE:
		{
            MatrixCursor mc = new MatrixCursor(CURSOR_COLUMN_STATE);
            mc.addRow( new Object[] {mAction, mCode});
            c = mc;
            break;
		}
		case CODE_CONTENT:
		{
		    MatrixCursor mc = new MatrixCursor(CURSOR_COLUMN_CONTENT);
            mc.addRow( new Object[] {mMagnet, mBaseUrl, mLocalApp, mLocalRes});
            c = mc;
		    break;
		}
		case CODE_APP:
		{
		    MatrixCursor mc = new MatrixCursor(CURSOR_COLUMN_APP);
		    mc.addRow( new Object[] {mAppHash, mAppPercent});
		    c = mc;
		    break;
		}
		case CODE_RESOURCE:
		{
            MatrixCursor mc = new MatrixCursor(CURSOR_COLUMN_RESOURCE);
            for(CrsyncInfo.ResInfo x : mRes) {
                mc.addRow( new Object[] { x.mName, x.mHash, x.mSize, x.mPercent });
            }
            c = mc;
		    break;
		}
		default:
            logger.severe("Unknown URI: " + uri);
            throw new IllegalArgumentException("query Unknown URI: " + uri);
		}
		return c;
	}

	@Override
	public String getType(final Uri uri) {
		int match = sURIMatcher.match(uri);
        switch (match) {
        case CODE_CONTENT:
        case CODE_STATE:
        case CODE_APP:
            return TYPE_ITEM;
        case CODE_RESOURCE:
            return TYPE_DIR;
        default:
            logger.severe("Unknown URI: " + uri);
        	throw new IllegalArgumentException("getType Unknown URI: " + uri);
        }
	}

	@Override
	public Uri insert(Uri uri, ContentValues values) {
	    throw new IllegalArgumentException("insert Unimplement URI: " + uri);
	}

	@Override
    public int bulkInsert(Uri uri, ContentValues[] values) {
	    int match = sURIMatcher.match(uri);
        switch (match) {
        case CODE_RESOURCE:
            mRes.clear();
            for(ContentValues v : values) {
                CrsyncInfo.ResInfo r = new CrsyncInfo.ResInfo();
                r.mName = v.getAsString(CrsyncConstants.COLUMN_RES_NAME);
                r.mHash = v.getAsString(CrsyncConstants.COLUMN_RES_HASH);
                r.mSize = v.getAsInteger(CrsyncConstants.COLUMN_RES_SIZE);
                r.mPercent = v.getAsInteger(CrsyncConstants.COLUMN_RES_PERCENT);
                mRes.add(r);
            }
            notifyChange(uri);
            return values.length;
        default:
            logger.severe("Unknown URI: " + uri);
            throw new IllegalArgumentException("bulkInsert Unknown URI: " + uri);
        }
    }

    @Override
	public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new IllegalArgumentException("delete Unimplement URI: " + uri);
	}

	@Override
	public int update(final Uri uri, final ContentValues values, final String selection, final String[] selectionArgs) {
		int match = sURIMatcher.match(uri);
		switch (match) {
		case CODE_STATE:
		    mAction = values.getAsInteger(CrsyncConstants.COLUMN_STATE_ACTION);
		    mCode = values.getAsInteger(CrsyncConstants.COLUMN_STATE_CODE);
		    notifyChange(uri);
		    return 1;
        case CODE_CONTENT:
            mMagnet = values.getAsString(CrsyncConstants.COLUMN_CONTENT_MAGNET);
            mBaseUrl = values.getAsString(CrsyncConstants.COLUMN_CONTENT_BASEURL);
            mLocalApp = values.getAsString(CrsyncConstants.COLUMN_CONTENT_LOCALAPP);
            mLocalRes = values.getAsString(CrsyncConstants.COLUMN_CONTENT_LOCALRES);
            notifyChange(uri);
            return 1;
        case CODE_APP:
            mAppHash = values.getAsString(CrsyncConstants.COLUMN_APP_HASH);
            mAppPercent = values.getAsInteger(CrsyncConstants.COLUMN_APP_PERCENT);
            notifyChange(uri);
            return 1;
        case CODE_RESOURCE:
            CrsyncInfo.ResInfo r = new CrsyncInfo.ResInfo();
            r.mName = values.getAsString(CrsyncConstants.COLUMN_RES_NAME);
            r.mHash = values.getAsString(CrsyncConstants.COLUMN_RES_HASH);
            r.mSize = values.getAsInteger(CrsyncConstants.COLUMN_RES_SIZE);
            r.mPercent = values.getAsInteger(CrsyncConstants.COLUMN_RES_PERCENT);
            for (CrsyncInfo.ResInfo x : mRes) {
                if(x.mName.equalsIgnoreCase(r.mName) || x.mHash.equalsIgnoreCase(r.mHash)) {
                    x.mPercent = r.mPercent;
                    notifyChange(uri);
                    return 1;
                }
            }
            logger.severe("update resource not found : " + r.toString());
            return 0;
        default:
            logger.severe("Unknown URI: " + uri);
            throw new IllegalArgumentException("update Unknown URI: " + uri);
        }
	}

	private void startCrsyncService() {
		if(CrsyncService.isRunning) {
			return;
		}
		Context ctx = getContext();
        ComponentName result = ctx.startService(new Intent(ctx, CrsyncService.class));
        logger.info("CrsyncProvider startService : " + result);
	}

	private void notifyChange(Uri uri) {
		getContext().getContentResolver().notifyChange(uri, null);
		startCrsyncService();
	}
}
