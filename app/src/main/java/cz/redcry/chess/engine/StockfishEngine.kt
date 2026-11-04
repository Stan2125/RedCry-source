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
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.resume
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import java.util.concurrent.CompletableFuture
import java.util.concurrent.Executors
import java.util.concurrent.RejectedExecutionException
import java.util.concurrent.Semaphore
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException

/**
 * Motor Stockfish 19. Ctvrty engine v RedCry a nejsilnejsi.
 *
 * Kod Stockfishe je prevzaty beze zmeny (app/src/main/cpp/stockfish/PUVOD.md),
 * vrstva UCI je stockfish_uci.cpp, viz docs/POZNAMKY-STOCKFISH.md. Dvojce
 * [CraftyEngine]: stejne prikazy UCI, stejna mezipamet, stejne vlakno pro
 * hledani. Tah adapter NEHRAJE, jen vysle [EngineEvent.BestMove].
 *
 * ROZDIL PROTI CRAFTY: MOST SE ZAKLADA NA VLAKNE MOTORU
 *
 * nativeNew trva 219-222 ms na Xiaomi 17 a 303 ms v emulatoru (medián z 10,
 * POZNAMKY-STOCKFISH.md, krok S3/A): Engine nahrava sit do pameti. To je na
 * hlavnim vlakne moc, a proto konstruktor jen posle zalozeni na vlakno motoru
 * a hned se vrati. Do dokonceni zalozeni adapter bezpecne odpovida:
 *
 *  - [currentFen], [legalMoves], [san], [stav] ctou vychozi snimek (vychozi
 *    FEN, zadne tahy, stav normalni), coz je presne pozice, ve ktere
 *    zalozeni skonci,
 *  - [newGame] nic nedela (zalozeni konci ve vychozi pozici),
 *  - [go] pocka ve sve korutine (neblokuje),
 *  - [applyMove] a [setPosition] vraci vysledek synchronne, a tak pockaji
 *    blokujicne, NEJVYS 10 s. Delka cekani jde do logu,
 *  - [stop] pred dokoncenim si zapamatuje jen tehdy, kdyz ceka go
 *    ([StopPredZalozenim]); go pak hleda do hloubky 1.
 *
 * Kdyz zalozeni selze (dosla pamet, predchozi instance se neuvolnila do
 * 10 s, cekani vyprselo), vysle adapter [EngineEvent.Chyba] a zadny
 * BestMove uz po ni neprijde. Uvolnit ho musi prijemce (ViewModel).
 *
 * JEDINA INSTANCE. Most smi mit nejvys jednu nativni instanci (pamet: sit,
 * hash, vlakna - hlavicka stockfish_jni_bridge.cpp). Pri prepnuti
 * Stockfish -> Crafty -> Stockfish muze nove zalozeni prijit driv, nez
 * stary adapter svuj most zavre (zavira ho na svem vlakne), a proto se ceka
 * na semafor [ZIVA_INSTANCE], nejvys 10 s.
 *
 * Mezipamet je jeden @Volatile [Snimek]: plni ho vlakno motoru (zalozeni)
 * i volajici vlakno (applyMove, setPosition), cte UI.
 */
class StockfishEngine(private val scope: CoroutineScope) : ChessEngine {

    // Prioritu je nutne nastavit ZEVNITR beziciho vlakna - setThreadPriority
    // bez prvniho argumentu plati na aktualni vlakno. Bez toho se zdedi po
    // tvurci a Android by engine mohl odsunout na mala jadra.
    private val engineThread = Executors.newSingleThreadExecutor { r ->
        Thread({
            android.os.Process.setThreadPriority(
                android.os.Process.THREAD_PRIORITY_DEFAULT
            )
            r.run()
        }, "Stockfish").apply { isDaemon = true }
    }
    private val engineDispatcher = engineThread.asCoroutineDispatcher()

    private val _events = MutableSharedFlow<EngineEvent>(extraBufferCapacity = 64)
    override val events: SharedFlow<EngineEvent> = _events

    @Volatile private var searching = false
    private var searchJob: Job? = null

    /** Most, az je zalozeny. Do te doby a po selhani null. */
    @Volatile private var bridge: StockfishBridge? = null

    /** Dokonci ho zalozeni: most, nebo vyjimka pri selhani. */
    private val zalozeni = CompletableFuture<StockfishBridge>()

    @Volatile private var uvolneno = false
    @Volatile private var chybaOhlasena = false

    private val stopPredZalozenim = StopPredZalozenim()

    /** FEN, tahy, SAN a stav jedne pozice. Meni se vzdy cely naraz. */
    private class Snimek(
        val fen: String,
        val tahy: Map<Square, List<Square>>,
        /** Zapis UCI -> zapis SAN. Plni se z jednoho `sanmoves`. */
        val san: Map<String, String>,
        /** Obracena [san]: zapis SAN -> tah. Z tehoz `sanmoves`. */
        val zeSan: Map<String, Move>,
        /** Stav partie. Z tehoz `d`, ze ktereho je FEN. */
        val stav: String,
    )

    @Volatile private var snimek = Snimek(
        VYCHOZI_FEN, emptyMap(), emptyMap(), emptyMap(), STAV_NORMALNI
    )

    init {
        engineThread.execute { zaloz() }
    }

    // --- zakladani a uvolneni (vlakno motoru) ------------------------------

    /**
     * Do logu jde rozpis doby (krok S3/B, bod 2): cekani na semafor,
     * nativeNew (konstruktor mostu), uci + ucinewgame + position
     * a naplneni mezipameti, vse v ms.
     */
    private fun zaloz() {
        val t0 = System.nanoTime()
        fun ms(od: Long) = (System.nanoTime() - od) / 1_000_000
        if (!ZIVA_INSTANCE.tryAcquire(CEKANI_S, TimeUnit.SECONDS)) {
            selhani(ChybaMotoru.Neuvolnil(CEKANI_S.toInt()), "semafor nedostal do $CEKANI_S s")
            return
        }
        if (uvolneno) {
            ZIVA_INSTANCE.release()
            zalozeni.completeExceptionally(IllegalStateException("uvolneno pred zalozenim"))
            return
        }
        val t1 = System.nanoTime()
        val b = try {
            StockfishBridge()
        } catch (e: Throwable) {
            // IllegalStateException z mostu (nativeNew = 0), ale taky
            // UnsatisfiedLinkError nebo OutOfMemoryError - nic z toho
            // nesmi shodit vlakno bez hlaseni.
            ZIVA_INSTANCE.release()
            selhani(if (e is MotorUzBeziException) ChybaMotoru.UzBezi else ChybaMotoru.Nespustil, e.toString())
            return
        }
        val t2 = System.nanoTime()
        val t3: Long
        try {
            b.command("uci")
            b.command("ucinewgame")
            b.command("position startpos")
            t3 = System.nanoTime()
            obnovMezipamet(b)
        } catch (e: Throwable) {
            b.close()
            ZIVA_INSTANCE.release()
            selhani(if (e is MotorUzBeziException) ChybaMotoru.UzBezi else ChybaMotoru.Nespustil, e.toString())
            return
        }
        // Kontrola z kroku S3/B: FEN vychoziho snimku (vraceny pred
        // dokoncenim, z nej je history[0]) musi byt bajtove stejny jako FEN,
        // ktery Stockfish vrati z `d` pro vychozi pozici.
        val fenZMostu = snimek.fen
        android.util.Log.d(
            "Stockfish",
            "fen po zalozeni '$fenZMostu', vychozi snimek '$VYCHOZI_FEN', shodne ${fenZMostu == VYCHOZI_FEN}",
        )
        bridge = b
        zalozeni.complete(b)
        android.util.Log.d(
            "Stockfish",
            "zalozeni ${ms(t0)} ms: semafor ${(t1 - t0) / 1_000_000}, nativeNew ${(t2 - t1) / 1_000_000}, " +
                "uci+ucinewgame+position ${(t3 - t2) / 1_000_000}, mezipamet ${ms(t3)}",
        )
    }

    /** [detail] jen do logu, na obrazovku jde [druh] (krok J2). */
    private fun selhani(druh: ChybaMotoru, detail: String) {
        zalozeni.completeExceptionally(IllegalStateException(detail))
        ohlasChybu(druh, detail)
    }

    /**
     * Vysle [EngineEvent.Chyba], nejvys jednou za zivot adapteru. [events] je
     * SharedFlow bez replay a ViewModel se k odberu prihlasi az na hlavnim
     * vlakne - kdyby zalozeni selhalo hned, udalost by nikdo nezachytil.
     * Proto se ceka na odberatele, nejvys 10 s.
     */
    private fun ohlasChybu(druh: ChybaMotoru, detail: String) {
        if (chybaOhlasena || uvolneno) return
        chybaOhlasena = true
        android.util.Log.d("Stockfish", "chyba: $druh ($detail)")
        scope.launch {
            withTimeoutOrNull(CEKANI_S * 1000) { _events.subscriptionCount.first { it > 0 } }
            if (!uvolneno) _events.emit(EngineEvent.Chyba(druh, EngineId.STOCKFISH.label))
        }
    }

    /** Na vlakne motoru, jako posledni uloha - po zalozeni i po hledani. */
    private fun zavri() {
        val b = bridge
        bridge = null
        if (b != null) {
            b.close()
            ZIVA_INSTANCE.release()
        }
    }

    /**
     * Most bez cekani, nebo po cekani na zalozeni (nejvys 10 s), nebo null.
     * Vyprseni = [EngineEvent.Chyba]. Po chybe uz se neceka znovu.
     */
    private fun pockejNaMost(kdo: String): StockfishBridge? {
        bridge?.let { return it }
        if (chybaOhlasena || uvolneno) return null
        val t0 = System.nanoTime()
        val b = try {
            zalozeni.get(CEKANI_S, TimeUnit.SECONDS)
        } catch (e: TimeoutException) {
            ohlasChybu(ChybaMotoru.NespustilDo(CEKANI_S.toInt()), "zalozeni nedobehlo do $CEKANI_S s")
            null
        } catch (e: Exception) {
            // selhani zalozeni - Chyba uz je ohlasena ze zaloz()
            null
        }
        android.util.Log.d("Stockfish", "$kdo cekal na zalozeni ${(System.nanoTime() - t0) / 1_000_000} ms")
        return b
    }

    // --- cteni stavu, vzdy z mezipameti ------------------------------------

    /**
     * Z mezipameti; pred dokoncenim zakladani [VYCHOZI_FEN].
     * Patri k neblokujicim volanim, ktera ViewModel smi volat i pred
     * dokoncenim zakladani (test prepnutiNaNepripravenyMotorVolaJenNeblokujiciMetody):
     * nejde pres most a neceka na zakladani.
     */
    override fun currentFen(): String = snimek.fen

    override fun legalMoves(from: Square): List<Square> =
        snimek.tahy[from].orEmpty()

    /**
     * Cisty lookup v mezipameti, zadne volani do nativni strany - proto se to
     * smi volat i za beziciho hledani, kdy je zamek na prikazy drzeny.
     *
     * Promena bez urcene figury: `applyMove` necha nativni stranu doplnit
     * damu, takze se tady pri nenalezu zkusi totez.
     */
    override fun san(move: Move): String? {
        val uci = move.toUci()
        val san = snimek.san
        return san[uci] ?: if (uci.length == 4) san[uci + "q"] else null
    }

    /**
     * Opacny smer: zapis SAN na tah. Zase jen lookup v mezipameti. Kdyz
     * presna shoda selze, zkusi se porovnani bez koncoveho `+` a `#`, jako
     * u [CraftyEngine.moveFromSan].
     */
    override fun moveFromSan(san: String): Move? {
        val hledany = san.trim()
        val zeSan = snimek.zeSan
        zeSan[hledany]?.let { return it }
        val hole = bezZnamenka(hledany)
        return zeSan.entries.firstOrNull { bezZnamenka(it.key) == hole }?.value
    }

    /**
     * Z mezipameti; pred dokoncenim zakladani [STAV_NORMALNI].
     * Patri k neblokujicim volanim, ktera ViewModel smi volat i pred
     * dokoncenim zakladani (test prepnutiNaNepripravenyMotorVolaJenNeblokujiciMetody):
     * nejde pres most a neceka na zakladani.
     */
    override fun stav(): String = snimek.stav

    /**
     * true, az je most zalozeny a mezipamet naplnena: zaloz() nastavi
     * [bridge] az po obnovMezipamet. Po selhani nebo po release je false.
     */
    override fun jePripraven(): Boolean = bridge != null

    /**
     * Krok U7/A: totez cekani jako [pockejNaMost], ale bez blokovani
     * volajiciho. Na dokonceni [zalozeni] se ceka pres `whenComplete`, ne
     * pres `await()` z kotlinx.coroutines.future - to by pri zruseni
     * korutiny zrusilo i samotnou future a zalozeni by se uz nedokoncilo.
     * Vyprseni po [CEKANI_S] s je [EngineEvent.Chyba], stejne jako v
     * [pockejNaMost].
     */
    override suspend fun pockejNaPripravenost(): Boolean {
        if (bridge != null) return true
        if (chybaOhlasena || uvolneno) return false
        val ok = withTimeoutOrNull(CEKANI_S * 1000) {
            suspendCancellableCoroutine<Boolean> { k ->
                // Po zruseni korutiny se resume ignoruje.
                zalozeni.whenComplete { b, e -> k.resume(e == null && b != null) }
            }
        }
        if (ok == null) {
            ohlasChybu(ChybaMotoru.NespustilDo(CEKANI_S.toInt()), "zalozeni nedobehlo do $CEKANI_S s")
            return false
        }
        return ok && !uvolneno
    }

    /**
     * Krok S6: `setoption` UCI_Elo / UCI_LimitStrength jako uloha na vlakne
     * motoru. Volajiciho (UI) to neblokuje a nic se neposila mimo poradi:
     * vlakno je jedno, zakladani je na nem prvni uloha a hledani bezi taky
     * na nem, takze se volba dostane ke Stockfishi po zalozeni a pred dalsim
     * `go`. ViewModel silu meni jen v klidu, ne za hledani. Pred dokoncenim
     * zakladani se tedy ceka sama od sebe; [jePripraven] se k tomu nevola.
     * Po selhani zakladani (most null) nebo po release se nic neposle.
     */
    override fun nastavSilu(elo: Int?) {
        val prikazy = prikazySily(elo)
        try {
            engineThread.execute {
                val b = bridge ?: return@execute
                prikazy.forEach { b.command(it) }
                android.util.Log.d("Stockfish", "sila: " + (elo?.let { "Elo $it" } ?: "plna"))
            }
        } catch (e: RejectedExecutionException) {
            // uvolneno - motor uz nic hledat nebude
        }
    }

    /** Proc ne pres [san], viz [ChessEngine.jePromena]. */
    override fun jePromena(from: Square, to: Square): Boolean =
        snimek.san.containsKey(Move(from, to).toUci() + "q")

    // --- zmeny pozice -------------------------------------------------------

    /**
     * Pred dokoncenim zakladani nic (zakladani konci ve vychozi pozici).
     * Patri k neblokujicim volanim, ktera ViewModel smi volat i pred
     * dokoncenim zakladani (test prepnutiNaNepripravenyMotorVolaJenNeblokujiciMetody):
     * nejde pres most a neceka na zakladani.
     */
    override fun newGame() {
        if (searching) { stop(); return }
        stopPredZalozenim.zrus()
        // Pred dokoncenim zalozeni nic: zalozeni konci ve vychozi pozici
        // a applyMove ani setPosition do te doby nic nezmeni (cekaji).
        val b = bridge ?: return
        b.command("ucinewgame")
        b.command("position startpos")
        obnovMezipamet(b)
    }

    override fun setPosition(fen: String): Boolean = setPosition(fen, emptyList())

    override fun setPosition(startFen: String, moves: List<Move>): Boolean {
        if (searching) return false
        stopPredZalozenim.zrus()
        val b = pockejNaMost("setPosition") ?: return false
        val odpoved = b.command(prikazPosition(startFen, moves))
        if (odpoved.any { it.contains("neplatne") }) return false
        obnovMezipamet(b)
        return true
    }

    /**
     * Zahraje tah a vrati novy FEN, nebo null pokud je tah neplatny (nebo
     * zalozeni do 10 s nedobehlo - pak prijde i [EngineEvent.Chyba]).
     *
     * Pozor na promenu: kdyz [Move.promotion] zustane null a tah je promena,
     * nativni strana doplni damu.
     */
    override fun applyMove(move: Move): String? {
        if (searching) return null
        val b = pockejNaMost("applyMove") ?: return null
        val ok = b.applyMove(move.toUci())
        if (!ok) return null
        obnovMezipamet(b)
        return snimek.fen
    }

    // --- hledani ------------------------------------------------------------

    override fun go(depth: Int, casMs: Int) {
        if (searching) return
        searching = true
        stopPredZalozenim.goCeka()

        searchJob = scope.launch {
            // Uloha na vlakne motoru zacne az po zalozeni (jedno vlakno,
            // zalozeni je prvni uloha), takze tady se na nej ceka bez
            // blokovani UI.
            val b = try {
                withContext(engineDispatcher) { bridge }
            } catch (e: RejectedExecutionException) {
                null   // uvolneno
            }
            if (b == null) {
                searching = false
                stopPredZalozenim.zrus()
                // Zadny BestMove: po Chyba uz zadny prijit nesmi.
                ohlasChybu(ChybaMotoru.Nespustil, "go bez mostu")
                return@launch
            }

            // Stop, ktery prisel pred zalozenim: hledani do hloubky 1, at
            // "Hraj nejlepsi" dostane legalni tah hned.
            val prikaz = if (stopPredZalozenim.spoustim()) "go depth 1"
                         else prikazGo(depth, casMs)

            // sonda: kazdych 80 ms se zepta, jakou hloubku uz engine dokoncil
            val sonda = launch {
                var posledni = ""
                while (isActive) {
                    delay(80)
                    val s = b.searchInfo()
                    if (s.isNotEmpty() && s != posledni) {
                        posledni = s
                        parseInfo(s)?.let { _events.emit(it) }
                    }
                }
            }

            val radky = try {
                withContext(engineDispatcher) { b.command(prikaz) }
            } finally {
                sonda.cancel()
                searching = false
            }

            zaverecneInfo(radky)
                ?.let { parseInfo(it) }
                ?.let { _events.emit(it) }

            // Tah se ZAMERNE nezahraje, stejne jako u [CraftyEngine]: kdo
            // tah zahraje, rozhoduje ViewModel volanim applyMove.
            _events.emit(EngineEvent.BestMove(bestMove(radky)?.let { parseMove(it) }))
        }
    }

    /**
     * Priznak se nastavuje VZDY, i kdyz uz most je: go muze byt zrovna mezi
     * dokoncenim zalozeni a spustenim prikazu, a tam by se nativni stop
     * ztratil (vrstva ho na zacatku `go` nuluje). [StopPredZalozenim] ho
     * ulozi jen kdyz go jeste ceka, jinak nic.
     *
     * Patri k neblokujicim volanim, ktera ViewModel smi volat i pred
     * dokoncenim zakladani (test prepnutiNaNepripravenyMotorVolaJenNeblokujiciMetody):
     * nejde pres most a neceka na zakladani.
     */
    override fun stop() {
        stopPredZalozenim.stop()
        bridge?.stop()
    }

    /**
     * Bezpecne volat vicekrat. Nejdriv stop (bez zamku, prerusi bezici
     * hledani), pak zavreni mostu jako POSLEDNI uloha na vlakne motoru - tedy
     * az po dokonceni zalozeni i po skonceni hledani.
     */
    override fun release() {
        if (uvolneno) return
        uvolneno = true
        searchJob?.cancel()
        bridge?.stop()
        try {
            engineThread.execute { zavri() }
        } catch (e: RejectedExecutionException) {
            // uz vypnute - nemelo by nastat, release je hlidane priznakem
        }
        engineThread.shutdown()
    }

    // --- pomocne ------------------------------------------------------------

    private fun obnovMezipamet(b: StockfishBridge) {
        // Prikaz `d` vrati na prvnim radku FEN a na druhem "stav ...".
        val deska = b.command("d")
        val fen = deska.firstOrNull() ?: snimek.fen
        val stav = deska.getOrNull(1)?.removePrefix("stav ")?.trim() ?: STAV_NORMALNI

        val tahy = b.legalMoves()
            .mapNotNull { uci ->
                val od = squareFromName(uci.take(2)) ?: return@mapNotNull null
                val kam = squareFromName(uci.substring(2, 4)) ?: return@mapNotNull null
                od to kam
            }
            .groupBy({ it.first }, { it.second })
            .mapValues { (_, cile) -> cile.distinct() }

        // Jeden radek dvojic "<uci>:<san>", oddelene mezerou.
        val zapisy = b.command("sanmoves")
            .firstOrNull()
            .orEmpty()
            .trim()
            .split(' ')
            .mapNotNull { dvojice ->
                val delic = dvojice.indexOf(':')
                if (delic <= 0 || delic == dvojice.length - 1) null
                else dvojice.substring(0, delic) to dvojice.substring(delic + 1)
            }
        val zeSan = zapisy
            .mapNotNull { (uci, san) -> parseMove(uci)?.let { san to it } }
            .toMap()
        snimek = Snimek(fen, tahy, zapisy.toMap(), zeSan, stav)
    }

    /** Zapis bez koncoveho sachu nebo matu - `Ra8+` na `Ra8`. */
    private fun bezZnamenka(san: String): String = san.trimEnd('+', '#')

    /**
     * Jako parseInfo u [CraftyEngine], jen s logem. Cteni samo je
     * v companionu ([ctiInfo]), aby na nej sel napsat JVM test -
     * android.util.Log v nem spadne na "Method not mocked".
     *
     * Klic "time" ma jen zaverecny info radek; vypis do logu proto visi na
     * nem, jinak by kazde hledani zaplavilo log.
     */
    private fun parseInfo(s: String): EngineEvent.Info? {
        val info = ctiInfo(s) ?: return null
        val casMs = s.split(' ').let { w -> w.indexOf("time").takeIf { it >= 0 }?.let { w.getOrNull(it + 1) } }
        if (casMs != null) {
            android.util.Log.d("Stockfish", "hloubka ${info.depth}, ${info.nodes} uzlu, $casMs ms")
        }
        return info
    }

    internal companion object {
        /** Matove skore v setinach pesce, aby UI nemuselo znat `score mate`. */
        const val SKORE_MATU = 30_000

        /**
         * FEN vychoziho snimku, ktery adapter vraci do dokonceni zakladani.
         * Je to tataz konstanta, kterou pouziva ViewModel (Position.START_FEN,
         * napr. v exportPgn pro znacku [FEN]), a Stockfish vraci z `d` pro
         * vychozi pozici bajtove stejny retezec (overeno v logu, krok S3/B).
         */
        val VYCHOZI_FEN: String = Position.START_FEN

        /** Nejdelsi cekani na zalozeni nebo na uvolneni predchozi instance. */
        const val CEKANI_S = 10L

        /**
         * Nejvys jedna ziva nativni instance v procesu. Drzi ho adapter od
         * zalozeni mostu do jeho zavreni.
         */
        private val ZIVA_INSTANCE = Semaphore(1)

        /**
         * "depth 5 score cp 12 nodes 15632 pv e2e4 e7e5" na [EngineEvent.Info].
         *
         * Stejne jako parseInfo u Crafty. `score mate N` se prepocita na velke
         * cp: N > 0 (strana na tahu matuje) = SKORE_MATU - N, N < 0 (dostava
         * mat) = -SKORE_MATU - N. Dostavany mat po Kb8 Vh8# hlasi jako
         * `mate -1` Stockfish i od kroku U2 Crafty a RedMAX (drive `mate -2`),
         * znamenko sedi u vsech (AdapteryTest). `pv` nese celou variantu.
         */
        fun ctiInfo(s: String): EngineEvent.Info? {
            val slova = s.trim().split(' ')
            var depth: Int? = null
            var score: Int? = null
            var nodes = 0L
            val pv = mutableListOf<Move>()

            var i = 0
            while (i < slova.size) {
                when (slova[i]) {
                    "depth" -> depth = slova.getOrNull(++i)?.toIntOrNull()
                    "nodes" -> nodes = slova.getOrNull(++i)?.toLongOrNull() ?: 0L
                    "time" -> i++
                    "score" -> when (slova.getOrNull(i + 1)) {
                        "cp" -> { i += 2; score = slova.getOrNull(i)?.toIntOrNull() }
                        "mate" -> {
                            i += 2
                            val n = slova.getOrNull(i)?.toIntOrNull()
                            if (n != null) score = if (n > 0) SKORE_MATU - n else -SKORE_MATU - n
                        }
                    }
                    "pv" -> {
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
            return EngineEvent.Info(d, score, nodes, pv)
        }

        /** "e2e4" nebo "a7a8q" na [Move]. */
        fun parseMove(uci: String): Move? {
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
    }
}

/**
 * Rozhodnuti o stop() pred dokoncenim zakladani mostu (krok S3/B).
 *
 * Stop, ktery prijde, dokud most neni zalozeny, nema komu jit. Zapamatuje se
 * JEN tehdy, kdyz ceka go - stop bez cekajiciho go (napr. newGame v klidu)
 * nesmi pozdejsi normalni hledani zkratit na hloubku 1. newGame
 * a setPosition priznak mazou.
 *
 * Samostatna trida bez Androidu a bez mostu, aby na ni sel JVM test.
 */
internal class StopPredZalozenim {
    private var goCeka = false
    private var stopPozadovan = false

    /** go ceka na zalozeni. Stop z predchoziho cekani se zahodi. */
    @Synchronized fun goCeka() {
        goCeka = true
        stopPozadovan = false
    }

    /** Stop pred zalozenim. Bez cekajiciho go se nic neulozi. */
    @Synchronized fun stop() {
        if (goCeka) stopPozadovan = true
    }

    /** newGame, setPosition, nebo go, ktere skoncilo bez hledani. */
    @Synchronized fun zrus() {
        goCeka = false
        stopPozadovan = false
    }

    /** go se spousti. true = mezitim prisel stop, hledat jen do hloubky 1. */
    @Synchronized fun spoustim(): Boolean {
        val s = stopPozadovan
        goCeka = false
        stopPozadovan = false
        return s
    }
}
