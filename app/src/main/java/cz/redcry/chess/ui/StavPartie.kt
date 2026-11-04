package cz.redcry.chess.ui

import cz.redcry.chess.engine.ChessEngine
import cz.redcry.chess.engine.STAV_MAT
import cz.redcry.chess.engine.STAV_PAT
import cz.redcry.chess.engine.STAV_REMIZA50
import cz.redcry.chess.model.Move
import cz.redcry.chess.model.PieceType
import cz.redcry.chess.model.Position
import cz.redcry.chess.model.Side
import cz.redcry.chess.model.Square
import cz.redcry.chess.model.squareFromName

// Stav hledani, konec partie, opakovani pozice a tah odboceni.
// Vzniklo v kroku R1 presunem z GameViewModel.kt.

/**
 * Stav jednoho hledani. Driv to popisovaly dva nezavisle booleany - `thinking`
 * ve stavu a `prerusenoUzivatelem` vedle - a udalost BestMove nenese zadnou
 * identitu, takze se nedalo poznat, ke kteremu hledani patri.
 *
 * | z              | podnet     | do             | bestmove se        |
 * |----------------|------------|----------------|--------------------|
 * | KLID           | Premyslej  | HLEDA          |                    |
 * | HLEDA          | fialove    | DOBIHA_ZAHRAT  |                    |
 * | HLEDA          | BestMove   | KLID           | zahraje            |
 * | DOBIHA_ZAHRAT  | BestMove   | KLID           | zahraje            |
 * | KLID           | Analyza    | ANALYZA        |                    |
 * | ANALYZA        | BestMove   | ANALYZA        | preda se rizeni    |
 * | ANALYZA        | konec      | KLID           |                    |
 *
 * Jadro opravy je v tom, ze z DOBIHA_ZAHRAT vede ven JEDINE prichod
 * BestMove, tedy skutecny konec hledani - ne stisk tlacitka.
 *
 * Do kroku 18 tu byl jeste DOBIHA_ZAHODIT pro cervene Zastav, ktere tah
 * zahodilo. Tlacitko zmizelo a stav s nim; jinak na nej nic nevedlo.
 */
enum class StavHledani {
    /** Nehleda se, deska i pozice jsou volne. */
    KLID,

    /** Hledani bezi. */
    HLEDA,

    /** Uzivatel dal Hraj nejlepsi - bestmove se po prichodu zahraje. */
    DOBIHA_ZAHRAT,

    /**
     * Bezi analyza partie (krok A1/B, docs/POZNAMKY-ANALYZA.md). BestMove
     * jde do [cz.redcry.chess.analyza.RizeniAnalyzy] a nikdy se nezahraje.
     * Stav trva od spusteni po obnovu motoru a desky, i pres prepnuti motoru.
     */
    ANALYZA,
}

/** Jak skoncila partie v pozici na desce. Null = neskoncila. */
enum class KonecPartie { MAT_VYHRAL_BILY, MAT_VYHRAL_CERNY, PAT, REMIZA_50, REMIZA_OPAKOVANIM }

/**
 * Klic pozice pro trojí opakovani podle FIDE 9.2.2 (krok U4): prvni ctyri
 * pole FEN - rozestaveni, strana na tahu, rosady a pole mimochodem - bez
 * obou pocitadel. Pole mimochodem jen tehdy, kdyz braní mimochodem opravdu
 * jde, jinak "-".
 *
 * Proc: motory pisou pole kazdy jinak - RedMAX a Light Red po kazdem
 * dvojkroku, Crafty kdyz vedle stoji souperuv pesec, Stockfish jen pri
 * legalnim brani. Do U4 se porovnavalo pole tak, jak prislo, a pozice po
 * dvojkroku se od tehoz rozestaveni jinak dosazeneho lisila, takze se
 * opakovani poznalo pozde (stejna chyba jako U3 v rozhodcim zapasu).
 *
 * Jestli brani jde, rika MOTOR, ne Kotlin: [legalni] jsou legalni tahy
 * AKTUALNI pozice z mezipameti adapteru ([ChessEngine.legalMoves]). Brani
 * mimochodem = tah pescem strany na tahu na pole mimochodem; ktere pole
 * nese pesce, je jen cteni desky z FEN, zadne pravidlo. Jiny pesci tah na
 * to pole byt nemuze (pole pred nim zabira pesec, ktery dvojkrok udelal).
 *
 * Volat jen ve chvili, kdy motor na [fen] opravdu stoji - jinak [legalni]
 * patri jine pozici. Ciste, aby na to sel napsat JVM test.
 */
fun klicOpakovani(fen: String, legalni: (Square) -> List<Square>): String {
    val pole = fen.trim().split(' ')
    val ctyri = pole.take(4)
    val ep = ctyri.getOrNull(3)
    if (ep == null || ep == "-") return ctyri.joinToString(" ")
    val cil = squareFromName(ep)
    val pozice = Position.fromFen(fen)
    val jde = cil != null && pozice != null && (0 until 64).any { sq ->
        val f = pozice[sq]
        f != null && f.type == PieceType.PAWN && f.side == pozice.sideToMove && cil in legalni(sq)
    }
    return (ctyri.take(3) + (if (jde) ep else "-")).joinToString(" ")
}

/**
 * Kolikrat se pozice `klice[index]` vyskytuje v `klice[0..index]`, vcetne
 * sebe. Porovnavaji se prvni ctyri pole - ViewModel sem dava klice
 * z [klicOpakovani] (od kroku U4), u holeho FEN jsou to prvni ctyri pole
 * vcetne pole mimochodem tak, jak je. Viz docs/POZNAMKY-UI.md.
 */
fun pocetVyskytu(history: List<String>, index: Int): Int {
    if (index !in history.indices) return 0
    fun klic(fen: String) = fen.trim().split(' ').take(4)
    val hledany = klic(history[index])
    return (0..index).count { klic(history[it]) == hledany }
}

/**
 * Stav od motoru ([STAV_MAT], [STAV_PAT], [STAV_REMIZA50], jinak normalni)
 * a pocet vyskytu pozice ([pocetVyskytu]) na [KonecPartie]. Mat znamena, ze
 * prohrala strana NA TAHU.
 *
 * Poradi je zavazne: mat a pat > 50 tahu > opakovani. Opakovani motor
 * nehlasi, pocita ho Kotlin z historie, a tahle funkce je porad JEDINE misto,
 * kde se o konci partie rozhoduje - obrazovka i vysledek v PGN jdou pres ni.
 *
 * Tatáž logika slouzi dvema vecem, ktere se ptaji na RUZNE pozice: oznameni
 * na obrazovce na pozici pod kurzorem, vysledek v PGN na konec zaznamu.
 * Proto je tu jen prevod a pozici si vybira volajici.
 *
 * Od kroku 16 motor hlasi i remizu podle pravidla 50 tahu, od kroku 17 se
 * pocita trojí opakovani. Nedostatek materialu se nehlasi - viz
 * docs/POZNAMKY-UI.md.
 */
fun konecZeStavu(stav: String, naTahu: Side?, vyskytu: Int = 1): KonecPartie? = when {
    stav == STAV_MAT ->
        if (naTahu == Side.WHITE) KonecPartie.MAT_VYHRAL_CERNY else KonecPartie.MAT_VYHRAL_BILY
    stav == STAV_PAT -> KonecPartie.PAT
    stav == STAV_REMIZA50 -> KonecPartie.REMIZA_50
    vyskytu >= 3 -> KonecPartie.REMIZA_OPAKOVANIM
    else -> null
}

/**
 * Tah odboceni (krok U6/B): prvni index, kde se [aktualni] tahy lisi od
 * [referencni]ch. Chybejici tah se pocita jako jiny, takze:
 *  - partie, ktera jen pokracuje za koncem reference, odbocila na konci
 *    reference (`referencni.size`),
 *  - zaznam, ktery je jen kratsi, odbocil na svem konci. Do kroku U7/B
 *    k tomu vedl tyz tah, jaky je v referenci, zahrany ze starsi pozice;
 *    od U7/B takovy tah zaznam neurizne a sem se z aplikace nedojde.
 * Null = zaznamy jsou shodne, neni k cemu se vracet.
 *
 * Ciste, aby na to sel napsat JVM test.
 */
fun tahOdboceni(aktualni: List<Move>, referencni: List<Move>): Int? =
    (0 until maxOf(aktualni.size, referencni.size))
        .firstOrNull { aktualni.getOrNull(it) != referencni.getOrNull(it) }
