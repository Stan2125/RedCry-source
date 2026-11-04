package cz.redcry.chess.analyza

import cz.redcry.chess.engine.EngineEvent
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.model.Move
import cz.redcry.chess.texty.Texty
import cz.redcry.chess.ui.Jazyk
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

/**
 * Stav analyzy pro obrazovku. null v GameUiState = zadna analyza ani okno.
 *
 * @param okno           okno pred spustenim, null za behu
 * @param pruh           text pruhu dole, jen za behu
 * @param dotazPreruseni dialog "Přerušit analýzu?"
 */
data class AnalyzaUi(
    val okno: OknoAnalyzy? = null,
    val bezi: Boolean = false,
    val pruh: String? = null,
    val dotazPreruseni: Boolean = false,
)

/**
 * Okno "Analýza partie". Motor, cas a rozsah si drzi obrazovka, dokud se
 * nespusti; tady jsou jen meze a to, co obrazovka sama nezjisti.
 *
 * @param minTah, maxTah  meze "od tahu - do tahu" (cisla jako "Tah N")
 * @param slozka          jmeno zvolene slozky, null = vybrat
 * @param pokracovani     nedokoncena analyza TETO partie
 */
data class OknoAnalyzy(
    val minTah: Int,
    val maxTah: Int,
    val slozka: String?,
    val pokracovani: PokracovaniAnalyzy?,
)

/** Co nabidnout u "Pokračovat od tahu N" - prevzate z prubehu, needitovatelne. */
data class PokracovaniAnalyzy(
    val motor: EngineId,
    val sekundy: Int,
    val odTahu: Int,
    val doTahu: Int,
    val odTahuPokracovani: Int,
)

/**
 * Co od GameViewModelu potrebuje okno a spousteni analyzy navic k tomu, co
 * potrebuje samo rizeni ([HostitelAnalyzy]). ViewModel to implementuje
 * jednim objektem; vsechno ostatni o analyze je tady a v [RizeniAnalyzy],
 * aby ViewModel nerostl.
 */
interface HraProAnalyzu : HostitelAnalyzy {
    val scope: CoroutineScope

    /** Straze ViewModelu: posle hlasku a vrati true, kdyz se nesmi zacit. */
    fun zaneprazdneno(): Boolean

    fun hlaska(text: String)

    /** Texty pro hlasky a pruh analyzy (krok J1), aktualni jazyk (J2). */
    val texty: Texty

    /** Aktualni jazyk; pri spusteni se zafixuje do prubehu (krok J3). */
    val jazyk: Jazyk

    /** Stav hledani je ANALYZA. */
    val analyzuje: Boolean

    /** Cela partie, jak je ted v zaznamu. */
    fun partie(): AnalyzovanaPartie
    fun vychoziFen(): String

    /** GameUiState.analyza. */
    var ui: AnalyzaUi?

    /** Kam se vraci onChybaMotoru; analyza ho po sobe vraci. */
    var predchoziMotor: EngineId

    /** Stav hledani ANALYZA, vymazany vyber a hodnoceni. */
    fun zacni()

    /** Puvodni motor (jen kdyz se prepinal), jeho sila, deska a KLID. */
    fun obnov(puvodniMotor: EngineId)
}

/**
 * Vstupy obrazovky pro analyzu partie (krok A1/B): okno, spusteni,
 * pokracovani, pauza a preruseni. ViewModel ji vystavuje jako
 * `analyza` a sam do ni posila jen Info, BestMove a Chyba motoru.
 */
class NapojeniAnalyzy(private val hra: HraProAnalyzu) {

    /**
     * Soubory a baterie. Nastavi obrazovka jednou (applicationContext)
     * a nenuluje ji: ViewModel prezije Activity a bezici analyza by pri
     * zapisu narazila na null.
     */
    var uloziste: UlozisteAnalyzy? = null

    private var rizeni: RizeniAnalyzy? = null
    private var beh: Job? = null

    /** Nedokoncena analyza teto partie, nalezena pri otevreni okna. */
    private var nactenyPrubeh: PrubehAnalyzy? = null

    fun naInfo(info: EngineEvent.Info) {
        rizeni?.naInfo(info)
    }

    fun naBestMove(tah: Move?) {
        rizeni?.naBestMove(tah)
    }

    /**
     * Chyba motoru za analyzy: rizeni skonci BEZ vlastni obnovy - motor,
     * silu a pozici obnovi onChybaMotoru jednim prepnutim. true = analyza
     * bezela (ViewModel pak spoji hlasky do jedne).
     */
    fun zrusPriChybe(): Boolean {
        if (rizeni == null) return false
        rizeni = null
        beh?.cancel()
        beh = null
        return true
    }

    /** Menu Hra: za analyzy se neotevre (Nacist by jinak nejdriv otevrelo vyber souboru). */
    fun onMenuHra(): Boolean {
        if (!hra.analyzuje) return true
        hra.hlaska(hra.texty.analyzaBezi)
        return false
    }

    /** Polozka "Analýza partie…": otevre okno, s nabidkou pokracovani. */
    fun onDotaz() {
        if (hra.zaneprazdneno()) return
        val partie = hra.partie()
        if (partie.tahyUci.isEmpty()) {
            hra.hlaska(hra.texty.partieNemaTah)
            return
        }
        val ul = uloziste ?: return
        val fen = hra.vychoziFen()
        val c = partie.cislovani
        hra.scope.launch {
            val prubeh = ul.ctiPrubeh()?.let { ctiPrubeh(it) }
                ?.takeIf { it.jeTataPartie(fen, partie.tahyUci) && it.dalsiPozice() != null }
            nactenyPrubeh = prubeh
            val pokracovani = prubeh?.let {
                val posledni = c.cisloTahu(it.rozsah.doPultahu)
                PokracovaniAnalyzy(
                    motor = it.motor,
                    sekundy = it.sekundy,
                    odTahu = c.cisloTahu(it.rozsah.od),
                    doTahu = posledni,
                    odTahuPokracovani = minOf(c.cisloTahu(it.dalsiPozice()!!), posledni),
                )
            }
            hra.ui = AnalyzaUi(
                okno = OknoAnalyzy(
                    minTah = c.cisloTahu(0),
                    maxTah = c.cisloTahu(partie.tahyUci.size - 1),
                    slozka = ul.jmenoSlozky(),
                    pokracovani = pokracovani,
                ),
            )
        }
    }

    fun onZavrit() {
        if (rizeni == null) hra.ui = null
    }

    /** Po vyberu slozky: jmeno do okna. */
    fun onSlozkaZmenena() {
        val ul = uloziste ?: return
        hra.scope.launch {
            val jmeno = ul.jmenoSlozky()
            val a = hra.ui ?: return@launch
            val okno = a.okno ?: return@launch
            hra.ui = a.copy(okno = okno.copy(slozka = jmeno))
        }
    }

    /** "Začít znovu": rozpracovana analyza se smaze. */
    fun onZacitZnovu() {
        nactenyPrubeh = null
        val a = hra.ui
        val okno = a?.okno
        if (a != null && okno != null) hra.ui = a.copy(okno = okno.copy(pokracovani = null))
        val ul = uloziste ?: return
        hra.scope.launch { ul.smazPrubeh() }
    }

    fun onSpustit(motor: EngineId, sekundy: Int, odTahu: Int, doTahu: Int) {
        if (hra.zaneprazdneno()) return
        if (hra.ui?.okno?.slozka == null) {
            hra.hlaska(hra.texty.nejdrivVyberSlozku)
            return
        }
        val partie = hra.partie()
        val rozsah = rozsahPultahu(odTahu, doTahu, partie.tahyUci.size, partie.cislovani)
        if (rozsah == null) {
            hra.hlaska(hra.texty.vZadanemRozsahuNeniTah)
            return
        }
        spust(
            PrubehAnalyzy(
                vychoziFen = hra.vychoziFen(),
                tahyUci = partie.tahyUci,
                motor = motor,
                sekundy = sekundy.coerceIn(MIN_SEKUND_ANALYZY, MAX_SEKUND_ANALYZY),
                rozsah = rozsah,
                nazev = jmenoAnalyzy(),
                uriPgn = null,
                uriCsv = null,
                vysledky = emptyMap(),
                jazyk = hra.jazyk,
            ),
            partie,
        )
    }

    fun onPokracovat() {
        if (hra.zaneprazdneno()) return
        nactenyPrubeh?.let { spust(it, hra.partie()) }
    }

    private fun spust(prubeh: PrubehAnalyzy, partie: AnalyzovanaPartie) {
        val ul = uloziste ?: return
        nactenyPrubeh = null
        val puvodniMotor = hra.vybranyMotor
        val puvodniPredchozi = hra.predchoziMotor
        val r = RizeniAnalyzy(hra, ul, partie, prubeh) { hra.texty }
        rizeni = r
        hra.zacni()
        hra.ui = AnalyzaUi(bezi = true)
        beh = hra.scope.launch {
            val konec = r.beh()
            rizeni = null
            beh = null
            hra.obnov(puvodniMotor)
            // Prepnuti zpet predchozi motor prepise; plati ten pred analyzou.
            hra.predchoziMotor = puvodniPredchozi
            hra.hlaska(r.hlaska(konec))
        }
    }

    /** Prepnuti jazyka (krok J2): pruh bezici analyzy v novem jazyce. */
    fun naZmenuJazyka() {
        rizeni?.obnovPruh()
    }

    /** ON_STOP. Bez bezici analyzy nic (treba otevreny vyber slozky). */
    fun onPozastav(duvod: DuvodPauzy) {
        rizeni?.pozastav(duvod)
    }

    /** ON_START. Bez bezici analyzy nic. */
    fun onPokracuj(duvod: DuvodPauzy) {
        rizeni?.pokracuj(duvod)
    }

    /** Klepnuti na pruh nebo Zpet: dialog "Přerušit analýzu?". */
    fun onPrerusDotaz() {
        val a = hra.ui ?: return
        if (rizeni != null) hra.ui = a.copy(dotazPreruseni = true)
    }

    fun onPrerusZrusen() {
        val a = hra.ui ?: return
        hra.ui = a.copy(dotazPreruseni = false)
    }

    fun onPrerus() {
        onPrerusZrusen()
        rizeni?.prerus()
    }
}
