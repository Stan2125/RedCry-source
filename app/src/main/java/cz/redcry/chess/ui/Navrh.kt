package cz.redcry.chess.ui

import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

// Rozmery obrazovky podle navrhu docs/navrhy/redcry-layout-v3.json.
// Vzniklo v kroku R1 presunem z GameScreen.kt.

/**
 * Rozvrzeni obrazovky podle navrhu `docs/navrhy/redcry-layout-v3.json`
 * (krok U5). Navrh je kresleny na platne 976 px na sirku; vsechny rozmery
 * nize jsou v pixelech toho platna a na dp se prepocitavaji pomerem
 * [dpNaPx] - na Pixelu 4a je to 393 dp / 976 px, tedy asi 0,40.
 *
 * Deska je jedina pruzna vec. V navrhu je oriznuta (886 x 807, prvni rada
 * chybi - je to prekresleny screenshot s odrolovanou obrazovkou), takze
 * velikost z nej vzit nejde. Je ctvercova a dostane, co zbyde po pevnych
 * radcich, nejvic [DESKA] na sirku. Na nizke obrazovce se tak zmensi deska,
 * ne ovladani, a nic se nemusi rolovat.
 *
 * Kdyby na desku nezbylo ani [DESKA_MIN] (rozlozeny Fold, obrazovka skoro
 * ctvercova), zmensi se cely navrh podle vysky a vycentruje se.
 *
 * Krok U6/A: rada Nova partie/Otocit desku zmizela a Nacist/Ulozit se
 * presunulo do menu Hra, radek Sila ELO je o pul radku vys. Vsechno
 * uvolnene misto dostala deska - porad nejvic [DESKA], tedy plna sirka obsahu.
 *
 * Krok U15: levy sloupec ovladani zacina radkem s prepinacem jazyka a (i),
 * vysokym jako tlacitka rady dole ([RADEK_JAZYKA]), a Engine s vyberem
 * motoru se vejde do jednoho radku. Radek jazyka je o [JAZYK_NAVIC] vyssi
 * nez ostatni, o tolik je mensi [MEZERA_OVLADANI] a vetsi [OVLADANI].
 * [PEVNE] se nemeni, deska je tedy vsude stejne velka.
 *
 * Krok U16 (replika uzivatele `docs/navrhy/u16-replika.json`): pod radkem
 * jazyka pribyla [MEZERA_POD_JAZYKEM], radky Engine, Plna sila a Sila ELO
 * jsou o ni niz. Vezme se z [MEZERA_SIPKY], takze tlacitka dole i [PEVNE]
 * zustavaji. Pravy sloupec zacina o 20 vlevo ([LEVY_SLOUPEC] 559).
 */
internal class Navrh(private val dpNaPx: Float, val deska: Dp) {

    fun px(n: Int): Dp = (n * dpNaPx).dp

    fun px(n: Float): Dp = (n * dpNaPx).dp

    companion object {
        const val PLATNO = 976
        /** Sirka obsahu: od x = 44 do x = 930. */
        const val OBSAH = 886
        const val DESKA = 886
        const val DESKA_MIN = 700

        // Svisle, shora dolu. Mezery jsou rozdily y sousednich prvku v JSON.
        const val NAHORE = 2
        const val MEZERA_TAH = 31        // board -> turnLeft
        const val TAH = 68               // turnLeft
        const val MEZERA_PREMYSLEJ = 24  // turnLeft -> think
        const val PREMYSLEJ = 132        // think
        const val MEZERA_DLAZDICE = 30   // think -> currentBox
        const val DLAZDICE = 113         // currentBox, depthBox
        /**
         * O kolik je radek jazyka (krok U15) vyssi nez ostatni radky ovladani:
         * ma vysku tlacitek [SIPKY], ostatni [RADEK_OVLADANI].
         */
        const val JAZYK_NAVIC = 12
        /** currentBox -> engineLabel 53, od kroku U15 o [JAZYK_NAVIC] min. */
        const val MEZERA_OVLADANI = 53 - JAZYK_NAVIC
        /** O kolik je radek Sila ELO vys nez v navrhu (krok U6/A): pul jeho vysky 58. */
        const val ELO_VYS = 29
        /**
         * Mezera mezi radkem jazyka a radkem Engine (krok U16). Mezera pod
         * CZ/(i) a nad tlacitky dole pak vyjde 54 : 55.
         */
        const val MEZERA_POD_JAZYKEM = 10
        /**
         * engineLabel.y 1260 .. eloLabel spodek 1618 = 358, od kroku U6/A
         * o [ELO_VYS] min, od kroku U15 o [JAZYK_NAVIC] vic, od kroku U16
         * o [MEZERA_POD_JAZYKEM] vic (351).
         */
        const val OVLADANI = 358 - ELO_VYS + JAZYK_NAVIC + MEZERA_POD_JAZYKEM
        /** eloLabel -> navBack 36, od kroku U16 o [MEZERA_POD_JAZYKEM] min (26). */
        const val MEZERA_SIPKY = 36 - MEZERA_POD_JAZYKEM
        const val SIPKY = 100            // navBack .. navSave

        // Radky leveho sloupce ovladani (krok U15), dohromady [OVLADANI].
        /** Prepinac jazyka a (i): vysoke jako tlacitka rady dole. Pod nim [MEZERA_POD_JAZYKEM]. */
        const val RADEK_JAZYKA = SIPKY
        /** Engine a Plna sila (fullPowerLabel, vyska 88). */
        const val RADEK_OVLADANI = 88
        /** Plna sila -> Sila ELO: 36 v navrhu, od kroku U6/A o [ELO_VYS] min. */
        const val MEZERA_ELO = 36 - ELO_VYS
        /** Sila ELO (eloLabel, vyska 58). */
        const val RADEK_ELO = 58
        const val DOLE = 22              // spodni okraj pod radou tlacitek

        /** Pevne radky bez desky. */
        const val PEVNE = NAHORE + MEZERA_TAH + TAH + MEZERA_PREMYSLEJ + PREMYSLEJ +
            MEZERA_DLAZDICE + DLAZDICE + MEZERA_OVLADANI + OVLADANI + MEZERA_SIPKY +
            SIPKY + DOLE

        // Vodorovne.
        const val MEZERA_VEDLE = 26      // currentBox -> depthBox
        const val MEZERA_SIPEK = 18      // mezi tlacitky rady ‹ › ⟳ Hra
        /**
         * Levy sloupec ovladani: od x = 44 po zacatek praveho (secondsLabel.x
         * = 623, od kroku U16 o 20 vlevo, 603). Steppery praveho sloupce (300)
         * pak konci 27 pred krajem obsahu, podobne jako popisky vlevo (24).
         */
        const val LEVY_SLOUPEC = 559

        /** Vahy rady dole: tlacitka ‹ › ⟳ a Hra ve zbytku (krok U6/A). */
        const val VAHA_TLACITKA = 160f
        const val VAHA_HRA = 174f
        /**
         * Sirka tlacitek ‹, › a ⟳ v rade dole, asi 203,55 px. Prepinac
         * jazyka a (i) maji tutez (krok U15).
         */
        const val SIRKA_TLACITKA =
            (OBSAH - 3 * MEZERA_SIPEK) * VAHA_TLACITKA / (3 * VAHA_TLACITKA + VAHA_HRA)
        /**
         * Pravy kraj (i) = pravy kraj tlacitka ›, asi 425,1 px od leveho
         * kraje obsahu. K nemu se zprava zarovnava ovladani pod (krok U15).
         */
        const val KRAJ_OVLADANI = 2 * SIRKA_TLACITKA + MEZERA_SIPEK

        fun pro(sirka: Dp, vyska: Dp): Navrh {
            val dpNaPx = minOf(sirka.value / PLATNO, vyska.value / (PEVNE + DESKA_MIN))
            val deska = minOf(DESKA * dpNaPx, vyska.value - PEVNE * dpNaPx)
            return Navrh(dpNaPx, deska.dp)
        }
    }
}
