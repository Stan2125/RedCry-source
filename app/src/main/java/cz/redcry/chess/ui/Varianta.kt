package cz.redcry.chess.ui

// Hlavni varianta v okne (i) na cislovane radky (krok U14/B).

/**
 * Varianta v SAN na cislovane radky okna (i): od cerneho "14... Nbxd5",
 * "15. Qa4+ Qd7", od bileho "15. Re1 Nf6".
 *
 * [cernyNaTahu] a [cisloTahu] patri pozici, ve ktere hledani bezi (koren
 * hledani), ne desce po tahu motoru: varianta zacina tahem z korene.
 *
 * Radky sklada [zapisPartie], stejne jako zaznam partie, takze se cislovani
 * nemuze rozejit. Ciste, aby na to sel napsat JVM test.
 */
fun radkyVarianty(san: List<String>, cernyNaTahu: Boolean, cisloTahu: Int): List<String> =
    zapisPartie(san, cernyNaTahu, cisloTahu).map { r ->
        if (r.bily == null) {
            r.cislo.toString() + "... " + r.cerny!!.san
        } else {
            r.cislo.toString() + ". " + r.bily.san + (r.cerny?.let { " " + it.san } ?: "")
        }
    }
