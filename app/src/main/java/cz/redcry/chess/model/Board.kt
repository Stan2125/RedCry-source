package cz.redcry.chess.model

/**
 * Ctverec desky je index 0..63, kde 0 = a8 a 63 = h1.
 * Je to stejne poradi, v jakem se cte FEN, takze prevod je primocary.
 *
 * Pozor: nativni Crafty pouziva vlastni cislovani (a1 = 0) a jeste k tomu
 * obracene poradi bitu v bitboardu. Prevod patri do JNI vrstvy, ne sem.
 */
typealias Square = Int

fun squareName(sq: Square): String = "abcdefgh"[sq % 8] + (8 - sq / 8).toString()

fun squareFromName(name: String): Square? {
    if (name.length != 2) return null
    val file = "abcdefgh".indexOf(name[0])
    val rank = name[1] - '1'
    if (file < 0 || rank !in 0..7) return null
    return (7 - rank) * 8 + file
}

enum class PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING }

enum class Side { WHITE, BLACK;
    fun other() = if (this == WHITE) BLACK else WHITE
}

data class Piece(val type: PieceType, val side: Side) {
    fun toChar(): Char {
        val c = when (type) {
            PieceType.PAWN -> 'p'
            PieceType.KNIGHT -> 'n'
            PieceType.BISHOP -> 'b'
            PieceType.ROOK -> 'r'
            PieceType.QUEEN -> 'q'
            PieceType.KING -> 'k'
        }
        return if (side == Side.WHITE) c.uppercaseChar() else c
    }

    companion object {
        fun fromChar(c: Char): Piece? {
            val type = when (c.lowercaseChar()) {
                'p' -> PieceType.PAWN
                'n' -> PieceType.KNIGHT
                'b' -> PieceType.BISHOP
                'r' -> PieceType.ROOK
                'q' -> PieceType.QUEEN
                'k' -> PieceType.KING
                else -> return null
            }
            return Piece(type, if (c.isUpperCase()) Side.WHITE else Side.BLACK)
        }
    }
}

data class Move(val from: Square, val to: Square, val promotion: PieceType? = null) {
    /** Zapis v UCI notaci, napr. "e2e4" nebo "e7e8q". */
    fun toUci(): String {
        val p = when (promotion) {
            PieceType.QUEEN -> "q"
            PieceType.ROOK -> "r"
            PieceType.BISHOP -> "b"
            PieceType.KNIGHT -> "n"
            else -> ""
        }
        return squareName(from) + squareName(to) + p
    }

    override fun toString(): String = squareName(from) + "–" + squareName(to)
}

/**
 * Opak [Move.toUci]: "e2e4" nebo "e7e8q" na [Move], cokoli jineho null.
 * Jen prevod textu, zadne pravidlo - jestli je tah legalni, rekne motor
 * pri applyMove. Pouziva ho knihovna zahajeni (krok K1/B).
 */
fun moveZUci(uci: String): Move? {
    if (uci.length !in 4..5) return null
    val z = squareFromName(uci.substring(0, 2)) ?: return null
    val na = squareFromName(uci.substring(2, 4)) ?: return null
    val promena = when (uci.getOrNull(4)) {
        null -> null
        'q' -> PieceType.QUEEN
        'r' -> PieceType.ROOK
        'b' -> PieceType.BISHOP
        'n' -> PieceType.KNIGHT
        else -> return null
    }
    return Move(z, na, promena)
}

/**
 * Cteci snimek pozice, urceny VYHRADNE k vykresleni.
 *
 * Autoritativni stav partie drzi engine. Tahle trida z nej jen umi precist
 * FEN a rozlozit ho na 64 policek. Nikdy sem nepridavej generovani tahu ani
 * pravidla - jinak vzniknou dve implementace sachu, ktere se casem rozejdou.
 */
class Position private constructor(
    val squares: Array<Piece?>,
    val sideToMove: Side,
    val fullmoveNumber: Int,
) {
    operator fun get(sq: Square): Piece? = squares[sq]

    companion object {
        const val START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

        fun fromFen(fen: String): Position? {
            val parts = fen.trim().split(Regex("\\s+"))
            if (parts.isEmpty()) return null
            val ranks = parts[0].split("/")
            if (ranks.size != 8) return null

            val squares = arrayOfNulls<Piece>(64)
            for (r in 0 until 8) {
                var file = 0
                for (c in ranks[r]) {
                    when {
                        c.isDigit() -> file += c - '0'
                        else -> {
                            if (file > 7) return null
                            squares[r * 8 + file] = Piece.fromChar(c) ?: return null
                            file++
                        }
                    }
                }
                if (file != 8) return null
            }

            val side = if (parts.getOrNull(1) == "b") Side.BLACK else Side.WHITE
            val moveNo = parts.getOrNull(5)?.toIntOrNull() ?: 1
            return Position(squares, side, moveNo)
        }
    }
}
