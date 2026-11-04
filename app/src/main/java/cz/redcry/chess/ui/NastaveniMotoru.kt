package cz.redcry.chess.ui

import cz.redcry.chess.engine.EngineId

// Meze hledani kazdeho motoru (hloubka, sekundy, Elo) a jejich zadani cislem.
// Vzniklo v kroku R1 presunem z GameViewModel.kt.

/**
 * Oba stropy jednoho hledani. Hledani skonci na tom z nich, ktery prijde driv.
 *
 * [maxSekund] = 0 znamena BEZ CASOVEHO OMEZENI, viz [EngineId.BEZ_OMEZENI].
 *
 * [elo] (krok S6): sila motoru podle Elo, null = plna sila. Jinou nez null
 * ma jen motor s [EngineId.minElo].
 */
data class MezeHledani(val maxHloubka: Int, val maxSekund: Int, val elo: Int? = null)

/**
 * Max sekund o jeden krok nahoru ([smer] > 0) nebo dolu. Krok je nelinearni:
 * po 1 do 10, po 5 do 60, po 30 vys. Po jednicce az k 900 by to bylo osm set
 * klepnuti, po tricitce by zase pod minutou nesly nastavit bezne hodnoty.
 *
 * Hodnota zadana cislem nemusi na mrizce lezet (37). Krok z ni vede na
 * nejblizsi bod mrizky v danem smeru - 37 nahoru je 40, dolu 35 - takze se
 * po jednom klepnuti na mrizku zase vrati. Proto se velikost kroku dolu
 * bere podle `ted - 1`: z 60 dolu je to 55, ne 30.
 *
 * Vysledek je orezany do [EngineId.MIN_SEKUND]..[EngineId.MAX_SEKUND], takze
 * na kraji vrati tutez hodnotu. Ciste, aby na to sel napsat JVM test.
 */
fun krokSekund(ted: Int, smer: Int): Int {
    fun krok(x: Int) = when {
        x < 10 -> 1
        x < 60 -> 5
        else -> 30
    }
    val nova = if (smer > 0) {
        val k = krok(ted)
        (ted / k + 1) * k
    } else {
        val k = krok(ted - 1)
        (ted - 1) / k * k
    }
    return nova.coerceIn(EngineId.MIN_SEKUND, EngineId.MAX_SEKUND)
}

/** Co z textoveho pole vyslo, viz [ctiZadani]. */
sealed interface Zadani {
    /** Prazdne pole - dialog se zavre a nic se nemeni. */
    data object Prazdne : Zadani

    data object NeniCislo : Zadani

    /** [orezano] = true, kdyz zadane cislo lezelo mimo meze a sedlo na ne. */
    data class Hodnota(val hodnota: Int, val orezano: Boolean) : Zadani
}

/**
 * Text z dialogu na hodnotu v [min]..[max].
 *
 * Cte se pres Long: "99999999999" je porad cislo, jen prilis velke, a ma se
 * oriznout na mez, ne ohlasit jako "neni cislo".
 */
fun ctiZadani(text: String, min: Int, max: Int): Zadani {
    val t = text.trim()
    if (t.isEmpty()) return Zadani.Prazdne
    val cislo = t.toLongOrNull() ?: return Zadani.NeniCislo
    val orezane = cislo.coerceIn(min.toLong(), max.toLong()).toInt()
    return Zadani.Hodnota(orezane, orezano = orezane.toLong() != cislo)
}

/**
 * Nastaveni hledani pro KAZDY motor zvlast - max hloubka i max sekundy.
 *
 * Spolecna hodnota nedava smysl: osmicka je pro Light Red skoro strop
 * (max 12) a pro RedMAX skoro nic (max 50). Prepnuti motoru by navic cizi
 * hodnotu privleklo s sebou. Do kroku 12 to byla trida HloubkyMotoru, ktera
 * drzela jen hloubku.
 *
 * Je to vlastni trida, a ne mapa uvnitr ViewModelu, aby na to sel napsat JVM
 * test: prepnuti motoru ve ViewModelu vyrabi skutecny motor pres
 * EngineFactory a v testu by spadlo na UnsatisfiedLinkError. "Kazdy motor si
 * drzi svoje hodnoty pres prepnuti a zpatky" se tedy da overit jedine tady.
 */
class NastaveniMotoru {

    // Vychozi hodnoty se orezou do mezi motoru, takze i motor s uzkym
    // rozsahem dostane platnou.
    private val hodnoty: MutableMap<EngineId, MezeHledani> =
        EngineId.entries.associateWith {
            MezeHledani(
                maxHloubka = it.vychoziHloubka.coerceIn(it.minHloubka, it.maxHloubka),
                maxSekund = it.vychoziSekundy
                    .coerceIn(EngineId.MIN_SEKUND, EngineId.MAX_SEKUND),
            )
        }.toMutableMap()

    operator fun get(id: EngineId): MezeHledani = hodnoty.getValue(id)

    /**
     * Posune max hloubku motoru o [delta] a vrati novou hodnotu, orezanou do
     * mezi, ktere motor sam hlasi na prikaz `uci` ([EngineId.minHloubka]).
     *
     * Na kraji rozsahu vrati tutez hodnotu, jakou mel - volajici tim pozna, ze
     * se nic nezmenilo, a muze to rict.
     */
    fun zmenHloubku(id: EngineId, delta: Int): Int {
        val nova = (hodnoty.getValue(id).maxHloubka + delta)
            .coerceIn(id.minHloubka, id.maxHloubka)
        hodnoty[id] = hodnoty.getValue(id).copy(maxHloubka = nova)
        return nova
    }

    /**
     * Totez pro casovy strop, jen o jeden NELINEARNI krok ve smeru [smer],
     * viz [krokSekund]. Meze uz nejsou motoru vlastni - nehlasi je
     * v odpovedi na `uci`, je to volba RedCry, a proto jsou spolecne
     * ([EngineId.MIN_SEKUND] az [EngineId.MAX_SEKUND]).
     */
    fun zmenSekundy(id: EngineId, smer: Int): Int {
        val nova = krokSekund(hodnoty.getValue(id).maxSekund, smer)
        hodnoty[id] = hodnoty.getValue(id).copy(maxSekund = nova)
        return nova
    }

    /** Presna hodnota z dialogu. Orezava se i tady, at to neni jen na volajicim. */
    fun nastavHloubku(id: EngineId, hloubka: Int): Int {
        val nova = hloubka.coerceIn(id.minHloubka, id.maxHloubka)
        hodnoty[id] = hodnoty.getValue(id).copy(maxHloubka = nova)
        return nova
    }

    fun nastavSekundy(id: EngineId, sekundy: Int): Int {
        val nova = sekundy.coerceIn(EngineId.MIN_SEKUND, EngineId.MAX_SEKUND)
        hodnoty[id] = hodnoty.getValue(id).copy(maxSekund = nova)
        return nova
    }

    /**
     * Posledni Elo kazdeho motoru, i kdyz je prave zapnuta plna sila - at se
     * po vypnuti Plne sily vrati to, co uzivatel nastavil naposledy.
     */
    private val posledniElo = mutableMapOf<EngineId, Int>()

    /**
     * Elo motoru (krok S6), orezane do [EngineId.minElo]..[EngineId.maxElo];
     * null = plna sila. Motor bez Elo ma vzdy null a vrati se null.
     */
    fun nastavElo(id: EngineId, elo: Int?): Int? {
        val min = id.minElo ?: return null
        val max = id.maxElo ?: return null
        val nova = elo?.coerceIn(min, max)
        if (nova != null) posledniElo[id] = nova
        hodnoty[id] = hodnoty.getValue(id).copy(elo = nova)
        return nova
    }

    /**
     * Elo pro vypnuti Plne sily: naposledy nastavene, jinak
     * [EngineId.VYCHOZI_ELO_OSLABENI]. Null u motoru bez Elo.
     */
    fun eloPriOslabeni(id: EngineId): Int? {
        val min = id.minElo ?: return null
        val max = id.maxElo ?: return null
        return (posledniElo[id] ?: EngineId.VYCHOZI_ELO_OSLABENI).coerceIn(min, max)
    }
}
