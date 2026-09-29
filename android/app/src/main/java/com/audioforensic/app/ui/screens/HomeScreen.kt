package com.audioforensic.app.ui.screens

import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.audioforensic.app.model.ForensicReport
import com.audioforensic.app.ui.theme.*

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(
    isAnalyzing: Boolean,
    currentStage: String,
    currentProgress: Float,
    fastMode: Boolean,
    onFastModeToggle: (Boolean) -> Unit,
    onPickSingleFile: () -> Unit,
    onPickFolder: () -> Unit,
    recentReports: List<ForensicReport>,
    onSelectReport: (ForensicReport) -> Unit
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text(
                            "Audio Forensic",
                            fontWeight = FontWeight.Bold,
                            fontSize = 20.sp,
                            color = Color.White
                        )
                        Text(
                            "Lossless Authenticity & Codec Inspection",
                            fontSize = 12.sp,
                            color = CyanAccent
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = BackgroundDark
                )
            )
        },
        containerColor = BackgroundDark
    ) { padding ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Live Analysis Card
            item {
                AnimatedVisibility(visible = isAnalyzing) {
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                        shape = RoundedCornerShape(16.dp)
                    ) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Row(
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.SpaceBetween,
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text(
                                    "Analyzing Audio...",
                                    fontWeight = FontWeight.Bold,
                                    color = Color.White
                                )
                                CircularProgressIndicator(
                                    progress = { currentProgress },
                                    modifier = Modifier.size(24.dp),
                                    color = CyanAccent
                                )
                            }
                            Spacer(modifier = Modifier.height(8.dp))
                            LinearProgressIndicator(
                                progress = { currentProgress },
                                modifier = Modifier.fillMaxWidth().height(6.dp).clip(RoundedCornerShape(3.dp)),
                                color = CyanAccent,
                                trackColor = BorderDark
                            )
                            Spacer(modifier = Modifier.height(8.dp))
                            Text(
                                "Stage: $currentStage",
                                fontSize = 12.sp,
                                color = Color.Gray
                            )
                        }
                    }
                }
            }

            // Action Buttons
            item {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Button(
                        onClick = onPickSingleFile,
                        modifier = Modifier.weight(1f).height(56.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = CyanAccent),
                        shape = RoundedCornerShape(12.dp),
                        enabled = !isAnalyzing
                    ) {
                        Icon(Icons.Default.AudioFile, contentDescription = null, tint = Color.Black)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text("Scan Track", color = Color.Black, fontWeight = FontWeight.Bold)
                    }

                    OutlinedButton(
                        onClick = onPickFolder,
                        modifier = Modifier.weight(1f).height(56.dp),
                        shape = RoundedCornerShape(12.dp),
                        colors = ButtonDefaults.outlinedButtonColors(contentColor = Color.White),
                        enabled = !isAnalyzing
                    ) {
                        Icon(Icons.Default.Folder, contentDescription = null, tint = CyanAccent)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text("Batch Album")
                    }
                }
            }

            // Fast Mode Setting Toggle
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(containerColor = SurfaceDark),
                    shape = RoundedCornerShape(12.dp)
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column {
                            Text("Fast Mode (60s probe)", fontWeight = FontWeight.SemiBold, color = Color.White)
                            Text("Quickly scans representative clip to save battery", fontSize = 12.sp, color = Color.Gray)
                        }
                        Switch(
                            checked = fastMode,
                            onCheckedChange = onFastModeToggle,
                            colors = SwitchDefaults.colors(checkedThumbColor = CyanAccent)
                        )
                    }
                }
            }

            // Recent Scans Header
            item {
                Text(
                    "Recent Inspections",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Bold,
                    color = Color.White,
                    modifier = Modifier.padding(top = 8.dp)
                )
            }

            if (recentReports.isEmpty()) {
                item {
                    Box(
                        modifier = Modifier.fillMaxWidth().padding(32.dp),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            "No files scanned yet.\nPick a FLAC, WAV, MP3, or DSD file to begin.",
                            color = Color.Gray,
                            fontSize = 14.sp,
                            lineHeight = 20.sp
                        )
                    }
                }
            } else {
                items(recentReports) { report ->
                    val score = report.mainScore
                    val scoreColor = getScoreColor(score)

                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clickable { onSelectReport(report) },
                        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                        shape = RoundedCornerShape(12.dp)
                    ) {
                        Row(
                            modifier = Modifier.padding(16.dp).fillMaxWidth(),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    report.fileName,
                                    fontWeight = FontWeight.SemiBold,
                                    color = Color.White,
                                    maxLines = 1
                                )
                                Spacer(modifier = Modifier.height(4.dp))
                                Text(
                                    "${report.technical.codec} • ${report.technical.sample_rate / 1000.0} kHz • ${report.technical.precision}-bit",
                                    fontSize = 12.sp,
                                    color = Color.Gray
                                )
                            }

                            // Score Badge
                            Box(
                                modifier = Modifier
                                    .clip(RoundedCornerShape(8.dp))
                                    .background(scoreColor.copy(alpha = 0.2f))
                                    .padding(horizontal = 12.dp, vertical = 6.dp),
                                contentAlignment = Alignment.Center
                            ) {
                                Text(
                                    text = "$score / 100",
                                    color = scoreColor,
                                    fontWeight = FontWeight.Bold,
                                    fontSize = 13.sp
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
