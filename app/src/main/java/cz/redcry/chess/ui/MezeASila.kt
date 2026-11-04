package cz.redcry.chess.ui

import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.texty.Texty

// Meze hledani (hloubka, sekundy) a sila motoru (Plna sila, Elo) z ovladani
// obrazovky. Vzniklo v kroku R2/A presunem z GameViewModel.kt; ViewModel
// handlery jen deleguje a sem pousti jen [HraProMeze].

/**
 * Co [MezeASila] potrebuje od ViewModelu (krok R2/A). Vybrany motor i motor
 * pro [nastavSilu] se ctou az pri volani - oba se meni pri prepnuti motoru
 * a po [cz.redcry.chess.engine.EngineEvent.Chyba].
 */
interface HraProMeze {
    val vybranyMotor: EngineId

    /** Texty hlasek (krok J1). */
    val texty: Texty

    /** Straz beziciho hledani, viz GameViewModel.zaneprazdneno. */
    fun zaneprazdneno(): Boolean

    fun hlaska(text: String)

    /** Sila do PRAVE aktualniho motoru. */
    fun nastavSilu(elo: Int?)

    fun upravStav(zmena: (GameUiState) -> GameUiState)
}

/** Meze Elo motoru do stavu, null u motoru bez Elo. */
internal fun rozsahElo(id: EngineId): IntRange? {
    val min = id.minElo ?: return null
    val max = id.maxElo ?: return null
    return min..max
}

/**
 * Handlery mezi a sily (krok R2/A). [nastaveni] vlastni ViewModel - cte je
 * i pri zalozeni stavu, prepnuti motoru a Premyslej.
 */
class MezeASila(private val hra: HraProMeze, private val nastaveni: NastaveniMotoru) {

    private val vybranyMotor: EngineId get() = hra.vybranyMotor

    private fun zaneprazdneno(): Boolean = hra.zaneprazdneno()

    private fun notify(text: String) = hra.hlaska(text)

    private val texty: Texty get() = hra.texty

    /**
     * Sila vybraneho motoru do motoru (krok S6). Po kazdem zalozeni motoru
     * a pri kazde zmene. Motor bez Elo nedostane nic - ani volani, at je
     * jejich chovani beze zmeny.
     */
    fun posliSilu() {
        if (vybranyMotor.minElo == null) return
        hra.nastavSilu(nastaveni[vybranyMotor].elo)
    }

    /**
     * Posune max hloubku vybraneho motoru o [delta]. Meze si nese sam motor -
     * jsou opsane z jeho odpovedi na `uci`, viz [EngineId.minHloubka].
     *
     * Na kraji se tlacitko nevypina, ale rekne se duvod - tataz uvaha jako
     * u sipek historie: vypnute tlacitko na dotykaku nerekne proc.
     */
    fun onHloubkaZmena(delta: Int) {
        if (zaneprazdneno()) return
        val ted = nastaveni[vybranyMotor].maxHloubka
        val nova = nastaveni.zmenHloubku(vybranyMotor, delta)
        if (nova == ted) {
            notify(
                if (delta < 0) texty.nizsiHloubkaNejde(vybranyMotor.minHloubka)
                else texty.umiNejvysHloubku(vybranyMotor.label, vybranyMotor.maxHloubka)
            )
            return
        }
        hra.upravStav { it.copy(maxHloubka = nova) }
    }

    /**
     * Totez pro casovy strop hledani, o jeden nelinearni krok ve smeru [smer]
     * ([krokSekund]). Meze jsou spolecne vsem motorum - nula (bez omezeni)
     * az [EngineId.MAX_SEKUND].
     *
     * Bez omezeni uz neni past, jakou by bylo drive: hledani jde kdykoli utnout
     * fialovym Hraj nejlepsi, takze se motor da nechat
     * pocitat a zastavit ho, az hloubka staci.
     */
    fun onSekundyZmena(smer: Int) {
        if (zaneprazdneno()) return
        val ted = nastaveni[vybranyMotor].maxSekund
        val nova = nastaveni.zmenSekundy(vybranyMotor, smer)
        if (nova == ted) {
            notify(
                if (smer < 0) texty.meneNezBezOmezeni
                else texty.nejvysSekund(EngineId.MAX_SEKUND)
            )
            return
        }
        hra.upravStav { it.copy(maxSekund = nova) }
    }

    /**
     * Max hloubka zadana cislem z dialogu. Mimo meze motoru se orizne a rekne
     * se to; prazdne pole necha hodnotu byt a nerika nic - to je "rozmyslel
     * jsem si to", ne chyba.
     */
    fun onHloubkaZadana(text: String) {
        if (zaneprazdneno()) return
        val id = vybranyMotor
        when (val z = ctiZadani(text, id.minHloubka, id.maxHloubka)) {
            Zadani.Prazdne -> Unit
            Zadani.NeniCislo -> notify(neniCislo(text))
            is Zadani.Hodnota -> {
                val nova = nastaveni.nastavHloubku(id, z.hodnota)
                hra.upravStav { it.copy(maxHloubka = nova) }
                if (z.orezano) notify(
                    if (nova == id.minHloubka) texty.nizsiHloubkaNastaveno(nova)
                    else texty.umiHloubkuNastaveno(id.label, nova)
                )
            }
        }
    }

    /** Totez pro max sekund. Nula je platna hodnota - bez omezeni. */
    fun onSekundyZadane(text: String) {
        if (zaneprazdneno()) return
        when (val z = ctiZadani(text, EngineId.MIN_SEKUND, EngineId.MAX_SEKUND)) {
            Zadani.Prazdne -> Unit
            Zadani.NeniCislo -> notify(neniCislo(text))
            is Zadani.Hodnota -> {
                val nova = nastaveni.nastavSekundy(vybranyMotor, z.hodnota)
                hra.upravStav { it.copy(maxSekund = nova) }
                if (z.orezano) notify(
                    if (nova == EngineId.MIN_SEKUND) texty.zapornyCasNastaveno
                    else texty.nejvysSekundNastaveno(nova)
                )
            }
        }
    }

    private fun neniCislo(text: String) = texty.neniCislo(text)

    /**
     * Prepinac Plna sila (krok S6). Vypnuti nastavi naposledy pouzite Elo,
     * jinak [EngineId.VYCHOZI_ELO_OSLABENI]. Motor bez Elo nic.
     */
    fun onPlnaSila(zapnuto: Boolean) {
        if (zaneprazdneno()) return
        val id = vybranyMotor
        if (id.minElo == null) return
        val elo = if (zapnuto) null else nastaveni.eloPriOslabeni(id)
        if (elo == nastaveni[id].elo) return
        val nova = nastaveni.nastavElo(id, elo)
        hra.upravStav { it.copy(elo = nova, eloOslabeni = nastaveni.eloPriOslabeni(id)) }
        posliSilu()
    }

    /** Elo zadane cislem z dialogu, orez do mezi motoru jako u hloubky. */
    fun onEloZadane(text: String) {
        if (zaneprazdneno()) return
        val id = vybranyMotor
        val min = id.minElo ?: return
        val max = id.maxElo ?: return
        when (val z = ctiZadani(text, min, max)) {
            Zadani.Prazdne -> Unit
            Zadani.NeniCislo -> notify(neniCislo(text))
            is Zadani.Hodnota -> {
                val nova = nastaveni.nastavElo(id, z.hodnota)
                hra.upravStav { it.copy(elo = nova, eloOslabeni = nastaveni.eloPriOslabeni(id)) }
                posliSilu()
                if (z.orezano) notify(
                    texty.umiEloNastaveno(id.label, min, max, nova)
                )
            }
        }
    }
}
