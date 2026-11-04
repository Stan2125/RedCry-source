package cz.redcry.chess.pgn

import java.time.LocalDate
import java.time.LocalDateTime

/**
 * Textova vrstva formatu PGN (Portable Game Notation, standard z roku 1994).
 *
 * ZADNA zavislost na enginu ani na android.* - je to prevod mezi textem
 * a seznamem tokenu SAN, nic vic. Diky tomu na to jde napsat obycejny JVM
 * test a diky tomu se tady taky nesmi objevit nic ze sachovych pravidel:
 * tokeny jsou retezce, kterym tahle vrstva vubec nerozumi. Rozumi jim az
 * motor, ktery si je porovna s vlastnim seznamem legalnich tahu.
 *
 * CTENI JE SHOVIVAVE, ZAPIS PRISNY. Je to zamerna nesouměrnost, kterou ma
 * i sam standard (import format v §3.1, export format v §3.2): prijmout se
 * ma i to, co nekdo napsal rukou, ale vydavat se ma jen jeden presne dany
 * tvar. Podrobnosti v docs/POZNAMKY-PGN.md.
 */

/** Jedna partie: hlavicka, tokeny SAN a vysledek. */
data class PgnPartie(
    val tagy: LinkedHashMap<String, String>,
    /** Tokeny SAN bez cisel tahu a bez ukoncovaciho tokenu. */
    val tahy: List<String>,
    /** "1-0" / "0-1" / "1/2-1/2" / "*". */
    val vysledek: String,
)

/**
 * Sedm povinnych znacek v poradi, v jakem je zapis MUSI vydat (§8.1.1).
 * Poradi neni kosmetika - export format ho predepisuje presne takhle.
 */
val ZNACKY_POVINNE = listOf("Event", "Site", "Date", "Round", "White", "Black", "Result")

/** Neznama hodnota znacky (§8.1.1). */
const val NEZNAMA = "?"

/** Nezname datum: otazniky za kazdou neznamou cislici (§8.1.1.3). */
const val NEZNAME_DATUM = "????.??.??"

/** Nedohrana nebo neznama partie (§8.1.1.7). */
const val BEZ_VYSLEDKU = "*"

/**
 * Nejvetsi povolena delka radku pohyboveho textu. Standard v §8.2.1 rika
 * "mene nez 80 tisknutelnych znaku", tedy nejvys 79.
 */
const val SIRKA_RADKU = 79

/** Ukoncovaci tokeny pohyboveho textu (§8.2.6). */
private val UKONCOVACI = setOf("1-0", "0-1", "1/2-1/2", "*")

/** Znaky, ktere token vzdycky ukoncuji, i kdyz mezi nimi neni mezera. */
private const val ODDELOVACE = "{};()[]"

// ---------------------------------------------------------------------------
//  CTENI (import format, §3.1)
// ---------------------------------------------------------------------------

/**
 * Precte partii. Nikdy nevyhodi vyjimku - co necte, to preskoci.
 *
 * Zvlada: znacky `[Jmeno "hodnota"]` vcetne `\"` uvnitr a prazdnych hodnot,
 * cisla tahu s libovolnym poctem tecek, komentare `{ }` i `;`, varianty
 * `( )` vcetne vnoreni, NAG `$12`, pripony hodnoceni (`!`, `?`, `!?`, ...)
 * a koncove `e.p.`. CRLF i LF.
 *
 * Varianty se cely preskoci - RedCry je nezobrazuje, a kdyby se jejich tahy
 * dostaly mezi hlavni, partie by se pri nacitani rozsypala na prvnim z nich.
 */
fun ctiPgn(text: String): PgnPartie {
    val tagy = LinkedHashMap<String, String>()
    val tahy = ArrayList<String>()
    var vysledek: String? = null

    // Hloubka vnorenych variant. Nad nulou se tokeny zahazuji, ale komentare
    // a dalsi varianty se uvnitr porad musi poznat - proto se ctou stejne.
    var varianta = 0
    var i = 0

    while (i < text.length) {
        val c = text[i]
        when {
            c.isWhitespace() -> i++

            // Komentar do konce radku. CR se resi sam - je to bily znak.
            c == ';' -> while (i < text.length && text[i] != '\n') i++

            // Komentar v zavorkach. Standard rika, ze se nevnoruji, takze
            // staci najit prvni '}'.
            c == '{' -> {
                while (i < text.length && text[i] != '}') i++
                if (i < text.length) i++
            }

            c == '(' -> { varianta++; i++ }
            c == ')' -> { if (varianta > 0) varianta--; i++ }

            c == '[' -> i = ctiZnacku(text, i, tagy)

            else -> {
                val zacatek = i
                while (i < text.length && !text[i].isWhitespace() && text[i] !in ODDELOVACE) i++
                // Pojistka proti zacykleni: kdyby token vysel prazdny, posun
                // se musi stat tak jako tak.
                if (i == zacatek) i++
                if (varianta == 0) {
                    val token = ocistiToken(text.substring(zacatek, i))
                    when {
                        token == null -> {}
                        token in UKONCOVACI -> vysledek = token
                        else -> tahy.add(token)
                    }
                }
            }
        }
    }

    // Kdyz ukoncovaci token v textu chybi, plati znacka Result; kdyz neni
    // ani ta, je partie nedohrana.
    val konec = vysledek
        ?: tagy["Result"]?.takeIf { it.isNotBlank() }
        ?: BEZ_VYSLEDKU

    return PgnPartie(tagy, tahy, konec)
}

/**
 * Precte jednu znacku od '[' a vrati index za ni. Kdyz to znacka neni,
 * preskoci se jen ten jeden znak - shovivave cteni se na nesmyslu nezastavuje.
 */
private fun ctiZnacku(text: String, od: Int, tagy: LinkedHashMap<String, String>): Int {
    var i = od + 1
    while (i < text.length && text[i].isWhitespace()) i++

    val zacatekJmena = i
    while (i < text.length && (text[i].isLetterOrDigit() || text[i] == '_')) i++
    val jmeno = text.substring(zacatekJmena, i)

    while (i < text.length && text[i].isWhitespace()) i++
    if (jmeno.isEmpty() || i >= text.length || text[i] != '"') return od + 1

    i++ // za otviraci uvozovku
    val hodnota = StringBuilder()
    while (i < text.length && text[i] != '"') {
        // §7: uvnitr hodnoty se pres zpetne lomitko pisou `\"` a `\\`.
        // Bere se cokoli, co za nim stoji - je to shovivavejsi a nic to nekazi.
        if (text[i] == '\\' && i + 1 < text.length) {
            hodnota.append(text[i + 1])
            i += 2
        } else {
            hodnota.append(text[i])
            i++
        }
    }
    if (i < text.length) i++ // zaviraci uvozovka

    // Zbytek do ']' zahodit - poradi znacek ani smeti za hodnotou neresime.
    while (i < text.length && text[i] != ']' && text[i] != '\n') i++
    if (i < text.length && text[i] == ']') i++

    // Neznamou znacku si necháváme; opakovanou prepise ta pozdejsi.
    tagy[jmeno] = hodnota.toString()
    return i
}

/**
 * Z jednoho syroveho tokenu udela token SAN, ukoncovaci token, nebo null
 * (cislo tahu, NAG, smeti).
 *
 * Cislo tahu muze byt k tahu prilepene (`1.d4`, `62...Qfc1#`), proto se
 * odkrajuje tady a ne zvlast.
 */
private fun ocistiToken(syrovy: String): String? {
    // Musi byt prvni: "1-0" i "1/2-1/2" zacinaji cislici a jako cislo tahu
    // by se rozpadly.
    if (syrovy in UKONCOVACI) return syrovy

    var t = syrovy.trimStart('.')
    val cislo = t.takeWhile { it.isDigit() }
    if (cislo.isNotEmpty()) {
        val zbytek = t.substring(cislo.length)
        // Cislo tahu je bud samo o sobe, nebo za nim stoji tecky. Kdyz za nim
        // stoji neco jineho, nebylo to cislo tahu a token se nechava cely.
        if (zbytek.isEmpty() || zbytek[0] == '.') t = zbytek.trimStart('.')
    }

    if (t.startsWith("$")) return null // NAG

    // Pripony a `e.p.` se strhavaji dokola, protoze se daji kombinovat
    // ("exd6e.p.!?"). Cyklus konci, jakmile se uz nic neurve.
    while (true) {
        val pred = t
        while (t.isNotEmpty() && (t.last() == '!' || t.last() == '?')) t = t.dropLast(1)
        if (t.endsWith("e.p.")) t = t.dropLast(4)
        else if (t.endsWith("e.p")) t = t.dropLast(3)
        if (t == pred) break
    }

    if (t.isEmpty()) return null
    return t
}

// ---------------------------------------------------------------------------
//  ZAPIS (export format, §3.2)
// ---------------------------------------------------------------------------

/**
 * Slozi text PGN. Sedm povinnych znacek prvni a v predepsanem poradi,
 * prazdny radek, pohybovy text zalomeny pod 80 znaku a zakonceny
 * ukoncovacim tokenem, prazdny radek.
 *
 * @param prvniJeCerny      partie zacina cernym (import z FEN), prvni radek
 *                          je pak "12... Nf6"
 * @param cisloPrvnihoTahu  cislo tahu vychozi pozice partie
 */
fun zapisPgn(
    partie: PgnPartie,
    prvniJeCerny: Boolean = false,
    cisloPrvnihoTahu: Int = 1,
): String {
    val sb = StringBuilder()

    for (jmeno in ZNACKY_POVINNE) {
        val hodnota = when (jmeno) {
            // Vysledek je v partii autoritativne v `vysledek`, ne ve znacce -
            // pri cteni se bere i z pohyboveho textu.
            "Result" -> partie.vysledek.ifBlank { BEZ_VYSLEDKU }
            "Date" -> partie.tagy[jmeno]?.ifBlank { null } ?: NEZNAME_DATUM
            else -> partie.tagy[jmeno]?.ifBlank { null } ?: NEZNAMA
        }
        sb.append(znacka(jmeno, hodnota))
    }
    // Neznamé znacky se nezahazuji, jen stoji az za sedmi povinnymi.
    for ((jmeno, hodnota) in partie.tagy) {
        if (jmeno !in ZNACKY_POVINNE) sb.append(znacka(jmeno, hodnota))
    }

    sb.append('\n')
    for (radek in zalom(pohyboveTokeny(partie, prvniJeCerny, cisloPrvnihoTahu))) {
        sb.append(radek).append('\n')
    }
    sb.append('\n')
    return sb.toString()
}

private fun znacka(jmeno: String, hodnota: String): String {
    val escapovana = hodnota.replace("\\", "\\\\").replace("\"", "\\\"")
    return "[" + jmeno + " \"" + escapovana + "\"]\n"
}

/**
 * Pohybovy text jako seznam tokenu: cisla tahu, tahy a nakonec ukoncovaci
 * token. Zalomeni pak jen sklada radky, takze se token nikdy nerozpuli.
 */
internal fun pohyboveTokeny(
    partie: PgnPartie,
    prvniJeCerny: Boolean,
    cisloPrvnihoTahu: Int,
): List<String> {
    val ven = ArrayList<String>()
    var cislo = cisloPrvnihoTahu
    var i = 0

    if (prvniJeCerny && partie.tahy.isNotEmpty()) {
        ven.add(cislo.toString() + "...")
        ven.add(partie.tahy[0])
        cislo++
        i = 1
    }
    while (i < partie.tahy.size) {
        ven.add(cislo.toString() + ".")
        ven.add(partie.tahy[i])
        partie.tahy.getOrNull(i + 1)?.let { ven.add(it) }
        cislo++
        i += 2
    }

    // §8.2.6: ukoncovaci token je posledni prvek pohyboveho textu, vzdycky.
    ven.add(partie.vysledek.ifBlank { BEZ_VYSLEDKU })
    return ven
}

/** Slozi tokeny do radku kratsich nez 80 znaku. Token se nikdy nedeli. */
internal fun zalom(tokeny: List<String>, sirka: Int = SIRKA_RADKU): List<String> {
    val radky = ArrayList<String>()
    val radek = StringBuilder()
    for (t in tokeny) {
        when {
            radek.isEmpty() -> radek.append(t)
            radek.length + 1 + t.length > sirka -> {
                radky.add(radek.toString())
                radek.setLength(0)
                radek.append(t)
            }
            else -> radek.append(' ').append(t)
        }
    }
    if (radek.isNotEmpty()) radky.add(radek.toString())
    return radky
}

// ---------------------------------------------------------------------------
//  Hlavicka pro partii exportovanou z aplikace
// ---------------------------------------------------------------------------

/** Dnesni datum ve tvaru RRRR.MM.DD (§8.1.1.3). */
fun dnesniDatum(dnes: LocalDate = LocalDate.now()): String =
    dnes.year.toString().padStart(4, '0') + "." +
        dnes.monthValue.toString().padStart(2, '0') + "." +
        dnes.dayOfMonth.toString().padStart(2, '0')

/**
 * Navrh jmena pri ukladani: redcry-20261004-153207.pgn (U10).
 *
 * Cas je na sekundy, protoze se samotnym datem (a i s minutami) by druhe
 * ulozeni v tentyz den prepsalo to prvni. Datum i cas se berou z jednoho
 * `ted` - dve samostatna cteni hodin by se kolem pulnoci mohla rozejit
 * (datum z jednoho dne, cas z druheho).
 */
fun navrhJmenaSouboru(ted: LocalDateTime = LocalDateTime.now()): String =
    "redcry-" + dnesniDatum(ted.toLocalDate()).replace(".", "") + "-" +
        ted.hour.toString().padStart(2, '0') +
        ted.minute.toString().padStart(2, '0') +
        ted.second.toString().padStart(2, '0') + ".pgn"

/**
 * Hlavicka partie, kterou zapisuje RedCry: datum je dnesni, ostatni nezname.
 * Hraci ani turnaj aplikace nezna a vymyslet si je by bylo horsi nez "?".
 */
fun vychoziZnacky(datum: String = dnesniDatum()): LinkedHashMap<String, String> {
    val tagy = LinkedHashMap<String, String>()
    tagy["Event"] = NEZNAMA
    tagy["Site"] = NEZNAMA
    tagy["Date"] = datum
    tagy["Round"] = NEZNAMA
    tagy["White"] = NEZNAMA
    tagy["Black"] = NEZNAMA
    return tagy
}
