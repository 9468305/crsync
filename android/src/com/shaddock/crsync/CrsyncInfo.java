package com.shaddock.crsync;

import static com.shaddock.crsync.CrsyncConstants.logger;

import java.util.Vector;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;

public class CrsyncInfo {
    public static class StateInfo {
        public int mAction = Crsync.Action_Idle;
        public int mCode = Crsync.Code_OK;

        public void dump() {
            logger.info("####StateInfo Action : " + mAction);
            logger.info("####StateInfo Code : " + mCode);
        }
    }
    
    public static StateInfo queryState(ContentResolver cr) {
        StateInfo info = new StateInfo();

        Cursor c = cr.query(CrsyncConstants.URI_STATE, null, null, null, null);

        if(c != null && c.moveToFirst()) {
            int actionColumn = c.getColumnIndex(CrsyncConstants.COLUMN_STATE_ACTION);
            int codeColumn = c.getColumnIndex(CrsyncConstants.COLUMN_STATE_CODE);

            info.mAction = c.getInt( actionColumn );
            info.mCode = c.getInt( codeColumn );
        } else {
            logger.severe("####StateInfo query URI fail");
        }
        if(c != null) {
            c.close();
        }
        return info;
    }

    public static void updateState(ContentResolver cr, StateInfo info) {
        ContentValues values = new ContentValues();
        values.put(CrsyncConstants.COLUMN_STATE_ACTION, info.mAction);
        values.put(CrsyncConstants.COLUMN_STATE_CODE, info.mCode);
        cr.update(CrsyncConstants.URI_STATE, values, null, null);
    }

	public static class ContentInfo {
	    public String mMagnet = "";
		public String mBaseUrl = "";
		public String mLocalApp = "";
		public String mLocalRes = "";

		public void dump() {
			logger.info("####ContentInfo mMagnet : " + mMagnet);
			logger.info("####ContentInfo mBaseUrl : " + mBaseUrl);
			logger.info("####ContentInfo mLocalApp : " + mLocalApp);
			logger.info("####ContentInfo mLocalRes : " + mLocalRes);
		}
	}

	public static ContentInfo queryContent(ContentResolver cr) {
		ContentInfo info = new ContentInfo();
		Cursor c = cr.query(CrsyncConstants.URI_CONTENT, null, null, null, null);

		if(c != null && c.moveToFirst()) {
	    	int magnetColumn = c.getColumnIndex(CrsyncConstants.COLUMN_CONTENT_MAGNET);
	    	int baseUrlColumn = c.getColumnIndex(CrsyncConstants.COLUMN_CONTENT_BASEURL);
	    	int localAppColumn = c.getColumnIndex(CrsyncConstants.COLUMN_CONTENT_LOCALAPP);
	    	int localResColumn = c.getColumnIndex(CrsyncConstants.COLUMN_CONTENT_LOCALRES);
	
	    	info.mMagnet = c.getString(magnetColumn);
	    	info.mBaseUrl = c.getString(baseUrlColumn);
	    	info.mLocalApp = c.getString(localAppColumn);
            info.mLocalRes = c.getString(localResColumn);
		} else {
			logger.severe("####ContentInfo query URI fail");
		}
		if(c != null) {
			c.close();
		}
    	return info;
	}

	public static void updateContent(ContentResolver cr, ContentInfo info) {
		ContentValues values = new ContentValues();
        values.put(CrsyncConstants.COLUMN_CONTENT_MAGNET, info.mMagnet);
        values.put(CrsyncConstants.COLUMN_CONTENT_BASEURL, info.mBaseUrl);
        values.put(CrsyncConstants.COLUMN_CONTENT_LOCALAPP, info.mLocalApp);
        values.put(CrsyncConstants.COLUMN_CONTENT_LOCALRES, info.mLocalRes);
        cr.update(CrsyncConstants.URI_CONTENT, values, null, null);
    }

    public static class AppInfo {
        public String mHash = "";
        public int mPercent = 0;

	    public void dump() {
	        logger.info("####AppInfo Hash : " + mHash);
	        logger.info("####AppInfo Percent : " + mPercent);
	    }
	}

    public static AppInfo queryApp(ContentResolver cr) {
        AppInfo info = new AppInfo();

        Cursor c = cr.query(CrsyncConstants.URI_APP, null, null, null, null);

        if(c != null && c.moveToFirst()) {
            int hashColumn = c.getColumnIndex(CrsyncConstants.COLUMN_APP_HASH);
            int percentColumn = c.getColumnIndex(CrsyncConstants.COLUMN_APP_PERCENT);

            info.mHash = c.getString(hashColumn);
            info.mPercent = c.getInt( percentColumn );
        } else {
            logger.severe("####AppInfo query URI fail");
        }
        if(c != null) {
            c.close();
        }
        return info;
    }

    public static void updateApp(ContentResolver cr, AppInfo info) {
        ContentValues values = new ContentValues();
        values.put(CrsyncConstants.COLUMN_APP_HASH, info.mHash);
        values.put(CrsyncConstants.COLUMN_APP_PERCENT, info.mPercent);
        cr.update(CrsyncConstants.URI_APP, values, null, null);
    }

    public static class ResInfo {
        public String mName = "";
        public String mHash = "";
        public int mPercent = 0;
        
        public void dump() {
            logger.info("####ResInfo name : " + mName);
            logger.info("####ResInfo hash : " + mHash);
            logger.info("####ResInfo Percent : " + mPercent);
        }
    }
    
    public static Vector<ResInfo> queryRes(ContentResolver cr) {
        Vector<ResInfo> infos = new Vector<ResInfo>();
        Cursor c = cr.query(CrsyncConstants.URI_RES, null, null, null, null);
        if(c != null && c.moveToFirst()) {

            int nameColumn = c.getColumnIndex(CrsyncConstants.COLUMN_RES_NAME);
            int hashColumn = c.getColumnIndex(CrsyncConstants.COLUMN_RES_HASH);
            int percentColumn = c.getColumnIndex(CrsyncConstants.COLUMN_RES_PERCENT);

            for( ; !c.isAfterLast(); c.moveToNext()) {
                ResInfo info = new ResInfo();
                info.mName = c.getString(nameColumn);
                info.mHash = c.getString(hashColumn);
                info.mPercent = c.getInt(percentColumn);
                infos.add(info);
            }
        } else {
            logger.severe("ResInfo query URI fail");
        }

        if(c != null) {
            c.close();
        }
        return infos;
    }

    public static void bulkInsertRes(ContentResolver cr, ResInfo[] infos) {
        ContentValues values[] = new ContentValues[infos.length];
        for(int i=0; i<infos.length; i++) {
            ContentValues v = new ContentValues();
            v.put(CrsyncConstants.COLUMN_RES_NAME, infos[i].mName);
            v.put(CrsyncConstants.COLUMN_RES_HASH, infos[i].mHash);
            v.put(CrsyncConstants.COLUMN_RES_PERCENT, infos[i].mPercent);
            values[i] = v;
        }
        cr.bulkInsert(CrsyncConstants.URI_RES, values);
    }

    public static void updateRes(ContentResolver cr, ResInfo info) {
        ContentValues values = new ContentValues();
        values.put(CrsyncConstants.COLUMN_RES_NAME, info.mName);
        values.put(CrsyncConstants.COLUMN_RES_HASH, info.mHash);
        values.put(CrsyncConstants.COLUMN_RES_PERCENT, info.mPercent);
        cr.update(CrsyncConstants.URI_RES, values, null, null);
    }
}
