package cz.redcry.chess.engine

import cz.redcry.chess.model.Move
import cz.redcry.chess.model.PieceType
import cz.redcry.chess.model.Position
import cz.redcry.chess.model.Square
import cz.redcry.chess.model.squareFromName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors

/**
 * Motor Crafty 18.15 (Robert Hyatt) v podobe RedCrafty 4.81. Treti engine
 * v RedCry, nejsilnejsi z vlastnich.
 *
 * Prevedeny z C++ Builderu (RedCrafty_481_f.cpp) do C++17, viz
 * docs/POZNAMKY-CRAFTY.md. Dvojce [RedMaxEngine]: stejne prikazy UCI, stejna
 * mezipamet, stejne vlakno pro hledani.
 *
 * JEDINA INSTANCE. Nativni stav Crafty jsou globaly, takze zalozit druhy
 * [CraftyEngine], dokud prvni zije, skonci vyjimkou z [CraftyBridge]
 * (srozumitelnou, ne padem). Prepnuti motoru ve ViewModelu stary motor
 * uvolni drive, nez zalozi novy, takze Crafty -> RedMax -> Crafty funguje.
 *
 * ROZDELENI PRACE MEZI VLAKNA
 *
 * Nativni strana ma jeden zamek na prikazy. [go] proto bezi na vlastnim
 * jednovlaknovem dispatcheru a neblokuje. Vsechno ostatni je synchronni
 * a obsluhuje se z volajiciho vlakna - jsou to volani v radu mikrosekund.
 *
 * [currentFen], [legalMoves], [san], [moveFromSan] i [stav] se NIKDY neptaji
 * nativni strany. Ctou se
 * z mezipameti, ktera se obnovi po kazde zmene pozice. Diky tomu je UI
 * vlakno nemuze zablokovat ani kdyz engine premysli.
 *
 * Zmeny pozice ([setPosition], [applyMove], [newGame]) se za beziciho
 * hledani odmitnou. UI ma v te dobe desku stejne zamcenou a tlacitko
 * ukazuje "Hraj nejlepsi".
 *
 * ROZDIL PROTI REDMAXU
 *
 * Zadny v Kotlinu. Rozdily jsou jen na nativni strane (`bestmove 0000` misto
 * `bestmove (none)`, strop hloubky 60) a sdilene funkce z ChessEngine.kt
 * ([prikazGo], [bestMove], [zaverecneInfo]) je pokryvaji. Zbytek ma byt
 * s [RedMaxEngine] shodny radek po radku.
 */
class CraftyEngine(private val scope: CoroutineScope) : ChessEngine {

    private val bridge = CraftyBridge()

    // Prioritu je nutne nastavit ZEVNITR beziciho vlakna - setThreadPriority
    // bez prvniho argumentu plati na aktualni vlakno. Bez toho se zdedi po
    // tvurci a Android by engine mohl odsunout na mala jadra.
    private val engineThread = Executors.newSingleThreadExecutor { r ->
        Thread({
            android.os.Process.setThreadPriority(
                android.os.Process.THREAD_PRIORITY_DEFAULT
            )
            r.run()
        }, "Crafty").apply { isDaemon = true }
    }
    private val engineDispatcher = engineThread.asCoroutineDispatcher()

    private val _events = MutableSharedFlow<EngineEvent>(extraBufferCapacity = 64)
    override val events: SharedFlow<EngineEvent> = _events

    @Volatile private var searching = false
    private var searchJob: Job? = null

    private var cachedFen: String = Position.START_FEN
    private var cachedMoves: Map<Square, List<Square>> = emptyMap()

    /** Zapis UCI -> zapis SAN pro aktualni pozici. Plni se z jednoho `sanmoves`. */
    private var cachedSan: Map<String, String> = emptyMap()

    /** Obracena [cachedSan]: zapis SAN -> tah. Plni se z tehoz `sanmoves`. */
    private var cachedZeSan: Map<String, Move> = emptyMap()

    /** Stav partie v aktualni pozici. Plni se z tehoz `d`, ze ktereho je FEN. */
    private var cachedStav: String = STAV_NORMALNI

    init {
        bridge.command("uci")
        bridge.command("ucinewgame")
        bridge.command("position startpos")
        obnovMezipamet()
    }

    // --- cteni stavu, vzdy z mezipameti ------------------------------------

    override fun currentFen(): String = cachedFen

    override fun legalMoves(from: Square): List<Square> =
        cachedMoves[from].orEmpty()

    /**
     * Cisty lookup v mezipameti, zadne volani do nativni strany - proto se to
     * smi volat i za beziciho hledani, kdy je zamek na prikazy drzeny.
     *
     * Promena bez urcene figury: `applyMove` necha nativni stranu doplnit
     * damu, takze se tady pri nenalezu zkusi totez. Bez toho by tah cloveka
     * na posledni radu nemel v zaznamu partie zapis.
     */
    override fun san(move: Move): String? {
        val uci = move.toUci()
        return cachedSan[uci] ?: if (uci.length == 4) cachedSan[uci + "q"] else null
    }

    /**
     * Opacny smer: zapis SAN na tah. Zase jen lookup v mezipameti.
     *
     * Kdyz presna shoda selze, zkusi se porovnani bez koncoveho `+` a `#`.
     * Nektere programy je do zapisu nepisou a je to levna odolnost - mapa ma
     * radove desitky prvku a na tuhle vetev se jde jen pri nacitani partie.
     * Srazit se dva tahy na jeden zapis nemuzou: rozliseni (Nbd2 proti Nfd2)
     * se do zapisu dostane driv nez znamenko sachu.
     */
    override fun moveFromSan(san: String): Move? {
        val hledany = san.trim()
        cachedZeSan[hledany]?.let { return it }
        val hole = bezZnamenka(hledany)
        return cachedZeSan.entries.firstOrNull { bezZnamenka(it.key) == hole }?.value
    }

    override fun stav(): String = cachedStav

    /** Proc ne pres [san], viz [ChessEngine.jePromena]. */
    override fun jePromena(from: Square, to: Square): Boolean =
        cachedSan.containsKey(Move(from, to).toUci() + "q")

    // --- zmeny pozice -------------------------------------------------------

    override fun newGame() {
        if (searching) { stop(); return }
        bridge.command("ucinewgame")
        bridge.command("position startpos")
        obnovMezipamet()
    }

    override fun setPosition(fen: String): Boolean = setPosition(fen, emptyList())

    override fun setPosition(startFen: String, moves: List<Move>): Boolean {
        if (searching) return false
        val odpoved = bridge.command(prikazPosition(startFen, moves))
        if (odpoved.any { it.contains("neplatne") }) return false
        obnovMezipamet()
        return true
    }

    /**
     * Zahraje tah a vrati novy FEN, nebo null pokud je tah neplatny.
     *
     * Pozor na promenu: kdyz [Move.promotion] zustane null a tah je promena,
     * nativni strana doplni damu. Je to zamerne - UI se na figuru zepta jen
     * kdyz chce.
     */
    override fun applyMove(move: Move): String? {
        if (searching) return null
        val ok = bridge.applyMove(move.toUci())
        if (!ok) return null
        obnovMezipamet()
        return cachedFen
    }

    // --- hledani ------------------------------------------------------------

    override fun go(depth: Int, casMs: Int) {
        if (searching) return
        searching = true

        searchJob = scope.launch {
            // sonda: kazdych 80 ms se zepta, jakou hloubku uz engine dokoncil
            val sonda = launch {
                var posledni = ""
                while (isActive) {
                    delay(80)
                    val s = bridge.searchInfo()
                    if (s.isNotEmpty() && s != posledni) {
                        posledni = s
                        parseInfo(s)?.let { _events.emit(it) }
                    }
                }
            }

            // Hloubka i casovy strop zaroven: hledani skonci na tom, co
            // prijde driv. Skladani prikazu je spolecne vsem motorum, viz
            // [prikazGo].
            val prikaz = prikazGo(depth, casMs)

            val radky = try {
                withContext(engineDispatcher) { bridge.command(prikaz) }
            } finally {
                sonda.cancel()
                searching = false
            }

            // Zaverecny info radek a bestmove - oboje sdilenymi funkcemi,
            // proc filtr "info depth " a proc dva tvary "zadny tah", viz
            // [zaverecneInfo] a [bestMove].
            zaverecneInfo(radky)
                ?.let { parseInfo(it) }
                ?.let { _events.emit(it) }

            // Tah se ZAMERNE nezahraje. Autoritativni pozici drzi engine,
            // ale kdo tah zahraje, rozhoduje ViewModel volanim applyMove.
            // Kdyby ho zahral i adapter, provedl by se dvakrat. (Button3Click
            // v RedCrafty si tah zahraval sam, MakeMoveRoot na r. 206;
            // nativni hledej() to uz nedela.)
            _events.emit(EngineEvent.BestMove(bestMove(radky)?.let { parseMove(it) }))
        }
    }

    override fun stop() {
        bridge.stop()
    }

    override fun release() {
        searchJob?.cancel()
        bridge.stop()
        engineThread.shutdown()
        bridge.close()
    }

    // --- pomocne ------------------------------------------------------------

    private fun obnovMezipamet() {
        // Prikaz `d` vrati na prvnim radku FEN a na druhem "stav ...". Je to
        // jedno volani misto jednoho (drive `fen()`), takze stav partie
        // prijde zadarmo.
        val deska = bridge.command("d")
        cachedFen = deska.firstOrNull() ?: cachedFen
        cachedStav = deska.getOrNull(1)?.removePrefix("stav ")?.trim() ?: STAV_NORMALNI

        cachedMoves = bridge.legalMoves()
            .mapNotNull { uci ->
                val od = squareFromName(uci.take(2)) ?: return@mapNotNull null
                val kam = squareFromName(uci.substring(2, 4)) ?: return@mapNotNull null
                od to kam
            }
            .groupBy({ it.first }, { it.second })
            .mapValues { (_, cile) -> cile.distinct() }

        // Jeden radek dvojic "<uci>:<san>", oddelene mezerou. Dvojtecka se
        // nevyskytuje ani v jednom z obou zapisu, takze staci prvni vyskyt.
        // Prazdny radek (mat nebo pat) da command() jako prazdny seznam.
        val zapisy = bridge.command("sanmoves")
            .firstOrNull()
            .orEmpty()
            .trim()
            .split(' ')
            .mapNotNull { dvojice ->
                val delic = dvojice.indexOf(':')
                if (delic <= 0 || delic == dvojice.length - 1) null
                else dvojice.substring(0, delic) to dvojice.substring(delic + 1)
            }
        cachedSan = zapisy.toMap()
        // Obracena mapa z TEHOZ jednoho volani, zadny dalsi prikaz do C++.
        cachedZeSan = zapisy
            .mapNotNull { (uci, san) -> parseMove(uci)?.let { san to it } }
            .toMap()
    }

    /** Zapis bez koncoveho sachu nebo matu - `Ra8+` na `Ra8`. */
    private fun bezZnamenka(san: String): String = san.trimEnd('+', '#')

    /** "e2e4" nebo "a7a8q" na [Move]. */
    private fun parseMove(uci: String): Move? {
        if (uci.length < 4) return null
        val od = squareFromName(uci.take(2)) ?: return null
        val kam = squareFromName(uci.substring(2, 4)) ?: return null
        val promena = when (uci.getOrNull(4)?.lowercaseChar()) {
            'q' -> PieceType.QUEEN
            'r' -> PieceType.ROOK
            'b' -> PieceType.BISHOP
            'n' -> PieceType.KNIGHT
            else -> null
        }
        return Move(od, kam, promena)
    }

    /**
     * "depth 5 score cp 12 nodes 15632 pv e2e4 e7e5" na [EngineEvent.Info].
     *
     * Stejny jako u RedMAXu - crafty_uci.cpp hlasi totez co
     * red_max_uci.cpp. Umi `score mate N` - prepocita se na velke cp, aby
     * UI nemuselo znat dva tvary. A `pv` nese celou variantu, ne jen prvni
     * tah, takze se cte az do konce radku.
     *
     * Klic "time" ma jen zaverecny info radek od enginu; prubezne radky ze
     * sondy ho neobsahuji. Vypis do logu proto visi na [casMs] - jinak by
     * kazde hledani zaplavilo log jednou hlaskou za dokoncenou hloubku.
     */
    private fun parseInfo(s: String): EngineEvent.Info? {
        val slova = s.trim().split(' ')
        var depth: Int? = null
        var score: Int? = null
        var nodes = 0L
        var casMs: Long? = null
        val pv = mutableListOf<Move>()

        var i = 0
        while (i < slova.size) {
            when (slova[i]) {
                "depth" -> depth = slova.getOrNull(++i)?.toIntOrNull()
                "nodes" -> nodes = slova.getOrNull(++i)?.toLongOrNull() ?: 0L
                "time" -> casMs = slova.getOrNull(++i)?.toLongOrNull()
                "score" -> when (slova.getOrNull(i + 1)) {
                    "cp" -> { i += 2; score = slova.getOrNull(i)?.toIntOrNull() }
                    "mate" -> {
                        i += 2
                        val n = slova.getOrNull(i)?.toIntOrNull()
                        if (n != null) score = if (n > 0) SKORE_MATU - n else -SKORE_MATU - n
                    }
                }
                "pv" -> {
                    // zbytek radku jsou tahy hlavni varianty
                    while (i + 1 < slova.size) {
                        val m = parseMove(slova[i + 1]) ?: break
                        pv.add(m)
                        i++
                    }
                }
            }
            i++
        }
        val d = depth ?: return null
        if (casMs != null) {
            android.util.Log.d("Crafty", "hloubka $d, $nodes uzlu, $casMs ms")
        }
        return EngineEvent.Info(d, score, nodes, pv)
    }

    private companion object {
        /** Matove skore v setinach pesce, aby UI nemuselo znat `score mate`. */
        const val SKORE_MATU = 30_000
    }
}
