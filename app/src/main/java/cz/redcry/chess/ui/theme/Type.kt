package cz.redcry.chess.ui.theme

import androidx.compose.material3.Typography
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

/**
 * Work Sans zatim NENI v projektu - Compose pouzije systemove bezpatkove pismo.
 *
 * Az stahnes z fonts.google.com soubory WorkSans-Regular.ttf a WorkSans-Medium.ttf,
 * uloz je do app/src/main/res/font/ jako work_sans_regular.ttf a work_sans_medium.ttf
 * a nahrad telo AppFontFamily timhle:
 *
 *   val AppFontFamily = FontFamily(
 *       Font(R.font.work_sans_regular, FontWeight.Normal),
 *       Font(R.font.work_sans_medium, FontWeight.Medium),
 *   )
 *
 * Jmena souboru v res/font musi byt mala pismena, cislice a podtrzitko.
 */
val AppFontFamily = FontFamily.SansSerif

/** Dve velikosti a dve vahy, jak jsme se dohodli. Nic navic se nepridava. */
val RedCryTypography = Typography(
    bodyLarge = TextStyle(
        fontFamily = AppFontFamily,
        fontWeight = FontWeight.Normal,
        fontSize = 14.sp,
        lineHeight = 21.sp,
    ),
    bodyMedium = TextStyle(
        fontFamily = AppFontFamily,
        fontWeight = FontWeight.Normal,
        fontSize = 13.sp,
        lineHeight = 19.sp,
    ),
    labelLarge = TextStyle(
        fontFamily = AppFontFamily,
        fontWeight = FontWeight.Medium,
        fontSize = 14.sp,
    ),
)
