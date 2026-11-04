package cz.redcry.chess.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import cz.redcry.chess.analyza.AnalyzaUi
import cz.redcry.chess.analyza.AnalyzovanaPartie
import cz.redcry.chess.analyza.Cislovani
import cz.redcry.chess.analyza.HraProAnalyzu
import cz.redcry.chess.analyza.NapojeniAnalyzy
import cz.redcry.chess.engine.ChessEngine
import cz.redcry.chess.engine.EngineEvent
import cz.redcry.chess.engine.EngineFactory
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.engine.MotorUzBeziException
import cz.redcry.chess.engine.SanVarianty
import cz.redcry.chess.kniha.KnihaZahajeni
import cz.redcry.chess.kniha.Zahajeni
import cz.redcry.chess.model.Move
import cz.redcry.chess.model.PieceType
import cz.redcry.chess.model.Position
import cz.redcry.chess.model.Side
import cz.redcry.chess.model.Square
import cz.redcry.chess.model.moveZUci
import cz.redcry.chess.pgn.PgnPartie
import cz.redcry.chess.pgn.ctiPgn
import cz.redcry.chess.pgn.zapisPgn
import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.textChybyMotoru
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.random.Random

/**
 * Monotonni hodiny v milisekundach. System.nanoTime, ne currentTimeMillis -
 * ta skace pri zmene casu v telefonu a odpocet by pak ukazal nesmysl nebo
 * zaporne cislo. Ne ani SystemClock.elapsedRealtime: v JVM testu by spadla
 * na "Method not mocked" v kazdem testu, ktery stiskne Premyslej.
 */
private fun monotonniMs(): Long = System.nanoTime() / 1_000_000

class GameViewModel(
    vlastniMotor: ChessEngine? = null,
    /** Zdroj casu pro odpocet hledani. Test sem dava virtualni cas scheduleru. */
    private val hodiny: () -> Long = ::monotonniMs,
    /**
     * Vyroba motoru pri prepnuti a po [EngineEvent.Chyba]. V aplikaci
     * [EngineFactory.create]; test sem dava atrapy, protoze skutecny motor
     * nacita nativni knihovnu a v JVM testu by spadl na UnsatisfiedLinkError.
     */
    private val vyrobMotor: (EngineId, CoroutineScope) -> ChessEngine = EngineFactory::create,
    /**
     * Motor, se kterym ViewModel zacina - a za ktery stoji [vlastniMotor].
     * V aplikaci [EngineId.vychozi]; testy psane pro start na RedMAX (pred
     * krokem U9) sem davaji [EngineId.RED_MAX].
     */
    prvniMotor: EngineId = EngineId.vychozi,
    /**
     * Prevod hlavni varianty do SAN (krok U14/B): FEN korene hledani a tahy
     * varianty na zapisy. V aplikaci [SanVarianty.preved] (nativni, pro
     * vsechny motory); test sem dava nahradu.
     */
    private val sanVarianty: (String, List<Move>) -> List<String> = SanVarianty::preved,
    /**
     * Nacteni knihovny zahajeni (krok K1/B). V aplikaci cte assets
     * ([GameViewModelFactory]), test sem dava malou knihovnu z textu.
     * Null = bez knihovny: Premyslej vzdycky hleda a nazev zahajeni neni.
     */
    private val zdrojKnihy: (() -> KnihaZahajeni)? = null,
    /**
     * Kde se knihovna nacita. Mimo hlavni vlakno, at nebrzdi start; test sem
     * dava svuj dispatcher, aby nacteni ridil advanceUntilIdle().
     */
    private val knihaDispatcher: CoroutineDispatcher = Dispatchers.IO,
    /** Nahoda pro vyber tahu z knihovny. Test sem dava Random se seedem. */
    private val nahoda: Random = Random.Default,
    /**
     * Pamet zvoleneho jazyka (krok J2). V aplikaci SharedPreferences
     * ([GameViewModelFactory]); vychozi nic neuklada a nic nema.
     */
    private val ulozisteJazyka: UlozisteJazyka = BezUlozeniJazyka,
    /**
     * Jazyk telefonu ("cs", "en", ...) pro start bez ulozene volby. Vychozi
     * "cs": testy startuji cesky, stejne jako do J2.
     */
    jazykTelefonu: String = Jazyk.CESTINA.kod,
) : ViewModel() {

    /**
     * Vybrany motor. Autoritativni je tahle promenna, do stavu se zrcadli
     * jen popisek k vykresleni - stejne jako u zaznam.index.
     */
    private var vybranyMotor: EngineId = prvniMotor

    private val nastaveni = NastaveniMotoru()

    /**
     * Texty hlasek (krok J1, docs/POZNAMKY-JAZYK.md) v jazyce ze stavu
     * (krok J2). Cte se az pri skladani hlasky, takze hlaska po prepnuti
     * je v novem jazyce a ta, ktera uz svitila, se neprekladala.
     */
    private val texty: Texty get() = _state.value.jazyk.texty

    /**
     * Motor se vyrabi az tady v tele tridy, ne v delegujicim konstruktoru:
     * viewModelScope je vlastnost ViewModelu, takze pred dokoncenim super()
     * jeste neexistuje. [vlastniMotor] je jen vstup pro testy.
     */
    private var engine: ChessEngine =
        vlastniMotor ?: vyrobMotor(vybranyMotor, viewModelScope)

    /**
     * Kam se vratit, kdyz aktualni motor ohlasi [EngineEvent.Chyba], viz
     * [onChybaMotoru]. Nastavuje se pri kazdem prepnuti v [onEngineSelected].
     */
    private var predchoziMotor: EngineId = prvniMotor

    /**
     * Zaznam partie: pozice, klice opakovani, SAN, tahy, index na desce,
     * stav po poslednim tahu a reference pro ⟳ (krok R2/C). Klic i stav se
     * ptaji motoru az pri volani - toho, ktery je prave aktualni.
     */
    private val zaznam = ZaznamPartie(
        klicPozice = { klicOpakovani(it, engine::legalMoves) },
        stavMotoru = { engine.stav() },
    )

    /**
     * Knihovna zahajeni (krok K1/B), null dokud se nenacte (nebo kdyz neni
     * [zdrojKnihy]). Do te doby Premyslej normalne hleda - na knihovnu se
     * neceka. Prirazuje se na hlavnim vlakne, po navratu z [knihaDispatcher].
     */
    private var kniha: KnihaZahajeni? = null

    private val _state = MutableStateFlow(
        GameUiState(
            position = Position.fromFen(Position.START_FEN)!!,
            jazyk = Jazyk.priStartu(ulozisteJazyka.cti(), jazykTelefonu),
        )
    )
    val state: StateFlow<GameUiState> = _state.asStateFlow()

    /**
     * Jednorazove hlasky pro Snackbar. Zamerne to NENI soucast GameUiState -
     * trvaly stav by se po kazde rekompozici cetl znovu a hlaska by se
     * ukazovala porad dokola. Kanal kazdou zpravu doruci prave jednou.
     *
     * Posila se sem jen to, co z desky videt neni: odmitnuty tah, zastaveni
     * hledani, konec partie, kraj historie pri listovani a tah, u ktereho
     * se zadrhlo nacitani partie. "Vybrano e2" nebo "Zahrano f1-b5" ukazuje
     * sama deska, takze se neposilaji vubec.
     *
     * Hlasky o samotne praci se souborem (necitelny soubor, ulozeno) sem
     * nechodi - ty patri GameScreenu, ktery jediny o Uri vi.
     */
    private val _messages = Channel<String>(Channel.BUFFERED)
    val messages: Flow<String> = _messages.receiveAsFlow()

    private var eventsJob: Job? = null

    /**
     * Meze hledani a sila motoru (krok R2/A). Az za [_state] a pred init:
     * init vola [MezeASila.posliSilu].
     */
    private val mezeASila = MezeASila(
        object : HraProMeze {
            override val vybranyMotor: EngineId get() = this@GameViewModel.vybranyMotor
            override val texty: Texty get() = this@GameViewModel.texty
            override fun zaneprazdneno() = this@GameViewModel.zaneprazdneno()
            override fun hlaska(text: String) = notify(text)
            override fun nastavSilu(elo: Int?) = engine.nastavSilu(elo)
            override fun upravStav(zmena: (GameUiState) -> GameUiState) {
                _state.value = zmena(_state.value)
            }
        },
        nastaveni,
    )

    init {
        zaznam.zacni(engine.currentFen())
        val (cerny, cislo) = cislovani(zaznam.history[0])
        val meze = nastaveni[vybranyMotor]
        _state.value = _state.value.copy(
            engineName = vybranyMotor.label,
            rozsahHloubky = vybranyMotor.minHloubka..vybranyMotor.maxHloubka,
            maxHloubka = meze.maxHloubka,
            maxSekund = meze.maxSekund,
            elo = meze.elo,
            rozsahElo = rozsahElo(vybranyMotor),
            eloOslabeni = nastaveni.eloPriOslabeni(vybranyMotor),
            prvniJeCerny = cerny,
            cisloPrvnihoTahu = cislo,
        )
        mezeASila.posliSilu()
        sledujUdalosti(engine)
        nactiKnihu()
    }

    /**
     * Knihovna zahajeni mimo hlavni vlakno (krok K1/B). Po nacteni se jen
     * doplni nazev zahajeni; nic jineho na knihovnu neceka. Kdyz nacteni
     * selze, rekne se to a hraje se bez knihovny.
     */
    private fun nactiKnihu() {
        val zdroj = zdrojKnihy ?: return
        viewModelScope.launch {
            kniha = try {
                withContext(knihaDispatcher) { zdroj() }
            } catch (e: CancellationException) {
                // ViewModel skoncil driv, nez se knihovna nacetla.
                throw e
            } catch (e: Exception) {
                notify(texty.knihovnaChyba)
                null
            }
            _state.value = _state.value.copy(zahajeni = zahajeni())
        }
    }

    /**
     * Nazev zahajeni partie na desce: posledni pojmenovana pozice
     * v historii od zacatku po [ZaznamPartie.index] (krok K1/B). Vola se vsude,
     * kde se do stavu zapisuje historyIndex.
     */
    private fun zahajeni(): Zahajeni? =
        kniha?.nazevPartie(zaznam.history.subList(0, zaznam.index + 1))

    /**
     * Barva a cislo prvniho tahu podle vychozi pozice partie. Vraci se to
     * a nedrzi ve vlastnim poli schvalne - autoritativni je [GameUiState]
     * a dve mista, kde tatáž vec zije, se casem rozejdou.
     */
    private fun cislovani(fen: String): Pair<Boolean, Int> {
        val vychozi = Position.fromFen(fen)
        return (vychozi?.sideToMove == Side.BLACK) to (vychozi?.fullmoveNumber ?: 1)
    }

    /**
     * Koren hledani, varianta, predchozi volba, skore a odpocet sekund
     * (krok R2/B). Pred [sekundyHledani], ktere z nej vznika pri konstrukci.
     */
    private val prubehHledani = PrubehHledani(
        hodiny = hodiny,
        sanVarianty = sanVarianty,
        sanZMotoru = { engine.san(it) },
        stav = { _state.value },
    )

    /**
     * Prihlasi se k udalostem daneho motoru a zrusi odber toho predchoziho.
     * Bez zruseni by ViewModel po prepnuti poslouchal i motor, ktery uz je
     * uvolneny.
     */
    private fun sledujUdalosti(motor: ChessEngine) {
        eventsJob?.cancel()
        eventsJob = viewModelScope.launch {
            motor.events.collect { event ->
                // Udalost od motoru, ktery uz neni aktualni, se zahodi. Tim
                // se po Chyba ignoruje i pripadny BestMove chybneho motoru -
                // aktualni je v te chvili uz nahradni.
                if (motor !== engine) return@collect
                when (event) {
                    // Co se z Info do stavu dostane a proc, viz
                    // PrubehHledani.poInfo.
                    is EngineEvent.Info -> {
                        _state.value = prubehHledani.poInfo(event)
                        if (stavHledani == StavHledani.ANALYZA) analyza.naInfo(event)
                    }
                    is EngineEvent.BestMove -> onSearchFinished(event.move)
                    is EngineEvent.Chyba -> onChybaMotoru(motor, texty.textChybyMotoru(event.druh, event.motor))
                }
            }
        }
    }

    /**
     * Stav bez vysledku hledani: hloubka, varianta, hodnoceni a predchozi
     * volba (krok U14/C). Jedine misto, kde se mazou - do U14/C se varianta
     * mazala na devitu mistech zvlast a predchozi volba se s ni nesmi
     * rozejit. Po tahu motoru (onSearchFinished) se nevola, vysledek tam
     * patri k hledani, ktere tah vybralo (U12/D).
     *
     * Krok K1/B: maze i priznak tahu z knihovny ([GameUiState.zKnihy]) -
     * patri k nemu stejne jako hloubka k hledani.
     */
    private fun GameUiState.bezHledani(): GameUiState = copy(
        depth = null,
        varianta = emptyList(),
        skoreCp = null,
        predchoziVolba = null,
        zKnihy = false,
    )

    /** Zkratka - autoritativni je [GameUiState.stavHledani], jinde nezije. */
    private val stavHledani: StavHledani get() = _state.value.stavHledani

    /** Cele sekundy od stisku Premyslej, jen za stavu HLEDA, viz [PrubehHledani.sekundy]. */
    val sekundyHledani: Flow<Int?> = prubehHledani.sekundy(_state)

    /**
     * Posle hlasku a vrati true, kdyz se prave nesmi sahnout na pozici enginu.
     *
     * Straz se vaze na "stav neni KLID", ne na "hledani bezi": po stisku
     * Hraj nejlepsi nativni strana JESTE HLEDA, dokud nedorazi bestmove, a v tom okne
     * kazdou zmenu pozice odmitne. Driv se `thinking` rusilo hned pri stisku,
     * takze to okno zustavalo nehlidane.
     */
    private fun zaneprazdneno(): Boolean {
        if (stavHledani == StavHledani.KLID) return false
        notify(
            when (stavHledani) {
                StavHledani.HLEDA -> texty.enginePremysli
                StavHledani.ANALYZA -> texty.analyzaBezi
                else -> texty.hledaniDobiha
            },
        )
        return true
    }

    /**
     * Posle hlasku a vrati true, kdyz se motor jeste zaklada
     * ([ChessEngine.jePripraven] = false, zatim jen Stockfish). Straz pro
     * vsechno, co synchronne vola applyMove nebo setPosition - ty by na
     * zakladani cekaly a zablokovaly UI. Premyslej ji nema: go ceka
     * v korutine.
     *
     * Bez ni by klepnuti na figuru po prepnuti narazilo na prazdne legalni
     * tahy a UI by hlasilo zavadejici "Tam ta figura nemuze".
     */
    private fun nacitaSe(): Boolean {
        // Od kroku U7/A i tehdy, kdyz motor uz je pripraveny, ale pozice
        // z historie k nemu jeste nedosla (korutina v poziceDoMotoru).
        if (engine.jePripraven() && cekaniNaPozici == null) return false
        notify(texty.motorSeNacita(vybranyMotor.label))
        return true
    }

    fun onSquareClick(square: Square) {
        if (zaneprazdneno()) return
        if (nacitaSe()) return
        if (skoncila()) return
        val s = _state.value
        // Na starsi pozici se klepat SMI. Varovani o useknutem zaznamu posila
        // az play(), tedy az se tah opravdu hraje - jinak by Snackbar otravoval
        // pri kazdem klepnuti na figuru behem prohlizeni.

        val piece = s.position[square]
        val selected = s.selected

        if (selected == null) {
            if (piece == null) return
            if (piece.side != s.position.sideToMove) {
                notify(texty.jeNaTahu(s.position.sideToMove))
                return
            }
            select(square)
            return
        }

        if (square == selected) {
            _state.value = s.copy(selected = null, targets = emptyList())
            return
        }

        if (square in s.targets) {
            // Promenu nehrat hned: nativni strana by bez figury doplnila damu
            // a podpromena by se rukou zahrat nedala. Jestli to promena je,
            // rekne motor, ne deska - viz ChessEngine.jePromena.
            if (engine.jePromena(selected, square)) {
                _state.value = s.copy(cekaNaPromenu = selected to square)
                return
            }
            play(Move(selected, square))
            return
        }

        if (piece != null && piece.side == s.position.sideToMove) {
            select(square)
            return
        }

        notify(texty.tamFiguraNemuze)
    }

    /** Vybrana figura z dialogu promeny. Tah se zahraje az ted. */
    fun onPromenaVybrana(typ: PieceType) {
        if (zaneprazdneno()) return
        val (z, na) = _state.value.cekaNaPromenu ?: return
        _state.value = _state.value.copy(cekaNaPromenu = null)
        play(Move(z, na, typ))
    }

    /** Dialog promeny zavreny bez vyberu: nic se nehraje, vyber je pryc. */
    fun onPromenaZrusena() {
        if (zaneprazdneno()) return
        _state.value = _state.value.copy(
            cekaNaPromenu = null,
            selected = null,
            targets = emptyList(),
        )
    }

    private fun select(square: Square) {
        _state.value = _state.value.copy(
            selected = square,
            targets = engine.legalMoves(square),
        )
    }

    private fun play(move: Move) {
        // Vlastni straz, i kdyz ji onSquareClick uz ma: play() mysli na pozici
        // enginu a nesmi stat na tom, ze ho nekdo zavola jen ve spravnou chvili.
        if (zaneprazdneno()) return
        if (nacitaSe()) return
        // PRED applyMove: mapa zapisu plati pro pozici pred tahem.
        val san = engine.san(move)
        // Taky pred applyMove - pushHistory uz zaznam.index posune. Tah, ktery
        // je v zaznamu dalsi, nic neurizne (krok U7/B), tak se ani nevaruje.
        val zeStarsiPozice = zaznam.index != zaznam.history.size - 1 && !zaznam.jeDalsiVZaznamu(move)
        val fen = engine.applyMove(move)
        if (fen == null) {
            notify(texty.neplatnyTah)
            return
        }
        // Az tady, ne pri klepnuti na figuru: odmitnuty tah nic neuseknul.
        if (zeStarsiPozice) notify(texty.starsiPozice)
        zaznam.pushHistory(fen, san ?: move.toUci(), move)
        // Krok U12/D: hodnoceni patrilo pozici pred tahem, pro novou by
        // lhalo. Po tahu motoru (onSearchFinished) zustava - tam patri
        // k hledani, ktere tah vybralo.
        _state.value = _state.value.bezHledani().copy(
            position = Position.fromFen(fen)!!,
            selected = null,
            targets = emptyList(),
            // Dlazdice ukazuje POSLEDNI zahrany tah, at ho zahral kdokoli.
            // Bez tohohle na ni po lidskem tahu visel tah motoru.
            bestMove = move,
            bestMoveSan = san,
            historyIndex = zaznam.index,
            historySize = zaznam.history.size,
            sanZaznam = sanZaznam(),
            konecPartie = konecNaDesce(),
            lzeObnovit = zaznam.tahOdboceni() != null,
            zahajeni = zahajeni(),
        )
    }

    /**
     * Tlacitko Premyslej: v KLIDu spusti hledani. Za hledani ho obrazovka
     * nahradi fialovym Hraj nejlepsi, takze sem stisk nedojde; kdyby dosel,
     * jen se to rekne - zastavit a tah zahodit od kroku 18 nejde.
     */
    fun onThinkClick() {
        when (stavHledani) {
            StavHledani.DOBIHA_ZAHRAT -> {
                notify(texty.hledaniDobiha)
                return
            }
            StavHledani.HLEDA -> {
                notify(texty.enginePremysli)
                return
            }
            StavHledani.ANALYZA -> {
                notify(texty.analyzaBezi)
                return
            }
            StavHledani.KLID -> Unit
        }
        // Krok U7/A: po prepnuti motoru by se hledalo v pozici, kterou motor
        // ma po zalozeni, ne v te z historie. Jen dokud se na ni ceka -
        // motor, ktery uz na spravne pozici stoji, se nebrzdi (go na
        // zalozeni pocka samo, viz nacitaSe).
        if (cekaniNaPozici != null) {
            notify(texty.motorSeNacita(vybranyMotor.label))
            return
        }
        // V koncove pozici se nehleda: motor by vratil prazdny bestmove
        // a teprve onSearchFinished by rekl "zadny tah". Tahle straz tu
        // cestu zavira uz pri stisku.
        if (skoncila()) return
        if (zahrajZKnihy()) return
        // Varuje, ale neblokuje - rozehrat partii jinam ze starsi pozice je
        // legitimni zamer, jen pushHistory() pri zahrani tahu usekne vsechno
        // za aktualnim indexem. Do kroku 18 slo hledani zastavit cervenym
        // Zastav a nic se nezahodilo; od te doby tah motoru prijde vzdycky,
        // takze varovani uz jen rika, co se stane.
        // Od kroku U7/B se urizne jen jiny tah, nez je v zaznamu dalsi -
        // jaky motor zahraje, se tu jeste nevi, proto jina hlaska nez v play().
        if (zaznam.index != zaznam.history.size - 1) {
            notify(texty.starsiPoziceHledani)
        }
        // PRED prepnutim do HLEDA: prechod stavu rozbehne tikani a to si
        // zacatek hned precte. Kazde hledani tak zacina od nuly.
        prubehHledani.zacni(zaznam.history[zaznam.index])
        // Varianta ani hodnoceni z minuleho hledani uz pro tuhle pozici
        // neplati.
        _state.value = _state.value.bezHledani().copy(
            stavHledani = StavHledani.HLEDA,
            selected = null,
            targets = emptyList(),
        )
        val meze = nastaveni[vybranyMotor]
        // Sekundy na milisekundy az tady: uzivatel i stav mysli v sekundach,
        // UCI v milisekundach. Nula projde jako nula - adapter pri ni token
        // `movetime` neposle vubec.
        engine.go(meze.maxHloubka, meze.maxSekund * 1000)
    }

    /**
     * Premyslej v pozici z knihovny zahajeni (krok K1/B): tah se vybere
     * nahodne podle vah a zahraje stejnou cestou jako tah motoru
     * ([zahrajTahMotoru]), bez hledani a bez go(). Vola se az za strazemi
     * [onThinkClick]. Analyza knihovnu nepouziva.
     *
     * True = tah z knihovny je zahrany. False = knihovna vypnuta, jeste
     * nenactena, pozice v ni neni, nebo motor tah odmitl (nemelo by
     * nastat) - pak se normalne hleda. Kdyz se motor jeste zaklada, rekne
     * se to a vrati true: applyMove je synchronni a cekal by na zalozeni
     * v hlavnim vlakne (go na nej ceka v korutine, proto Premyslej mimo
     * knihovnu tuhle straz nema).
     *
     * Varovani ze starsi pozice je presne jako v [play]: tah je znamy
     * predem, takze se rekne jen tehdy, kdyz opravdu neco urizne
     * (rozhodnuti uzivatele v kroku K1/B).
     */
    private fun zahrajZKnihy(): Boolean {
        if (!_state.value.knihovna) return false
        val tah = kniha?.vyber(zaznam.history[zaznam.index], nahoda)?.let(::moveZUci) ?: return false
        if (nacitaSe()) return true
        val zeStarsiPozice = zaznam.index != zaznam.history.size - 1 && !zaznam.jeDalsiVZaznamu(tah)
        if (!zahrajTahMotoru(tah, zKnihy = true)) return false
        if (zeStarsiPozice) notify(texty.starsiPozice)
        return true
    }

    /** Polozka menu Hra "Knihovna zahajeni" (krok K1/B): prepne a rekne to. */
    fun onKnihovna() {
        val zapnuto = !_state.value.knihovna
        _state.value = _state.value.copy(knihovna = zapnuto)
        notify(if (zapnuto) texty.knihovnaZapnuta else texty.knihovnaVypnuta)
    }

    /**
     * Fialove tlacitko, od kroku 18 jedine za hledani: hledani se ukonci
     * a nalezeny tah se po prichodu ZAHRAJE. `stop()` vrati bestmove s tim
     * nejlepsim, co motor stihl.
     *
     * Stisk pred dokoncenim prvni hloubky: RedMAX posle `bestmove (none)`
     * ("preruseno pred prvni dokoncenou hloubkou"), a to se v
     * [onSearchFinished] pozna. Light Red nedokonceny tah zahodi a zahraje
     * prvni legalni tah (`prvni_legalni` v light_red_search.cpp) - null od nej
     * neprijde.
     */
    fun onHrajNejlepsi() {
        when (stavHledani) {
            StavHledani.HLEDA -> {
                engine.stop()
                _state.value = _state.value.copy(stavHledani = StavHledani.DOBIHA_ZAHRAT)
            }
            StavHledani.DOBIHA_ZAHRAT -> notify(texty.hledaniDobiha)
            StavHledani.ANALYZA -> notify(texty.analyzaBezi)
            // V KLIDu se nehleda a tlacitko neni ani videt - cisty no-op.
            StavHledani.KLID -> Unit
        }
    }

    // --- Meze hledani a sila motoru (krok R2/A) --------------------------------
    //
    // Handlery i s komentari jsou v [MezeASila], tady jen delegace.

    fun onHloubkaZmena(delta: Int) = mezeASila.onHloubkaZmena(delta)

    fun onSekundyZmena(smer: Int) = mezeASila.onSekundyZmena(smer)

    fun onHloubkaZadana(text: String) = mezeASila.onHloubkaZadana(text)

    fun onSekundyZadane(text: String) = mezeASila.onSekundyZadane(text)

    fun onPlnaSila(zapnuto: Boolean) = mezeASila.onPlnaSila(zapnuto)

    fun onEloZadane(text: String) = mezeASila.onEloZadane(text)

    /**
     * Konec hledani. Tah zahraje ViewModel - adapter ho jen ohlasi a sam ho
     * nezahraje, takze applyMove je tady a nikde jinde. Druhe volani by tah
     * provedlo dvakrat.
     */
    private fun onSearchFinished(move: Move?) {
        // Analyza PRED prepnutim do KLID: jinak by se stav ztratil driv, nez
        // BestMove dojde k rizeni. Tah se v analyze nehraje nikdy.
        if (stavHledani == StavHledani.ANALYZA) {
            analyza.naBestMove(move)
            return
        }
        // Jedine misto, kde se z DOBIHA_ZAHRAT vychazi.
        val predtim = stavHledani
        // Krok U14/C: konec hledani uzavre i posledni hloubku.
        val volba = prubehHledani.volbaPoKonci()
        _state.value = _state.value.copy(stavHledani = StavHledani.KLID, predchoziVolba = volba)
        // Bestmove, ktere nikdo necekal (stav uz byl KLID), se zahodi.
        if (predtim == StavHledani.KLID) return
        if (move == null) {
            notify(
                if (predtim == StavHledani.DOBIHA_ZAHRAT) {
                    // Fialove stisknute driv, nez dobehla prvni hloubka -
                    // RedMAX pak nema co vratit. Neni to konec partie.
                    texty.motorNenaselTah
                } else {
                    // Pojistka. Od kroku 15 se sem nema dojit - Premyslej se
                    // v koncove pozici odmitne driv, nez se go() vubec zavola.
                    texty.zadnyTahKonecPartie
                }
            )
            return
        }
        if (!zahrajTahMotoru(move, zKnihy = false)) notify(texty.engineVratilNeplatnyTah)
    }

    /**
     * Zahraje tah motoru: z hledani ([onSearchFinished]) nebo z knihovny
     * zahajeni ([zahrajZKnihy], krok K1/B, vytazeno z onSearchFinished).
     * False = motor tah odmitl a nic se nezmenilo.
     *
     * Po tahu z hledani zustava jeho vysledek (U12/D). Tah z knihovny zadne
     * hledani nema, takze se vysledky mazou a misto hloubky je
     * [GameUiState.zKnihy].
     */
    private fun zahrajTahMotoru(move: Move, zKnihy: Boolean): Boolean {
        // PRED applyMove, ze stejneho duvodu jako v play().
        val san = engine.san(move)
        val fen = engine.applyMove(move) ?: return false
        zaznam.pushHistory(fen, san ?: move.toUci(), move)
        val s = _state.value
        val zaklad = if (zKnihy) {
            s.bezHledani().copy(zKnihy = true, selected = null, targets = emptyList())
        } else {
            s
        }
        _state.value = zaklad.copy(
            position = Position.fromFen(fen)!!,
            bestMove = move,
            bestMoveSan = san,
            historyIndex = zaznam.index,
            historySize = zaznam.history.size,
            sanZaznam = sanZaznam(),
            konecPartie = konecNaDesce(),
            lzeObnovit = zaznam.tahOdboceni() != null,
            zahajeni = zahajeni(),
        )
        return true
    }

    fun onNewGame() {
        // Bez teto strazi by newGame() za hledani jen zastavil hledani
        // a pozici nechal byt, UI by ale uz hlasilo novou partii.
        if (zaneprazdneno()) return
        if (nacitaSe()) return
        novaPartie()
    }

    /**
     * Nova partie z menu Hra (krok U6/A). Kdyz zaznam ma aspon jeden tah,
     * nejdriv se zepta dialogem ([GameUiState.potvrzeniNovePartie]); na
     * prazdne desce neni co ztratit, takze rovnou [onNewGame].
     *
     * Straze jsou tytez jako v [onNewGame] a jsou uz tady: za hledani by
     * se jinak ukazal dialog, jehoz potvrzeni by stejne neproslo.
     * Rozhoduje cely zaznam ([ZaznamPartie.tahy]), ne [ZaznamPartie.index] - kdo se vratil
     * sipkou na zacatek, ma partii porad rozehranou.
     */
    fun onNovaPartieDotaz() {
        if (zaneprazdneno()) return
        if (nacitaSe()) return
        if (zaznam.tahy.isEmpty()) {
            onNewGame()
            return
        }
        val s = _state.value
        _state.value = s.copy(
            potvrzeniNovePartie = pocetTahuZaznamu(zaznam.sanHistory, s.prvniJeCerny, s.cisloPrvnihoTahu),
        )
    }

    /** Dialog potvrzen: presne to, co drive delalo tlacitko Nova partie. */
    fun onNovaPartiePotvrzena() {
        _state.value = _state.value.copy(potvrzeniNovePartie = null)
        onNewGame()
    }

    /** Dialog zrusen: jen se zavre, partie zustava. */
    fun onNovaPartieZrusena() {
        _state.value = _state.value.copy(potvrzeniNovePartie = null)
    }

    /**
     * Nova partie bez strazi. Vola ji [onNewGame] a [onEngineSelected] -
     * ten ji potrebuje i s motorem, ktery se jeste zaklada ([ChessEngine.jePripraven]
     * = false): newGame() takoveho motoru na zakladani neceka a stav UI se
     * po prepnuti srovnat musi.
     */
    private fun novaPartie() {
        engine.stop()
        engine.newGame()
        // Krok U6/B: k nactene partii se z nove uz nevraci. Stav nize je
        // cely nanovo, takze lzeObnovit spadne na false sam.
        zaznam.zahodReferenci()
        zaznam.zacni(engine.currentFen())
        val (cerny, cislo) = cislovani(zaznam.history[0])
        // Cely stav nanovo, takze sanZaznam i varianta spadnou na vychozi
        // prazdne hodnoty samy.
        val meze = nastaveni[vybranyMotor]
        _state.value = GameUiState(
            position = Position.fromFen(engine.currentFen())!!,
            flipped = _state.value.flipped,
            engineName = vybranyMotor.label,
            rozsahHloubky = vybranyMotor.minHloubka..vybranyMotor.maxHloubka,
            maxHloubka = meze.maxHloubka,
            maxSekund = meze.maxSekund,
            elo = meze.elo,
            rozsahElo = rozsahElo(vybranyMotor),
            eloOslabeni = nastaveni.eloPriOslabeni(vybranyMotor),
            prvniJeCerny = cerny,
            cisloPrvnihoTahu = cislo,
            konecPartie = konecNaDesce(),
            // Krok K1/B: prepinac z menu patri aplikaci, ne partii.
            knihovna = _state.value.knihovna,
            // Krok J2: jazyk taky.
            jazyk = _state.value.jazyk,
            zahajeni = zahajeni(),
        )
    }

    /**
     * Prepinac CZ/EN (krok J2). Kdykoli, i za hledani a analyzy - straze
     * [zaneprazdneno] se na jazyk nevztahuji. Volba se ulozi; pruh bezici
     * analyzy se hned prepise novym jazykem.
     */
    fun onJazyk(jazyk: Jazyk) {
        if (jazyk == _state.value.jazyk) return
        _state.value = _state.value.copy(jazyk = jazyk)
        ulozisteJazyka.uloz(jazyk)
        analyza.naZmenuJazyka()
    }

    fun onFlip() {
        _state.value = _state.value.copy(flipped = !_state.value.flipped)
    }

    /**
     * Prepnuti motoru zachova partii (krok U7/A): stary motor se uvolni, novy
     * dostane pozici z historie presne na [ZaznamPartie.index] ([poziceDoMotoru],
     * tataz cesta jako po [EngineEvent.Chyba]). Historie, SAN, klice
     * opakovani i reference pro ⟳ zustanou. Duvod: nactenou partii jde
     * rozebrat ruznymi motory. Do kroku U7 se tu zakladala nova partie
     * a vyber byl odemceny jen na prazdne desce.
     *
     * Nedostupny motor se odmitne uz tady - EngineFactory.create() by na nem
     * vyhodil IllegalArgumentException.
     */
    fun onEngineSelected(id: EngineId) {
        // UI to hlida uz pres selectable v GameScreen, ale ViewModel se nesmi
        // spolehat na to, ze ho nekdo vola jen ve spravnou chvili - prepnuti
        // uvolnuje motor, na kterem prave bezi hledani.
        // Vyber uz vybraneho motoru je cisty no-op, takze test na nej patri
        // PRED straz - nema o cem hlasit ani kdyz hledani bezi.
        if (id == vybranyMotor) return
        if (zaneprazdneno()) return
        if (!id.dostupny) {
            notify(texty.motorNeniKDispozici(id.label))
            return
        }
        prepniMotor(id, StavHledani.KLID)?.let { notify(it) }
    }

    /**
     * Jadro [onEngineSelected] bez strazi (krok A1/B): stejnou cestou prepina
     * i analyza partie, tam a zpet. [stav] je stav hledani po prepnuti -
     * v analyze ANALYZA, aby se straze neotevrely driv, nez pujde prvni go.
     *
     * Vraci zpravu, kdyz motor nesel zalozit a vratil se predchozi, jinak
     * null. Hlasku posila volajici: analyza ji spojuje se svou do jedne.
     */
    private fun prepniMotor(id: EngineId, stav: StavHledani): String? {
        val predchozi = vybranyMotor
        var chyba: String? = null
        engine.release()
        engine = try {
            vyrobMotor(id, viewModelScope).also {
                vybranyMotor = id
                predchoziMotor = predchozi
            }
        } catch (e: IllegalStateException) {
            // Crafty smi mit jen jednu nativni instanci; kdyby druha nesla
            // zalozit, CraftyBridge to hlasi [MotorUzBeziException]. Misto
            // padu se to rekne a vrati se predchozi motor. Text je z tabulky
            // (krok J2), zprava vyjimky na obrazovku nejde.
            chyba = if (e is MotorUzBeziException) texty.motorUzBezi(e.motor)
                    else texty.motorSeNepodariloSpustit(id.label)
            vyrobMotor(predchozi, viewModelScope)
        }
        nastavStavMotoru(vybranyMotor, stav)
        mezeASila.posliSilu()
        sledujUdalosti(engine)
        poziceDoMotoru()
        return chyba
    }

    /**
     * Ceka na pripraveny motor, aby mu poslala pozici z historie (krok
     * U7/A). Dokud neni null, plati straz [nacitaSe] i pro Premyslej.
     */
    private var cekaniNaPozici: Job? = null

    /**
     * Pozice z historie do [engine], presne na [ZaznamPartie.index]: vychozi FEN
     * + tahy. Spolecne pro [onEngineSelected] a [onChybaMotoru].
     *
     * Motor, ktery se jeste zaklada (Stockfish), UI nebrzdi: pozice se mu
     * posle az z korutiny po [ChessEngine.pockejNaPripravenost]. Do te doby
     * plati straz "… se ještě načítá". Kdyz motor na pozici uz stoji (prazdna
     * deska, vychozi postaveni - po zalozeni na ni stoji kazdy motor), neceka
     * se vubec: Premyslej po prepnuti tak funguje hned jako do kroku U7.
     *
     * Konec partie se vyhodnoti znovu z noveho motoru, az na pozici stoji.
     * [ZaznamPartie.stavPoPoslednimTahu] se nemeni - patri partii, ne motoru.
     */
    private fun poziceDoMotoru() {
        cekaniNaPozici?.cancel()
        cekaniNaPozici = null
        // Rozklepnuta figura a cekajici promena plati pro stary motor.
        _state.value = _state.value.copy(
            selected = null,
            targets = emptyList(),
            cekaNaPromenu = null,
        )
        val motor = engine
        if (zaznam.index == 0 && motor.currentFen() == zaznam.history[0]) {
            if (motor.jePripraven()) _state.value = _state.value.copy(konecPartie = konecNaDesce())
            return
        }
        if (motor.jePripraven()) {
            nastavPoziciZHistorie()
            return
        }
        cekaniNaPozici = viewModelScope.launch {
            val pripraven = motor.pockejNaPripravenost()
            // Prepnuto nebo Chyba mezitim: tohle cekani uz zrusil novejsi
            // poziceDoMotoru(), sem se pak nedojde.
            cekaniNaPozici = null
            if (pripraven && motor === engine) nastavPoziciZHistorie()
        }
    }

    private fun nastavPoziciZHistorie() {
        if (!engine.setPosition(zaznam.history[0], zaznam.tahy.take(zaznam.index))) {
            notify(texty.poziciNesloObnovit)
        }
        _state.value = _state.value.copy(konecPartie = konecNaDesce())
    }

    /**
     * Stav UI pro nove vybrany motor: jmeno, rozsah hloubky, jeho meze
     * a vynulovane vysledky hledani. Spolecne pro [onEngineSelected]
     * i [onChybaMotoru]; lisi se jen tim, co nasleduje (nova partie proti
     * obnoveni pozice z historie).
     */
    private fun nastavStavMotoru(id: EngineId, stav: StavHledani = StavHledani.KLID) {
        val meze = nastaveni[id]
        _state.value = _state.value.bezHledani().copy(
            stavHledani = stav,
            engineName = id.label,
            rozsahHloubky = id.minHloubka..id.maxHloubka,
            maxHloubka = meze.maxHloubka,
            maxSekund = meze.maxSekund,
            elo = meze.elo,
            rozsahElo = rozsahElo(id),
            eloOslabeni = nastaveni.eloPriOslabeni(id),
            // Historie se prepnutim nemeni, nazev tedy taky ne - jen pro
            // jistotu, ze se se stavem nerozejde (krok K1/B).
            zahajeni = zahajeni(),
        )
    }

    /**
     * Aktualni motor ohlasil, ze se nepodarilo ho zalozit ([EngineEvent.Chyba],
     * zatim jen Stockfish se zakladanim na pozadi).
     *
     * Rekne se zprava, chybny motor se uvolni (uvolnuje prijemce, ne motor
     * sam) a zalozi se predchozi. Pozice se obnovi z historie, a to presne
     * na [ZaznamPartie.index] - historie ani zaznam partie se nemeni. Automat
     * hledani jde do KLID, i kdyz Chyba prisla za hledani.
     *
     * Zadna smycka: kdyby predchozi motor byl tentyz, ktery selhal, vrati se
     * [EngineId.vychozi], a kdyby selhal prave on (od kroku U9 je jim
     * Stockfish), [EngineId.zalozni]. Po kazde Chyba je predchozim
     * [EngineId.zalozni], takze druha Chyba hned po prvni vede na nej.
     *
     * Vola se ZEVNITR odberu udalosti chybneho motoru, a [sledujUdalosti]
     * tenhle odber rusi. Proto je az na konci: zbytek funkce nema bod
     * preruseni, takze by dobehl i tak, ale takhle se na to nemusi spolehat.
     */
    private fun onChybaMotoru(motor: ChessEngine, zprava: String) {
        // Za analyzy (krok A1/B) rizeni jen skonci. Motor, silu a pozici
        // obnovi zbytek teto funkce, jednim prepnutim. Hlaska jedna, dve za
        // sebou by collectLatest z U11 spolkl.
        val zaAnalyzy = analyza.zrusPriChybe()
        notify(if (zaAnalyzy) texty.analyzaPrerusenaChybou(zprava) else zprava)
        val selhal = vybranyMotor
        motor.release()
        val id = when {
            predchoziMotor != selhal -> predchoziMotor
            EngineId.vychozi != selhal -> EngineId.vychozi
            else -> EngineId.zalozni
        }
        engine = vyrobMotor(id, viewModelScope)
        vybranyMotor = id
        predchoziMotor = EngineId.zalozni
        nastavStavMotoru(id)
        mezeASila.posliSilu()
        poziceDoMotoru()
        sledujUdalosti(engine)
        if (zaAnalyzy) obnovDeskuPoAnalyze()
    }

    fun onStepBack() = stepTo(zaznam.index - 1)

    fun onStepForward() = stepTo(zaznam.index + 1)

    /**
     * Skok na pozici po konkretnim pultahu - klepnutim v zaznamu partie.
     * Cil si nese sam [PulTah.historyIndex], tady uz se nic nedopocitava.
     */
    fun skocNaTah(historyIndex: Int) = stepTo(historyIndex)

    private fun stepTo(index: Int) {
        // Listovani za behu hledani se odmitne, misto aby se hledani zastavilo.
        // Drivejsi engine.stop() tady budil dojem, ze tahle funkce bezici
        // hledani osetri - neosetri, stop() je asynchronni.
        if (zaneprazdneno()) return
        if (nacitaSe()) return
        // Vypnute tlacitko na dotykaku nerekne proc nejde zmacknout, proto
        // sipky zustavaji aktivni a na kraji partie to rekne Snackbar.
        if (index < 0) {
            notify(texty.jsiNaZacatku)
            return
        }
        if (index >= zaznam.history.size) {
            notify(texty.jsiNaPoslednimTahu)
            return
        }

        val fen = zaznam.history[index]
        // Obranna vetev z kroku 5. Od kroku 11 uz neni jedinou pojistkou -
        // okno mezi stiskem Hraj nejlepsi a koncem hledani hlida DOBIHA_ZAHRAT -
        // ale zustava: zaznam.index se nesmi posunout po neuspesnem setPosition.
        // Vychozi FEN + tahy, ne holý history[index]: RedMAX by jinak videl
        // jedinou pozici a opakovani by po navigaci v hledani nepoznal.
        if (!engine.setPosition(zaznam.history[0], zaznam.tahy.take(index))) {
            notify(texty.poziciNesloNastavit)
            return
        }
        zaznam.index = index
        // Krok U12/D: hodnoceni z hledani patri jine pozici nez te, na
        // kterou se ted skace.
        _state.value = _state.value.bezHledani().copy(
            position = Position.fromFen(fen)!!,
            selected = null,
            targets = emptyList(),
            cekaNaPromenu = null,
            historyIndex = index,
            historySize = zaznam.history.size,
            konecPartie = konecNaDesce(),
            // Dlazdice "Aktualni tah" ukazuje tah, ktery vedl do pozice na
            // desce - pultah i vede do history[i + 1], proto index - 1.
            // Ve vychozi pozici zadny neni, tedy "—".
            bestMoveSan = zaznam.sanHistory.getOrNull(index - 1),
            // Vynulovat VYSLOVNE: dlazdice cte bestMoveSan ?: bestMove, takze
            // by ve vychozi pozici jinak prosvital stary tah motoru.
            bestMove = null,
            zahajeni = zahajeni(),
        )
    }

    /** FEN aktualni pozice. Celou partii do souboru vydava [exportPgn]. */
    fun exportFen(): String = engine.currentFen()

    /** Zaznam odehranych tahu v SAN. Odsud jde i zapis do PGN. */
    fun sanZaznam(): List<String> = zaznam.sanHistory.toList()

    /**
     * Nacte partii z textu PGN. Vraci true, kdyz se zahraly vsechny tahy.
     *
     * Tokeny se NEPARSUJI. Kazdy se predlozi motoru jako [ChessEngine.moveFromSan],
     * tedy se porovna se zapisy vsech legalnich tahu v dane pozici. Sachova
     * pravidla tim zustavaji jen v C++, kde uz overena jsou.
     *
     * Kdyz nejaky token nesedi, nacitani se PRERUSI a partie zustane nactena
     * po ten tah - ne prazdna. Je to pro uzivatele uzitecnejsi nez holé
     * odmitnuti: vidi, kam az to slo, a Snackbar rekne, o ktery tah slo.
     *
     * Vychozi pozice se bere ze znacky FEN, kdyz ji partie ma; jinak je to
     * zacatek partie.
     */
    fun nactiPgn(text: String): Boolean {
        // Tataz straz jako vsude, kde se meni pozice (Krok 5/B).
        if (zaneprazdneno()) return false
        if (nacitaSe()) return false

        val partie = ctiPgn(text)
        val vychozi = partie.tagy["FEN"]?.takeIf { it.isNotBlank() } ?: Position.START_FEN
        if (!engine.setPosition(vychozi)) {
            notify(texty.vychoziPoziciNesloNastavit)
            return false
        }

        // Stav po poslednim tahu z motoru (v zacni) plati pro partii bez
        // tahu i pro FEN, ktery uz sam mat je; kazdy zahrany tah to pak
        // v pushHistory prepise.
        zaznam.zacni(engine.currentFen())

        var chyba: String? = null
        for ((poradi, token) in partie.tahy.withIndex()) {
            val tah = engine.moveFromSan(token)
            // PRED applyMove, ze stejneho duvodu jako v play(): mapa zapisu
            // plati pro pozici pred tahem. Bere se zapis od motoru, ne token
            // ze souboru - ten muze mit treba vynechane znamenko sachu.
            val san = tah?.let { engine.san(it) }
            val fen = tah?.let { engine.applyMove(it) }
            if (fen == null) {
                chyba = texty.tahNejdeZahrat(poradi + 1, token)
                break
            }
            // pushHistory posouva zaznam.index, takze po dojeti stoji index
            // sam od sebe na konci partie.
            zaznam.pushHistory(fen, san ?: token, tah!!)
        }

        // Reference je to, co se opravdu nacetlo - i partie useknuta na
        // nezahratelnem tahu.
        zaznam.ulozReferenci(partie.tagy)

        val (cerny, cislo) = cislovani(zaznam.history[0])
        _state.value = _state.value.bezHledani().copy(
            lzeObnovit = false,
            position = Position.fromFen(zaznam.history[zaznam.index])!!,
            selected = null,
            targets = emptyList(),
            stavHledani = StavHledani.KLID,
            bestMove = null,
            bestMoveSan = null,
            historyIndex = zaznam.index,
            historySize = zaznam.history.size,
            sanZaznam = sanZaznam(),
            prvniJeCerny = cerny,
            cisloPrvnihoTahu = cislo,
            // Motor ted stoji na konci nacteneho zaznamu. stepTo(0) hned
            // pod tim to prepocita pro zacatek partie, kam se skace.
            konecPartie = konecNaDesce(),
            zahajeni = zahajeni(),
        )
        // Nactena partie stoji na ZACATKU - kdo si ji nacetl, chce si ji
        // projit, ne koukat na koncovku. stepTo() to umi cele: nastavi pozici
        // enginu i stav a nic neoreze, takze se tu nic noveho nepocita.
        stepTo(0)
        chyba?.let { notify(it) }
        return chyba == null
    }

    /**
     * Cela partie jako text PGN. Hlavicka je skoupa schvalne - hraci ani
     * turnaj se z aplikace nedaji zjistit a vymyslet si je by bylo horsi
     * nez predepsany otaznik.
     *
     * Krok U7/C: kdyz je nactena partie ([ZaznamPartie.tagyReference]), vezme se z ni Event,
     * Site, Date, Round, White a Black, co v ni je. Result se neopisuje,
     * pocita se z aktualniho stavu ([vysledekPartie]) - po odbocce uz
     * puvodni vysledek neplati. Ostatni znacky reference (ECO, puvodni
     * FEN...) se taky neopisuji: FEN a SetUp patri vychozi pozici, ktera
     * se bere z historie.
     */
    fun exportPgn(): String {
        val stav = _state.value
        return zapisPgn(
            PgnPartie(znackyPartie(zaznam, kniha), sanZaznam(), vysledekPartie(zaznam)),
            stav.prvniJeCerny,
            stav.cisloPrvnihoTahu,
        )
    }

    /**
     * Konec partie v pozici `zaznam.history[zaznam.index]`, tedy na desce. Motor na
     * ni PRAVE stoji - `engine.stav()` plati jen pro ni - takze se vola az po
     * zmene pozice v motoru i po posunu [ZaznamPartie.index].
     */
    private fun konecNaDesce(index: Int = zaznam.index): KonecPartie? = konecZeStavu(
        engine.stav(),
        Position.fromFen(zaznam.history[index])?.sideToMove,
        pocetVyskytu(zaznam.klice, index),
    )

    /**
     * Vrati true, kdyz partie na desce skoncila. Straz pro Premyslej i pro
     * klepnuti na desku: v koncove pozici neni co hrat, takze se nehleda
     * ani nevybira pole.
     *
     * Hlasku uz neposila (U11). Duvod konce trvale ukazuje stavovy radek
     * (krok 15/B), hlaska by ho jen opakovala a zakryvala tlacitka dole.
     */
    private fun skoncila(): Boolean = _state.value.konecPartie != null

    fun importFen(fen: String): Boolean {
        if (zaneprazdneno()) return false
        if (nacitaSe()) return false
        if (!engine.setPosition(fen)) return false
        val normalized = engine.currentFen()
        // Stav po poslednim tahu (v zacni) rovnou z motoru, ne natvrdo
        // STAV_NORMALNI: importovany FEN sam o sobe muze byt mat nebo pat.
        zaznam.zacni(normalized)
        zaznam.ulozReferenci(emptyMap())
        // Z FEN muze partie zacinat cernym a jinym cislem tahu nez 1.
        val (cerny, cislo) = cislovani(zaznam.history[0])
        _state.value = _state.value.bezHledani().copy(
            position = Position.fromFen(normalized)!!,
            selected = null,
            targets = emptyList(),
            cekaNaPromenu = null,
            stavHledani = StavHledani.KLID,
            bestMove = null,
            bestMoveSan = null,
            historyIndex = 0,
            historySize = 1,
            sanZaznam = emptyList(),
            prvniJeCerny = cerny,
            cisloPrvnihoTahu = cislo,
            konecPartie = konecNaDesce(),
            lzeObnovit = false,
            zahajeni = zahajeni(),
        )
        return true
    }

    /**
     * Tlacitko ⟳ (krok U6/B): vrati nactenou partii a postavi se na tah
     * odboceni ([ZaznamPartie.tahOdboceni]) - tam, kde se od ni aktualni partie odchylila.
     * Historie, SAN, tahy i klice opakovani se vezmou z reference, pozice se
     * motoru posle stejne jako pri pohybu v zaznamu (vychozi FEN + tahy).
     *
     * Za hledani se odmitne hlaskou jako sipky ([stepTo]), ne zastavi:
     * stop() je asynchronni a zmenu pozice by nativni strana stejne odmitla.
     */
    fun onObnovPartii() {
        if (zaneprazdneno()) return
        if (nacitaSe()) return
        val index = zaznam.tahOdboceni() ?: return
        // PRED prepsanim zaznamu: kdyz motor pozici odmitne, nic se nemeni.
        val (vychozi, tahyReference) = zaznam.poziceReference(index) ?: return
        if (!engine.setPosition(vychozi, tahyReference)) {
            notify(texty.poziciNesloNastavit)
            return
        }
        zaznam.obnovZReference(index)
        // Krok U12/D: jako v stepTo, hodnoceni plati pro jinou pozici.
        _state.value = _state.value.bezHledani().copy(
            position = Position.fromFen(zaznam.history[index])!!,
            selected = null,
            targets = emptyList(),
            cekaNaPromenu = null,
            historyIndex = index,
            historySize = zaznam.history.size,
            sanZaznam = sanZaznam(),
            konecPartie = konecNaDesce(),
            // Jako v stepTo: dlazdice ukazuje tah, ktery do pozice vedl.
            bestMoveSan = zaznam.sanHistory.getOrNull(index - 1),
            bestMove = null,
            lzeObnovit = false,
            zahajeni = zahajeni(),
        )
    }

    private fun notify(text: String) {
        _messages.trySend(text)
    }

    // --- Analyza partie (krok A1/B) ------------------------------------------
    //
    // Okno, spusteni a pauzy jsou v [NapojeniAnalyzy], prubeh v RizeniAnalyzy
    // (balik cz.redcry.chess.analyza, docs/POZNAMKY-ANALYZA.md). Tady jen
    // to, co saha na motor, historii a stav.

    private val hraProAnalyzu = object : HraProAnalyzu {
        override val scope: CoroutineScope get() = viewModelScope
        override val vybranyMotor: EngineId get() = this@GameViewModel.vybranyMotor
        override val texty: Texty get() = this@GameViewModel.texty
        override val jazyk: Jazyk get() = _state.value.jazyk
        override var predchoziMotor: EngineId
            get() = this@GameViewModel.predchoziMotor
            set(id) { this@GameViewModel.predchoziMotor = id }
        override val analyzuje: Boolean get() = stavHledani == StavHledani.ANALYZA
        override var ui: AnalyzaUi?
            get() = _state.value.analyza
            set(a) { _state.value = _state.value.copy(analyza = a) }

        override fun zaneprazdneno() = this@GameViewModel.zaneprazdneno()
        override fun hlaska(text: String) = notify(text)
        override fun vychoziFen() = zaznam.history[0]
        override fun partie() = _state.value.let {
            AnalyzovanaPartie(
                znackyPartie(zaznam, kniha), sanZaznam(), zaznam.tahy.map { t -> t.toUci() }, vysledekPartie(zaznam),
                Cislovani(it.prvniJeCerny, it.cisloPrvnihoTahu),
            )
        }

        override fun zacni() {
            _state.value = _state.value.bezHledani().copy(
                stavHledani = StavHledani.ANALYZA,
                selected = null,
                targets = emptyList(),
                cekaNaPromenu = null,
            )
        }

        override fun obnov(puvodniMotor: EngineId) {
            if (this@GameViewModel.vybranyMotor != puvodniMotor) {
                prepniMotor(puvodniMotor, StavHledani.KLID)
            } else {
                mezeASila.posliSilu()
                poziceDoMotoru()
            }
            obnovDeskuPoAnalyze()
        }

        override fun prepniMotor(id: EngineId): String? =
            this@GameViewModel.prepniMotor(id, StavHledani.ANALYZA)

        // Pozice z historie po prepnuti (cekaniNaPozici) musi dorazit DRIV
        // nez pozice analyzy, jinak by motor hledal jinde.
        override suspend fun pockejNaMotor(): Boolean {
            cekaniNaPozici?.join()
            val motor = engine
            return motor.pockejNaPripravenost() && motor === engine
        }

        override fun nastavSilu(elo: Int?) = engine.nastavSilu(elo)

        override fun ukazPozici(p: Int): Boolean {
            if (!engine.setPosition(zaznam.history[0], zaznam.tahy.take(p))) return false
            prubehHledani.korenHledani = zaznam.history[p]
            // PRED go: zive skore (PrubehHledani.skoreNaBileho) se pocita ze state.position.
            _state.value = _state.value.copy(
                position = Position.fromFen(zaznam.history[p])!!,
                konecPartie = konecNaDesce(p),
                bestMove = null,
                bestMoveSan = null,
            )
            return true
        }

        override fun stav(): String = engine.stav()
        override fun go(hloubka: Int, casMs: Int) = engine.go(hloubka, casMs)
        override fun stop() = engine.stop()
        override fun san(tah: Move): String? = engine.san(tah)
        override fun applyMove(tah: Move): String? = engine.applyMove(tah)
        override fun pruh(text: String) {
            ui = ui?.copy(pruh = text)
        }
    }

    /** Vstupy obrazovky pro analyzu partie. */
    val analyza = NapojeniAnalyzy(hraProAnalyzu)

    /**
     * Deska, kde uzivatel stal, a vymazane hodnoceni (shodne s U12/D).
     * Motor na tu pozici posila [poziceDoMotoru], tady jen stav.
     */
    private fun obnovDeskuPoAnalyze() {
        _state.value = _state.value.bezHledani().copy(
            stavHledani = StavHledani.KLID,
            analyza = null,
            position = Position.fromFen(zaznam.history[zaznam.index])!!,
            selected = null,
            targets = emptyList(),
            bestMove = null,
            bestMoveSan = zaznam.sanHistory.getOrNull(zaznam.index - 1),
        )
    }

    override fun onCleared() {
        engine.release()
    }
}
