package cz.redcry.chess.ui

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Snackbar
import androidx.compose.material3.SnackbarDuration
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.SwipeToDismissBox
import androidx.compose.material3.SwipeToDismissBoxValue
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.rememberSwipeToDismissBoxState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.AccessibilityManager
import androidx.compose.ui.platform.LocalAccessibilityManager
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import cz.redcry.chess.analyza.Cislovani
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.model.Side
import cz.redcry.chess.pgn.navrhJmenaSouboru
import cz.redcry.chess.texty.LocalTexty
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull

/**
 * Tlacitko "Hraj nejlepsi". Zamerne mimo schema motivu: na miste Premyslej
 * se objevi za hledani a musi byt na prvni pohled jine - stisk totiz zahraje
 * tah. Do kroku 18 stalo vedle cerveneho Zastav.
 */
private val FIALOVA = Color(0xFF7B1FA2)

/**
 * Typ, pod kterym se uklada partie. Pri NACITANI se schvalne nepouziva:
 * spousta spravcu souboru `application/x-chess-pgn` nezna a soubor by pod
 * timhle filtrem vubec neukazala. Tam se proto posila zastupny typ,
 * ktery projde vsim.
 */
internal const val TYP_PGN = "application/x-chess-pgn"

/**
 * Jedina cesta pro hlasky, at uz prijdou z ViewModelu nebo od souboroveho
 * dialogu. Cerstva nahradi tu predchozi, at se necekaji fronty.
 *
 * dismiss() na zacatku zabere jen u hlasek z jine korutiny (nacteni,
 * ulozeni). Hlasky z ViewModelu chodi jednou korutinou za sebou, tam frontu
 * rusi az collectLatest v GameScreen - viz komentar tam.
 */
private suspend fun ukazHlasku(
    host: SnackbarHostState,
    text: String,
    a11y: AccessibilityManager?,
) {
    host.currentSnackbarData?.dismiss()
    // Delka podle textu (U11), ale prepoctena pres AccessibilityManager:
    // vychozi SnackbarDuration.Short se podle nastaveni pristupnosti
    // v Androidu ("Cas na reakci") prodluzuje a vlastni casovac by to jinak
    // obesel. Bez spravce pristupnosti (napr. v nahledu) plati zaklad.
    val zaklad = trvaniHlasky(text)
    val ms = a11y?.calculateRecommendedTimeoutMillis(
        zaklad, containsIcons = false, containsText = true, containsControls = false,
    ) ?: zaklad
    // Indefinite + vlastni timeout: preruseni showSnackbar hlasku
    // z obrazovky sunda, stejne jako dismiss() klepnutim nebo pretazenim.
    withTimeoutOrNull(ms) {
        host.showSnackbar(text, duration = SnackbarDuration.Indefinite)
    }
}

@Composable
fun GameScreen(viewModel: GameViewModel = viewModel(factory = GameViewModelFactory)) {
    // Krok J1: texty cele obrazovky. Od J2 podle jazyka ze stavu ViewModelu.
    val jazyk = viewModel.state.collectAsStateWithLifecycle().value.jazyk
    CompositionLocalProvider(LocalTexty provides jazyk.texty) {
        ObrazovkaHry(viewModel)
    }
}

@Composable
private fun ObrazovkaHry(viewModel: GameViewModel) {
    val texty = LocalTexty.current
    val state by viewModel.state.collectAsStateWithLifecycle()
    // Zvlast, ne ze state: tika jen dokud ho obrazovka sbira, viz
    // GameViewModel.sekundyHledani.
    val sekundyHledani by viewModel.sekundyHledani.collectAsStateWithLifecycle(null)
    val snackbarHostState = remember { SnackbarHostState() }
    var zaznamOtevren by remember { mutableStateOf(false) }
    // Ktera z obou mezi se prave zadava cislem; null = dialog zavreny.
    var zadavanaMez by remember { mutableStateOf<ZadavanaMez?>(null) }
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val a11y = LocalAccessibilityManager.current

    // Krok A1/B: soubory analyzy. Do ViewModelu jednou a uz se nenuluje -
    // ViewModel prezije Activity a bezici analyza by pri zapisu narazila
    // na null.
    val ulozisteAnalyzy = remember(viewModel) {
        (viewModel.analyza.uloziste as? UlozisteAnalyzyAndroid)
            ?: UlozisteAnalyzyAndroid(context).also { viewModel.analyza.uloziste = it }
    }
    // Spustit bez vybrane slozky nejdriv otevre vyber; tohle pak spusti.
    var cekajiciSpusteni by remember { mutableStateOf<SpusteniAnalyzy?>(null) }
    val vyberSlozky = rememberLauncherForActivityResult(
        ActivityResultContracts.OpenDocumentTree()
    ) { uri ->
        if (uri != null) {
            ulozisteAnalyzy.nastavSlozku(uri)
            viewModel.analyza.onSlozkaZmenena()
        } else {
            cekajiciSpusteni = null
        }
    }
    val slozkaAnalyzy = state.analyza?.okno?.slozka
    LaunchedEffect(slozkaAnalyzy) {
        val s = cekajiciSpusteni ?: return@LaunchedEffect
        if (slozkaAnalyzy == null) return@LaunchedEffect
        cekajiciSpusteni = null
        viewModel.analyza.onSpustit(s.motor, s.sekundy, s.odTahu, s.doTahu)
    }
    AnalyzaEfekty(viewModel, bezi = state.analyza?.bezi == true)

    LaunchedEffect(viewModel) {
        // collectLatest, ne collect (U11): s collect se dalsi hlaska vzala
        // az po dobehnuti predchozi, takze 3x rychle <- drzelo "Jsi na
        // zacatku partie." 6 s misto 2 s (zmereno na emulatoru). Nova
        // hlaska predchozi ukazHlasku zrusi a tim ji sunda z obrazovky.
        viewModel.messages.collectLatest { message ->
            ukazHlasku(snackbarHostState, message, a11y)
        }
    }

    // MainActivity je ComponentActivity, takze staci tyhle dva launchery tady
    // a do aktivity se nic dopisovat nemusi. Oprávnění k úložišti SAF
    // nepotrebuje, do manifestu proto nepribylo nic.
    //
    // Uri ani contentResolver se do ViewModelu NEDOSTANOU - dostane a vrati
    // jen cisty String. Diky tomu zustava cely testovatelny v JVM.
    val nacist = rememberLauncherForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri ->
        if (uri != null) {
            scope.launch {
                val text = withContext(Dispatchers.IO) {
                    runCatching {
                        context.contentResolver.openInputStream(uri)?.use {
                            it.readBytes().toString(Charsets.UTF_8)
                        }
                    }.getOrNull()
                }
                when {
                    text == null -> ukazHlasku(snackbarHostState, texty.souborNeslPrecist, a11y)
                    // Kdyz nacteni neprojde cele, hlasku o tom uz poslal
                    // ViewModel sam a rekl v ni, u ktereho tahu to spadlo.
                    viewModel.nactiPgn(text) -> ukazHlasku(snackbarHostState, texty.partieNactena, a11y)
                }
            }
        }
    }

    val ulozit = rememberLauncherForActivityResult(
        ActivityResultContracts.CreateDocument(TYP_PGN)
    ) { uri ->
        if (uri != null) {
            // Slozeni retezce je prace na mikrosekundy a musi se stat driv,
            // nez se uzivatel stihne pohnout po desce; mimo hlavni vlakno jde
            // az sam zapis.
            val text = viewModel.exportPgn()
            scope.launch {
                val ok = withContext(Dispatchers.IO) {
                    runCatching {
                        // "wt" = zkrátit; bez nej by pri prepisu delsiho
                        // souboru zustal na konci ocasek stareho obsahu.
                        val proud = context.contentResolver.openOutputStream(uri, "wt")
                            ?: error("outputStream je null")
                        proud.use { it.write(text.toByteArray(Charsets.UTF_8)) }
                    }.isSuccess
                }
                ukazHlasku(
                    snackbarHostState,
                    if (ok) texty.partieUlozena else texty.souborNeslUlozit,
                    a11y,
                )
            }
        }
    }

    Scaffold(
        // Hlaska dole zakryva radu tlacitek, proto jde zavrit klepnutim
        // i pretazenim do strany, obema smery (U11).
        // Krok A1/B: pod hlaskou pruh analyzy - hlaska vyjede nad nim,
        // takze ho nikdy nezakryje ani nezrusi.
        snackbarHost = {
            Column {
            SnackbarHost(snackbarHostState) { data ->
                // key(data): stav pretazeni se nesmi prenest na dalsi hlasku,
                // jinak by nova vyjela uz odsunuta.
                key(data) {
                    val swipe = rememberSwipeToDismissBoxState(
                        confirmValueChange = { hodnota ->
                            if (hodnota != SwipeToDismissBoxValue.Settled) data.dismiss()
                            true
                        },
                    )
                    SwipeToDismissBox(state = swipe, backgroundContent = {}) {
                        Snackbar(
                            snackbarData = data,
                            modifier = Modifier.clickable(
                                onClickLabel = texty.zavritHlasku,
                            ) { data.dismiss() },
                        )
                    }
                }
            }
            state.analyza?.pruh?.let { PruhAnalyzy(it, viewModel.analyza::onPrerusDotaz) }
            }
        },
        contentWindowInsets = WindowInsets.safeDrawing,
    ) { innerPadding ->
        // Krok U5: rozvrzeni podle docs/navrhy/redcry-layout-v3.json. Bez
        // rolovani - vsechno se vejde na obrazovku, viz [Navrh].
        BoxWithConstraints(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding),
            contentAlignment = Alignment.TopCenter,
        ) {
            val n = Navrh.pro(maxWidth, maxHeight)
            Column(modifier = Modifier.width(n.px(Navrh.OBSAH))) {
                Spacer(Modifier.height(n.px(Navrh.NAHORE)))

                BoardCanvas(
                    position = state.position,
                    selected = state.selected,
                    targets = state.targets,
                    flipped = state.flipped,
                    onSquareClick = viewModel::onSquareClick,
                    modifier = Modifier
                        .align(Alignment.CenterHorizontally)
                        .size(n.deska)
                        .border(1.dp, Color(0xFF808080)),
                )

                Spacer(Modifier.height(n.px(Navrh.MEZERA_TAH)))

                // Box, ne Row se SpaceBetween (krok U12/B): Hodnota ma stat
                // uprostred radku, ne uprostred mezery mezi levym a pravym
                // textem, ktere jsou ruzne siroke.
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(n.px(Navrh.TAH)),
                ) {
                    val vlevo = Modifier.align(Alignment.CenterStart)
                    // Konec partie trvale v radku, ne Snackbarem: ten zmizi a na
                    // desce by pak nic nerikalo, proc se neda tahnout.
                    when (state.konecPartie) {
                        KonecPartie.MAT_VYHRAL_BILY -> LabelledValue(texty.konecMat, texty.vyhravaBily, vlevo)
                        KonecPartie.MAT_VYHRAL_CERNY -> LabelledValue(texty.konecMat, texty.vyhravaCerny, vlevo)
                        KonecPartie.PAT -> LabelledValue(texty.konecPat, texty.remiza, vlevo)
                        KonecPartie.REMIZA_50 -> LabelledValue(texty.konecRemiza, texty.pravidlo50Tahu, vlevo)
                        KonecPartie.REMIZA_OPAKOVANIM -> LabelledValue(texty.konecRemiza, texty.trojiOpakovani, vlevo)
                        null -> LabelledValue(
                            texty.naTahu,
                            if (state.position.sideToMove == Side.WHITE) texty.bily else texty.cerny,
                            vlevo,
                            // Krok J2: "White to move" - strana pred popiskem.
                            hodnotaPrvni = texty.naTahuZaStranou,
                        )
                    }
                    // Totez skore jako Hodnoceni v okne (i), jen kratce. Na konci
                    // partie ne: text konce je dlouhy a hodnota uz nic nerika.
                    if (state.konecPartie == null) {
                        LabelledValue(
                            texty.hodnota,
                            formatujSkoreKratce(state.skoreCp, texty),
                            Modifier.align(Alignment.Center),
                        )
                    }
                    LabelledValue(
                        texty.tah,
                        state.position.fullmoveNumber.toString(),
                        Modifier.align(Alignment.CenterEnd),
                    )
                }

                Spacer(Modifier.height(n.px(Navrh.MEZERA_PREMYSLEJ)))

                // Za hledani je jen jedno tlacitko pres celou sirku: fialove Hraj
                // nejlepsi s odpoctem. Cervene Zastav od kroku 18 neni - tah se
                // uz zahodit neda, jen zahrat driv.
                //
                // Dobihajici hledani vypada zamcene, ale zmacknout se da: vypnute
                // tlacitko na dotykaku nerekne, proc nejde zmacknout, a ViewModel
                // na stisk odpovi hlaskou o dobihani. Tataz uvaha jako u sipek.
                val premyslejModifier = Modifier
                    .fillMaxWidth()
                    .height(n.px(Navrh.PREMYSLEJ))
                if (state.stavHledani == StavHledani.HLEDA) {
                    Button(
                        onClick = viewModel::onHrajNejlepsi,
                        colors = ButtonDefaults.buttonColors(containerColor = FIALOVA),
                        modifier = premyslejModifier,
                    ) {
                        Text(
                            // Uplynuly cas, ne zbyvajici: pri max sekund = 0
                            // (bez omezeni) by odpocet nemel od ceho odcitat.
                            text = sekundyHledani?.let { texty.hrajNejlepsiSekundy(it) } ?: texty.hrajNejlepsi,
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Medium,
                            maxLines = 1,
                        )
                    }
                } else {
                    val dobiha = state.stavHledani == StavHledani.DOBIHA_ZAHRAT
                    // Za analyzy (A1/B) sede jako pri dobihani, ale text
                    // zustava; stisk rekne "Probíhá analýza partie."
                    val sede = dobiha || state.stavHledani == StavHledani.ANALYZA
                    Button(
                        onClick = viewModel::onThinkClick,
                        colors = if (sede) {
                            ButtonDefaults.buttonColors(
                                containerColor = MaterialTheme.colorScheme.surfaceVariant,
                                contentColor = MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                        } else {
                            ButtonDefaults.buttonColors()
                        },
                        modifier = premyslejModifier,
                    ) {
                        Text(
                            text = if (dobiha) texty.dobiha else texty.premyslej,
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Medium,
                            maxLines = 1,
                        )
                    }
                }

                Spacer(Modifier.height(n.px(Navrh.MEZERA_DLAZDICE)))

                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(n.px(Navrh.DLAZDICE)),
                    horizontalArrangement = Arrangement.spacedBy(n.px(Navrh.MEZERA_VEDLE)),
                ) {
                    // SAN, kdyz ho motor umi; jinak souradnicovy tvar z modelu.
                    // Move.toString() se kvuli tomu nemeni - model o notaci vedet nema.
                    //
                    // Popisek se prepina podle stavu: za hledani je v dlazdici
                    // nejlepsi NALEZENY tah, ktery jeste nikdo nezahral; jindy je
                    // to naposledy zahrany tah, at ho zahral motor nebo ruka.
                    // Za analyzy (A1/B) je tam taky nalezeny tah z Info.
                    val hleda = state.stavHledani == StavHledani.HLEDA ||
                        state.stavHledani == StavHledani.ANALYZA
                    MetricCard(
                        if (hleda) texty.nejlepsiTah else texty.aktualniTah,
                        state.bestMoveSan ?: state.bestMove?.toString() ?: "—",
                        Modifier.weight(1f).fillMaxHeight(),
                    )
                    // Krok K1/B: po tahu z knihovny zadna hloubka neni.
                    MetricCard(
                        texty.hloubka,
                        textHloubky(state.depth, state.zKnihy, texty),
                        Modifier.weight(1f).fillMaxHeight(),
                    )
                }

                Spacer(Modifier.height(n.px(Navrh.MEZERA_OVLADANI)))

                // Pevna vyska bloku: radky Stockfishe maji misto vyhrazene
                // i u ostatnich motoru, takze prepnuti motoru nic neposune.
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(n.px(Navrh.OVLADANI)),
                ) {
                    Column(modifier = Modifier.width(n.px(Navrh.LEVY_SLOUPEC))) {
                        RadekJazyka(
                            n = n,
                            jazyk = state.jazyk,
                            onJazyk = viewModel::onJazyk,
                            onInfo = { zaznamOtevren = true },
                        )
                        // Krok U16: Engine a pod nim Sila o kousek niz.
                        Spacer(Modifier.height(n.px(Navrh.MEZERA_POD_JAZYKEM)))
                        EngineBlok(
                            n = n,
                            engineName = state.engineName,
                            // Od kroku U7/A i v rozehrane partii: novy motor
                            // dostane pozici z historie. Za hledani ne.
                            selectable = state.stavHledani == StavHledani.KLID,
                            onEngineSelected = viewModel::onEngineSelected,
                        )
                        // Krok S6: jen u motoru, ktery silu podle Elo umi (Stockfish).
                        if (state.rozsahElo != null) {
                            SilaBlok(
                                n = n,
                                elo = state.elo,
                                odemceno = state.stavHledani == StavHledani.KLID,
                                onPlnaSila = viewModel::onPlnaSila,
                                onZadat = { zadavanaMez = ZadavanaMez.ELO },
                            )
                        }
                    }
                    MezeSloupec(
                        n = n,
                        maxHloubka = state.maxHloubka,
                        maxSekund = state.maxSekund,
                        // Tytez situace jako u vyberu motoru: za hledani ani za jeho
                        // dobihani se meze menit nesmi, prislo by to az pristi tah.
                        odemceno = state.stavHledani == StavHledani.KLID,
                        onHloubkaZmena = viewModel::onHloubkaZmena,
                        onSekundyZmena = viewModel::onSekundyZmena,
                        onZadat = { zadavanaMez = it },
                        modifier = Modifier.weight(1f),
                    )
                }

                Spacer(Modifier.height(n.px(Navrh.MEZERA_SIPKY)))

                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(n.px(Navrh.SIPKY)),
                    horizontalArrangement = Arrangement.spacedBy(n.px(Navrh.MEZERA_SIPEK)),
                ) {
                    // Sipky zustavaji aktivni i na krajich historie - vypnute
                    // tlacitko by nereklo proc. Na kraji to hlasi Snackbar.
                    //
                    // Krok U6/A: stejne siroka tlacitka a Hra ve zbytku,
                    // v pomeru 160 : 160 : 160 : 174.
                    //
                    // Krok U15: (i) je nahore vedle prepinace jazyka, takze ho
                    // uz nezakryje hlaska dole.
                    val sirkaTlacitka = Modifier.weight(Navrh.VAHA_TLACITKA).fillMaxHeight()
                    StepButton(
                        icon = Icons.AutoMirrored.Filled.KeyboardArrowLeft,
                        contentDescription = texty.predchoziTahPopis,
                        onClick = viewModel::onStepBack,
                        modifier = sirkaTlacitka,
                    )
                    StepButton(
                        icon = Icons.AutoMirrored.Filled.KeyboardArrowRight,
                        contentDescription = texty.dalsiTahPopis,
                        onClick = viewModel::onStepForward,
                        modifier = sirkaTlacitka,
                    )
                    StepButton(
                        icon = Icons.Default.Refresh,
                        contentDescription = texty.zpetKNactenePartii,
                        onClick = viewModel::onObnovPartii,
                        // Krok U6/B: zasedle bez nactene partie a tehdy, kdyz
                        // se od ni aktualni partie neodchylila. Za hledani
                        // aktivni jako sipky - ViewModel rekne, proc ne.
                        enabled = state.lzeObnovit,
                        zvyrazneny = true,
                        modifier = sirkaTlacitka,
                    )
                    HraMenu(
                        analyzaPovolena = state.sanZaznam.isNotEmpty(),
                        knihovnaZapnuta = state.knihovna,
                        onOtevrit = viewModel.analyza::onMenuHra,
                        onPolozka = { polozka ->
                            provedPolozkuHry(
                                polozka,
                                viewModel,
                                nacist = { nacist.launch(arrayOf("*/*")) },
                                ulozit = { ulozit.launch(navrhJmenaSouboru()) },
                            )
                        },
                        modifier = Modifier.weight(Navrh.VAHA_HRA).fillMaxHeight(),
                    )
                }
            }
        }

        // Barva figur v dialogu je barva strany na tahu - tahne ta, ktera
        // promenuje, a pozice se za otevreneho dialogu nemeni.
        if (state.cekaNaPromenu != null) {
            PromenaDialog(
                strana = state.position.sideToMove,
                onVyber = viewModel::onPromenaVybrana,
                onDismiss = viewModel::onPromenaZrusena,
            )
        }

        state.analyza?.okno?.let { okno ->
            OknoAnalyzyDialog(
                okno = okno,
                cislovani = Cislovani(state.prvniJeCerny, state.cisloPrvnihoTahu),
                pocetPultahu = state.sanZaznam.size,
                onSpustit = { motor, sekundy, od, `do` ->
                    if (okno.slozka == null) {
                        cekajiciSpusteni = SpusteniAnalyzy(motor, sekundy, od, `do`)
                        vyberSlozky.launch(null)
                    } else {
                        viewModel.analyza.onSpustit(motor, sekundy, od, `do`)
                    }
                },
                onPokracovat = viewModel.analyza::onPokracovat,
                onZacitZnovu = viewModel.analyza::onZacitZnovu,
                onZmenitSlozku = { vyberSlozky.launch(null) },
                onZavrit = viewModel.analyza::onZavrit,
            )
        }
        if (state.analyza?.dotazPreruseni == true) {
            PrerusitAnalyzuDialog(
                onPrerusit = viewModel.analyza::onPrerus,
                onPokracovat = viewModel.analyza::onPrerusZrusen,
            )
        }

        state.potvrzeniNovePartie?.let { pultahu ->
            AlertDialog(
                onDismissRequest = viewModel::onNovaPartieZrusena,
                title = { Text(texty.zahajitNovouPartii) },
                text = { Text(texty.potvrzeniNovePartie(pultahu)) },
                confirmButton = {
                    TextButton(onClick = viewModel::onNovaPartiePotvrzena) { Text(texty.novaPartie) }
                },
                dismissButton = {
                    TextButton(onClick = viewModel::onNovaPartieZrusena) { Text(texty.zrusit) }
                },
            )
        }

        zadavanaMez?.let { mez ->
            val zavri = { zadavanaMez = null }
            when (mez) {
                ZadavanaMez.HLOUBKA -> ZadaniMezeDialog(
                    nadpis = texty.maxHloubka,
                    rozsah = "" + state.rozsahHloubky.first + "–" + state.rozsahHloubky.last,
                    hodnota = state.maxHloubka,
                    onPotvrdit = { viewModel.onHloubkaZadana(it); zavri() },
                    onDismiss = zavri,
                )
                ZadavanaMez.SEKUNDY -> ZadaniMezeDialog(
                    nadpis = texty.maxSekund,
                    rozsah = texty.rozsahSekund(EngineId.MIN_SEKUND, EngineId.MAX_SEKUND),
                    hodnota = state.maxSekund,
                    onPotvrdit = { viewModel.onSekundyZadane(it); zavri() },
                    onDismiss = zavri,
                )
                ZadavanaMez.ELO -> state.rozsahElo?.let { rozsah ->
                    ZadaniMezeDialog(
                        nadpis = texty.silaElo,
                        rozsah = "" + rozsah.first + "–" + rozsah.last,
                        hodnota = state.elo ?: rozsah.first,
                        onPotvrdit = { viewModel.onEloZadane(it); zavri() },
                        onDismiss = zavri,
                    )
                }
            }
        }

        if (zaznamOtevren) {
            // Cislovani i zaznam jsou obe ve stavu, takze se popup prekresli
            // zaroven - at uz tah pribyl za jeho otevreni, nebo se nactela
            // cela partie z PGN, ktera zacina cernym a jinym cislem tahu.
            ZaznamDialog(
                radky = zapisPartie(
                    state.sanZaznam,
                    state.prvniJeCerny,
                    state.cisloPrvnihoTahu,
                ),
                aktivniIndex = state.historyIndex,
                zahajeni = state.zahajeni,
                hloubka = textHloubky(state.depth, state.zKnihy, texty),
                varianta = state.varianta,
                predchoziVolba = state.predchoziVolba,
                skore = state.skoreCp,
                onSkok = { index ->
                    viewModel.skocNaTah(index)
                    // Smyslem klepnuti je podivat se na desku, kterou dialog
                    // prekryva - takze se zaroven zavre.
                    zaznamOtevren = false
                },
                onDismiss = { zaznamOtevren = false },
            )
        }
    }
}
