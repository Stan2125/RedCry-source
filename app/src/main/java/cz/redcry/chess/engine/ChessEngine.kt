package cz.redcry.chess.engine

import cz.redcry.chess.model.Move
import cz.redcry.chess.model.Square
import kotlinx.coroutines.flow.Flow

/**
 * Hranice mezi UI a sachovym motorem.
 *
 * Vetsina metod odpovida prikazum UCI, protoze presne ty pak potecou pres JNI
 * do nativni vrstvy: setPosition -> "position fen ...", go -> "go depth N
 * movetime M",
 * stop -> "stop", events -> radky "info ..." a "bestmove ...".
 *
 * Tri metody v UCI nejsou - legalMoves, applyMove a san. Ty potrebuje UI kvuli
 * zvyrazneni cilovych poli, kvuli tomu, ze autoritativni pozici drzi engine,
 * a kvuli zaznamu partie. Nativni vrstva je vystavi nad svym generatorem tahu.
 * Nikdy je nenahrazuj vlastni implementaci pravidel v Kotlinu.
 */
interface ChessEngine {

    /** Proud udalosti z hledani. Odbira se v ViewModelu. */
    val events: Flow<EngineEvent>

    /** Aktualni pozice ve FEN. Engine je jediny vlastnik stavu partie. */
    fun currentFen(): String

    fun newGame()

    /** Vraci true, pokud byl FEN prijat. */
    fun setPosition(fen: String): Boolean

    /**
     * Pozice jako VYCHOZI FEN partie + zahrane tahy, tedy `position fen
     * <startFen> moves <uci...>`. Vraci true, kdyz motor prijal FEN i vsechny
     * tahy.
     *
     * Proc ne rovnou FEN cilove pozice: RedMAX si pri kazdem `position`
     * vymaze historii partie a naplni ji jen z tahu za `moves`. Holý FEN by
     * mu nechal jedinou pozici a detekce opakovani v hledani by po kazde
     * navigaci v zaznamu prestala fungovat. Pocitadlo 50 tahu se pritom
     * dopocita z vychoziho FEN a tahu stejne.
     */
    fun setPosition(startFen: String, moves: List<Move>): Boolean

    /** Kam smi figura z daneho pole. Prazdny seznam, pokud tam figura neni. */
    fun legalMoves(from: Square): List<Square>

    /** Zahraje tah. Vraci novy FEN, nebo null pokud je tah neplatny. */
    fun applyMove(move: Move): String?

    /**
     * Zapis tahu v algebraicke notaci (SAN) pro AKTUALNI pozici, nebo null,
     * kdyz tah v teto pozici legalni neni.
     *
     * Mapa se plni z jednoho prikazu `sanmoves` po kazde zmene pozice, takze
     * tohle je cisty lookup a nativni strany se nepta - smi se to volat
     * i za beziciho hledani, kdy je zamek na prikazy drzeny.
     *
     * Prevod souradnic na SAN v Kotlinu NEPIS. Rozliseni (Nbd2 proti Nfd2)
     * potrebuje generator tahu, a ten je v C++.
     */
    fun san(move: Move): String?

    /**
     * Opacny smer k [san]: tah podle zapisu v SAN pro AKTUALNI pozici,
     * nebo null, kdyz takovy tah v teto pozici legalni neni.
     *
     * Lookup v tyz mezipameti, tedy zase zadne volani do nativni strany.
     *
     * Tohle je jedine spravne "parsovani" SAN: vygenerovat zapisy vsech
     * legalnich tahu a porovnat s tokenem. Vlastni parser zapisu v Kotlinu
     * NEPIS - potreboval by znat sachova pravidla.
     */
    fun moveFromSan(san: String): Move?

    /**
     * Je tah z [from] na [to] v AKTUALNI pozici promena?
     *
     * Ptame se motoru, ne desky: pesec na predposledni rade jeste neznamena,
     * ze tah na posledni radu je legalni. Lookup v tyz mezipameti jako [san],
     * zadne volani do nativni strany.
     *
     * [san] se k tomu pouzit NESMI - od kroku 6 ma fallback na "+q", takze
     * by odpovedel "ano" i na tah bez figury a rozdil by se ztratil.
     */
    fun jePromena(from: Square, to: Square): Boolean

    /**
     * Stav partie v AKTUALNI pozici: [STAV_NORMALNI], [STAV_MAT], [STAV_PAT]
     * nebo [STAV_REMIZA50]. Cte se z mezipameti, plni se z tehoz prikazu `d`, ze
     * ktereho se bere FEN.
     */
    fun stav(): String

    /**
     * Spusti hledani. Neblokuje - vysledky chodi pres events.
     *
     * Hledani skonci na tom z obou limitu, ktery prijde driv: dokoncena
     * [depth], nebo vyprseny [casMs]. Proto muze pri vysoke hloubce skoncit
     * na hloubce nizsi - rozhodl cas, a to je spravne chovani, ne chyba.
     *
     * [casMs] = 0 znamena BEZ CASOVEHO OMEZENI. Adapter v tom pripade token
     * `movetime` neposle vubec - nula jako hodnota by motoru rekla "nemas
     * cas" a vratil by tah po prvni iteraci.
     */
    fun go(depth: Int, casMs: Int)

    /** Prerusi hledani. Engine posle bestmove s tim, co ma. */
    fun stop()

    /**
     * false jen u motoru, ktery se zaklada na pozadi (StockfishEngine, krok
     * S3/B); dokud je false, ViewModel nevola nic, co by blokovalo na
     * zakladani (applyMove, setPosition). Ostatni motory jsou pripravene
     * hned po konstruktoru.
     */
    fun jePripraven(): Boolean = true

    /**
     * Pocka, az bude motor pripraveny, a NEBLOKUJE volajiciho (krok U7/A):
     * ViewModel po prepnuti motoru ceka v korutine, nez mu posle pozici
     * z historie. true = pripraven, false = zalozeni selhalo nebo byl motor
     * uvolnen (pak prijde nebo uz prisla [EngineEvent.Chyba]).
     *
     * Ostatni motory jsou pripravene hned, proto vychozi okamzite true.
     */
    suspend fun pockejNaPripravenost(): Boolean = true

    /**
     * Sila motoru podle Elo (krok S6): [elo] v mezich [EngineId.minElo]..
     * [EngineId.maxElo], null = plna sila. Plati od pristiho [go].
     *
     * Umi to jen Stockfish (`UCI_LimitStrength` + `UCI_Elo`). Ostatni motory
     * Elo nemaji a volani IGNORUJI - proto vychozi prazdna implementace.
     * ViewModel je navic vola jen u motoru, ktery Elo ma.
     */
    fun nastavSilu(elo: Int?) {}

    fun release()
}

/**
 * Prikazy UCI pro [ChessEngine.nastavSilu] (krok S6): null = plna sila,
 * jinak nejdriv Elo a pak zapnuti, at uz prvni hledani po zapnuti bezi na
 * spravnem Elo. Mimo tridu adapteru ze stejneho duvodu jako [prikazGo].
 */
fun prikazySily(elo: Int?): List<String> =
    if (elo == null) listOf("setoption name UCI_LimitStrength value false")
    else listOf(
        "setoption name UCI_Elo value $elo",
        "setoption name UCI_LimitStrength value true",
    )

/**
 * Prikaz UCI pro jedno hledani. Sdileji ho OBA adaptery - kdyby si ho kazdy
 * skladal sam, rozejdou se: do kroku 12 to tak bylo a Light Red jako jediny
 * `movetime` neposilal vubec, takze zadny casovy strop nemel.
 *
 * Pri [casMs] = 0 se token `movetime` NEPOSILA. Nula jako hodnota by motoru
 * rekla "nemas cas" a vratil by tah po prvni iteraci; motory ji navic samy
 * berou jako "zadny cas zadan" (`if (movetime > 0)` v uci.cpp
 * i red_max_uci.cpp), takze poslat ji je v lepsim pripade zbytecne.
 *
 * Mimo tridu adapteru schvalne, aby na to sel napsat JVM test - uvnitr by ho
 * nesla zavolat bez nativni knihovny.
 */
fun prikazGo(depth: Int, casMs: Int): String =
    if (casMs > 0) "go depth $depth movetime $casMs" else "go depth $depth"

/**
 * Prikaz UCI pro [ChessEngine.setPosition] s tahy. Sdileji ho oba adaptery,
 * ze stejneho duvodu jako [prikazGo]. Bez tahu je to holé `position fen`,
 * token `moves` s prazdnym seznamem se neposila.
 */
fun prikazPosition(startFen: String, moves: List<Move>): String =
    if (moves.isEmpty()) "position fen $startFen"
    else "position fen $startFen moves " + moves.joinToString(" ") { it.toUci() }

/**
 * Tah z odpovedi na `go` ve tvaru "e2e4", nebo null, kdyz zadny neni (mat
 * nebo pat).
 *
 * Motory "zadny tah" hlasi kazdy jinak: Light Red `bestmove 0000` (uci.cpp),
 * RedMAX `bestmove (none)` (red_max_uci.cpp). Oba adaptery i oba mosty berou
 * OBA tvary. Do kroku 13 byla tahle funkce trikrat: v companionu
 * RedMaxBridge (oba tvary) a inline v LightRedBridge.go() - v kroku 13/B
 * smazane jako mrtve - a v LightRedEngine (jen `0000`).
 *
 * Mimo tridy schvalne, ze stejneho duvodu jako [prikazGo]: companion mostu
 * nacita nativni knihovnu, takze by na to v JVM testu nesel sahnout.
 */
fun bestMove(radky: List<String>): String? {
    val tah = radky.firstOrNull { it.startsWith("bestmove ") }
        ?.removePrefix("bestmove ")
        ?.trim()
    return if (tah == null || tah == "(none)" || tah == "0000") null else tah
}

/**
 * Zaverecny souhrn z odpovedi na `go` BEZ uvodniho "info ", pripraveny pro
 * parseInfo adapteru - nebo null, kdyz v odpovedi zadny neni.
 *
 * Filtr je "info depth ", ne jen "info ": kdyz hledani utne cas, prida
 * uci.cpp za souhrn jeste radek `info string hledani preruseno`, a ten by
 * jako POSLEDNI "info " souhrn prebil - parseInfo by na nem vratil null
 * a cele zaverecne hlaseni by se ztratilo. Opraveno v kroku 12 v obou
 * adapterech, od kroku 13 je to jedna funkce, at se to nerozejde znovu.
 */
fun zaverecneInfo(radky: List<String>): String? =
    radky.lastOrNull { it.startsWith("info depth ") }?.removePrefix("info ")

/** Ve hre se da normalne pokracovat. */
const val STAV_NORMALNI = "normalni"

/** Strana na tahu je matovana - prohrala. */
const val STAV_MAT = "mat"

/** Strana na tahu nema tah a neni v sachu - remiza. */
const val STAV_PAT = "pat"

/**
 * Aspon 100 pultahu bez brani a tahu pescem a strana na tahu MA tah - mat
 * a pat maji prednost. Hlasi to jen prikaz `d` pro UI, hledani o pravidle
 * nevi. Proc padesat tahu jako konec a ne jen pravo reklamovat, viz
 * docs/POZNAMKY-UI.md.
 */
const val STAV_REMIZA50 = "remiza50"

sealed interface EngineEvent {
    /** Prubezny vysledek jedne iterace prohlubovani. */
    data class Info(
        val depth: Int,
        val scoreCp: Int?,
        val nodes: Long,
        val pv: List<Move>,
    ) : EngineEvent

    /** Konec hledani. Tah muze byt null, pokud zadny neexistuje. */
    data class BestMove(val move: Move?) : EngineEvent

    /**
     * Motor se nepodarilo zalozit: dosla pamet, predchozi instance se
     * neuvolnila vcas, nebo vyprselo cekani na zalozeni. Od kroku J2 nese
     * [druh] chyby a jmeno [motor]; vetu pro uzivatele sklada ViewModel
     * z tabulky textu (cz.redcry.chess.texty.textChybyMotoru).
     *
     * Vysila ji JEN motor, ktery se zaklada na pozadi ([StockfishEngine],
     * krok S3/B). Ostatni motory selhani hlasi vyjimkou z konstruktoru.
     *
     * Po ni uz z motoru NESMI prijit [BestMove] a motor je k nicemu. Uvolni ho
     * PRIJEMCE (ViewModel volanim release), adapter sam sebe ne - release
     * je jedine misto, kde se motor uvolnuje, a musi zustat jedno.
     */
    data class Chyba(val druh: ChybaMotoru, val motor: String) : EngineEvent
}

/**
 * Druh selhani motoru (krok J2). V engine/ neni zadny text pro uzivatele:
 * vetu slozi ViewModel nebo analyza z aktualni tabulky textu.
 */
sealed interface ChybaMotoru {
    /** Nespustil se: chyba mostu, dosla pamet. */
    data object Nespustil : ChybaMotoru

    /** Zalozeni nedobehlo do [sekund] s (vyprselo cekani). */
    data class NespustilDo(val sekund: Int) : ChybaMotoru

    /** Predchozi instance se neuvolnila do [sekund] s. */
    data class Neuvolnil(val sekund: Int) : ChybaMotoru

    /** Nativni instance uz existuje (nativeNew vratil 0). */
    data object UzBezi : ChybaMotoru
}

/**
 * Most nedostal nativni instanci, protoze jina uz bezi (nebo dosla pamet).
 * Zprava je jen pro log; ViewModel z ni nic neukazuje (krok J2).
 */
class MotorUzBeziException(val motor: String) :
    IllegalStateException("$motor: nativeNew vratil 0 (instance uz bezi nebo dosla pamet)")
