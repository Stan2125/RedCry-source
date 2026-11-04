package cz.redcry.chess.analyza

import cz.redcry.chess.pgn.navrhJmenaSouboru
import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz
import java.time.LocalDateTime

/**
 * Analyza partie (krok A1): ciste funkce bez Androidu a bez motoru.
 *
 * Indexy jsou tytez jako v GameViewModelu: pozice `p` je `history[p]`,
 * pultah `i` vede z `history[i]` do `history[i + 1]`. Analyzuji se pultahy
 * [RozsahAnalyzy.od]..[RozsahAnalyzy.doPultahu], hleda se v pozicich
 * `od .. doPultahu + 1` - kazdy tah se hodnoti pozici PRED nim (nejlepsi
 * tah, varianta) i pozici PO nem (hodnota, kterou tah nechal).
 *
 * Sachova pravidla tu nejsou: SAN, legalni tahy i mat/pat dodava motor
 * pres ViewModel. Tady se jen cisluje, pocita a sklada text.
 */

/** Koncova pozice, ve ktere se nehleda (zjisti ji `engine.stav()`). */
enum class KonecPozice { MAT, PAT }

/**
 * Vysledek hledani v jedne pozici.
 *
 * @param skoreBily  setiny pesce Z POHLEDU BILEHO (jako GameUiState.skoreCp),
 *                   mat jako +-(SKORE_MATU - N)
 * @param tahUci     nejlepsi tah v UCI; porovnava se se zahranym tahem
 * @param varianta   hlavni varianta v SAN, prvni prvek je nejlepsi tah.
 *                   Muze byt kratsi nez to, co motor poslal: prevod konci
 *                   na prvnim tahu, ktery motor neprijal.
 * @param konec      koncova pozice - pak se nehledalo a ostatni pole jsou null
 */
data class VysledekPozice(
    val hloubka: Int? = null,
    val skoreBily: Int? = null,
    val tahUci: String? = null,
    val varianta: List<String> = emptyList(),
    val konec: KonecPozice? = null,
) {
    /** Nejlepsi tah v SAN, null kdyz se ho prevest nepodarilo. */
    val tahSan: String? get() = varianta.firstOrNull()
}

/**
 * Cislovani partie: stejne vstupy jako zapisPartie a zapisPgn. Na jednom
 * miste, aby PGN, CSV, rozsah i pruh cislovaly stejne.
 */
data class Cislovani(val prvniJeCerny: Boolean, val cisloPrvnihoTahu: Int) {

    /** Tahne pultahem [i] cerny? Je to zaroven "cerny na tahu v pozici i". */
    fun cernyTahne(i: Int): Boolean = (i + (if (prvniJeCerny) 1 else 0)) % 2 == 1

    /** Cislo tahu pultahu [i], tedy ukazatel "Tah N" v pozici `history[i]`. */
    fun cisloTahu(i: Int): Int = cisloPrvnihoTahu + (i + (if (prvniJeCerny) 1 else 0)) / 2
}

/**
 * Analyzovane pultahy `od..doPultahu` (oba vcetne). Pozice k hledani jsou
 * o jednu delsi: [pozice].
 */
data class RozsahAnalyzy(val od: Int, val doPultahu: Int) {
    val pultahy: IntRange get() = od..doPultahu
    val pozice: IntRange get() = od..(doPultahu + 1)
    val pocetPozic: Int get() = doPultahu - od + 2
}

/**
 * "Od tahu - do tahu" (cisla tahu jako ukazatel "Tah N") na pultahy.
 * Od tahu N zacina prvnim pultahem s cislem N, do tahu M konci poslednim
 * pultahem s cislem M - tah 12 tedy znamena 12. tah bileho i cerneho.
 *
 * null, kdyz v partii zadny takovy pultah neni (prazdna partie, od > do,
 * cisla mimo partii).
 */
fun rozsahPultahu(odTahu: Int, doTahu: Int, pocetPultahu: Int, c: Cislovani): RozsahAnalyzy? {
    val vybrane = (0 until pocetPultahu).filter { c.cisloTahu(it) in odTahu..doTahu }
    if (vybrane.isEmpty()) return null
    return RozsahAnalyzy(vybrane.first(), vybrane.last())
}

/** Cela partie: pultahy 0..pocet-1, null pro partii bez tahu. */
fun celaPartie(pocetPultahu: Int): RozsahAnalyzy? =
    if (pocetPultahu <= 0) null else RozsahAnalyzy(0, pocetPultahu - 1)

/** "1 pozice", "3 pozice", "81 pozic". Tvary slov v tabulce textu (krok J1). */
fun pocetPozicText(n: Int, texty: Texty = TextyCz): String = texty.pocetPozic(n)

/**
 * Doba nad minutu zaokrouhlena na minuty: "7 min", "2 h 5 min". Pod minutu
 * vraci null - kazde misto ji rika jinak (odhad v sekundach, pruh "méně
 * než minuta").
 */
private fun minutyText(sekund: Int, texty: Texty): String? {
    if (sekund < 60) return null
    val min = (sekund + 30) / 60
    if (min < 60) return texty.dobaMinuty(min)
    val h = min / 60
    val zbytek = min % 60
    return texty.dobaHodiny(h, zbytek)
}

/** Odhad v okne: "81 pozic × 5 s ≈ 7 min". Koncove pozice se pocitaji taky. */
fun odhadText(pocetPozic: Int, sekundNaPozici: Int, texty: Texty = TextyCz): String {
    val celkem = pocetPozic * sekundNaPozici
    return texty.odhadAnalyzy(
        pocetPozicText(pocetPozic, texty),
        sekundNaPozici,
        minutyText(celkem, texty) ?: texty.sekund(celkem),
    )
}

/** "zbývá asi 3 min" / "zbývá méně než minuta". */
fun zbyvaText(sekund: Int, texty: Texty = TextyCz): String =
    minutyText(sekund, texty)?.let { texty.zbyvaAsi(it) } ?: texty.zbyvaMeneNezMinuta

/**
 * Text pruhu dole: "Probíhá analýza partie – tah 12/40, zbývá asi 3 min".
 * Lomitko, ne "ze 40": predlozka by zavisela na vysloveni cisla (z 12,
 * ze 40) a to se tu skladat nevyplati.
 *
 * @param tah        cislo tahu pozice, nad kterou motor premysli
 * @param zTahu      cislo tahu posledniho analyzovaneho pultahu
 * @param zbyvaPozic pozice, ktere jeste nemaji vysledek, vcetne te soucasne
 */
fun textPruhu(
    tah: Int,
    zTahu: Int,
    zbyvaPozic: Int,
    sekundNaPozici: Int,
    texty: Texty = TextyCz,
): String = texty.pruhAnalyzy(tah, zTahu, zbyvaText(zbyvaPozic * sekundNaPozici, texty))

/** Znacka Annotator v PGN analyzy. */
fun textAnnotatoru(jmenoMotoru: String, sekundNaPozici: Int, texty: Texty = TextyCz): String =
    texty.annotator(jmenoMotoru, sekundNaPozici)

/**
 * Spolecne jmeno obou vystupu bez pripony: "redcry-analyza-20261011-161427".
 *
 * Razitko se bere z [navrhJmenaSouboru] (U10), aby se cas skladal na jednom
 * miste; ta funkce se kvuli analyze nemeni. PGN i CSV dostanou tytez
 * razitko z jednoho `ted`, takze k sobe jdou i podle jmena.
 */
fun jmenoAnalyzy(ted: LocalDateTime = LocalDateTime.now()): String =
    "redcry-analyza-" + navrhJmenaSouboru(ted).removePrefix("redcry-").removeSuffix(".pgn")
