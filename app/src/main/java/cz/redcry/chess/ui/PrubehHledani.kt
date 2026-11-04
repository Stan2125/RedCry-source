package cz.redcry.chess.ui

import cz.redcry.chess.engine.EngineEvent
import cz.redcry.chess.model.Move
import cz.redcry.chess.model.Position
import cz.redcry.chess.model.Side
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.transformLatest

// Prubeh hledani pro okno (i) a tlacitko Hraj nejlepsi: koren hledani,
// varianta, predchozi volba, skore na pohled bileho a odpocet sekund.
// Vzniklo v kroku R2/B presunem z GameViewModel.kt.

/** Jak casto se prepocita cislo na tlacitku Hraj nejlepsi. */
private const val TIK_MS = 250L

/**
 * Vysledky beziciho hledani do stavu obrazovky (krok R2/B). Bez korutin
 * a bez ViewModelu: [stav] a [sanZMotoru] se ctou az pri volani - motor
 * se meni pri prepnuti a po [EngineEvent.Chyba].
 *
 * @param hodiny       zdroj casu, jako v GameViewModel
 * @param sanVarianty  prevod hlavni varianty do SAN (krok U14/B)
 * @param sanZMotoru   SAN tahu z mezipameti PRAVE aktualniho motoru
 * @param stav         aktualni stav obrazovky
 */
class PrubehHledani(
    private val hodiny: () -> Long,
    private val sanVarianty: (String, List<Move>) -> List<String>,
    private val sanZMotoru: (Move) -> String?,
    private val stav: () -> GameUiState,
) {
    /**
     * FEN pozice, ve ktere bezi posledni hledani (koren hledani, krok
     * U14/B). Nastavuje se tam, kde hledani zacina: Premyslej
     * ([GameViewModel.onThinkClick], pres [zacni]) a pozice analyzy
     * ([cz.redcry.chess.analyza.HraProAnalyzu.ukazPozici]). Po tahu
     * motoru se deska posune, koren ne - varianta se cisluje podle nej.
     */
    var korenHledani: String = Position.START_FEN

    /** Hlaseni beziciho hledani pro predchozi volbu (krok U14/C). */
    private val sledovaniVolby = SledovaniVolby()

    /** Kdy padl stisk Premyslej, podle [hodiny]. Plati jen ve stavu HLEDA. */
    private var zacatekHledani = 0L

    private val stavHledani: StavHledani get() = stav().stavHledani

    /**
     * Zacatek hledani za Premyslej: cas, koren a prazdne sledovani volby.
     * Vola se PRED prepnutim do HLEDA, viz [GameViewModel.onThinkClick].
     */
    fun zacni(koren: String) {
        zacatekHledani = hodiny()
        korenHledani = koren
        sledovaniVolby.vynuluj()
    }

    /**
     * Stav po hlaseni [EngineEvent.Info]: hloubka, tah, varianta,
     * predchozi volba a skore. Zapisuje ho ViewModel.
     */
    fun poInfo(event: EngineEvent.Info): GameUiState {
        // Info chodi po kazde dokoncene hloubce, takze cislo
        // v dlazdici Hloubka roste za behu.
        //
        // engine.san() se tu vola ZA BEZICIHO HLEDANI. Je to jen
        // cteni z mezipameti motoru, ktera plati pro pozici pred
        // tahem - tedy presne pro tuhle - takze se na zamku
        // nativni strany necekat nemusi.
        val prvni = event.pv.firstOrNull()
        // Jeden prevod pro variantu i predchozi volbu (U14/C).
        val san = if (event.pv.isEmpty()) emptyList()
                  else sanVarianty(korenHledani, event.pv)
        sledujVolbu(event.depth, prvni, san)
        return stav().copy(
            depth = event.depth,
            bestMove = prvni,
            bestMoveSan = prvni?.let { sanZMotoru(it) },
            // Varianta se sklada TADY, ne v composable -
            // jinak by na to nesel napsat JVM test. Totez
            // plati o prepoctu skore na pohled bileho.
            varianta = slozVariantu(event.pv, san),
            predchoziVolba = predchoziVolbaZaHledani(),
            skoreCp = skoreNaBileho(event.scoreCp),
        )
    }

    /**
     * Predchozi volba po BestMove (krok U14/C): konec hledani uzavre
     * i posledni hloubku. Mimo Premyslej zustava ta ze stavu.
     */
    fun volbaPoKonci(): PredchoziVolba? =
        if (sledujeVolbu()) sledovaniVolby.vysledek(konec = true)
        else stav().predchoziVolba

    /**
     * Hlavni varianta na radky okna (i).
     *
     * Od kroku U14/B cela v SAN s cisly tahu: [san] je prevod [sanVarianty]
     * z [korenHledani] a [radkyVarianty] z nej slozi radky "14... Nbxd5",
     * "15. Qa4+ Qd7". Prevod je nativni a pro vsechny motory, bez zamku,
     * takze jde i za beziciho hledani.
     *
     * Kdyz prevod vrati prazdny seznam (nativni knihovna chybi, nesedi
     * koren), zustava drivejsi tvar, jeden tah na prvek: prvni tah v SAN
     * z mezipameti motoru, ostatni souradnicove.
     */
    private fun slozVariantu(pv: List<Move>, san: List<String>): List<String> {
        if (pv.isEmpty()) return emptyList()
        if (san.isNotEmpty()) {
            val koren = Position.fromFen(korenHledani)
            return radkyVarianty(
                san,
                cernyNaTahu = koren?.sideToMove == Side.BLACK,
                cisloTahu = koren?.fullmoveNumber ?: 1,
            )
        }
        val prvni = sanZMotoru(pv.first()) ?: pv.first().toUci()
        return listOf(prvni) + pv.drop(1).map { it.toUci() }
    }

    /**
     * Prida hlaseni do [sledovaniVolby] (krok U14/C). Jen za Premyslej
     * (HLEDA, DOBIHA_ZAHRAT); analyza predchozi volbu nesleduje. SAN tahu je
     * prvni prvek prevodu varianty, kdyz prevod nevratil nic, tak
     * z mezipameti motoru, jinak souradnicove. Cas z [hodiny] od zacatku
     * hledani.
     */
    private fun sledujVolbu(hloubka: Int, prvni: Move?, san: List<String>) {
        if (prvni == null || !sledujeVolbu()) return
        sledovaniVolby.pridej(
            HlaseniVolby(
                hloubka = hloubka,
                tah = prvni.toUci(),
                san = san.firstOrNull() ?: sanZMotoru(prvni) ?: prvni.toUci(),
                ms = hodiny() - zacatekHledani,
            )
        )
    }

    private fun sledujeVolbu() =
        stavHledani == StavHledani.HLEDA || stavHledani == StavHledani.DOBIHA_ZAHRAT

    /** Prubezna predchozi volba: nejvyssi hloubka je jeste otevrena. */
    private fun predchoziVolbaZaHledani(): PredchoziVolba? =
        if (sledujeVolbu()) sledovaniVolby.vysledek(konec = false) else null

    /**
     * Skore z hlaseni motoru prepoctene na pohled BILEHO.
     *
     * Oba motory hlasi podle UCI, tedy z pohledu strany na tahu: RedMAX proto,
     * ze `hledej` vraci korenovou hodnotu negamaxu a jeho hodnoceni je
     * z pohledu strany na tahu (POZNAMKY-REDMAX.md, "Zrcadlovy invariant");
     * Light Red proto, ze `pozice[0].alfa` je maximum sudych hladin, ktere
     * patri strane na tahu v koreni, a `co_schramstne` se na lichych hladinach
     * otaci. Jinymi slovy: obe jde prepocitat stejne.
     *
     * Hlaseni plati pro pozici, ve ktere hledani bezi - tedy pro tu, kterou
     * ma stav prave ted.
     */
    private fun skoreNaBileho(cp: Int?): Int? {
        if (cp == null) return null
        return if (stav().position.sideToMove == Side.WHITE) cp else -cp
    }

    /**
     * Cele sekundy od stisku Premyslej, jen za stavu HLEDA; jinak null.
     *
     * Prubezne hlaseni motoru cas nenese, takze tohle je jediny zdroj - a je
     * to spravny zdroj: meri se totez, co omezuje max sekund.
     *
     * Tika se jen ve stavu HLEDA. Po fialovem tlacitku i po
     * bestmove zmena stavu tikani zrusi (transformLatest), takze v KLID ani
     * v DOBIHA_* nic nebezi. Je to studeny Flow, a ne pole ve
     * [GameViewModel.state], schvalne: tika jen dokud ho nekdo sbira.
     * Obrazovka ho sbira pres
     * collectAsStateWithLifecycle, takze s aplikaci na pozadi tikani stoji -
     * a JVM testy, ktere nechavaji hledani bezet a volaji advanceUntilIdle(),
     * se v nekonecne smycce delay() nezacykli.
     *
     * [zacatekHledani] se cte az pri kazdem tiku, ne pri vzniku Flow.
     */
    @OptIn(ExperimentalCoroutinesApi::class)
    fun sekundy(stavy: Flow<GameUiState>): Flow<Int?> = stavy
        .map { it.stavHledani == StavHledani.HLEDA }
        .distinctUntilChanged()
        .transformLatest { hleda ->
            if (!hleda) {
                emit(null)
                return@transformLatest
            }
            while (true) {
                emit(((hodiny() - zacatekHledani) / 1000).toInt())
                delay(TIK_MS)
            }
        }
        .distinctUntilChanged()
}
