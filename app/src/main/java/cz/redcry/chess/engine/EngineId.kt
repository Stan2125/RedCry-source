package cz.redcry.chess.engine

import kotlinx.coroutines.CoroutineScope

/**
 * Seznam motoru pro rozbalovaci nabidku "Engine".
 *
 * Poradi je od nejslabsiho k nejsilnejsimu a je zamerne - je to zaroven
 * poradi, v jakem se do RedCry pridavaji.
 *
 * [dostupny] = false znamena, ze polozka se v nabidce ukaze, ale neda se
 * vybrat. Uzivatel tak vidi, co se chysta, a nedostane vyjimku.
 *
 * [minHloubka] a [maxHloubka] NEJSOU vymyslene - jsou opsane z radku
 * `option name Depth type spin ...`, kterym motor sam odpovida na prikaz
 * `uci`. Kazdy motor ma svoje, proto to sedi na polozce a ne na jednom
 * spolecnem miste.
 *
 * [vychoziHloubka] a [vychoziSekundy] naopak vymyslene JSOU - je to volba
 * RedCry, ne nic, co by motor hlasil. Od kroku 18 je hloubka u vsech na
 * stropu motoru (RedMAX 50, Light Red 12, Crafty 60, Stockfish 245)
 * a hledani ve skutecnosti rozhoduje cas, pet sekund - na fialovem tlacitku bezi odpocet a tah jde kdykoli
 * zahrat driv.
 *
 * [minElo] a [maxElo] (krok S6) jsou taky opsane z odpovedi na `uci`, jen
 * z radku `option name UCI_Elo type spin ...`. Ma ho jen Stockfish; null
 * = motor silu podle Elo nastavit neumi.
 */
enum class EngineId(
    val label: String,
    val popis: String,
    val dostupny: Boolean,
    val minHloubka: Int,
    val maxHloubka: Int,
    val vychoziHloubka: Int,
    val vychoziSekundy: Int,
    val minElo: Int? = null,
    val maxElo: Int? = null,
) {
    /** Meze z `uci.cpp`: `option name Depth type spin default 4 min 1 max 12`. */
    LIGHT_RED(
        label = "LightRed",
        popis = "nejslabsi, prepis vlastniho enginu z roku 2005",
        dostupny = true,
        minHloubka = 1,
        maxHloubka = 12,
        vychoziHloubka = 12,
        vychoziSekundy = 5,
    ),

    /** Meze z `red_max_uci.cpp`: `option name Depth type spin default 6 min 1 max 50`. */
    RED_MAX(
        label = "RedMax",
        popis = "silnejsi, podle knihy D. Steinwendera Sachy na PC",
        dostupny = true,
        minHloubka = 1,
        maxHloubka = 50,
        vychoziHloubka = 50,
        vychoziSekundy = 5,
    ),

    /**
     * Meze z `crafty_uci.cpp`: `option name Depth type spin default 6 min 1
     * max 60` - strop je iteracni smycka v Iterate (`iteration_depth <= 60`).
     */
    CRAFTY(
        label = "Crafty",
        popis = "Robert Hyatt, verze 18.15 (RedCrafty 4.81)",
        dostupny = true,
        minHloubka = 1,
        maxHloubka = 60,
        vychoziHloubka = 60,
        vychoziSekundy = 5,
    ),

    /**
     * Meze ze `stockfish_uci.cpp`: `option name Depth type spin default 20
     * min 1 max 245` - strop je iteracni smycka hledani Stockfishe
     * (`rootDepth + 1 < MAX_PLY`, MAX_PLY = 246), viz stockfish_uci.h.
     *
     * [vychoziSekundy] 5 po mereni na telefonech ponechano
     * (POZNAMKY-STOCKFISH.md, kroky S4/A a S4/B).
     *
     * Elo z `stockfish_uci.cpp`: `option name UCI_Elo type spin default 1320
     * min 1320 max 3190` (Search::Skill, krok S6).
     */
    STOCKFISH(
        label = "Stockfish",
        popis = "Stockfish 19",
        dostupny = true,
        minHloubka = 1,
        maxHloubka = 245,
        vychoziHloubka = 245,
        vychoziSekundy = 5,
        minElo = 1320,
        maxElo = 3190,
    );

    companion object {
        /**
         * Motor, se kterym se aplikace spousti (od kroku U9, driv RedMAX).
         * Stockfish je nejsilnejsi a od kroku S3 se zaklada na pozadi, takze
         * start aplikace nebrzdi - do dokonceni zakladani jen hlasi, ze se
         * jeste nacita.
         */
        val vychozi = STOCKFISH

        /**
         * Kam se ViewModel vrati, kdyz selze motor a predchozi ani [vychozi]
         * pouzit nejde (selhal prave on). RedMAX se zaklada synchronne
         * a [EngineEvent.Chyba] nehlasi, takze tady smycka skonci.
         */
        val zalozni = RED_MAX

        /**
         * Casovy strop na jedno hledani neni nic, co by motor hlasil - je to
         * volba RedCry, a proto je spolecny vsem, na rozdil od mezi hloubky.
         *
         * Spodni mez je NULA, ne jednicka: znamena "bez omezeni" a adapter
         * pri ni token `movetime` neposle vubec. Drive by to byla past -
         * hluboka pozice by pocitala minuty a nesla utnout. Dnes jde hledani
         * kdykoli ukoncit fialovym "Hraj nejlepsi", takze se motor
         * da nechat pocitat a utnout ho, az hloubka staci.
         *
         * Horni mez je od kroku 14 patnact minut (driv minuta). Stepper po ni
         * jde nelinearne, viz cz.redcry.chess.ui.krokSekund, a presnou
         * hodnotu jde zadat klepnutim na cislo.
         */
        const val BEZ_OMEZENI = 0
        const val MIN_SEKUND = BEZ_OMEZENI
        const val MAX_SEKUND = 900

        /**
         * Elo, na ktere se motor nastavi, kdyz uzivatel poprve vypne Plnou
         * silu (krok S6). Volba RedCry, ne nic, co by motor hlasil; orezava
         * se do [minElo]..[maxElo] motoru.
         */
        const val VYCHOZI_ELO_OSLABENI = 1500
    }
}

/**
 * Jedine misto, kde se motor vytvari.
 *
 * Drive tu byla i atrapa motoru z kroku 1; smazana v kroku U1.
 */
object EngineFactory {

    fun create(id: EngineId, scope: CoroutineScope): ChessEngine = when (id) {
        EngineId.LIGHT_RED -> LightRedEngine(scope)
        EngineId.RED_MAX -> RedMaxEngine(scope)
        EngineId.CRAFTY -> CraftyEngine(scope)
        EngineId.STOCKFISH -> StockfishEngine(scope)
    }

    /** Motory, ktere se daji skutecne vybrat. */
    fun dostupne(): List<EngineId> = EngineId.entries.filter { it.dostupny }

    /** Vsechny, vcetne nedostupnych - pro vykresleni nabidky. */
    fun vse(): List<EngineId> = EngineId.entries.toList()
}
