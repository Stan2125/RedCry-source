package cz.redcry.chess.texty

import cz.redcry.chess.model.Side
import cz.redcry.chess.ui.PolozkaHry

// Anglicke texty aplikace (krok J2). Poradi a skupiny jako v [TextyCz],
// kde je u kazdeho textu, kde se ukazuje. Americky pravopis (Analyze,
// favor). Od kroku J3 i desetinna tecka a CSV s carkou.

object TextyEn : Texty {
    // --- Hlavni obrazovka: stavovy radek pod deskou -------------------------

    /**
     * "White to move": v anglictine stoji strana PRED popiskem. Novy clen
     * rozhrani v J2 (v TextyCz false): stavovy radek pak ukaze tucne
     * stranu a za ni [naTahu].
     */
    override val naTahuZaStranou = true
    override val naTahu = "to move"
    override val bily = "White"
    override val cerny = "Black"
    /** "Eval +0.24" */
    override val hodnota = "Eval"
    /** "Move 12" */
    override val tah = "Move"
    /** "Checkmate — White wins" */
    override val konecMat = "Checkmate —"
    override val vyhravaBily = "White wins"
    override val vyhravaCerny = "Black wins"
    /** "Stalemate — draw" */
    override val konecPat = "Stalemate —"
    override val remiza = "draw"
    /** "Draw — 50-move rule" / "Draw — threefold repetition" */
    override val konecRemiza = "Draw —"
    override val pravidlo50Tahu = "50-move rule"
    override val trojiOpakovani = "threefold repetition"

    // --- Hlavni obrazovka: velke tlacitko a dlazdice ------------------------

    override val premyslej = "Think"
    override val dobiha = "Finishing…"
    override val hrajNejlepsi = "Play best"
    override fun hrajNejlepsiSekundy(sekund: Int) = "Play best (" + sekund + " s)"
    override val nejlepsiTah = "Best move"
    override val aktualniTah = "Last move"
    override val hloubka = "Depth"
    override val hloubkaKniha = "book"

    // --- Hlavni obrazovka: ovladani motoru ----------------------------------

    override val engine = "Engine"
    override val plnaSila = "Full strength"
    override val silaElo = "Elo strength"
    override val maxHloubka = "Max depth"
    override val maxSekund = "Max seconds"
    override val hra = "Game"

    // --- Menu Hra -----------------------------------------------------------

    override fun polozkaHry(polozka: PolozkaHry) = when (polozka) {
        PolozkaHry.NOVA_PARTIE -> "New game…"
        PolozkaHry.OTOCIT -> "Flip board"
        PolozkaHry.KNIHOVNA -> "Opening book"
        PolozkaHry.ANALYZA -> "Analyze game…"
        PolozkaHry.NACIST -> "Load game"
        PolozkaHry.ULOZIT -> "Save game"
    }

    // --- Dialogy na hlavni obrazovce ----------------------------------------

    override val zahajitNovouPartii = "Start a new game?"

    override fun potvrzeniNovePartie(pocetTahu: Int): String {
        val tahu = if (pocetTahu == 1) "1 move" else "" + pocetTahu + " moves"
        return "The current game (" + tahu + ") will be discarded. If you want to keep it, save it first."
    }

    override val novaPartie = "New game"
    override val zrusit = "Cancel"
    override val nastavit = "Set"
    override fun rozsahSekund(min: Int, max: Int) = "" + min + "–" + max + ", 0 = no limit"
    override val promena = "Promotion"

    override val dama = "Queen"
    override val vez = "Rook"
    override val strelec = "Bishop"
    override val jezdec = "Knight"

    // --- Okno (i) -----------------------------------------------------------

    override val zahajeni = "Opening"
    override val zaznamPartie = "Game record"
    override val zatimZadnyTah = "No moves yet."
    override val hodnoceni = "Evaluation"
    override val varianta = "Line"
    override val predchoziVolba = "Previous choice"
    /** "Ng5, through depth 4, changed after 0.3 s" */
    override fun doHloubky(hloubka: Int) = "through depth " + hloubka
    /** Nedelitelna mezera: "s" se neodtrhne od cisla (krok J2, pismo 1,3). */
    override fun zmenaPo(sekundy: String) = "changed at " + sekundy + " s"
    override val poznamkaObnoveni =
        "⟳ restores the loaded game and goes to the move where the current " +
            "game departed from it."
    override val poznamkaElo =
        "Elo strength is approximate, based on the CCRL computer rating list " +
            "(blitz) – not human FIDE Elo."
    override val poznamkaSlabsiStockfish =
        "In the app, Stockfish plays about 200 Elo weaker than the value set " +
            "(measured at Elo 2650)."
    override val poznamkaHodnoceni =
        "Positive evaluations favor White, negative ones favor Black. +M3 means " +
            "mate in three moves for White, -M3 for Black."
    override val poznamkaPredchoziVolba =
        "Previous choice is the move the engine considered best before it found " +
            "a better one at a greater depth. It is not necessarily the " +
            "objectively second-best move."
    override val poznamkaSoukromi =
        "Privacy: RedCry does not collect, send or share any personal " +
            "data. It has no internet access, no ads and no analytics. " +
            "Games and analyses are saved to files you choose; settings " +
            "and analysis progress are kept on the device. Full policy: " +
            "github.com/Stan2125/RedCry-source/blob/main/PRIVACY.md"
    override val zavrit = "Close"
    override val eloMotor = "Engine"
    override val eloElo = "Elo"
    override val eloMereni = "5 s/move (emulator)"
    override val eloZmereno = "measured"
    override val eloPrepocet = "estimated"
    override val eloUdajAutoru = "per Stockfish authors"

    // --- Okno Analyza partie a pruh analyzy ---------------------------------

    override val analyzaPartie = "Game analysis"
    override val nedokoncenaAnalyza = "This game has an unfinished analysis."
    override val motor = "Engine"
    override val casNaPozici = "Time per position"
    override val casNaPoziciSekundy = "Time/position (s)"
    override fun sekund(sekund: Int) = "" + sekund + " s"
    override val tahy = "Moves"
    override val odTahu = "From move"
    override val doTahu = "To move"
    override val slozkaProVysledek = "Folder for results"
    override val slozkaNevybrana = "not selected"
    override val zmenitSlozku = "Change…"
    override val napovedaSlozky =
        "Choose a subfolder, e.g. Documents/RedCry. Android does not allow " +
            "the storage root or the Download folder itself."
    override val vRozsahuNeniTah = "There are no moves in the range."
    override fun pokracovatOdTahu(tah: Int) = "Continue from move $tah"
    override val spustit = "Start"
    override val zacitZnovu = "Start over"
    override fun pocetPozic(pocet: Int) = pocet.toString() + if (pocet == 1) " position" else " positions"
    override fun dobaMinuty(minut: Int) = "$minut min"
    override fun dobaHodiny(hodin: Int, minut: Int) = if (minut == 0) "$hodin h" else "$hodin h $minut min"
    override fun odhadAnalyzy(pozice: String, sekundNaPozici: Int, celkem: String) =
        pozice + " × " + sekundNaPozici + " s ≈ " + celkem
    override fun zbyvaAsi(doba: String) = "about $doba left"
    override val zbyvaMeneNezMinuta = "less than a minute left"
    override fun pruhAnalyzy(tah: Int, zTahu: Int, zbyva: String) =
        "Analyzing game – move $tah/$zTahu, " + zbyva
    override val pruhPozastavena = "Analysis paused"
    override val prerusitAnalyzuDotaz = "Stop the analysis?"
    override val prerusitAnalyzuText = "Finished positions stay saved and the analysis can be completed later."
    override val prerusit = "Stop"
    override val pokracovat = "Continue"

    // --- Vystupy analyzy (PGN a CSV) ----------------------------------------

    override fun annotator(jmenoMotoru: String, sekundNaPozici: Int) =
        "RedCry: $jmenoMotoru, $sekundNaPozici s/position"
    /** Hlavicka "Move,Side,...,Loss" (krok J3). */
    override val sloupceCsv = listOf(
        "Move", "Side", "Move played", "Eval", "Mate", "Depth", "Engine move", "Line", "Loss",
    )
    /** Anglicky Excel cte carku (a desetinnou tecku). */
    override val oddelovacCsv = ','

    // --- Cisla (krok J3) ----------------------------------------------------

    /** "+0.35", "changed at 2.7 s". */
    override val desetinnyOddelovac = '.'

    // --- Hlasky (Snackbar) --------------------------------------------------

    override val souborNeslPrecist = "Could not read the file."
    override val partieNactena = "Game loaded."
    override val partieUlozena = "Game saved."
    override val souborNeslUlozit = "Could not save the file."
    override fun motorSeNacita(motor: String) = motor + " is still loading, please wait a moment."
    override fun jeNaTahu(strana: Side) = "It's " + (if (strana == Side.WHITE) "White" else "Black") + "'s turn."
    override val tamFiguraNemuze = "That piece can't move there."
    override val neplatnyTah = "Illegal move."
    override val motorNenaselTah = "The engine hasn't found a move yet, try again."
    override val zadnyTahKonecPartie = "No move — the game is over."
    override val engineVratilNeplatnyTah = "The engine returned an illegal move."
    override fun motorNeniKDispozici(motor: String) = "Engine " + motor + " is not available yet."
    override fun motorSeNepodariloSpustit(motor: String) = "Engine " + motor + " could not be started."
    override val poziciNesloObnovit = "Could not restore the position."
    override val poziciNesloNastavit = "Could not set the position."
    override val jsiNaZacatku = "You are at the start of the game."
    override val jsiNaPoslednimTahu = "You are at the last move."
    override val vychoziPoziciNesloNastavit = "Could not set the starting position of the game."
    override fun tahNejdeZahrat(poradi: Int, token: String) = "Move " + poradi + " (" + token + ") cannot be played."
    override val enginePremysli = "The engine is thinking — wait, or press Play best."
    override val hledaniDobiha = "The previous search is still finishing."
    override val starsiPozice = "Playing from an earlier position — later moves will be discarded."
    override val starsiPoziceHledani =
        "Searching from an earlier position — a different move will discard later moves."
    override val knihovnaZapnuta = "Opening book on."
    override val knihovnaVypnuta = "Opening book off."
    override val knihovnaChyba = "Could not load the opening book."
    override fun nizsiHloubkaNejde(min: Int) = "Depth can't be lower than " + min + "."
    override fun umiNejvysHloubku(motor: String, max: Int) = motor + " supports depth up to " + max + "."
    override val meneNezBezOmezeni = "Can't go below no limit."
    override fun nejvysSekund(max: Int) = "At most " + max + " seconds per move."
    override fun nizsiHloubkaNastaveno(hloubka: Int) =
        "Depth can't be lower than " + hloubka + " — set to " + hloubka + "."
    override fun umiHloubkuNastaveno(motor: String, hloubka: Int) =
        motor + " supports depth up to " + hloubka + " — set to " + hloubka + "."
    override val zapornyCasNastaveno = "Time can't be negative — set to no limit."
    override fun nejvysSekundNastaveno(sekund: Int) =
        "At most " + sekund + " seconds per move — set to " + sekund + "."
    override fun neniCislo(text: String) = "“" + text.trim() + "” is not a number — nothing changed."
    override fun umiEloNastaveno(motor: String, min: Int, max: Int, elo: Int?) =
        motor + " supports Elo " + min + "–" + max + " — set to " + elo + "."
    override val analyzaBezi = "Game analysis in progress."
    override val partieNemaTah = "The game has no moves."
    override val nejdrivVyberSlozku = "Choose a folder for the results first."
    override val vZadanemRozsahuNeniTah = "There are no moves in the selected range."
    override fun analyzaPrerusenaChybou(zprava: String) = "Analysis stopped: $zprava"
    /** Za "Analysis did not start: ". */
    override fun motorNesloSpustitAnalyza(motor: String) = motor + " could not be started."
    /** Za "Analysis did not start: ", proto malym pismenem. */
    override val souboryNesloZalozit = "could not create the files in the folder."
    override val analyzaPozicNesloNastavit = "Analysis stopped: could not set the position."
    override val analyzaChybaZapisu = "Analysis stopped: could not write the files."
    override fun souboryAnalyzy(nazev: String) = nazev + ".pgn and .csv"
    override fun analyzaHotova(soubory: String) = "Analysis finished: $soubory"
    override val analyzaPrerusena = "Analysis stopped."
    override fun analyzaPrerusenaNaTahu(tah: Int, soubory: String) =
        "Analysis stopped at move " + tah + ", saved: " + soubory
    override fun analyzaPrerusenaBaterie(procent: Int) = "Analysis stopped: low battery ($procent%)."
    override fun analyzaSeNespustila(zprava: String) = "Analysis did not start: " + zprava

    // --- Chyby motoru (nove v J2; do J1 cesky primo v engine/) -------------

    /** StockfishEngine: zalozeni nedobehlo do [sekund] s. */
    override fun motorSeNespustilDo(motor: String, sekund: Int) =
        motor + " did not start within " + sekund + " s."
    /** StockfishEngine: predchozi instance se neuvolnila. */
    override fun motorSeNeuvolnil(motor: String, sekund: Int) =
        motor + " could not be started: the previous instance was not released within " + sekund + " s."
    /** StockfishBridge, CraftyBridge: nativni instance uz existuje. */
    override fun motorUzBezi(motor: String) =
        motor + " is already running in another instance (or memory ran out) – " +
            "two " + motor + " engines cannot run at the same time."

    // --- TalkBack (cteni obrazovky) -----------------------------------------

    override val zavritHlasku = "Dismiss message"
    override val predchoziTahPopis = "Previous move"
    override val dalsiTahPopis = "Next move"
    override val zpetKNactenePartii = "Back to the loaded game"
    override val zapnuto = "on"
    override val jazyk = "Language"
    override val nazevCestiny = "Czech"
    override val nazevAnglictiny = "English"
    override val zaznamPartiePopis = "Game record"
    override val prerusitAnalyzu = "Stop analysis"
    override val zadatCislo = "Enter a number"
}
