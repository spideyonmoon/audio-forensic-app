package com.audioforensic.app.worker

import android.content.Context
import androidx.work.CoroutineWorker
import androidx.work.WorkerParameters
import androidx.work.workDataOf
import com.audioforensic.app.jni.NativeBridge
import java.io.File

class ForensicScanWorker(
    appContext: Context,
    workerParams: WorkerParameters
) : CoroutineWorker(appContext, workerParams) {

    override suspend fun doWork(): Result {
        val path = inputData.getString("FILE_PATH") ?: return Result.failure()
        val fastMode = inputData.getBoolean("FAST_MODE", false)

        return try {
            setProgress(workDataOf("PROGRESS" to 0.1f, "STAGE" to "Initializing"))
            val report = NativeBridge.analyzeFile(path, fastMode) { stage, p ->
                // Report progress
            }
            Result.success(workDataOf(
                "SCORE" to (report.authenticity?.main_score ?: 0),
                "VERDICT" to (report.authenticity?.verdict_label ?: "UNKNOWN")
            ))
        } catch (e: Exception) {
            Result.failure(workDataOf("ERROR" to (e.message ?: "Analysis failed")))
        }
    }
}
