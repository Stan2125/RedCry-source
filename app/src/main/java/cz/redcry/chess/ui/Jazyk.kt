package cz.redcry.chess.ui

import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz
import cz.redcry.chess.texty.TextyEn

// Jazyk aplikace pro prepinac pod dlazdicemi (krok U15). Od kroku J2
// prepina texty cele aplikace, drzi ho GameViewModel a volba se pamatuje
// (docs/POZNAMKY-JAZYK.md).

/**
 * Jazyk, ktery ukazuje [PrepinacJazyka]: [zkratka] na kolecku, [kod] do
 * ulozene volby. Nazev pro cteni obrazovky je v tabulce textu
 * ([Texty.nazevCestiny]).
 */
enum class Jazyk(val zkratka: String, val kod: String) {
    CESTINA("CZ", "cs"),
    ANGLICTINA("EN", "en"),
    ;

    /** Tabulka textu tohoto jazyka. */
    val texty: Texty get() = when (this) {
        CESTINA -> TextyCz
        ANGLICTINA -> TextyEn
    }

    companion object {
        /** Ulozeny [kod] zpet na jazyk; neznamy nebo zadny = null. */
        fun zKodu(kod: String?): Jazyk? = entries.firstOrNull { it.kod == kod }

        /**
         * Jazyk pri startu: ulozena volba, bez ni podle jazyka telefonu -
         * cestina ("cs") cesky, cokoli jineho anglicky.
         */
        fun priStartu(ulozeny: Jazyk?, jazykTelefonu: String): Jazyk =
            ulozeny ?: if (jazykTelefonu == CESTINA.kod) CESTINA else ANGLICTINA
    }
}

/** Ten druhy z obou jazyku - kam prepinac prepne po klepnuti. */
internal fun Jazyk.druhy(): Jazyk = when (this) {
    Jazyk.CESTINA -> Jazyk.ANGLICTINA
    Jazyk.ANGLICTINA -> Jazyk.CESTINA
}

/**
 * Kde se pamatuje zvoleny jazyk (krok J2). V aplikaci SharedPreferences
 * ([UlozisteJazykaAndroid]), v testech atrapa - stejny vzor jako
 * [cz.redcry.chess.analyza.UlozisteAnalyzy].
 */
interface UlozisteJazyka {
    /** Ulozena volba, null = uzivatel jeste neprepinal. */
    fun cti(): Jazyk?

    fun uloz(jazyk: Jazyk)
}

/** Nic neuklada a nic nema - ViewModel bez nej startuje jako do J2 cesky. */
object BezUlozeniJazyka : UlozisteJazyka {
    override fun cti(): Jazyk? = null
    override fun uloz(jazyk: Jazyk) = Unit
}
