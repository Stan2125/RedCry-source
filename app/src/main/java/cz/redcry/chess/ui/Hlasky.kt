package cz.redcry.chess.ui

/** Jak dlouho visi kratka hlaska (do [HLASKA_KRATKA_ZNAKU] znaku vcetne). */
const val HLASKA_KRATKA_MS = 2_000L

/** Jak dlouho visi delsi hlaska. */
const val HLASKA_DLOUHA_MS = 4_000L

/** Hranice mezi kratkou a dlouhou hlaskou, pocitano v znacich textu. */
const val HLASKA_KRATKA_ZNAKU = 40

/**
 * Jak dlouho ma hlaska dole zustat videt, podle delky textu (U11).
 *
 * Rozhodnuti uzivatele: kratke hlasky ("Neplatný tah.", "Začátek
 * partie.") jsou typicky odpoved na klepnuti, prectou se hned a 4 s by jen
 * zbytecne zakryvaly tlacitka. Dlouhe ("… nastaveno …") rikaji, co aplikace
 * udelala misto toho, co uzivatel zadal, a musi se docist.
 *
 * Cista funkce bez Androidu, aby se dala testovat v JVM. Prepocet podle
 * nastaveni pristupnosti dela az GameScreen.
 */
fun trvaniHlasky(text: String): Long =
    if (text.length <= HLASKA_KRATKA_ZNAKU) HLASKA_KRATKA_MS else HLASKA_DLOUHA_MS
