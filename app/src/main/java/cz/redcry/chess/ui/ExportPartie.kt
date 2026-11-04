package cz.redcry.chess.ui

import cz.redcry.chess.kniha.KnihaZahajeni
import cz.redcry.chess.model.Position
import cz.redcry.chess.pgn.BEZ_VYSLEDKU
import cz.redcry.chess.pgn.vychoziZnacky

// Znacky a vysledek PGN ze zaznamu partie, pro ulozeni partie
// (GameViewModel.exportPgn) i pro PGN analyzy. Vzniklo v kroku R2/D
// presunem z GameViewModel.kt; ciste funkce nad [ZaznamPartie].

/** Znacky, ktere [GameViewModel.exportPgn] bere z nactene partie (krok U7/C). */
private val ZNACKY_Z_REFERENCE = listOf("Event", "Site", "Date", "Round", "White", "Black")

/** Znacky pro [GameViewModel.exportPgn] a PGN analyzy, viz tam. */
internal fun znackyPartie(zaznam: ZaznamPartie, kniha: KnihaZahajeni?): LinkedHashMap<String, String> {
    val tagy = vychoziZnacky()
    zaznam.tagyReference?.let { puvodni ->
        for (jmeno in ZNACKY_Z_REFERENCE) {
            puvodni[jmeno]?.takeIf { it.isNotBlank() }?.let { tagy[jmeno] = it }
        }
    }
    // Krok K1/B: zahajeni z knihovny podle CELE partie, jako vysledek.
    // ECO a Opening z nactene partie se neopisuji (U7/C) - pocitaji se.
    kniha?.nazevPartie(zaznam.history)?.let {
        tagy["ECO"] = it.eco
        tagy["Opening"] = it.nazev
    }
    // §9.7: partie, ktera nezacina vychozim postavenim, musi nest FEN
    // a SetUp - jinak ji nikdo (vcetne nas samych pri nacteni) nesestavi.
    if (zaznam.history[0] != Position.START_FEN) {
        tagy["SetUp"] = "1"
        tagy["FEN"] = zaznam.history[0]
    }
    return tagy
}

/**
 * Vysledek partie z konce zaznamu: mat znamena, ze prohrala strana NA
 * TAHU, pat je remiza, jinak je partie nedohrana.
 *
 * Cte se [ZaznamPartie.stavPoPoslednimTahu] a NE `engine.stav()`. Na `zaznam.index`
 * se tahle funkce neptá schvalne: exportuje se vzdycky cela partie, takze
 * i vysledek musi patrit jejímu konci - `history.last()` - a ne pozici,
 * na kterou se uzivatel zrovna prolistoval.
 */
internal fun vysledekPartie(zaznam: ZaznamPartie): String {
    val posledni = zaznam.history.size - 1
    return when (
        konecZeStavu(
            zaznam.stavPoPoslednimTahu,
            Position.fromFen(zaznam.history[posledni])?.sideToMove,
            pocetVyskytu(zaznam.klice, posledni),
        )
    ) {
        KonecPartie.MAT_VYHRAL_BILY -> "1-0"
        KonecPartie.MAT_VYHRAL_CERNY -> "0-1"
        KonecPartie.PAT, KonecPartie.REMIZA_50, KonecPartie.REMIZA_OPAKOVANIM -> "1/2-1/2"
        null -> BEZ_VYSLEDKU
    }
}
