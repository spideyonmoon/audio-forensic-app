package com.audioforensic.app.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val DarkColorScheme = darkColorScheme(
    primary = CyanAccent,
    secondary = GoldAccent,
    tertiary = PurpleAccent,
    background = BackgroundDark,
    surface = SurfaceDark,
    surfaceVariant = SurfaceCard,
    onPrimary = Color.Black,
    onSecondary = Color.Black,
    onBackground = Color(0xFFECEFF1),
    onSurface = Color(0xFFECEFF1),
    outline = BorderDark
)

@Composable
fun AudioForensicTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = DarkColorScheme,
        content = content
    )
}
