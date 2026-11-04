package cz.redcry.chess.analyza

import cz.redcry.chess.pgn.PgnPartie
import cz.redcry.chess.pgn.BEZ_VYSLEDKU
import cz.redcry.chess.pgn.zalom
import cz.redcry.chess.pgn.zapisPgn
import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz
import cz.redcry.chess.ui.MEZ_MATU
import cz.redcry.chess.ui.formatujSkoreKratce

/**
 * Partie, jak ji analyza zapisuje: tytez vstupy jako exportPgn (znacky,
 * SAN, vysledek, cislovani) a navic tahy v UCI, podle kterych se pozna,
 * jestli se zahrany tah lisi od nejlepsiho. SAN se neporovnava: tentyz tah
 * muze mit od dvou motoru jiny zapis jen tehdy, kdyby se lisily v pravidlech,
 * ale UCI je jednoznacne vzdycky.
 */
data class AnalyzovanaPartie(
    val tagy: LinkedHashMap<String, String>,
    val tahySan: List<String>,
    val tahyUci: List<String>,
    val vysledek: String,
    val cislovani: Cislovani,
)

/** Orezani hodnoty v CSV na +-20 pescu, i u matu (zadani A1). */
private const val STROP_CP = 2_000

// ---------------------------------------------------------------------------
//  PGN
// ---------------------------------------------------------------------------

/**
 * PGN s analyzou. Hlavicka je presne ta ze [zapisPgn] (vcetne poradi
 * znacek a escapovani), proto se z ni bere a nesklada se znovu; zapisPgn
 * se kvuli analyze nemeni. Pohybovy text pridava ke kazdemu analyzovanemu
 * tahu komentar a pripadne variantu, viz [tokenyAnalyzy].
 */
fun zapisPgnAnalyzy(
    partie: AnalyzovanaPartie,
    rozsah: RozsahAnalyzy,
    vysledky: Map<Int, VysledekPozice>,
    annotator: String,
    /** Desetinny oddelovac komentaru (krok J3). */
    texty: Texty = TextyCz,
): String {
    val tagy = LinkedHashMap(partie.tagy)
    tagy["Annotator"] = annotator
    // zapisPgn bez tahu = hlavicka, prazdny radek, ukoncovaci token,
    // prazdny radek. Hlavicka je vse pred prvnim prazdnym radkem - hodnota
    // znacky novy radek obsahovat nemuze, ctiZnacku ho do ni nepusti.
    val bezTahu = zapisPgn(
        PgnPartie(tagy, emptyList(), partie.vysledek),
        partie.cislovani.prvniJeCerny,
        partie.cislovani.cisloPrvnihoTahu,
    )
    val sb = StringBuilder(bezTahu.substringBefore("\n\n")).append("\n\n")
    for (radek in zalom(tokenyAnalyzy(partie, rozsah, vysledky, texty))) {
        sb.append(radek).append('\n')
    }
    sb.append('\n')
    return sb.toString()
}

/**
 * Pohybovy text analyzy jako tokeny pro [zalom]. Komentar i varianta jsou
 * slozene tak, aby zavorka byla soucasti tokenu (`{+0,35/24}`, `(14...`,
 * `O-O)`) - zalom pak radek nikdy nerozdeli uvnitr nich.
 *
 * Za tahem `i` z rozsahu:
 * - komentar s vysledkem pozice i+1 ([komentar]),
 * - varianta z pozice i, kdyz se zahrany tah lisi od nejlepsiho.
 * Cerny tah po komentari nebo variante dostane cislo s tremi teckami
 * (PGN §8.2.2.2), jinak cislo jen bily tah.
 */
internal fun tokenyAnalyzy(
    partie: AnalyzovanaPartie,
    rozsah: RozsahAnalyzy,
    vysledky: Map<Int, VysledekPozice>,
    texty: Texty = TextyCz,
): List<String> {
    val c = partie.cislovani
    val ven = ArrayList<String>()
    // Prvni tah partie zacinajici cernym cislo potrebuje taky.
    var cisloCernemu = true
    for (i in partie.tahySan.indices) {
        if (!c.cernyTahne(i)) ven.add(c.cisloTahu(i).toString() + ".")
        else if (cisloCernemu) ven.add(c.cisloTahu(i).toString() + "...")
        ven.add(partie.tahySan[i])
        cisloCernemu = false
        if (i in rozsah.pultahy) {
            komentar(vysledky[i + 1], texty)?.let {
                ven.add(it)
                cisloCernemu = true
            }
            jinaVarianta(partie, i, vysledky[i])?.let { tahy ->
                val tokeny = tokenyVarianty(tahy, i, c).toMutableList()
                tokeny[0] = "(" + tokeny[0]
                tokeny[tokeny.size - 1] = tokeny.last() + ")"
                ven.addAll(tokeny)
                cisloCernemu = true
            }
        }
    }
    ven.add(partie.vysledek.ifBlank { BEZ_VYSLEDKU })
    return ven
}

/**
 * `{+0,35/24}`, mat `{+M3/18}`, bez hloubky `{+0,35}`; anglicky `{+0.35/24}`
 * (krok J3). Bez skore a v koncove pozici (za matujicim tahem) komentar
 * neni: mat uz rika `#` u tahu.
 */
internal fun komentar(v: VysledekPozice?, texty: Texty = TextyCz): String? {
    if (v == null || v.konec != null || v.skoreBily == null) return null
    val hloubka = v.hloubka?.let { "/$it" } ?: ""
    return "{" + formatujSkoreKratce(v.skoreBily, texty) + hloubka + "}"
}

/** Varianta pozice i, kdyz motor chtel hrat jinak nez partie; jinak null. */
private fun jinaVarianta(partie: AnalyzovanaPartie, i: Int, v: VysledekPozice?): List<String>? {
    if (v?.tahUci == null || v.varianta.isEmpty()) return null
    if (v.tahUci == partie.tahyUci.getOrNull(i)) return null
    return v.varianta
}

/**
 * Varianta z pozice pred pultahem [i] s cisly tahu: od cerneho
 * `14... Nf6 15. O-O O-O`, od bileho `15. Re1 Nf6`. Bez zavorek - ty
 * pridava jen PGN, CSV je nechce.
 */
internal fun tokenyVarianty(tahy: List<String>, i: Int, c: Cislovani): List<String> {
    val ven = ArrayList<String>()
    for ((k, tah) in tahy.withIndex()) {
        val j = i + k
        if (!c.cernyTahne(j)) ven.add(c.cisloTahu(j).toString() + ".")
        else if (k == 0) ven.add(c.cisloTahu(j).toString() + "...")
        ven.add(tah)
    }
    return ven
}

// ---------------------------------------------------------------------------
//  CSV
// ---------------------------------------------------------------------------

/**
 * CSV analyzy pro tabulkovy procesor: UTF-8 s BOM (jinak Excel cte
 * diakritiku jako cp1250), konce radku CRLF. Jeden radek na analyzovany
 * pultah, i kdyz k nemu vysledky chybi (prerusena analyza) - pak jsou
 * bunky prazdne.
 *
 * Hlavicka, jmeno strany (od J1), oddelovac bunek a desetinny oddelovac
 * (od J3) jsou z [texty]: cesky `;` a carka (cesky Excel, presne jako do
 * J3), anglicky `,` a tecka (anglicky Excel).
 */
fun zapisCsvAnalyzy(
    partie: AnalyzovanaPartie,
    rozsah: RozsahAnalyzy,
    vysledky: Map<Int, VysledekPozice>,
    texty: Texty = TextyCz,
): String {
    val c = partie.cislovani
    val sb = StringBuilder("﻿").append(texty.hlavickaCsv).append("\r\n")
    for (i in rozsah.pultahy) {
        val pred = vysledky[i]
        val po = vysledky[i + 1]
        val bunky = listOf(
            c.cisloTahu(i).toString(),
            if (c.cernyTahne(i)) texty.cerny else texty.bily,
            partie.tahySan.getOrElse(i) { "" },
            hodnotaCsv(po, c.cernyTahne(i + 1), texty),
            matCsv(po),
            if (po?.konec == null) po?.hloubka?.toString() ?: "" else "",
            pred?.tahSan ?: "",
            pred?.varianta?.takeIf { it.isNotEmpty() }
                ?.let { tokenyVarianty(it, i, c).joinToString(" ") } ?: "",
            ztrataCsv(partie, i, pred, po, texty),
        )
        val oddelovac = texty.oddelovacCsv
        sb.append(bunky.joinToString(oddelovac.toString()) { bunkaCsv(it, oddelovac) }).append("\r\n")
    }
    return sb.toString()
}

/**
 * Hodnota pozice z pohledu bileho v setinach, orezana na +-[STROP_CP],
 * vcetne koncovych pozic: mat = strana na tahu prohrala, pat = 0.
 * null = nezname.
 */
internal fun hodnotaOrezana(v: VysledekPozice?, cernyNaTahu: Boolean): Int? = when {
    v == null -> null
    v.konec == KonecPozice.MAT -> if (cernyNaTahu) STROP_CP else -STROP_CP
    v.konec == KonecPozice.PAT -> 0
    else -> v.skoreBily?.coerceIn(-STROP_CP, STROP_CP)
}

private fun hodnotaCsv(v: VysledekPozice?, cernyNaTahu: Boolean, texty: Texty): String =
    hodnotaOrezana(v, cernyNaTahu)?.let { pesceCsv(it, texty) } ?: ""

/** `#` v koncove pozici s matem, "+M3" u matoveho skore, jinak prazdne. */
private fun matCsv(v: VysledekPozice?): String = when {
    v == null -> ""
    v.konec == KonecPozice.MAT -> "#"
    v.konec != null -> ""
    v.skoreBily == null -> ""
    v.skoreBily >= MEZ_MATU || v.skoreBily <= -MEZ_MATU -> formatujSkoreKratce(v.skoreBily)
    else -> ""
}

/**
 * Ztrata tahu v pescich z pohledu toho, kdo tahl: o kolik klesla hodnota
 * mezi pozici pred tahem a po nem, obe orezane na +-20, nejmene 0.
 * Zahral-li nejlepsi tah motoru, je 0 bez ohledu na cisla - rozdil by byl
 * jen sum mezi dvema hledanimi.
 */
private fun ztrataCsv(
    partie: AnalyzovanaPartie,
    i: Int,
    pred: VysledekPozice?,
    po: VysledekPozice?,
    texty: Texty,
): String {
    if (pred?.tahUci != null && pred.tahUci == partie.tahyUci.getOrNull(i)) return pesceCsv(0, texty)
    val c = partie.cislovani
    val a = hodnotaOrezana(pred, c.cernyTahne(i)) ?: return ""
    val b = hodnotaOrezana(po, c.cernyTahne(i + 1)) ?: return ""
    val pokles = if (c.cernyTahne(i)) b - a else a - b
    return pesceCsv(maxOf(0, pokles), texty)
}

/**
 * Setiny pesce bez "+": "0,35", "-1,20" (anglicky "0.35"). Bere se
 * z [formatujSkoreKratce], aby se cislo skladalo na jednom miste; vstup je
 * uz orezany, takze mat sem nedojde.
 */
private fun pesceCsv(cp: Int, texty: Texty): String = formatujSkoreKratce(cp, texty).removePrefix("+")

/**
 * Bunka s oddelovacem jazyka, `"` nebo koncem radku do uvozovek, uvozovky
 * zdvojene (RFC 4180). Cesky oddelovac `;` jako do J3, anglicky `,`.
 */
private fun bunkaCsv(s: String, oddelovac: Char): String =
    if (s.any { it == oddelovac || it == '"' || it == '\n' || it == '\r' }) {
        "\"" + s.replace("\"", "\"\"") + "\""
    } else {
        s
    }
