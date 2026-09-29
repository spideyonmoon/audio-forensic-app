package com.audioforensic.app.ui.screens

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.audioforensic.app.jni.NativeBridge
import com.audioforensic.app.model.ForensicReport
import com.audioforensic.app.ui.theme.*

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ReportDetailScreen(
    report: ForensicReport,
    onBack: () -> Unit
) {
    val scrollState = rememberScrollState()
    val auth = report.authenticity
    val score = auth?.main_score ?: 0
    val scoreColor = getScoreColor(score)

    var spectrogramBitmap by remember { mutableStateOf<Bitmap?>(null) }

    LaunchedEffect(report.filepath) {
        spectrogramBitmap = NativeBridge.generateSpectrogramBitmap(report.filepath)
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(report.fileName, maxLines = 1, color = Color.White) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back", tint = Color.White)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = BackgroundDark)
            )
        },
        containerColor = BackgroundDark
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .verticalScroll(scrollState)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Main Score & Verdict Hero Card
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                shape = RoundedCornerShape(16.dp)
            ) {
                Column(
                    modifier = Modifier.padding(20.dp).fillMaxWidth(),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    Box(
                        modifier = Modifier
                            .size(110.dp)
                            .clip(CircleShape)
                            .background(scoreColor.copy(alpha = 0.15f)),
                        contentAlignment = Alignment.Center
                    ) {
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(
                                text = "$score",
                                fontSize = 38.sp,
                                fontWeight = FontWeight.ExtraBold,
                                color = scoreColor
                            )
                            Text(
                                text = "SCORE / 100",
                                fontSize = 10.sp,
                                fontWeight = FontWeight.Bold,
                                color = Color.Gray
                            )
                        }
                    }

                    Spacer(modifier = Modifier.height(16.dp))

                    Box(
                        modifier = Modifier
                            .clip(RoundedCornerShape(8.dp))
                            .background(scoreColor.copy(alpha = 0.25f))
                            .padding(horizontal = 16.dp, vertical = 6.dp)
                    ) {
                        Text(
                            text = auth?.verdict_label ?: "UNKNOWN",
                            fontWeight = FontWeight.Bold,
                            color = scoreColor,
                            fontSize = 14.sp
                        )
                    }

                    Spacer(modifier = Modifier.height(12.dp))

                    Text(
                        text = auth?.primary_verdict ?: "Analysis Incomplete",
                        fontSize = 13.sp,
                        color = Color.LightGray,
                        lineHeight = 18.sp
                    )
                }
            }

            // Spectrogram Card
            spectrogramBitmap?.let { bitmap ->
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                    shape = RoundedCornerShape(16.dp)
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            "Fast Fourier Spectrogram",
                            fontWeight = FontWeight.Bold,
                            color = Color.White
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Image(
                            bitmap = bitmap.asImageBitmap(),
                            contentDescription = "Audio Spectrogram",
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(160.dp)
                                .clip(RoundedCornerShape(8.dp)),
                            contentScale = ContentScale.FillBounds
                        )
                        Spacer(modifier = Modifier.height(6.dp))
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Text("0 Hz", fontSize = 10.sp, color = Color.Gray)
                            Text("Cutoff: ${auth?.cutoff_hz_str ?: "N/A"}", fontSize = 10.sp, color = CyanAccent)
                            Text("${report.technical.sample_rate / 2000.0} kHz", fontSize = 10.sp, color = Color.Gray)
                        }
                    }
                }
            }

            // Evidence Findings Card
            if (auth != null && auth.evidence.isNotEmpty()) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                    shape = RoundedCornerShape(16.dp)
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.Warning, contentDescription = null, tint = VerdictLossy)
                            Spacer(modifier = Modifier.width(8.dp))
                            Text("Forensic Evidence", fontWeight = FontWeight.Bold, color = Color.White)
                        }
                        Spacer(modifier = Modifier.height(12.dp))
                        auth.evidence.forEach { ev ->
                            Row(modifier = Modifier.padding(vertical = 4.dp)) {
                                Text("✗", color = VerdictLossy, fontWeight = FontWeight.Bold)
                                Spacer(modifier = Modifier.width(8.dp))
                                Text(ev, fontSize = 12.sp, color = Color.LightGray)
                            }
                        }
                    }
                }
            }

            // Natural Preserved Credits Card
            if (auth != null && auth.natural_evidence.isNotEmpty()) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                    shape = RoundedCornerShape(16.dp)
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.CheckCircle, contentDescription = null, tint = VerdictGenuine)
                            Spacer(modifier = Modifier.width(8.dp))
                            Text("Acoustic & Mastering Credits", fontWeight = FontWeight.Bold, color = Color.White)
                        }
                        Spacer(modifier = Modifier.height(12.dp))
                        auth.natural_evidence.forEach { nev ->
                            Row(modifier = Modifier.padding(vertical = 4.dp)) {
                                Text("✓", color = VerdictGenuine, fontWeight = FontWeight.Bold)
                                Spacer(modifier = Modifier.width(8.dp))
                                Text(nev, fontSize = 12.sp, color = Color.LightGray)
                            }
                        }
                    }
                }
            }

            // Technical Details Card
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = SurfaceDark),
                shape = RoundedCornerShape(16.dp)
            ) {
                Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text("Technical Stream Details", fontWeight = FontWeight.Bold, color = Color.White)
                    Divider(color = BorderDark)
                    TechRow("Container Format", report.technical.codec)
                    TechRow("Sample Rate", "${report.technical.sample_rate} Hz")
                    TechRow("Bit Depth", "${report.technical.precision}-bit")
                    TechRow("Channels", "${report.technical.channels} channels")
                    TechRow("Analysis Time", "${String.format("%.2f", report.analysis_seconds)} s")
                }
            }
        }
    }
}

@Composable
private fun TechRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, fontSize = 13.sp, color = Color.Gray)
        Text(value, fontSize = 13.sp, color = Color.White, fontWeight = FontWeight.Medium)
    }
}
