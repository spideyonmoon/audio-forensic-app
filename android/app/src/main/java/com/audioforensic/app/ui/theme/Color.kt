package com.audioforensic.app.ui.theme

import androidx.compose.ui.graphics.Color

val BackgroundDark = Color(0xFF101114)
val SurfaceDark = Color(0xFF1A1C23)
val SurfaceCard = Color(0xFF232630)
val BorderDark = Color(0xFF2E323E)

val CyanAccent = Color(0xFF4DD0E1)
val GoldAccent = Color(0xFFFFD54F)
val PurpleAccent = Color(0xFFB388FF)

// Forensic Verdict Colors
val VerdictGenuine = Color(0xFF66BB6A)       // 0–10 Green
val VerdictLikelyGenuine = Color(0xFF9CCC65) // 11–30 Light Green
val VerdictCaution = Color(0xFFFFCA28)       // 31–54 Amber
val VerdictSuspicious = Color(0xFFFFA726)    // 55–85 Orange
val VerdictLossy = Color(0xFFEF5350)         // 86–100 Ruby Red

fun getScoreColor(score: Int): Color = when {
    score <= 10 -> VerdictGenuine
    score <= 30 -> VerdictLikelyGenuine
    score <= 54 -> VerdictCaution
    score <= 85 -> VerdictSuspicious
    else -> VerdictLossy
}
