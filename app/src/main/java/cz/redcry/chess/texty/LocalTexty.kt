package cz.redcry.chess.texty

import androidx.compose.runtime.staticCompositionLocalOf

/**
 * Texty pro composables (krok J1). Poskytuje je GameScreen; v J1 vzdycky
 * [TextyCz], od J2 podle prepinace jazyka. Static: jazyk se meni zridka
 * a pri zmene se ma prekreslit cela obrazovka.
 */
val LocalTexty = staticCompositionLocalOf<Texty> { TextyCz }
