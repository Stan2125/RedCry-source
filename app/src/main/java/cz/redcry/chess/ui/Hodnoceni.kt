package cz.redcry.chess.ui

// Hodnoceni pozice na text: okno (i) a stavovy radek pod deskou.
// Vzniklo v kroku R1 presunem z GameViewModel.kt.

import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz

/**
 * Matove skore v setinach pesce. Oba motory sem dojdou jinudy, ale na tuhle
 * hodnotu: RedMAX hlasi `score mate N` a [cz.redcry.chess.engine.RedMaxEngine]
 * si to prepocita na `SKORE_MATU - N`, Light Red vraci rovnou +-30000
 * z `na_centipawny`.
 */
internal const val SKORE_MATU = 30_000

/**
 * Nad touhle mezi uz jde o mat, ne o materialni prevahu. Tisic setin pesce
 * pod matem je s rezervou nad cimkoli, co muze vyjit z hodnoceni pozice.
 */
internal const val MEZ_MATU = 29_000

/**
 * Hodnoceni pozice na text kratce: pro stavovy radek pod deskou (krok U12/B)
 * a od kroku U14/A i pro okno (i). Bez pripony "(bílý)" a mat jako
 * "+M3" / "-M3" (znamenko podle toho, kdo matuje, stejne jako u cisel).
 *
 * Vstup je v setinach pesce a UZ Z POHLEDU BILEHO - prepocet ze strany na
 * tahu dela [GameViewModel] pri prijmu, tady se jen formatuje.
 *
 * Mat se pozna podle meze [MEZ_MATU]. Vzdalenost `N = SKORE_MATU - |cp|` se
 * pise jen tehdy, kdyz dava smysl (1..999); mimo to zbyde jen "+M" / "-M".
 * Light Red vzdalenost nenese a hlasi rovnou +-30000, takze z nej vyjde
 * "+M" bez cisla - to je spravne, ne chyba.
 *
 * Do kroku U14/A melo okno (i) vlastni dlouhy tvar formatujSkore
 * ("+0,42 (bílý)", "mat v 3 pro bílého"); oba ukazuji totez cislo, tak
 * zustal jeden.
 *
 * Desetinny oddelovac od kroku J3 z [texty] ("+0,35" / "+0.35"). Vychozi
 * [TextyCz] kvuli testum a CSV, ktere si jazyk predavaji samy.
 */
fun formatujSkoreKratce(cp: Int?, texty: Texty = TextyCz): String {
    if (cp == null) return "—"

    if (cp >= MEZ_MATU || cp <= -MEZ_MATU) {
        val znamenko = if (cp > 0) "+" else "-"
        val n = SKORE_MATU - kotlin.math.abs(cp)
        return if (n in 1..999) znamenko + "M" + n else znamenko + "M"
    }

    return formatujPesce(cp, texty.desetinnyOddelovac)
}

/** Setiny pesce na "+0,35" / "-1,20" / "0,00" pro [formatujSkoreKratce]. */
private fun formatujPesce(cp: Int, oddelovac: Char): String {
    val znamenko = if (cp > 0) "+" else if (cp < 0) "-" else ""
    val abs = kotlin.math.abs(cp)
    // Rucne, ne String.format: ten bere oddelovac z Locale zarizeni, ne
    // z jazyka aplikace (od J3 [Texty.desetinnyOddelovac]).
    val setiny = abs % 100
    return znamenko + (abs / 100) + oddelovac + (if (setiny < 10) "0" else "") + setiny
}
