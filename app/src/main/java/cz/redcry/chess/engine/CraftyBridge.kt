package cz.redcry.chess.engine

/**
 * Most do nativního enginu Crafty (crafty_jni_bridge.cpp).
 *
 * Dvojče [RedMaxBridge] s jinými nativními symboly. Sdílené to schválně
 * není: jméno javové třídy je zadrátované do jména exportovaného symbolu
 * (`Java_cz_redcry_chess_engine_CraftyBridge_nativeNew`), takže společná
 * základní třída by stejně musela mít vlastní sadu `external` metod v každém
 * potomkovi. Až přibude třetí a čtvrtý motor, bude vidět, co se zobecnit dá.
 *
 * Rozhraní je úzké schválně: dovnitř jde textový příkaz UCI, ven přijdou
 * odpovědi oddělené novým řádkem. Vedle toho jsou obálky pro to, co UI
 * potřebuje často a co přes UCI nejde ([legalMoves], [applyMove]).
 *
 * Obálky pro `go`, `ucinewgame`, `position` a FEN tu do kroku 13 byly taky,
 * ale nic je nevolalo - adaptér posílá ty příkazy rovnou přes [command],
 * `go` skládá jedině [prikazGo] a FEN čte z příkazu `d`. Nevracet: obálka
 * nad `go` si skládala příkaz sama a je to přesně to místo, kde by se to
 * rozešlo znovu.
 *
 * Vlákna: [command], [legalMoves] a [applyMove] patří jednomu vláknu
 * enginu. [stop] a [searchInfo] jsou jediné metody, které lze volat odjinud,
 * typicky z UI vlákna, když uživatel přeruší přemýšlení nebo se ptá na průběh.
 * Nativní strana u nich zámek nebere.
 *
 * Životnost: [close] uvolní nativní stranu. Po ní se už nesmí volat nic,
 * včetně [stop].
 */
class CraftyBridge : AutoCloseable {

    /**
     * Crafty smí mít NEJVÝŠ JEDNU nativní instanci naráz - jeho stav jsou
     * globály v knihovně (PORT-CRAFTY.md, oddíl 6a). Druhé nativeNew, dokud
     * první žije, vrátí 0, a tady se z toho stane srozumitelná chyba místo
     * pádu na prvním příkazu. ViewModel ji odchytí a ohlásí textem z tabulky
     * (krok J2).
     */
    private var handle: Long = nativeNew().also {
        if (it == 0L) throw MotorUzBeziException(EngineId.CRAFTY.label)
    }

    private fun zkontroluj(): Long {
        check(handle != 0L) { "CraftyBridge uz byl zavren" }
        return handle
    }

    /** Pošle příkaz UCI a vrátí řádky odpovědi. */
    fun command(prikaz: String): List<String> {
        val odpoved = nativeCommand(zkontroluj(), prikaz)
        return if (odpoved.isEmpty()) emptyList() else odpoved.split('\n')
    }

    /** Legální tahy v aktuální pozici. UI je používá ke zvýraznění polí. */
    fun legalMoves(): List<String> {
        val s = nativeLegalMoves(zkontroluj())
        return if (s.isBlank()) emptyList() else s.trim().split(' ')
    }

    /** Zahraje tah a přepne stranu na tahu. Vrací false, když je nelegální. */
    fun applyMove(tah: String): Boolean = nativeApplyMove(zkontroluj(), tah)

    /**
     * Snímek běžícího hledání, např. "depth 5 score cp 12 nodes 15632 pv e2e4",
     * nebo prázdný řetězec, když ještě žádná hloubka nedoběhla.
     *
     * Volatelné z jiného vlákna, než na kterém běží `go`, a bez čekání
     * na zámek.
     */
    fun searchInfo(): String {
        val h = handle
        return if (h == 0L) "" else nativeSearchInfo(h)
    }

    /** Přeruší běžící hledání. Volatelné z jiného vlákna. */
    fun stop() {
        val h = handle
        if (h != 0L) nativeStop(h)
    }

    override fun close() {
        val h = handle
        handle = 0L
        if (h != 0L) nativeDelete(h)
    }

    private companion object {
        init { System.loadLibrary("redcry") }

        @JvmStatic external fun nativeNew(): Long
        @JvmStatic external fun nativeDelete(handle: Long)
        @JvmStatic external fun nativeCommand(handle: Long, prikaz: String): String
        @JvmStatic external fun nativeStop(handle: Long)
        @JvmStatic external fun nativeSearchInfo(handle: Long): String
        @JvmStatic external fun nativeLegalMoves(handle: Long): String
        @JvmStatic external fun nativeApplyMove(handle: Long, tah: String): Boolean
        // Kotlin nevolá od kroku 13/B (FEN se čte z `d`); export v C++ zůstává.
        @JvmStatic external fun nativeFen(handle: Long): String
    }
}
