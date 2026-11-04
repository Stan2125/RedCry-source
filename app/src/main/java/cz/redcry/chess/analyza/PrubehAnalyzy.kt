package cz.redcry.chess.analyza

import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.ui.Jazyk

/** Meze casu na pozici v okne analyzy (zadani A1). */
const val MIN_SEKUND_ANALYZY = 1
const val MAX_SEKUND_ANALYZY = 60
const val VYCHOZI_SEKUNDY_ANALYZY = 5

/**
 * Rozpracovana analyza pro pokracovani po zabiti aplikace nebo preruseni
 * (zadani A1, bod 2.6). Uklada se po kazde pozici do filesDir.
 *
 * Identita partie je vychozi FEN + vsechny tahy v UCI: jen tak se pozna,
 * ze uzivatel ma na desce TUTEZ partii. SAN ani znacky se neukladaji,
 * pri pokracovani se berou z partie na desce, ktera je s ulozenou shodna.
 *
 * @param nazev   spolecne jmeno vystupu bez pripony ([jmenoAnalyzy])
 * @param uriPgn  Uri vystupniho PGN jako text, null = jeste nezalozen
 * @param jazyk   jazyk platny pri spusteni (krok J3): v nem jsou oba soubory
 *                i po prepnuti jazyka a pri pokracovani po restartu
 */
data class PrubehAnalyzy(
    val vychoziFen: String,
    val tahyUci: List<String>,
    val motor: EngineId,
    val sekundy: Int,
    val rozsah: RozsahAnalyzy,
    val nazev: String,
    val uriPgn: String?,
    val uriCsv: String?,
    val vysledky: Map<Int, VysledekPozice>,
    val jazyk: Jazyk = Jazyk.CESTINA,
) {
    /** Je to analyza teto partie? */
    fun jeTataPartie(fen: String, tahy: List<String>): Boolean =
        fen == vychoziFen && tahy == tahyUci

    /** Prvni pozice bez vysledku, null = hotovo. */
    fun dalsiPozice(): Int? = rozsah.pozice.firstOrNull { it !in vysledky }

    /** Pozice, ktere jeste nemaji vysledek. */
    fun zbyvaPozic(): Int = rozsah.pozice.count { it !in vysledky }
}

private const val HLAVICKA = "RedCry analyza 1"
private const val NIC = "-"
private val TAH_UCI = Regex("[a-h][1-8][a-h][1-8][qrbn]?")

/**
 * Prubeh do textu. Vlastni radkovy format, ne JSON: org.json je
 * v JVM testech jen atrapa, ktera nic neumi, a knihovna navic kvuli
 * jednomu souboru nestoji za to.
 *
 * Kazdy radek je `klic hodnota`, hodnota do konce radku (FEN i Uri tak
 * smi obsahovat mezery). Chybejici hodnota je `-`, coz zadny tah, SAN,
 * cislo ani Uri byt nemuze.
 *
 * Od kroku J3 radek `jazyk cs` / `jazyk en`. Je volitelny: prubeh z doby
 * pred J3 ho nema a cte se jako cestina, proto se nemeni ani [HLAVICKA].
 */
fun zapisPrubeh(p: PrubehAnalyzy): String {
    val sb = StringBuilder()
    sb.append(HLAVICKA).append('\n')
    sb.append("fen ").append(p.vychoziFen).append('\n')
    sb.append("tahy ").append(p.tahyUci.joinToString(" ")).append('\n')
    sb.append("motor ").append(p.motor.name).append('\n')
    sb.append("sekundy ").append(p.sekundy).append('\n')
    sb.append("rozsah ").append(p.rozsah.od).append(' ').append(p.rozsah.doPultahu).append('\n')
    sb.append("nazev ").append(p.nazev).append('\n')
    sb.append("pgn ").append(p.uriPgn ?: NIC).append('\n')
    sb.append("csv ").append(p.uriCsv ?: NIC).append('\n')
    sb.append("jazyk ").append(p.jazyk.kod).append('\n')
    for ((index, v) in p.vysledky.toSortedMap()) {
        sb.append("pozice ").append(index)
            .append(' ').append(v.hloubka?.toString() ?: NIC)
            .append(' ').append(v.skoreBily?.toString() ?: NIC)
            .append(' ').append(v.tahUci ?: NIC)
            .append(' ').append(v.konec?.name ?: NIC)
        for (tah in v.varianta) sb.append(' ').append(tah)
        sb.append('\n')
    }
    return sb.toString()
}

/**
 * Text na prubeh, nebo null, kdyz to prubeh neni, je poskozeny nebo
 * nesedi sam se sebou (rozsah mimo tahy, pozice mimo rozsah, motor, ktery
 * aplikace nezna). Radeji zahodit nez pokracovat v necem, co nesedi:
 * nejhorsi, co se stane, je analyza od zacatku.
 */
fun ctiPrubeh(text: String): PrubehAnalyzy? {
    val radky = text.replace("\r", "").split('\n').filter { it.isNotEmpty() }
    if (radky.firstOrNull() != HLAVICKA) return null

    val hodnoty = HashMap<String, String>()
    val vysledky = HashMap<Int, VysledekPozice>()
    for (radek in radky.drop(1)) {
        val klic = radek.substringBefore(' ')
        val hodnota = if (' ' in radek) radek.substringAfter(' ') else ""
        if (klic == "pozice") {
            val (index, v) = ctiPozici(hodnota) ?: return null
            if (vysledky.put(index, v) != null) return null
        } else {
            if (klic !in KLICE && klic !in VOLITELNE || hodnoty.put(klic, hodnota) != null) return null
        }
    }
    if (hodnoty.keys - VOLITELNE != KLICE) return null

    val fen = hodnoty.getValue("fen").takeIf { it.isNotBlank() } ?: return null
    val tahy = hodnoty.getValue("tahy").split(' ').filter { it.isNotEmpty() }
    if (tahy.any { !TAH_UCI.matches(it) }) return null
    val motor = EngineId.entries.firstOrNull { it.name == hodnoty["motor"] && it.dostupny }
        ?: return null
    val sekundy = hodnoty.getValue("sekundy").toIntOrNull()
        ?.takeIf { it in MIN_SEKUND_ANALYZY..MAX_SEKUND_ANALYZY } ?: return null
    val meze = hodnoty.getValue("rozsah").split(' ')
    if (meze.size != 2) return null
    val od = meze[0].toIntOrNull() ?: return null
    val doPultahu = meze[1].toIntOrNull() ?: return null
    if (od < 0 || doPultahu < od || doPultahu >= tahy.size) return null
    val rozsah = RozsahAnalyzy(od, doPultahu)
    if (vysledky.keys.any { it !in rozsah.pozice }) return null
    val nazev = hodnoty.getValue("nazev").takeIf { it.isNotBlank() && ' ' !in it } ?: return null
    // Bez radku jazyk = prubeh z doby pred J3, tedy cesky; neznamy kod = poskozeny.
    val jazyk = hodnoty["jazyk"]?.let { Jazyk.zKodu(it) ?: return null } ?: Jazyk.CESTINA

    return PrubehAnalyzy(
        vychoziFen = fen,
        tahyUci = tahy,
        motor = motor,
        sekundy = sekundy,
        rozsah = rozsah,
        nazev = nazev,
        uriPgn = hodnoty.getValue("pgn").takeIf { it != NIC && it.isNotBlank() },
        uriCsv = hodnoty.getValue("csv").takeIf { it != NIC && it.isNotBlank() },
        vysledky = vysledky,
        jazyk = jazyk,
    )
}

private val KLICE = setOf("fen", "tahy", "motor", "sekundy", "rozsah", "nazev", "pgn", "csv")

/** Klice, ktere chybet smi (krok J3: jazyk, bez nej cestina). */
private val VOLITELNE = setOf("jazyk")

private fun ctiPozici(hodnota: String): Pair<Int, VysledekPozice>? {
    val casti = hodnota.split(' ')
    if (casti.size < 5) return null
    val index = casti[0].toIntOrNull()?.takeIf { it >= 0 } ?: return null
    val hloubka = if (casti[1] == NIC) null else casti[1].toIntOrNull() ?: return null
    val skore = if (casti[2] == NIC) null else casti[2].toIntOrNull() ?: return null
    val tah = if (casti[3] == NIC) null else casti[3].takeIf { TAH_UCI.matches(it) } ?: return null
    val konec = if (casti[4] == NIC) null
    else KonecPozice.entries.firstOrNull { it.name == casti[4] } ?: return null
    val varianta = casti.drop(5)
    if (varianta.any { it.isEmpty() || it == NIC }) return null
    return index to VysledekPozice(hloubka, skore, tah, varianta, konec)
}
