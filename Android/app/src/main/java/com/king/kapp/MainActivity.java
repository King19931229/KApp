package com.king.kapp;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.DialogInterface;
import android.content.pm.ApplicationInfo;
import android.os.Environment;
import android.os.StatFs;
import android.os.storage.StorageManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.concurrent.Semaphore;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import static android.content.ContentValues.TAG;

public class MainActivity extends NativeActivity {

    private static boolean appFileDirectoryGuard = false;
    private static String appFileDirectory = "";

    private static byte[] unzipbuffer = new byte[512 * 1024];
    private static String APP_FILE_DIRECTORY_STAMP = "app_file_directory";
    private static long EXPECT_FREE_SPACE = 1024 * 1024 * 1024;

    static {
        // Load native library
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        ArrayList<String> folders = new ArrayList<String>();
        folders.add("");
        unzipBatch(folders);
        super.onCreate(savedInstanceState);
    }

    // Use a semaphore to create a modal dialog
    private final Semaphore semaphore = new Semaphore(0, true);

    public void showAlert(final String message)
    {
        final MainActivity activity = this;

        ApplicationInfo applicationInfo = activity.getApplicationInfo();
        final String applicationName = applicationInfo.nonLocalizedLabel.toString();

        this.runOnUiThread(new Runnable() {
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity, android.R.style.Theme_Material_Dialog_Alert);
                builder.setTitle(applicationName);
                builder.setMessage(message);
                builder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        semaphore.release();
                    }
                });
                builder.setCancelable(false);
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        });
        try {
            semaphore.acquire();
        }
        catch (InterruptedException e) { }
    }

    @SuppressWarnings("deprecation")
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
    private long getVolumeSize(String path) {
        long size = 0;
        try {
            StatFs stat = new StatFs(path);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                size = stat.getAvailableBlocksLong() * stat.getBlockSizeLong();
            } else {
                size = stat.getAvailableBlocks() * stat.getBlockSize();
            }
        } catch (Exception e) {}
        return size;
    }

    private void recordAppFileDirectory(String datapath, String path) {
        File file = new File(datapath, APP_FILE_DIRECTORY_STAMP);
        FileWriter writer;
        try {
            writer = new FileWriter(file);
            writer.write(path);
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private String locateAppFileDirectory() {
        String datapath = getFilesDir().getPath();
        String path = datapath;
        // Use files directory if external storage not available.
        if (Environment.isExternalStorageRemovable() && (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))) {
            recordAppFileDirectory(datapath, path);
            return path;
        }
        // Try to read application file directory from file.
        FileReader reader;
        try {
            reader = new FileReader(datapath + "/" + APP_FILE_DIRECTORY_STAMP);
            try {
                BufferedReader br = new BufferedReader(reader);
                path = br.readLine();
                br.close();
            } catch (IOException e) {
                path = null;
            }
        } catch (FileNotFoundException e) {
            path = null;
        }
        if (path != null) {
            File f = new File(path);
            if (f.exists() && f.isDirectory() && f.canWrite()) {
                Log.d(TAG, "locateAppFileDirectory, path from file: " + path);
                return path;
            }
        }
        // Here, path is null. Be careful!
        // Try external storage, use it if free space greater than 1G.
        File externalFile = getExternalFilesDir(null);
        if (externalFile != null && externalFile.canWrite())
        {
            path = externalFile.getPath();
            if (getVolumeSize(path) >= EXPECT_FREE_SPACE)
            {
                recordAppFileDirectory(datapath, path);
                return path;
            }
        }
        else
        {
            path = datapath;
        }
        // Here, path should never be null.
        // Iterate all external storage paths.
        long currMaxSize = getVolumeSize(path);
        StorageManager sm = (StorageManager)getSystemService(Context.STORAGE_SERVICE);
        Method method_getVolumePaths = null;
        Method method_getVolumeState = null;
        try {
            method_getVolumePaths = sm.getClass().getMethod("getVolumePaths");
            method_getVolumeState = sm.getClass().getMethod("getVolumeState", String.class);
        }
        catch(NoSuchMethodException ex) {
            recordAppFileDirectory(datapath, path);
            return path;
        }
        if (method_getVolumePaths != null && method_getVolumeState != null) {
            try {
                String []paths = (String[])method_getVolumePaths.invoke(sm);
                for(int i = 0; i < paths.length; ++i) {
                    Log.d(TAG, "locateAppFileDirectory, iterate volume: " + paths[i]);
                    String status = (String)method_getVolumeState.invoke(sm, paths[i]);
                    if (status.equals(Environment.MEDIA_MOUNTED)) {
                        long volumesize = getVolumeSize(paths[i]);
                        Log.d(TAG, "locateAppFileDirectory, volume size: " + String.valueOf(volumesize));
                        if (volumesize > currMaxSize) {
                            File dir = new File(paths[i]);
                            if (dir.canWrite()) {
                                File subdir = new File(dir, getPackageName());
                                subdir.mkdirs();
                                path = subdir.getPath();
                                currMaxSize = volumesize;
                                Log.d(TAG, "locateAppFileDirectory, create directory: " + path);
                            }
                        }
                    }
                }
            } catch (Exception e) {}
        }
        // Record it.
        recordAppFileDirectory(datapath, path);
        return path;
    }

    String getAppFileDirectory() {
        if (!appFileDirectoryGuard) {
            appFileDirectory = this.locateAppFileDirectory();
            appFileDirectoryGuard = true;
        }
        return appFileDirectory;
    }

    // Unzip batch.
    void unzipBatch(ArrayList<String> batchPaths)
    {
        // from HEX Studio
        try {
            String appFileDirectory = getAppFileDirectory();
            String packagePath = this.getPackageResourcePath();
            ZipFile zf = new ZipFile(packagePath);

            for (Enumeration<?> entries = zf.entries(); entries.hasMoreElements();) {
                ZipEntry zipEntry = ((ZipEntry) entries.nextElement());

                String entryName = zipEntry.getName();
                boolean bFound = false;
                for (String prefix : batchPaths) {
                    if (entryName.startsWith("assets/" + prefix)) {
                        bFound = true;
                        break;
                    }
                }
                if (!bFound) continue;

                String fpath = entryName.substring("assets".length());
                byte[] buffer = unzipbuffer;
                int count = 0;
                File file = null;

                if (zipEntry.isDirectory()) {
                    file = new File(appFileDirectory + '/' + fpath);
                    if (!file.exists()) {
                        file.mkdirs();
                    }
                } else {
                    InputStream in = zf.getInputStream(zipEntry);
                    file = new File(appFileDirectory + '/' + fpath);

                    if (!file.getParentFile().exists()) {
                        file.getParentFile().mkdirs();
                    }

                    if (!file.exists()) {
                        file.createNewFile();
                    }

                    FileOutputStream fileOutputStream = new FileOutputStream(file);
                    while ((count = in.read(buffer)) > 0) {
                        fileOutputStream.write(buffer, 0, count);
                    }
                    fileOutputStream.close();
                    in.close();
                }
            }
            zf.close();
        } catch (IOException e) {}
    }

    void unzipItem(String path) {
        AssetManager manager = this.getAssets();
        try {
            String[] files = manager.list(path);
            if (files.length == 0) {
                unzipFile(path);
            } else {
                for (String file : files) {
                    unzipItem(path + '/' + file);
                }
            }
        } catch (IOException e) {
            return;
        }
    }

    void unzipFile(String path) {
        AssetManager manager = this.getAssets();
        try {
            InputStream instream = manager.open(path);
            File file = new File(getAppFileDirectory() + '/' + path);
            if (file.exists()) {
                file.delete();
            }
            if (!file.getParentFile().exists()) {
                file.getParentFile().mkdirs();
            }
            OutputStream otstream = new FileOutputStream(file);
            byte[] buf = unzipbuffer;
            int len;
            while ((len = instream.read(buf)) > 0) {
                otstream.write(buf, 0, len);
            }
            instream.close();
            otstream.close();
        } catch (IOException e) {
            return;
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
