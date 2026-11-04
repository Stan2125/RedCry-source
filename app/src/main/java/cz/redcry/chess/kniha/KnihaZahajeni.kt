package cz.redcry.chess.kniha

import java.io.Reader
import kotlin.random.Random

/**
 * Knihovna zahajeni spolecna pro vsechny ctyri motory (krok K1).
 *
 * ZADNA sachova pravidla a zadna zavislost na android.*: knihovna jen
 * porovna pozici s tabulkou a vrati tah UCI. Zahraje ho motor pres
 * applyMove (krok K1/B), takze legalitu hlida porad C++.
 *
 * Data jsou v assets/kniha/ a vyrabi je tools/kniha/vyrob_knihu.py ze
 * souboru lichess-org/chess-openings (CC0, puvod v
 * tools/kniha/lichess/PUVOD.md):
 *
 * - kniha.txt: `pozice;tah vaha;tah vaha...` - jen hlavni varianty, tahy
 *   serazene podle vahy;
 * - nazvy.txt: `pozice;ECO;nazev` - vsechny pojmenovane varianty Lichess.
 *
 * Radky zacinajici # jsou komentare, prazdne radky se preskakuji. UTF-8, LF.
 * Podrobnosti v docs/POZNAMKY-KNIHA.md.
 *
 * Pozice se klicuje jen prvnimi tremi poli FEN ([klicPozice]): rozestaveni,
 * strana na tahu a rosady. Pole brani mimochodem motory zapisuji kazdy
 * jinak (jeden ho uvede po kazdem dvojkroku pesce, jiny jen kdyz brat jde)
 * a pocitadla tahu se pri prehozeni tahu lisi, i kdyz je pozice tataz.
 */
class KnihaZahajeni(
    private val tahy: Map<String, List<TahKnihy>>,
    private val nazvy: Map<String, Zahajeni>,
) {

    /** Tahy z knihovny pro pozici, serazene podle vahy; prazdne mimo knihovnu. */
    fun tahy(fen: String): List<TahKnihy> = tahy[klicPozice(fen)].orEmpty()

    /**
     * Tah UCI vybrany nahodne podle vah, at motor nehraje porad totez.
     * Null, kdyz pozice v knihovne neni.
     */
    fun vyber(fen: String, nahoda: Random): String? {
        val moznosti = tahy(fen)
        if (moznosti.isEmpty()) return null
        var zbyva = nahoda.nextInt(moznosti.sumOf { it.vaha })
        for (tah in moznosti) {
            zbyva -= tah.vaha
            if (zbyva < 0) return tah.uci
        }
        error("nedosazitelne: soucet vah nesedi")
    }

    /** ECO a nazev pozice, null kdyz pojmenovana neni. */
    fun nazev(fen: String): Zahajeni? = nazvy[klicPozice(fen)]

    /**
     * Nazev zahajeni partie: nazev POSLEDNI pojmenovane pozice v [historie]
     * (FEN od zacatku partie po aktualni pozici). Po odbocce z pojmenovanych
     * variant tak zustane nazev, ze ktereho partie vysla. Null, kdyz zadna
     * pozice pojmenovana neni.
     */
    fun nazevPartie(historie: List<String>): Zahajeni? =
        historie.asReversed().firstNotNullOfOrNull { nazev(it) }

    companion object {

        private val UCI = Regex("[a-h][1-8][a-h][1-8][qrbn]?")

        /**
         * Nacte oba soubory. Vadny radek (spatny pocet poli, spatny tah,
         * vaha, ktera neni kladne cislo, pozice dvakrat) skonci
         * IllegalArgumentException s cislem radku.
         */
        fun nacti(kniha: Reader, nazvy: Reader): KnihaZahajeni =
            KnihaZahajeni(nactiTahy(kniha), nactiNazvy(nazvy))

        private fun nactiTahy(r: Reader): Map<String, List<TahKnihy>> {
            val vysledek = HashMap<String, List<TahKnihy>>()
            radky(r) { cislo, radek ->
                val pole = radek.split(';')
                vadny(pole.size >= 2, "kniha", cislo, "chybi tahy")
                val klic = pozice(pole[0], "kniha", cislo)
                val seznam = pole.drop(1).map { t ->
                    val casti = t.split(' ')
                    vadny(casti.size == 2 && UCI.matches(casti[0]), "kniha", cislo, "tah '$t'")
                    val vaha = casti[1].toIntOrNull()
                    vadny(vaha != null && vaha > 0, "kniha", cislo, "vaha '$t'")
                    TahKnihy(casti[0], vaha!!)
                }
                vadny(vysledek.put(klic, seznam) == null, "kniha", cislo, "pozice podruhe")
            }
            return vysledek
        }

        private fun nactiNazvy(r: Reader): Map<String, Zahajeni> {
            val vysledek = HashMap<String, Zahajeni>()
            radky(r) { cislo, radek ->
                // Nazev muze obsahovat cokoli, proto nejvys tri pole.
                val pole = radek.split(';', limit = 3)
                vadny(pole.size == 3 && pole[1].isNotEmpty() && pole[2].isNotEmpty(),
                    "nazvy", cislo, "cekaji se tri pole")
                val klic = pozice(pole[0], "nazvy", cislo)
                vadny(vysledek.put(klic, Zahajeni(pole[1], pole[2])) == null,
                    "nazvy", cislo, "pozice podruhe")
            }
            return vysledek
        }

        /** Radky bez komentaru a prazdnych, s cislem radku od 1. */
        private fun radky(r: Reader, zpracuj: (Int, String) -> Unit) {
            r.buffered().useLines { radky ->
                radky.forEachIndexed { i, radek ->
                    if (radek.isNotBlank() && !radek.startsWith("#")) zpracuj(i + 1, radek)
                }
            }
        }

        private fun pozice(text: String, soubor: String, cislo: Int): String {
            vadny(text.split(' ').size == 3, soubor, cislo, "pozice '$text'")
            return text
        }

        private fun vadny(ok: Boolean, soubor: String, cislo: Int, co: String) {
            require(ok) { "$soubor, radek $cislo: $co" }
        }
    }
}

/** Klic pozice: prvni tri pole FEN (rozestaveni, strana na tahu, rosady). */
fun klicPozice(fen: String): String = fen.trim().split(Regex("\\s+")).take(3).joinToString(" ")

/** Tah z knihovny: UCI a vaha (pocet variant, ktere jim prochazeji). */
data class TahKnihy(val uci: String, val vaha: Int)

/** Pojmenovana pozice: kod ECO a anglicky nazev z Lichess. */
data class Zahajeni(val eco: String, val nazev: String)
