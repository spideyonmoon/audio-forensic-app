package com.audioforensic.app

import android.app.Application
import android.util.Log

class AudioForensicApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        try {
            System.loadLibrary("audioforensic")
            Log.i("AudioForensic", "Native libaudioforensic.so loaded successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("AudioForensic", "Failed to load native library", e)
        }
    }
}
