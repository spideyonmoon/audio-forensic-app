package com.audioforensic.app.ui

import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.*
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.lifecycleScope
import com.audioforensic.app.jni.NativeBridge
import com.audioforensic.app.model.ForensicReport
import com.audioforensic.app.ui.screens.HomeScreen
import com.audioforensic.app.ui.screens.ReportDetailScreen
import com.audioforensic.app.ui.theme.AudioForensicTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            AudioForensicTheme {
                var isAnalyzing by remember { mutableStateOf(false) }
                var currentStage by remember { mutableStateOf("") }
                var currentProgress by remember { mutableStateOf(0f) }
                var fastMode by remember { mutableStateOf(false) }

                val recentReports = remember { mutableStateListOf<ForensicReport>() }
                var selectedReport by remember { mutableStateOf<ForensicReport?>(null) }

                // Single File Picker
                val singleFileLauncher = rememberLauncherForActivityResult(
                    contract = ActivityResultContracts.OpenDocument()
                ) { uri: Uri? ->
                    uri?.let {
                        lifecycleScope.launch {
                            val localPath = copyUriToCache(it) ?: return@launch
                            runAnalysis(localPath, fastMode,
                                onStart = { isAnalyzing = true },
                                onProgress = { st, pr ->
                                    currentStage = st
                                    currentProgress = pr
                                },
                                onComplete = { report ->
                                    isAnalyzing = false
                                    recentReports.add(0, report)
                                    selectedReport = report
                                },
                                onError = { err ->
                                    isAnalyzing = false
                                    Toast.makeText(this@MainActivity, "Analysis error: $err", Toast.LENGTH_LONG).show()
                                }
                            )
                        }
                    }
                }

                // Directory Picker (Album Batch)
                val folderLauncher = rememberLauncherForActivityResult(
                    contract = ActivityResultContracts.OpenDocumentTree()
                ) { treeUri: Uri? ->
                    treeUri?.let {
                        lifecycleScope.launch {
                            val docTree = DocumentFile.fromTreeUri(this@MainActivity, it)
                            val files = docTree?.listFiles()?.filter { f ->
                                val name = f.name?.lowercase() ?: ""
                                name.endsWith(".flac") || name.endsWith(".wav") ||
                                name.endsWith(".mp3") || name.endsWith(".dsf") || name.endsWith(".dff")
                            } ?: emptyList()

                            if (files.isEmpty()) {
                                Toast.makeText(this@MainActivity, "No supported audio files in folder", Toast.LENGTH_SHORT).show()
                                return@launch
                            }

                            isAnalyzing = true
                            for ((idx, fileDoc) in files.withIndex()) {
                                currentStage = "Batch [${idx + 1}/${files.size}]: ${fileDoc.name}"
                                currentProgress = (idx.toFloat() / files.size)
                                val localPath = copyUriToCache(fileDoc.uri) ?: continue

                                try {
                                    val report = NativeBridge.analyzeFile(localPath, fastMode)
                                    recentReports.add(0, report)
                                } catch (e: Exception) {
                                    // continue batch
                                }
                            }
                            isAnalyzing = false
                            currentProgress = 1f
                            currentStage = "Batch scan completed"
                        }
                    }
                }

                if (selectedReport != null) {
                    ReportDetailScreen(
                        report = selectedReport!!,
                        onBack = { selectedReport = null }
                    )
                } else {
                    HomeScreen(
                        isAnalyzing = isAnalyzing,
                        currentStage = currentStage,
                        currentProgress = currentProgress,
                        fastMode = fastMode,
                        onFastModeToggle = { fastMode = it },
                        onPickSingleFile = {
                            singleFileLauncher.launch(arrayOf(
                                "audio/*",
                                "application/ogg",
                                "*/*"
                            ))
                        },
                        onPickFolder = {
                            folderLauncher.launch(null)
                        },
                        recentReports = recentReports,
                        onSelectReport = { selectedReport = it }
                    )
                }
            }
        }
    }

    private suspend fun copyUriToCache(uri: Uri): String? = withContext(Dispatchers.IO) {
        try {
            var fileName = "audio_track"
            contentResolver.query(uri, null, null, null, null)?.use { cursor ->
                if (cursor.moveToFirst()) {
                    val nameIdx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    if (nameIdx >= 0) fileName = cursor.getString(nameIdx)
                }
            }

            val cacheFile = File(cacheDir, fileName)
            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(cacheFile).use { output ->
                    input.copyTo(output)
                }
            }
            cacheFile.absolutePath
        } catch (e: Exception) {
            null
        }
    }

    private fun runAnalysis(
        path: String,
        fast: Boolean,
        onStart: () -> Unit,
        onProgress: (String, Float) -> Unit,
        onComplete: (ForensicReport) -> Unit,
        onError: (String) -> Unit
    ) {
        onStart()
        lifecycleScope.launch {
            try {
                val report = NativeBridge.analyzeFile(path, fast) { st, pr ->
                    lifecycleScope.launch(Dispatchers.Main) {
                        onProgress(st, pr)
                    }
                }
                onComplete(report)
            } catch (e: Exception) {
                onError(e.message ?: "Analysis failed")
            }
        }
    }
}
