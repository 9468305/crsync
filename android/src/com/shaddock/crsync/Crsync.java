package com.shaddock.crsync;

import java.util.Vector;

public class Crsync {

    public static final int Action_Idle             = 0;
    public static final int Action_Query            = 1;
    public static final int Action_UserConfirm      = 2;
    public static final int Action_UpdateApp        = 3;
    public static final int Action_UserInstall      = 4;
    public static final int Action_UpdateRes        = 5;
    public static final int Action_Done             = 6;

    /* The same as crsync.h ENUM CRSYNCcode */
    public static final int Code_OK             = 0;
    public static final int Code_Failed_Init    = 1;
    public static final int Code_Invalid_Opt    = 2;
    public static final int Code_File_Error     = 3;
    public static final int Code_CURL_Error     = 4;
    public static final int Code_BUG            = 100;

    public static final int OPT_MagnetID    = 0;
    public static final int OPT_BaseUrl     = 1;
    public static final int OPT_LocalApp    = 2;
    public static final int OPT_LocalRes    = 3;
    public static final int OPT_Action      = 4;

    private static OnepieceObserver mObserver = null;

    public static void setObserver(OnepieceObserver ob) {
        mObserver = ob;
    }

    public static void delObserver() {
        mObserver = null;
    }

    static {
        System.loadLibrary("crsync");
    }

    //native crsync functions
    public static native int    JNI_onepiece_init();
    public static native int    JNI_onepiece_setopt(int opt, String value);
    public static native String JNI_onepiece_getinfo_magnet();
    public static native int    JNI_onepiece_perform_query();
    public static native int    JNI_onepiece_perform_updateapp();
    public static native int    JNI_onepiece_perform_updateres();
    public static native void   JNI_onepiece_cleanup();

    public static void java_onepiece_xfer(String hash, int percent) {
        if(null != mObserver) {
            mObserver.onepiece_xfer(hash, percent);
        }
    }

    public static class Res {
        public String name = "";
        public String hash = "";
        public int size = 0;
    }

    public static class Magnet {
        public String curr_id = "";
        public String next_id = "";
        public String app_hash = "";
        public Vector<Res> res_list = new Vector<Res>();

        public static Magnet getValue(String value) {
            String [] s = value.split(";");
            if(s.length < 3) {
                return null;
            }
            int i = 0;
            Magnet m = new Magnet();
            m.curr_id = s[i++];
            m.next_id = s[i++];
            m.app_hash = s[i++];
            while(i<s.length) {
                Res r = new Res();
                r.name = s[i++];
                r.hash = s[i++];
                r.size = Integer.parseInt(s[i++]);
                m.res_list.add(r);
            }
            return m;
        }
    }

}
