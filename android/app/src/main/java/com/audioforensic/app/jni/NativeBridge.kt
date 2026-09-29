package com.audioforensic.app.jni

import android.graphics.Bitmap
import android.graphics.Color
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import com.audioforensic.app.model.ForensicReport

interface ProgressCallback {
    fun onProgress(stage: String, progress: Float)
}

object NativeBridge {
    private val json = Json {
        ignoreUnknownKeys = true
        isLenient = true
    }

    private external fun analyzeFileNative(
        filePath: String,
        maxSeconds: Double,
        callback: ProgressCallback?
    ): String

    private external fun generateSpectrogramNative(
        filePath: String,
        targetWidth: Int,
        targetHeight: Int
    ): IntArray?

    suspend fun analyzeFile(
        filePath: String,
        fastMode: Boolean = false,
        onProgress: (stage: String, progress: Float) -> Unit = { _, _ -> }
    ): ForensicReport = withContext(Dispatchers.Default) {
        val maxSec = if (fastMode) 60.0 else 0.0
        val cb = object : ProgressCallback {
            override fun onProgress(stage: String, progress: Float) {
                onProgress(stage, progress)
            }
        }
        val jsonStr = analyzeFileNative(filePath, maxSec, cb)
        json.decodeFromString<ForensicReport>(jsonStr)
    }

    suspend fun generateSpectrogramBitmap(
        filePath: String,
        width: Int = 512,
        height: Int = 256
    ): Bitmap? = withContext(Dispatchers.Default) {
        val pixels = generateSpectrogramNative(filePath, width, height) ?: return@withContext null
        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        bitmap.setPixels(pixels, 0, width, 0, 0, width, height)
        bitmap
    }
}
