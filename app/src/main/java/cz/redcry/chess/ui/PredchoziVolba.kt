package cz.redcry.chess.ui

import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz

// Predchozi volba motoru v okne (i) (krok U14/C): cista logika bez Compose
// a bez ViewModelu, aby na ni sel napsat JVM test.

/**
 * Tah, ktery motor povazoval za nejlepsi, nez ve vetsi hloubce nasel lepsi.
 *
 * @param san        zapis toho tahu v SAN
 * @param doHloubky  posledni uzavrena hloubka, ve ktere ho motor jeste mel
 * @param poMs       ms od zacatku hledani do prvniho hlaseni nasledujici
 *                   hloubky, ve kterem uz byl nynejsi nejlepsi tah
 */
data class PredchoziVolba(val san: String, val doHloubky: Int, val poMs: Long)

/**
 * Jedno hlaseni Info pro [SledovaniVolby]: hloubka, prvni tah varianty
 * (v UCI, podle nej se tahy porovnavaji), jeho SAN a ms od zacatku hledani.
 */
data class HlaseniVolby(val hloubka: Int, val tah: String, val san: String, val ms: Long)

/**
 * Sleduje hlaseni jednoho hledani a urcuje [PredchoziVolba].
 *
 * Pravidlo:
 *  - hloubka je UZAVRENA, kdyz prijde Info s vetsi hloubkou nebo skonci
 *    hledani; plati jeji POSLEDNI Info (Stockfish muze v jedne hloubce
 *    poslat vic radku, kolisani uvnitr hloubky se nepocita);
 *  - nad uzavrenymi hloubkami d1 < ... < dn s tahy m1 ... mn se hleda
 *    nejvetsi k, pro ktere mk != mn;
 *  - vysledek: tah mk, "do hloubky dk", "zmena po" = cas prvniho Info
 *    hloubky d(k+1), ve kterem je tah mn;
 *  - kdyz takove k neni, je vysledek null (oddil se v okne nezobrazi).
 *
 * Kazda hloubka drzi jen posledni hlaseni a cas prvniho vyskytu kazdeho
 * tahu, takze pamet neroste s poctem hlaseni ani pri hledani bez casoveho
 * omezeni.
 */
class SledovaniVolby {

    private class Hloubka(var posledni: HlaseniVolby, val prvniMs: MutableMap<String, Long>)

    private val hloubky = sortedMapOf<Int, Hloubka>()

    /** Nove hledani. */
    fun vynuluj() = hloubky.clear()

    fun pridej(h: HlaseniVolby) {
        val d = hloubky[h.hloubka]
        if (d == null) {
            hloubky[h.hloubka] = Hloubka(h, mutableMapOf(h.tah to h.ms))
        } else {
            d.posledni = h
            d.prvniMs.putIfAbsent(h.tah, h.ms)
        }
    }

    /**
     * Predchozi volba podle dosavadnich hlaseni. [konec] = hledani skoncilo
     * (BestMove), takze je uzavrena i nejvyssi hloubka.
     */
    fun vysledek(konec: Boolean): PredchoziVolba? {
        val vse = hloubky.values.toList()
        val uzavrene = if (konec) vse else vse.dropLast(1)
        if (uzavrene.size < 2) return null
        val mn = uzavrene.last().posledni.tah
        val k = uzavrene.indexOfLast { it.posledni.tah != mn }
        if (k < 0) return null
        val mk = uzavrene[k].posledni
        val dalsi = uzavrene[k + 1]
        return PredchoziVolba(
            san = mk.san,
            doHloubky = mk.hloubka,
            poMs = dalsi.prvniMs[mn] ?: dalsi.posledni.ms,
        )
    }
}

/**
 * "změna po 1,9 s": cas na desetiny sekundy s desetinnou carkou. Rucne, ne
 * String.format - ten bere oddelovac z Locale zarizeni (stejne jako
 * u hodnoceni).
 */
fun textZmenyPo(ms: Long, texty: Texty = TextyCz): String {
    val desetiny = (ms + 50) / 100
    return texty.zmenaPo("" + (desetiny / 10) + texty.desetinnyOddelovac + (desetiny % 10))
}
