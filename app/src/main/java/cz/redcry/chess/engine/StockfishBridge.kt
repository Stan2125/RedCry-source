package cz.redcry.chess.engine

/**
 * Most do nativniho enginu Stockfish 19 (stockfish_jni_bridge.cpp).
 *
 * Dvojce [CraftyBridge] s jinymi nativnimi symboly. Sdilene to schvalne
 * neni: jmeno javove tridy je zadratovane do jmena exportovaneho symbolu
 * (`Java_cz_redcry_chess_engine_StockfishBridge_nativeNew`), takze spolecna
 * zakladni trida by stejne musela mit vlastni sadu `external` metod v kazdem
 * potomkovi.
 *
 * Rozhrani je uzke schvalne: dovnitr jde textovy prikaz UCI, ven prijdou
 * odpovedi oddelene novym radkem. Vedle toho jsou obalky pro to, co UI
 * potrebuje casto a co pres UCI nejde ([legalMoves], [applyMove]).
 *
 * Vlakna: [command], [legalMoves] a [applyMove] patri jednomu vlaknu
 * enginu. [stop] a [searchInfo] jsou jedine metody, ktere lze volat odjinud,
 * typicky z UI vlakna, kdyz uzivatel prerusi premysleni nebo se pta na prubeh.
 * Nativni strana u nich zamek nebere.
 *
 * Zivotnost: [close] uvolni nativni stranu. Po ni se uz nesmi volat nic,
 * vcetne [stop]. Druhe [close] nic neudela.
 */
class StockfishBridge : AutoCloseable {

    /**
     * Stockfish smi mit NEJVYS JEDNU nativni instanci naraz. Ne kvuli
     * globalum jako Crafty, ale kvuli pameti: kazda instance nese vlastni
     * sit NNUE, hash tabulku a vlakna (hlavicka stockfish_jni_bridge.cpp).
     * Druhe nativeNew, dokud prvni zije, vrati 0, a tady se z toho stane
     * srozumitelna chyba misto padu na prvnim prikazu. ViewModel ji odchyti
     * a ohlasi textem z tabulky (krok J2).
     */
    private var handle: Long = nativeNew().also {
        if (it == 0L) throw MotorUzBeziException(EngineId.STOCKFISH.label)
    }

    private fun zkontroluj(): Long {
        check(handle != 0L) { "StockfishBridge uz byl zavren" }
        return handle
    }

    /** Posle prikaz UCI a vrati radky odpovedi. */
    fun command(prikaz: String): List<String> {
        val odpoved = nativeCommand(zkontroluj(), prikaz)
        return if (odpoved.isEmpty()) emptyList() else odpoved.split('\n')
    }

    /** Legalni tahy v aktualni pozici. UI je pouziva ke zvyrazneni poli. */
    fun legalMoves(): List<String> {
        val s = nativeLegalMoves(zkontroluj())
        return if (s.isBlank()) emptyList() else s.trim().split(' ')
    }

    /** Zahraje tah a prepne stranu na tahu. Vraci false, kdyz je nelegalni. */
    fun applyMove(tah: String): Boolean = nativeApplyMove(zkontroluj(), tah)

    /**
     * Snimek beziciho hledani, napr. "depth 18 score cp 37 nodes 112909 pv e2e4",
     * nebo prazdny retezec, kdyz jeste zadna hloubka nedobehla.
     *
     * Volatelne z jineho vlakna, nez na kterem bezi `go`, a bez cekani
     * na zamek.
     */
    fun searchInfo(): String {
        val h = handle
        return if (h == 0L) "" else nativeSearchInfo(h)
    }

    /** Prerusi bezici hledani. Volatelne z jineho vlakna. */
    fun stop() {
        val h = handle
        if (h != 0L) nativeStop(h)
    }

    override fun close() {
        val h = handle
        handle = 0L
        if (h != 0L) nativeDelete(h)
    }

    // Od kroku U14/B internal, ne private: [nativeSanVarianty] vola
    // SanVarianty. Ostatni metody zustavaji private.
    internal companion object {
        init { System.loadLibrary("redcry") }

        @JvmStatic private external fun nativeNew(): Long
        @JvmStatic private external fun nativeDelete(handle: Long)
        @JvmStatic private external fun nativeCommand(handle: Long, prikaz: String): String
        @JvmStatic private external fun nativeStop(handle: Long)
        @JvmStatic private external fun nativeSearchInfo(handle: Long): String
        @JvmStatic private external fun nativeLegalMoves(handle: Long): String
        @JvmStatic private external fun nativeApplyMove(handle: Long, tah: String): Boolean
        // Kotlin nevola (FEN se cte z `d`), export v C++ je kvuli shode
        // s ostatnimi mosty a testu mostu.
        @JvmStatic private external fun nativeFen(handle: Long): String

        /**
         * Hlavni varianta v SAN (krok U14/B), viz [SanVarianty]. Staticka,
         * bez handle: nepotrebuje instanci Stockfishe ani zamek, volatelna
         * z libovolneho vlakna i za hledani kterehokoli motoru.
         */
        @JvmStatic external fun nativeSanVarianty(fen: String, tahy: String): String
    }
}
