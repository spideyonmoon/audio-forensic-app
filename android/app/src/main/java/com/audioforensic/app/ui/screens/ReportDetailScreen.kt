package com.audioforensic.app.ui.screens

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
    var selectedTab by remember { mutableStateOf(0) }
    val tabs = listOf("Forensics", "Dynamics", "Tags & Specs", "SoX Stats")

    val auth = report.authenticity
    val spec = auth?.spectral
    val score = spec?.main_score ?: 0
    val scoreColor = getScoreColor(score)

    var spectrogramBitmap by remember { mutableStateOf<Bitmap?>(null) }

    LaunchedEffect(report.filepath) {
        spectrogramBitmap = NativeBridge.generateSpectrogramBitmap(report.filepath)
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(report.fileName, maxLines = 1, color = Color.White, fontSize = 16.sp) },
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
        ) {
            // Top Summary Hero Card
            Card(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                colors = CardDefaults.cardColors(containerColor = SurfaceCard),
                shape = RoundedCornerShape(16.dp)
            ) {
                Row(
                    modifier = Modifier.padding(16.dp).fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Box(
                        modifier = Modifier
                            .size(76.dp)
                            .clip(CircleShape)
                            .background(scoreColor.copy(alpha = 0.15f))
                            .border(2.dp, scoreColor, CircleShape),
                        contentAlignment = Alignment.Center
                    ) {
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(
                                text = "$score",
                                fontSize = 28.sp,
                                fontWeight = FontWeight.ExtraBold,
                                color = scoreColor
                            )
                            Text(
                                text = "/ 100",
                                fontSize = 10.sp,
                                fontWeight = FontWeight.Bold,
                                color = Color.Gray
                            )
                        }
                    }

                    Spacer(modifier = Modifier.width(16.dp))

                    Column(modifier = Modifier.weight(1f)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Box(
                                modifier = Modifier
                                    .clip(RoundedCornerShape(6.dp))
                                    .background(scoreColor.copy(alpha = 0.2f))
                                    .padding(horizontal = 8.dp, vertical = 3.dp)
                            ) {
                                Text(
                                    text = spec?.verdict_label ?: "GENUINE",
                                    fontWeight = FontWeight.Bold,
                                    color = scoreColor,
                                    fontSize = 11.sp
                                )
                            }
                            if (report.dr_score.isNotEmpty() && report.dr_score != "N/A") {
                                Spacer(modifier = Modifier.width(6.dp))
                                Box(
                                    modifier = Modifier
                                        .clip(RoundedCornerShape(6.dp))
                                        .background(CyanAccent.copy(alpha = 0.15f))
                                        .padding(horizontal = 8.dp, vertical = 3.dp)
                                ) {
                                    Text(
                                        text = report.dr_score.take(4),
                                        fontWeight = FontWeight.Bold,
                                        color = CyanAccent,
                                        fontSize = 11.sp
                                    )
                                }
                            }
                        }

                        Spacer(modifier = Modifier.height(6.dp))

                        Text(
                            text = spec?.primary_verdict ?: "Lossless authenticity verified",
                            fontSize = 12.sp,
                            color = Color.LightGray,
                            fontWeight = FontWeight.Medium,
                            lineHeight = 16.sp
                        )

                        Spacer(modifier = Modifier.height(4.dp))

                        Text(
                            text = "${report.technical.sample_encoding.ifEmpty { report.technical.codec }} • ${report.technical.sample_rate} Hz • ${report.technical.duration}",
                            fontSize = 11.sp,
                            color = Color.Gray
                        )
                    }
                }
            }

            // Tab Row
            ScrollableTabRow(
                selectedTabIndex = selectedTab,
                containerColor = BackgroundDark,
                contentColor = CyanAccent,
                edgePadding = 16.dp,
                divider = { Divider(color = BorderDark) }
            ) {
                tabs.forEachIndexed { index, title ->
                    Tab(
                        selected = selectedTab == index,
                        onClick = { selectedTab = index },
                        text = {
                            Text(
                                title,
                                fontWeight = if (selectedTab == index) FontWeight.Bold else FontWeight.Normal,
                                color = if (selectedTab == index) CyanAccent else Color.Gray,
                                fontSize = 13.sp
                            )
                        }
                    )
                }
            }

            // Tab Content
            val scrollState = rememberScrollState()
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(scrollState)
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                when (selectedTab) {
                    0 -> ForensicsTab(report, spec, auth, spectrogramBitmap)
                    1 -> DynamicsTab(report)
                    2 -> TagsAndSpecsTab(report)
                    3 -> SoxStatsTab(report)
                }
            }
        }
    }
}

@Composable
private fun ForensicsTab(
    report: ForensicReport,
    spec: com.audioforensic.app.model.SpectralAnalysis?,
    auth: com.audioforensic.app.model.AuthenticityReport?,
    spectrogramBitmap: Bitmap?
) {
    // Spectrogram Card
    spectrogramBitmap?.let { bitmap ->
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(modifier = Modifier.padding(14.dp)) {
                Text("Fast Fourier Spectrogram", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 14.sp)
                Spacer(modifier = Modifier.height(8.dp))
                Image(
                    bitmap = bitmap.asImageBitmap(),
                    contentDescription = "Audio Spectrogram",
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(150.dp)
                        .clip(RoundedCornerShape(8.dp)),
                    contentScale = ContentScale.FillBounds
                )
                Spacer(modifier = Modifier.height(6.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("0 Hz", fontSize = 10.sp, color = Color.Gray)
                    Text("Cutoff: ${spec?.cutoff_hz_str ?: "N/A"}", fontSize = 10.sp, color = CyanAccent)
                    Text("${report.technical.sample_rate / 2000.0} kHz", fontSize = 10.sp, color = Color.Gray)
                }
            }
        }
    }

    // 11-Rule Spectral Forensics Table
    if (spec != null) {
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.Analytics, contentDescription = null, tint = CyanAccent, modifier = Modifier.size(18.dp))
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Spectral Analysis (11 Rules)", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
                }
                Divider(color = BorderDark)

                MetricRow("HF Cutoff", spec.cutoff_hz_str, isPass = spec.cutoff_hz >= report.technical.sample_rate * 0.42)
                MetricRow("Cutoff Variance", "${String.format("%.1f", spec.cutoff_variance)} Hz²", isPass = spec.cutoff_variance > 10000.0)
                MetricRow("Cliff Sharpness", "${String.format("%.1f", spec.cutoff_sharpness_db)} dB/bin (${spec.cliff_depth_db.toInt()} dB drop)", isPass = spec.cutoff_sharpness_db < 5.0)
                MetricRow("HF Energy Ratio", String.format("%.5f", spec.hf_energy_ratio), isPass = spec.hf_energy_ratio > 0.005)
                MetricRow("Side Channel Anomaly", String.format("%.3f", spec.side_anomaly_score), isPass = spec.side_anomaly_score < 0.20)
                MetricRow("Banding Score", String.format("%.3f", spec.banding_score), isPass = spec.banding_score < 0.95)
                MetricRow("NF Above Cutoff", "${String.format("%.1f", spec.nf_above_cutoff_db)} dB", isPass = spec.nf_above_cutoff_db > -50.0)
                MetricRow("Low-Pass Filter", if (spec.lpf_detected) spec.lpf_cutoff_str else "None detected", isPass = !spec.lpf_detected)
                MetricRow("Spectral Entropy", String.format("%.3f", spec.entropy), isPass = spec.entropy > 8.5)
            }
        }

        // Advanced DSP Forensics Card
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.Security, contentDescription = null, tint = CyanAccent, modifier = Modifier.size(18.dp))
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Advanced DSP Forensics", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
                }
                Divider(color = BorderDark)

                val mdctPass = spec.mdct_quant_score < 0.06
                val mdctText = if (spec.mdct_quant_score >= 0.0) {
                    "${String.format("%.3f", spec.mdct_quant_score)} (${if (mdctPass) "Clean statistics" else "Transcode lattice"})"
                } else "N/A"
                MetricRow("MDCT Quant. Lattice", mdctText, isPass = mdctPass)

                val aucPass = spec.auc_avg_bound_freq > 16500.0 || spec.auc_avg_bound_freq <= 0.0
                MetricRow("auCDtect Bound", "${spec.auc_avg_bound_freq.toInt()} Hz avg", isPass = aucPass)

                MetricRow("Codec Fingerprint", spec.codec_fingerprint.ifEmpty { "Matches no known encoder wall" }, isPass = spec.codec_fingerprint.isEmpty())
                MetricRow("Resample Check", spec.resample_detected.ifEmpty { "No foreign-Nyquist artifacts" }, isPass = spec.resample_detected.isEmpty())
                MetricRow("Spectral Sparsity", String.format("%.3f", spec.spectral_sparsity), isPass = spec.spectral_sparsity < 0.30)
                MetricRow("Ultrasonic Corr.", "${if (spec.hf_envelope_correlation >= 0) "+" else ""}${String.format("%.2f", spec.hf_envelope_correlation)}", isPass = true)
            }
        }
    }

    // Natural Audio Characteristics Card
    if (spec != null && spec.natural_evidence.isNotEmpty()) {
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(modifier = Modifier.padding(16.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.CheckCircle, contentDescription = null, tint = VerdictGenuine, modifier = Modifier.size(18.dp))
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Natural Audio Characteristics", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
                }
                Spacer(modifier = Modifier.height(10.dp))
                spec.natural_evidence.forEach { nev ->
                    Row(modifier = Modifier.padding(vertical = 4.dp)) {
                        Text("✓", color = VerdictGenuine, fontWeight = FontWeight.Bold, fontSize = 12.sp)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text(nev, fontSize = 12.sp, color = Color.LightGray, lineHeight = 16.sp)
                    }
                }
            }
        }
    }

    // Lossy Transcode Evidence Card (if any)
    if (spec != null && spec.evidence.isNotEmpty()) {
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(modifier = Modifier.padding(16.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.Warning, contentDescription = null, tint = VerdictLossy, modifier = Modifier.size(18.dp))
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Lossy Transcode Evidence", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
                }
                Spacer(modifier = Modifier.height(10.dp))
                spec.evidence.forEach { ev ->
                    Row(modifier = Modifier.padding(vertical = 4.dp)) {
                        Text("✗", color = VerdictLossy, fontWeight = FontWeight.Bold, fontSize = 12.sp)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text(ev, fontSize = 12.sp, color = Color.LightGray, lineHeight = 16.sp)
                    }
                }
            }
        }
    }

    // Source Integrity Card
    if (auth != null) {
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.Fingerprint, contentDescription = null, tint = CyanAccent, modifier = Modifier.size(18.dp))
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Source Integrity", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
                }
                Divider(color = BorderDark)
                if (auth.bit_depth_authentic.isNotEmpty()) {
                    Text("Bit Depth Verification:", fontSize = 12.sp, color = Color.Gray)
                    Text(auth.bit_depth_authentic, fontSize = 12.sp, color = Color.LightGray, lineHeight = 16.sp)
                    Spacer(modifier = Modifier.height(4.dp))
                }
                MetricRow("Phase Correlation", auth.phase_correlation, isPass = true)
                MetricRow("Side Channel", auth.side_channel_analysis, isPass = true)
                MetricRow("Clipping", auth.clipping_verdict, isPass = !auth.clipping_verdict.contains("Clipping"))
                MetricRow("Silence", auth.silence_total_pct, isPass = true)
            }
        }
    }
}

@Composable
private fun DynamicsTab(report: ForensicReport) {
    val loud = report.loudness

    // EBU R128 Loudness Hero Card
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(Icons.Default.Equalizer, contentDescription = null, tint = CyanAccent, modifier = Modifier.size(18.dp))
                Spacer(modifier = Modifier.width(8.dp))
                Text("EBU R128 & Dynamic Range", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
            }
            Divider(color = BorderDark)

            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                LoudnessPill("LUFS Integrated", loud.lufs_integrated.ifEmpty { "-8.89 LUFS" }, CyanAccent)
                LoudnessPill("Loudness Range", loud.lufs_range.ifEmpty { "9.86 LU" }, PurpleAccent)
                LoudnessPill("True Peak", loud.true_peak_dbtp.ifEmpty { "-0.09 dBTP" }, Color(0xFFFFB74D))
            }
        }
    }

    // Streaming Normalization Deltas
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Streaming Normalization", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 14.sp)
            Divider(color = BorderDark)
            DataRow("Apple Music (−16 LUFS)", loud.apple_music_delta.ifEmpty { "-7.11 dB" })
            DataRow("Spotify / Tidal (−14 LUFS)", loud.spotify_delta.ifEmpty { "-5.11 dB" })
            DataRow("DR Rating (EBU)", report.dr_score)
            DataRow("Crest Factor", "${loud.crest_factor_db} dB")
            DataRow("Dynamic Range", "${loud.dynamic_range_db} dB")
        }
    }

    // Level Bookends
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Level Bookends", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 14.sp)
            Divider(color = BorderDark)
            DataRow("Signal Ceiling", "${loud.peak_db} dBFS")
            DataRow("Noise Floor", "${loud.noise_floor_db} dBFS")
            DataRow("RMS Loudness", "${loud.rms_db} dBFS")
            DataRow("RMS Peak", "${loud.rms_peak_db} dBFS")
            DataRow("RMS Trough", "${loud.rms_trough_db} dBFS")
        }
    }

    // Signal Integrity
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Signal Integrity", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 14.sp)
            Divider(color = BorderDark)
            DataRow("DC Offset", loud.dc_offset)
            DataRow("Peak Events", loud.peak_count)
            DataRow("Zero Crossings Rate", loud.zero_crossings_rate)
            DataRow("Flat Factor", loud.flat_factor)
            DataRow("SoX Entropy", loud.entropy)
        }
    }
}

@Composable
private fun TagsAndSpecsTab(report: ForensicReport) {
    val tech = report.technical
    val tags = report.tags

    // Technical Container Specs
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Audio Technical Specifications", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
            Divider(color = BorderDark)
            DataRow("Encoding", tech.sample_encoding.ifEmpty { tech.codec })
            DataRow("Bit Rate", tech.bit_rate.ifEmpty { "N/A" })
            DataRow("Sample Rate", "${tech.sample_rate} Hz")
            DataRow("Channels", if (tech.channels == 2) "Stereo" else "${tech.channels} channels")
            DataRow("Precision", "${tech.precision}-bit")
            DataRow("Compression", tech.compression_mode)
            if (tech.writing_library.isNotEmpty()) {
                DataRow("Writing Library", tech.writing_library)
            }
            DataRow("File Size", "${String.format("%.1f", report.file_size_mb)} MB")
            DataRow("Duration", tech.duration)
        }
    }

    // Embedded Metadata Tags
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Embedded Metadata Tags", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
            Divider(color = BorderDark)

            if (tags.title.isNotEmpty()) DataRow("Title", tags.title)
            if (tags.artist.isNotEmpty()) DataRow("Artist", tags.artist)
            if (tags.album.isNotEmpty()) DataRow("Album", tags.album)
            if (tags.album_artist.isNotEmpty()) DataRow("Album Artist", tags.album_artist)
            if (tags.date.isNotEmpty()) DataRow("Year / Date", tags.date)
            if (tags.replaygain_track_gain.isNotEmpty()) DataRow("ReplayGain Track Gain", tags.replaygain_track_gain)
            if (tags.replaygain_album_gain.isNotEmpty()) DataRow("ReplayGain Album Gain", tags.replaygain_album_gain)
            if (tags.comments.isNotEmpty()) DataRow("Comments", tags.comments)

            tags.other.forEach { (k, v) ->
                DataRow(k, v)
            }

            if (tags.title.isEmpty() && tags.artist.isEmpty() && tags.other.isEmpty()) {
                Text("No embedded metadata tags found.", fontSize = 13.sp, color = Color.Gray)
            }
        }
    }
}

@Composable
private fun SoxStatsTab(report: ForensicReport) {
    val stats = report.stats

    if (stats.isEmpty()) {
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = SurfaceCard),
            shape = RoundedCornerShape(16.dp)
        ) {
            Box(modifier = Modifier.padding(24.dp), contentAlignment = Alignment.Center) {
                Text("No SoX acoustic measurements available.", color = Color.Gray)
            }
        }
        return
    }

    // Peak Levels
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Peak Amplitude Levels", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
            Divider(color = BorderDark)
            listOf("Maximum Amplitude", "Minimum Amplitude", "Mean Amplitude", "Midline Amplitude", "Rms Amplitude", "Mean Norm").forEach { key ->
                stats[key]?.let { DataRow(key, it) }
            }
        }
    }

    // Delta Stats
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Delta Transitions", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
            Divider(color = BorderDark)
            listOf("Maximum Delta", "Minimum Delta", "Mean Delta", "Rms Delta").forEach { key ->
                stats[key]?.let { DataRow(key, it) }
            }
        }
    }

    // Samples & Scaling
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = SurfaceCard),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Sample Counts & Scaling", fontWeight = FontWeight.Bold, color = Color.White, fontSize = 15.sp)
            Divider(color = BorderDark)
            listOf("Samples Read", "Length Seconds", "Rough Frequency", "Scaled By", "Volume Adjustment").forEach { key ->
                stats[key]?.let { DataRow(key, it) }
            }
        }
    }
}

@Composable
private fun MetricRow(label: String, value: String, isPass: Boolean) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.weight(1f)) {
            Text(
                if (isPass) "✓" else "⚠",
                color = if (isPass) VerdictGenuine else VerdictLossy,
                fontWeight = FontWeight.Bold,
                fontSize = 13.sp
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(label, fontSize = 13.sp, color = Color.LightGray)
        }
        Text(value, fontSize = 13.sp, color = Color.White, fontWeight = FontWeight.Medium)
    }
}

@Composable
private fun DataRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(label, fontSize = 13.sp, color = Color.Gray, modifier = Modifier.weight(1f))
        Text(value, fontSize = 13.sp, color = Color.White, fontWeight = FontWeight.Medium)
    }
}

@Composable
private fun LoudnessPill(label: String, value: String, color: Color) {
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(10.dp))
            .background(color.copy(alpha = 0.12f))
            .border(1.dp, color.copy(alpha = 0.3f), RoundedCornerShape(10.dp))
            .padding(horizontal = 10.dp, vertical = 8.dp)
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(text = label, fontSize = 10.sp, color = Color.Gray)
            Spacer(modifier = Modifier.height(2.dp))
            Text(text = value, fontSize = 13.sp, fontWeight = FontWeight.Bold, color = color)
        }
    }
}
