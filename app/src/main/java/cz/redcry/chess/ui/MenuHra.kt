package cz.redcry.chess.ui

import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz

/**
 * Polozky menu "Hra ▾" v tomhle poradi (krok U6/A). Pod [NOVA_PARTIE] je
 * v menu oddelovac. Nahrazuji dve rady tlacitek Nacist/Ulozit a Nova
 * partie/Otocit desku, ktere byly na hlavni obrazovce do kroku U5.
 *
 * Popisky jsou od kroku J1 v tabulce textu ([Texty.polozkaHry]).
 */
enum class PolozkaHry {
    NOVA_PARTIE,
    OTOCIT,

    /**
     * Krok K1/B: zaskrtavaci, stav ukazuje fajfka ([GameUiState.knihovna]).
     * U Otocit desku, protoze obe jsou prepinace, ne prace se souborem.
     */
    KNIHOVNA,

    /** Krok A1: neaktivni, kdyz partie nema zadny tah. */
    ANALYZA,
    NACIST,
    ULOZIT,
}

/**
 * Co polozka udela. Mimo composable schvalne, at JVM test overi, ze
 * polozky volaji tytez funkce jako drivejsi tlacitka: Otocit desku
 * [GameViewModel.onFlip], Nacist a Ulozit otevrou tytez dialogy SAF
 * ([nacist], [ulozit] - Uri do ViewModelu nesmi, viz GameScreen). Jedina
 * zmena je Nova partie: jde pres potvrzeni [GameViewModel.onNovaPartieDotaz],
 * ktere po souhlasu vola drivejsi [GameViewModel.onNewGame].
 */
fun provedPolozkuHry(
    polozka: PolozkaHry,
    viewModel: GameViewModel,
    nacist: () -> Unit,
    ulozit: () -> Unit,
) {
    when (polozka) {
        PolozkaHry.NOVA_PARTIE -> viewModel.onNovaPartieDotaz()
        PolozkaHry.OTOCIT -> viewModel.onFlip()
        PolozkaHry.KNIHOVNA -> viewModel.onKnihovna()
        PolozkaHry.ANALYZA -> viewModel.analyza.onDotaz()
        PolozkaHry.NACIST -> nacist()
        PolozkaHry.ULOZIT -> ulozit()
    }
}

/**
 * Text dialogu pred novou partii (krok U6/A). [pocet] je pocet tahu
 * ([pocetTahuZaznamu]). Sklonovani podle cisla je od kroku J1 v tabulce
 * textu ([Texty.potvrzeniNovePartie]).
 */
fun textPotvrzeniNovePartie(pocet: Int, texty: Texty = TextyCz): String =
    texty.potvrzeniNovePartie(pocet)
