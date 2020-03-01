package com.example.japanapp;

import android.content.res.AssetManager;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

import com.cpp.shared.GenericTranslator;
import com.cpp.shared.LogicHandler;
import com.cpp.shared.VocabularyDeck;
import com.cpp.shared.VocabularyTranslator;

public class LogicHandlerAbstraction {

    static public LogicHandler getInstance() { return mLogicHandler; }

    static public VocabularyTranslator getTranslator() {
        return mLogicHandler.getVocabularyTranslator();
    }

    static void init(AssetManager assetManager, String userDirectory) {
        File dataBaseDir = new File(userDirectory + "/database/");
        dataBaseDir.mkdir();

        File userFilePath = new File(userDirectory + "/userData/");
        userFilePath.mkdir();

        copyAssetFilesToDirectory(assetManager, dataBaseDir.toString());
        mLogicHandler = new LogicHandler(dataBaseDir.toString(), userFilePath.toString());
    }

    static void copyAssetFilesToDirectory(AssetManager assetManager, String outputDir) {
        try {
            String[] fnames = assetManager.list("database");
            for (String fname : fnames) {
                String newPath = outputDir + '/' + fname;
                if (new File(newPath).exists())
                    continue;

                copyAssetFile(assetManager, outputDir, fname);
            }
        } catch (Exception e){
            Log.e("LogicHandlerAbstraction", e.toString());
        }
    }

    static void copyAssetFile(AssetManager assetManager, String outputDir, String filename) {
        try {
            InputStream in = assetManager.open("database/" + filename);
            OutputStream out = new FileOutputStream(outputDir + '/' + filename);
            byte[] buffer = new byte[1024];
            int read = in.read(buffer);
            while (read != -1) {
                out.write(buffer, 0, read);
                read = in.read(buffer);
            }
        } catch (Exception e) {
            Log.e("LogHandlerAbstraction", e.toString());
        }
    }

    static { System.loadLibrary("shared"); }
    static LogicHandler mLogicHandler;
}
