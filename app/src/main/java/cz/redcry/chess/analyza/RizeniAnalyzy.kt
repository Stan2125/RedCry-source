package cz.redcry.chess.analyza

import cz.redcry.chess.engine.EngineEvent
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.engine.STAV_MAT
import cz.redcry.chess.engine.STAV_PAT
import cz.redcry.chess.model.Move
import cz.redcry.chess.texty.Texty
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update

/** Pod tolik procent bez nabijeni se analyza prerusi (zadani A1). */
const val MIN_BATERIE_PROCENT = 15

/**
 * Proc analyza stoji. Pokracuje se, az nezbude zadny.
 *
 * Pauza kvuli teplu byla do kroku A1/D. Ochranu pred prehratim nechava
 * aplikace na systemu (rozhodnuti uzivatele, POZNAMKY-ANALYZA.md).
 */
enum class DuvodPauzy {
    /** Aplikace neni videt (ON_STOP). */
    POZADI,
}

data class StavBaterie(val procent: Int, val nabiji: Boolean)

/**
 * Soubory a baterie pro analyzu. Jen texty, zadne Uri ani Context: Uri jsou
 * tu retezce, se kterymi pracuje jen implementace na strane Androidu
 * (ui/UlozisteAnalyzyAndroid.kt). Diky tomu jde rizeni i ViewModel
 * testovat v JVM, stejne jako u nacitani PGN (POZNAMKY-PGN.md, SAF).
 */
interface UlozisteAnalyzy {
    /** Jmeno zvolene slozky, null = zadna nebo uz neni pristupna. */
    suspend fun jmenoSlozky(): String?

    /** Zalozi `<nazev>.pgn` a `<nazev>.csv` ve slozce; Uri obou, nebo null. */
    suspend fun zalozSoubory(nazev: String): Pair<String, String>?

    /** Prepise oba soubory celym obsahem. false = chyba zapisu. */
    suspend fun zapis(uriPgn: String, uriCsv: String, pgn: String, csv: String): Boolean

    suspend fun ulozPrubeh(text: String): Boolean
    suspend fun ctiPrubeh(): String?
    suspend fun smazPrubeh()

    /** Stav baterie, null = nezjisteno (pak se nic neprerusuje). */
    fun baterie(): StavBaterie?
}

/**
 * Co rizeni potrebuje od GameViewModelu. ViewModel je jediny vlastnik
 * motoru, historie i stavu obrazovky, tak to rizeni dela pres nej - samo
 * motor ani stav nedrzi.
 */
interface HostitelAnalyzy {
    val vybranyMotor: EngineId

    /**
     * Prepne motor tataz cestou jako vyber motoru, stav hledani zustane
     * ANALYZA. null = povedlo se, jinak zprava (motor se vratil puvodni).
     */
    fun prepniMotor(id: EngineId): String?

    /**
     * Pocka, az motor dostal pozici z historie (cekaniNaPozici po prepnuti)
     * a je pripraveny. Teprve pak smi rizeni volat setPosition a go -
     * jinak by pozice z historie mohla dorazit az za nim.
     */
    suspend fun pockejNaMotor(): Boolean

    fun nastavSilu(elo: Int?)

    /** Motor i deska na `history[p]`. false = motor pozici neprijal. */
    fun ukazPozici(p: Int): Boolean

    fun stav(): String
    fun go(hloubka: Int, casMs: Int)
    fun stop()
    fun san(tah: Move): String?
    fun applyMove(tah: Move): String?

    /** Text pruhu dole. */
    fun pruh(text: String)
}

sealed interface KonecAnalyzy {
    data object Hotovo : KonecAnalyzy
    data object Preruseno : KonecAnalyzy
    data class Baterie(val procent: Int) : KonecAnalyzy
    data class Chyba(val zprava: String) : KonecAnalyzy
    data class Nespustena(val zprava: String) : KonecAnalyzy
}

/**
 * Prubeh jedne analyzy (krok A1/B): pozice jedna po druhe, v kazde jedno
 * hledani, po kazde zapis obou souboru a prubehu. Tah motoru se nikdy
 * nehraje - BestMove sem preda ViewModel misto onSearchFinished.
 *
 * Bezi v jedne korutine ([beh]), vsechno ostatni ([pozastav], [prerus],
 * [naInfo], [naBestMove]) se vola z hlavniho vlakna mezi jejimi body
 * preruseni. Proto staci obycejne promenne bez zamku.
 *
 * Pauza i preruseni za hledani posle stop() a vysledek toho hledani
 * zahodi ([zahodit]). Na jeho BestMove se ale VZDYCKY pocka: dalsi go do
 * motoru, ktery jeste hleda, by nativni strana nevzala.
 */
class RizeniAnalyzy(
    private val host: HostitelAnalyzy,
    private val uloziste: UlozisteAnalyzy,
    private val partie: AnalyzovanaPartie,
    prubeh: PrubehAnalyzy,
    /**
     * Texty hlasek a pruhu, cte se pri kazdem pouziti: po prepnuti jazyka
     * (krok J2) jsou dalsi v novem jazyce.
     */
    private val aktualniTexty: () -> Texty,
) {
    private val texty: Texty get() = aktualniTexty()

    /**
     * Texty vystupu do souboru (Annotator, komentare, cela CSV) v jazyce
     * platnem pri spusteni ([PrubehAnalyzy.jazyk], krok J3). Nemeni se
     * prepnutim ani pri pokracovani po restartu.
     */
    private val textySouboru: Texty = prubeh.jazyk.texty

    var prubeh: PrubehAnalyzy = prubeh
        private set

    private data class Ridici(val duvody: Set<DuvodPauzy>, val preruseno: Boolean)

    private val ridici = MutableStateFlow(
        Ridici(emptySet(), preruseno = false),
    )

    /** Ceka se na BestMove beziciho hledani; null = nic nehleda. */
    private var hledani: CompletableDeferred<Move?>? = null

    /** Bezici hledani se zastavilo pauzou nebo prerusenim - nezapise se. */
    private var zahodit = false

    /** Posledni Info beziciho hledani, nuluje se pri kazdem go. */
    private var posledniInfo: EngineEvent.Info? = null

    /** Pozice, nad kterou se premysli nebo bude premyslet. */
    private var aktualni: Int = prubeh.dalsiPozice() ?: prubeh.rozsah.od

    private val annotator = textAnnotatoru(prubeh.motor.label, prubeh.sekundy, textySouboru)
    private val c = partie.cislovani
    private val posledniTah = c.cisloTahu(prubeh.rozsah.doPultahu)

    fun pozastav(duvod: DuvodPauzy) {
        ridici.update { it.copy(duvody = it.duvody + duvod) }
        zastavHledani()
        aktualizujPruh()
    }

    fun pokracuj(duvod: DuvodPauzy) {
        ridici.update { it.copy(duvody = it.duvody - duvod) }
        aktualizujPruh()
    }

    fun prerus() {
        ridici.update { it.copy(preruseno = true) }
        zastavHledani()
    }

    fun naInfo(info: EngineEvent.Info) {
        if (hledani != null && !zahodit) posledniInfo = info
    }

    fun naBestMove(tah: Move?) {
        val h = hledani ?: return
        hledani = null
        h.complete(tah)
    }

    private fun zastavHledani() {
        if (hledani == null || zahodit) return
        zahodit = true
        host.stop()
    }

    private val preruseno: Boolean get() = ridici.value.preruseno

    /** Cela analyza. Vraci, jak skoncila; obnovu motoru a desky dela ViewModel. */
    suspend fun beh(): KonecAnalyzy {
        // Pruh hned, i kdyz se motor jeste zaklada.
        aktualizujPruh()
        val id = prubeh.motor
        if (id != host.vybranyMotor) {
            host.prepniMotor(id)?.let { return KonecAnalyzy.Nespustena(it) }
        }
        if (!host.pockejNaMotor()) return KonecAnalyzy.Nespustena(texty.motorNesloSpustitAnalyza(id.label))
        // Az po prepnuti: prepnuti posle motoru silu podle nastaveni
        // uzivatele (posliSilu) a analyza chce plnou.
        if (id.minElo != null) host.nastavSilu(null)
        if (preruseno) return KonecAnalyzy.Preruseno
        aktualizujPruh()

        if (!pripravSoubory()) {
            return if (prubeh.uriPgn == null) {
                KonecAnalyzy.Nespustena(texty.souboryNesloZalozit)
            } else {
                KonecAnalyzy.Chyba(texty.analyzaChybaZapisu)
            }
        }

        while (true) {
            val p = prubeh.dalsiPozice() ?: break
            aktualni = p
            aktualizujPruh()
            ridici.first { it.duvody.isEmpty() || it.preruseno }
            if (preruseno) return KonecAnalyzy.Preruseno
            uloziste.baterie()?.let {
                if (it.procent < MIN_BATERIE_PROCENT && !it.nabiji) return KonecAnalyzy.Baterie(it.procent)
            }
            if (!host.ukazPozici(p)) return KonecAnalyzy.Chyba(texty.analyzaPozicNesloNastavit)
            val vysledek = when (host.stav()) {
                STAV_MAT -> VysledekPozice(konec = KonecPozice.MAT)
                STAV_PAT -> VysledekPozice(konec = KonecPozice.PAT)
                else -> hledej(p) ?: continue
            }
            prubeh = prubeh.copy(vysledky = prubeh.vysledky + (p to vysledek))
            if (!zapisVse()) return KonecAnalyzy.Chyba(texty.analyzaChybaZapisu)
            if (preruseno) return KonecAnalyzy.Preruseno
        }
        uloziste.smazPrubeh()
        return KonecAnalyzy.Hotovo
    }

    /**
     * Jedno hledani v pozici [p]. null = zastavene pauzou nebo prerusenim,
     * pozice se pak hleda znovu (nebo vubec, kdyz se prerusilo).
     *
     * Hloubka je strop motoru, ne nastaveni uzivatele: rozhoduje cas na
     * pozici, stejne jako u Premyslej od kroku 18.
     */
    private suspend fun hledej(p: Int): VysledekPozice? {
        posledniInfo = null
        zahodit = false
        val cekani = CompletableDeferred<Move?>()
        hledani = cekani
        host.go(prubeh.motor.maxHloubka, prubeh.sekundy * 1000)
        val tah = cekani.await()
        if (zahodit) return null

        val info = posledniInfo
        // Motor hlasi z pohledu strany na tahu v history[p], ne podle desky.
        val skore = info?.scoreCp?.let { if (c.cernyTahne(p)) -it else it }
        val pv = info?.pv.orEmpty()
        val tahy = when {
            tah == null -> emptyList()
            pv.firstOrNull() == tah -> pv
            else -> listOf(tah)
        }
        return VysledekPozice(
            hloubka = info?.depth,
            skoreBily = skore,
            tahUci = tah?.toUci(),
            varianta = prevedVariantu(p, tahy),
        )
    }

    /**
     * Varianta do SAN az po konci hledani, kdy motor nehleda: kazdy tah se
     * prevede v pozici, kam vedly ty pred nim (san + applyMove), na prvnim
     * tahu, ktery motor neprijme, varianta konci. Pak zpet na [p].
     * Prevod v Kotlinu by potreboval pravidla (Nbd2 proti Nfd2).
     */
    private fun prevedVariantu(p: Int, tahy: List<Move>): List<String> {
        if (tahy.isEmpty()) return emptyList()
        val ven = ArrayList<String>()
        for (tah in tahy) {
            val san = host.san(tah) ?: break
            ven.add(san)
            if (host.applyMove(tah) == null) break
        }
        host.ukazPozici(p)
        return ven
    }

    /**
     * Novou analyzu zalozi soubory, pokracovani pise do tychz. Kdyz uz
     * nejsou pristupne, zalozi nove a zapise do nich vsechno z prubehu.
     */
    private suspend fun pripravSoubory(): Boolean {
        if (prubeh.uriPgn != null && prubeh.uriCsv != null && zapisVse()) return true
        val nove = uloziste.zalozSoubory(prubeh.nazev) ?: return false
        prubeh = prubeh.copy(uriPgn = nove.first, uriCsv = nove.second)
        return zapisVse()
    }

    private suspend fun zapisVse(): Boolean {
        val pgn = zapisPgnAnalyzy(partie, prubeh.rozsah, prubeh.vysledky, annotator, textySouboru)
        val csv = zapisCsvAnalyzy(partie, prubeh.rozsah, prubeh.vysledky, textySouboru)
        return uloziste.zapis(prubeh.uriPgn ?: return false, prubeh.uriCsv ?: return false, pgn, csv) &&
            uloziste.ulozPrubeh(zapisPrubeh(prubeh))
    }

    /** Po prepnuti jazyka (krok J2): pruh znovu, uz v novem jazyce. */
    fun obnovPruh() = aktualizujPruh()

    private fun aktualizujPruh() {
        val duvody = ridici.value.duvody
        host.pruh(
            when {
                duvody.isNotEmpty() -> texty.pruhPozastavena
                else -> textPruhu(
                    minOf(c.cisloTahu(aktualni), posledniTah),
                    posledniTah,
                    prubeh.zbyvaPozic(),
                    prubeh.sekundy,
                    texty,
                )
            },
        )
    }

    /** Hlaska po konci. Jedna, protoze dalsi by ji v U11 hned sundala. */
    fun hlaska(konec: KonecAnalyzy): String {
        val soubory = texty.souboryAnalyzy(prubeh.nazev)
        return when (konec) {
            KonecAnalyzy.Hotovo -> texty.analyzaHotova(soubory)
            KonecAnalyzy.Preruseno ->
                if (prubeh.uriPgn == null) {
                    texty.analyzaPrerusena
                } else {
                    val p = prubeh.dalsiPozice() ?: prubeh.rozsah.doPultahu
                    texty.analyzaPrerusenaNaTahu(minOf(c.cisloTahu(p), posledniTah), soubory)
                }
            is KonecAnalyzy.Baterie -> texty.analyzaPrerusenaBaterie(konec.procent)
            is KonecAnalyzy.Chyba -> konec.zprava
            is KonecAnalyzy.Nespustena -> texty.analyzaSeNespustila(konec.zprava)
        }
    }
}
