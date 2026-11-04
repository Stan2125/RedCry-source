package cz.redcry.chess.ui

// Zaznam partie na cislovane radky.
// Vzniklo v kroku R1 presunem z GameViewModel.kt.

/**
 * Jeden pultah v zaznamu: jeho zapis a pozice, na kterou se po nem stoji.
 *
 * [historyIndex] je JEDINE misto, odkud obrazovka cil klepnuti bere - sama
 * uz zadnou aritmetiku nedela.
 */
data class PulTah(val san: String, val historyIndex: Int)

/**
 * Jeden cislovany radek zaznamu: "12. Nf3 Nc6".
 *
 * @param bily   null jen v prvnim radku partie, ktera zacina cernym
 * @param cerny  null v poslednim radku pri lichem poctu tahu
 */
data class RadekZaznamu(
    val cislo: Int,
    val bily: PulTah?,
    val cerny: PulTah?,
)

/**
 * Tahy v SAN na cislovane radky partie: "1. e4 e5", "2. Nf3 Nc6".
 *
 * Ciste, bez Compose i bez ViewModelu, aby na to sel napsat JVM test.
 * Az se bude zapisovat PGN, pujde se o tuhle funkci oprit - proto bere
 * cislo prvniho tahu a to, ci je, misto aby si myslela, ze partie zacina
 * bilym na tahu 1.
 *
 * Ta jednicka: pultah s poradim `i` (od nuly) vede do pozice
 * `history[i + 1]`, protoze `history[0]` je vychozi pozice. Tady a nikde
 * jinde se ta dvojka pocita.
 *
 * @param prvniJeCerny   partie zacina cernym (po importu z FEN), takze prvni
 *                       radek je "1... Nf6"
 * @param cisloPrvnihoTahu  cislo tahu vychozi pozice (FEN, seste pole)
 */
fun zapisPartie(
    tahy: List<String>,
    prvniJeCerny: Boolean,
    cisloPrvnihoTahu: Int,
): List<RadekZaznamu> {
    if (tahy.isEmpty()) return emptyList()

    val radky = ArrayList<RadekZaznamu>()
    var cislo = cisloPrvnihoTahu
    var i = 0

    if (prvniJeCerny) {
        radky.add(RadekZaznamu(cislo, bily = null, cerny = PulTah(tahy[0], 1)))
        cislo++
        i = 1
    }
    while (i < tahy.size) {
        val cerny = tahy.getOrNull(i + 1)?.let { PulTah(it, i + 2) }
        radky.add(RadekZaznamu(cislo, bily = PulTah(tahy[i], i + 1), cerny = cerny))
        cislo++
        i += 2
    }
    return radky
}

/**
 * Pocet tahu v zaznamu pro dialog pred novou partii (krok U7/C): ve
 * stejnych jednotkach jako ukazatel "Tah N" nahore. Ten ukazuje
 * `fullmoveNumber` pozice na desce, tedy cislo tahu - bily a cerny pultah
 * jsou jeden tah. Pocet je proto pocet cislovanych radku zaznamu
 * ([zapisPartie]): po 1. e4 je to 1, po 1. e4 e5 taky 1, po 2. Nf3 dva.
 * Do U7/C dialog pocital pultahy.
 */
fun pocetTahuZaznamu(tahy: List<String>, prvniJeCerny: Boolean, cisloPrvnihoTahu: Int): Int =
    zapisPartie(tahy, prvniJeCerny, cisloPrvnihoTahu).size
