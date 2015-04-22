package com.shaddock.crsync;

public class CrsyncJava {

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
    public static final int Code_Action_Error   = 5;
    public static final int Code_BUG            = 100;

    public static final int OPT_MagnetID    = 0;
    public static final int OPT_BaseUrl     = 1;
    public static final int OPT_LocalApp    = 2;
    public static final int OPT_LocalRes    = 3;
    public static final int OPT_Action      = 4;

    public static final int INFO_Magnet     = 0;

    static {
        System.loadLibrary("crsync");
    }

    //native crsync functions
    public static native int    native_crsync_init();
    public static native int    native_crsync_setopt(int opt, String value);
    public static native String native_crsync_getinfo(int info);
    public static native int    native_crsync_perform_query();
    public static native int    native_crsync_perform_updateapp();
    public static native int    native_crsync_perform_updateres();
    public static native int    native_crsync_cleanup();

    public static void java_callback() {
    }

    public static class Magnet {
        public String curr_id = "";
        public String next_id = "";
        public String appname = "";
        public String apphash = "";
        public String[] resname = null;
        public String[] reshash = null;

        public static Magnet getValue(String value) {
            String [] s = value.split(";");
            if(s.length < 4) {
                return null;
            }
            Magnet m = new Magnet();
            m.curr_id = s[0];
            m.next_id = s[1];
            m.appname = s[2];
            m.apphash = s[3];

            int size = (s.length - 4)/2;
            if(size == 0) {
                return m;
            }
            m.resname = new String[size];
            m.reshash = new String[size];
            for(int i = 0; 4+i*2 < s.length; i++) {
                m.resname[i] = s[4+i*2];
                m.reshash[i] = s[5+i*2];
            }
            return m;
        }
    }
    
}
