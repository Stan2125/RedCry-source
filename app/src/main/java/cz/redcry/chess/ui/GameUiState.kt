package cz.redcry.chess.ui

import cz.redcry.chess.analyza.AnalyzaUi
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.kniha.Zahajeni
import cz.redcry.chess.model.Move
import cz.redcry.chess.model.Position
import cz.redcry.chess.model.Square

// Stav obrazovky, ktery vystavuje GameViewModel.
// Vzniklo v kroku R1 presunem z GameViewModel.kt.

data class GameUiState(
    val position: Position,
    val selected: Square? = null,
    val targets: List<Square> = emptyList(),
    val flipped: Boolean = false,
    val stavHledani: StavHledani = StavHledani.KLID,
    /** Hloubka, kterou hledani UZ DOKONCILO. Strop hledani je [maxHloubka]. */
    val depth: Int? = null,
    /**
     * Hloubka, do ktere se nejvyse bude hledat po stisku Premyslej. Patri
     * VYBRANEMU motoru - kazdy si drzi svoji, viz [GameViewModel.nastaveni].
     *
     * Proti [depth] je to NASTAVENA mez, ne dosazena hodnota: hledani skonci
     * driv, kdyz vyprsi [maxSekund].
     */
    val maxHloubka: Int = EngineId.vychozi.vychoziHloubka,
    /**
     * Casovy strop jednoho hledani v sekundach, taky pro vybrany motor zvlast.
     * Nula znamena bez omezeni a v radku se vykresli jako nekonecno.
     */
    val maxSekund: Int = EngineId.vychozi.vychoziSekundy,
    /**
     * Meze [maxHloubka] vybraneho motoru - kvuli popisku v dialogu pro zadani
     * cislem. Meni se jen s motorem, a motor se meni jen pres onNewGame.
     */
    val rozsahHloubky: IntRange = EngineId.vychozi.minHloubka..EngineId.vychozi.maxHloubka,
    /**
     * Sila vybraneho motoru podle Elo (krok S6), null = plna sila nebo motor
     * bez Elo. Patri vybranemu motoru jako [maxHloubka].
     */
    val elo: Int? = null,
    /** Meze Elo vybraneho motoru; null = motor silu podle Elo nastavit neumi. */
    val rozsahElo: IntRange? = null,
    /**
     * Elo, na ktere motor spadne po vypnuti Plne sily (krok U5): naposledy
     * nastavene, jinak vychozi. Null u motoru bez Elo. Obrazovka ho od
     * kroku U12/A za plne sily neukazuje (radek Sila ELO je prazdny), stav
     * ho ale drzi dal: rika, co prijde po vypnuti Plne sily.
     */
    val eloOslabeni: Int? = null,
    val bestMove: Move? = null,
    /**
     * Zapis [bestMove] v SAN, kdyz ho motor umi. Null znamena, ze si dlazdice
     * ma vystacit s [bestMove] a jeho souradnicovym tvarem.
     */
    val bestMoveSan: String? = null,
    val engineName: String = EngineId.vychozi.label,
    val historyIndex: Int = 0,
    /**
     * Jazyk aplikace (krok J2). Drzi ho ViewModel, aby hlasky i obrazovka
     * byly v temze jazyce; meni ho [GameViewModel.onJazyk].
     */
    val jazyk: Jazyk = Jazyk.CESTINA,
    val historySize: Int = 1,
    /**
     * Odehrane tahy v SAN, tentyz obsah jako [GameViewModel.sanZaznam].
     * Je to soucast stavu schvalne: popup se zaznamem se otevira i za
     * beziciho hledani a musi se sam obnovit, kdyz behem toho tah pribude.
     */
    val sanZaznam: List<String> = emptyList(),
    /**
     * Hlavni varianta posledniho hlaseni hledani jako radky okna (i), od
     * kroku U14/B cela v SAN s cisly tahu ("14... Nbxd5", "15. Qa4+ Qd7"),
     * cislovana podle korene hledani. Kdyz prevod selze, zustava drivejsi
     * tvar: jeden tah na prvek, prvni v SAN, dalsi souradnicove. Viz
     * [GameViewModel.slozVariantu] a [radkyVarianty].
     */
    val varianta: List<String> = emptyList(),
    /**
     * Tah, ktery motor povazoval za nejlepsi, nez ve vetsi hloubce nasel
     * lepsi (krok U14/C, pravidlo v [SledovaniVolby]); null = zadny takovy
     * neni a okno (i) oddil nevykresli. Pocita se jen za Premyslej, maze se
     * vsude spolu s [varianta] a po tahu motoru zustava jako ona.
     */
    val predchoziVolba: PredchoziVolba? = null,
    /**
     * Hodnoceni pozice v setinach pesce, VZDY Z POHLEDU BILEHO: kladne cislo
     * je vyhoda bileho, at je na tahu kdokoli.
     *
     * Oba motory hlasi skore podle UCI, tedy z pohledu strany na tahu.
     * Prepocet na bileho dela ViewModel pri prijmu - cislo, ktere meni
     * znamenko podle toho, kdo je na tahu, je v zaznamu partie k nicemu.
     * Vykresluje se pres [formatujSkoreKratce] ve stavovem radku pod
     * deskou (krok U12/B) i v okne (i) (od kroku U14/A).
     */
    val skoreCp: Int? = null,
    /**
     * Cislovani zaznamu. Obe hodnoty se berou z VYCHOZI pozice partie, tedy
     * z `history[0]`, a meni se jen tam, kde se ta pozice meni - v nove
     * partii, pri importu FEN a pri nacteni PGN.
     *
     * Ve stavu jsou od kroku 8: nacteni partie z PGN je meni SPOLU se
     * seznamem tahu, takze popup, ktery cte `sanZaznam` ze stavu, by se jinak
     * prekreslil s novymi tahy a starym cislovanim.
     */
    val prvniJeCerny: Boolean = false,
    val cisloPrvnihoTahu: Int = 1,
    /**
     * Klepnuty tah (z, na), ktery je promenou a ceka na vyber figury. Dokud
     * neni null, ukazuje obrazovka dialog; tah se zahraje az v
     * [GameViewModel.onPromenaVybrana].
     */
    val cekaNaPromenu: Pair<Square, Square>? = null,
    /**
     * Konec partie v pozici NA DESCE, tedy pod kurzorem - ne na konci
     * zaznamu jako u vysledku v PGN. Kdo se sipkou vrati z matove pozice,
     * ma na desce partii, ktera matem nekonci.
     */
    val konecPartie: KonecPartie? = null,
    /**
     * Pocet tahu rozehrane partie, kdyz se ceka na potvrzeni dialogu
     * "Zahájit novou partii?" (krok U6/A). Null = dialog zavreny. Je ve
     * stavu, ne v obrazovce, at jde napsat JVM test, kdy se dialog ukaze.
     *
     * Od kroku U7/C v TAZICH, ne pultazich - tychz jednotkach jako ukazatel
     * "Tah N" nahore, viz [pocetTahuZaznamu].
     */
    val potvrzeniNovePartie: Int? = null,
    /**
     * Tlacitko ⟳ je aktivni (krok U6/B): existuje nactena partie a aktualni
     * zaznam se od ni odchylil, viz [tahOdboceni].
     */
    val lzeObnovit: Boolean = false,
    /** Okno, pruh a dialog analyzy partie (krok A1/B); null = nic z toho. */
    val analyza: AnalyzaUi? = null,
    /**
     * Knihovna zahajeni zapnuta (krok K1/B), polozka v menu Hra. Plati jen
     * pro vyber tahu po Premyslej; nazev zahajeni se ukazuje i bez ni.
     * Zije jen ve ViewModelu, po novem spusteni je zase zapnuto.
     */
    val knihovna: Boolean = true,
    /**
     * Posledni tah zahral motor z knihovny, ne z hledani (krok K1/B):
     * dlazdice Hloubka i okno (i) ukazuji "kniha". Maze se spolu
     * s vysledky hledani ([GameViewModel.bezHledani]).
     */
    val zKnihy: Boolean = false,
    /**
     * Nazev zahajeni partie na desce: posledni pojmenovana pozice
     * v historii od zacatku po [historyIndex] (krok K1/B); null = zadna
     * nebo knihovna jeste neni nactena.
     */
    val zahajeni: Zahajeni? = null,
)
