package cz.redcry.chess.engine

import cz.redcry.chess.model.Move

/**
 * Hlavni varianta v SAN pro VSECHNY motory (krok U14/B).
 *
 * Prevod dela C++ `sfredcry::san_varianty` (stockfish_uci.cpp) pres
 * staticky [StockfishBridge.nativeSanVarianty]: tahy se zahraji po jednom
 * na vlastni pozici Stockfishe a zapis skladaji tytez pravidla jako
 * `sanmoves`. Stockfish proto, ze jeho pravidla se v krizovem testu S2 shodla
 * s Crafty na 112 563 pozicich; jiny motor by musel mit vlastni prevod.
 * Kotlin sam zadne pravidlo nepocita - rozliseni (Nbd7) potrebuje
 * generator tahu.
 *
 * Bez instance Stockfishe a bez zamku: funguje i s Crafty, kdyz Stockfish
 * zalozeny neni, a da se volat za beziciho hledani kterehokoli motoru.
 */
object SanVarianty {

    /**
     * Tahy [tahy] zahrane z pozice [fen] v SAN, jeden tah na prvek. Neplatny
     * nebo nelegalni tah prevod ukonci (vrati se zapisy pred nim), neplatny
     * FEN da prazdny seznam.
     *
     * Prazdny seznam i tehdy, kdyz nativni knihovna neni nactena (JVM test:
     * [LinkageError]) - volajici pak pouzije zalozni tvar.
     */
    fun preved(fen: String, tahy: List<Move>): List<String> {
        if (tahy.isEmpty()) return emptyList()
        val san = try {
            StockfishBridge.nativeSanVarianty(fen, tahy.joinToString(" ") { it.toUci() })
        } catch (e: LinkageError) {
            return emptyList()
        }
        return if (san.isBlank()) emptyList() else san.trim().split(' ')
    }
}
