package cz.redcry.chess.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.LocalTextStyle
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import cz.redcry.chess.kniha.Zahajeni
import cz.redcry.chess.texty.LocalTexty
import cz.redcry.chess.texty.Texty
import cz.redcry.chess.texty.TextyCz

// Okno (i): zaznam partie, hloubka, hodnoceni, varianta a predchozi volba.
// Vzniklo v kroku R1 presunem z GameScreen.kt.

/**
 * Popup se zaznamem partie vlevo a hloubkou, hodnocenim, hlavni variantou
 * a predchozi volbou (krok U14/C) vpravo. Rozvrzeni od kroku U14/A podle predlohy
 * `docs/navrhy/u14-okno-i.png`.
 *
 * Ne AlertDialog: ten drzi text v jednom sloupci a dva vedle sebe se do nej
 * nevejdou. Dialog + Surface s vypnutou `usePlatformDefaultWidth` da sirku,
 * ve ktere ma zaznam i varianta misto.
 *
 * Levy sloupec je jen tak siroky, jak potrebuje zaznam (sirky bunek se meri
 * z textu, viz [ZaznamRadek]), pravy dostane zbytek.
 *
 * Vyska (krok U14/A): vsechno nad tlacitkem Zavrit se svisle posouva, takze
 * se okno vejde i na maly displej a pri vetsim pismu a Zavrit je videt
 * vzdycky. Zaznam ma uvnitr vlastni LazyColumn s vyskou nejvys
 * [VYSKA_ZAZNAMU] - LazyColumn uvnitr verticalScroll musi mit konecnou
 * vysku, jinak spadne, a odscrollovani na aktivni tah potrebuje okno,
 * ve kterem se da posouvat.
 *
 * Zavira se klepnutim vedle (onDismissRequest) i tlacitkem.
 */
@Composable
internal fun ZaznamDialog(
    radky: List<RadekZaznamu>,
    aktivniIndex: Int,
    /** Nazev zahajeni nahore pres celou sirku (krok K1/B); null = "—". */
    zahajeni: Zahajeni?,
    /** Hloubka uz jako text: cislo, "kniha" (K1/B) nebo "—", viz [textHloubky]. */
    hloubka: String,
    varianta: List<String>,
    predchoziVolba: PredchoziVolba?,
    skore: Int?,
    onSkok: (Int) -> Unit,
    onDismiss: () -> Unit,
) {
    val texty = LocalTexty.current
    // Radek, na kterem uzivatel prave stoji - jen kvuli odscrollovani.
    // Zvyraznuje se uz konkretni pultah, ne cely radek.
    val aktivniRadek = radky.indexOfFirst {
        it.bily?.historyIndex == aktivniIndex || it.cerny?.historyIndex == aktivniIndex
    }
    val stavSeznamu = rememberLazyListState()
    LaunchedEffect(aktivniRadek, radky.size) {
        if (aktivniRadek >= 0) stavSeznamu.animateScrollToItem(aktivniRadek)
    }

    // Sirky bunek zaznamu z textu ve stejnem stylu, jakym se kresli: cislo
    // je siroke jako "999.", tah jako sedm znaku ("exd8=Q+") plus
    // vodorovne odsazeni bunky z obou stran.
    val stylZaznamu = LocalTextStyle.current.merge(STYL_ZAZNAMU)
    val mereni = rememberTextMeasurer()
    val hustota = LocalDensity.current
    val sirkaCisla = with(hustota) { mereni.measure("999.", stylZaznamu).size.width.toDp() }
    val sirkaTahu = with(hustota) { mereni.measure("exd8=Q+", stylZaznamu).size.width.toDp() } +
        ODSAZENI_TAHU * 2

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false),
    ) {
        Surface(
            shape = RoundedCornerShape(16.dp),
            color = MaterialTheme.colorScheme.surface,
            tonalElevation = 6.dp,
            modifier = Modifier.fillMaxWidth(0.92f),
        ) {
            Column(modifier = Modifier.padding(16.dp)) {
                // Vsechno krome Zavrit. weight(fill = false): obsah si vezme
                // jen tolik, kolik potrebuje, a kdyz se nevejde, posouva se
                // a Zavrit pod nim zustane.
                Column(
                    modifier = Modifier
                        .weight(1f, fill = false)
                        .verticalScroll(rememberScrollState()),
                ) {
                    RadekZahajeni(zahajeni)
                    Spacer(Modifier.height(10.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(12.dp),
                    ) {
                        // --- vlevo zaznam partie ---
                        //
                        // LazyColumn, ne Column: partie o dvaašedesáti
                        // radcich by se cela skladala pri kazdem prekresleni
                        // a odscrollovat se na aktivni radek nedalo.
                        Column(modifier = Modifier.width(sirkaCisla + sirkaTahu * 2)) {
                            DialogNadpis(texty.zaznamPartie)
                            if (radky.isEmpty()) {
                                Text(
                                    texty.zatimZadnyTah,
                                    fontSize = 13.sp,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                            LazyColumn(
                                state = stavSeznamu,
                                modifier = Modifier.heightIn(max = VYSKA_ZAZNAMU),
                            ) {
                                items(radky) { radek ->
                                    ZaznamRadek(
                                        radek = radek,
                                        aktivniIndex = aktivniIndex,
                                        styl = stylZaznamu,
                                        sirkaCisla = sirkaCisla,
                                        sirkaTahu = sirkaTahu,
                                        onSkok = onSkok,
                                    )
                                }
                            }
                        }

                        // --- vpravo hloubka, hodnoceni a varianta ---
                        Column(modifier = Modifier.weight(1f)) {
                            HloubkaAHodnoceni(
                                hloubka = hloubka,
                                hodnoceni = formatujSkoreKratce(skore, texty),
                            )
                            Spacer(Modifier.height(10.dp))
                            DialogNadpis(texty.varianta)
                            // Tah na radek, ne jeden dlouhy retezec: souradnicove
                            // tahy se v uzkem sloupci lamou uprostred.
                            if (varianta.isEmpty()) {
                                Text("—", fontSize = 13.sp)
                            }
                            varianta.forEach { tah ->
                                Text(tah, fontSize = 13.sp, lineHeight = 18.sp)
                            }
                            // Krok U14/C; pro null se oddil nevykresli.
                            predchoziVolba?.let { volba ->
                                Spacer(Modifier.height(10.dp))
                                DialogNadpis(texty.predchoziVolba)
                                Text(volba.san, fontSize = 13.sp, lineHeight = 18.sp)
                                Text(
                                    texty.doHloubky(volba.doHloubky),
                                    fontSize = 12.sp,
                                    lineHeight = 16.sp,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                                Text(
                                    textZmenyPo(volba.poMs, texty),
                                    fontSize = 12.sp,
                                    lineHeight = 16.sp,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                        }
                    }

                    // Krok U6/B: co dela tlacitko ⟳.
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    Poznamka(texty.poznamkaObnoveni)

                    // Krok U5, od U14/A s tabulkou motoru, viz [TabulkaElo].
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    Poznamka(texty.poznamkaElo)
                    Spacer(Modifier.height(6.dp))
                    TabulkaElo()
                    Spacer(Modifier.height(6.dp))
                    Poznamka(texty.poznamkaSlabsiStockfish)

                    // Krok U12/B: jak cist hodnoceni a +M3.
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    Poznamka(texty.poznamkaHodnoceni)
                    // Krok U14/C.
                    Spacer(Modifier.height(6.dp))
                    Poznamka(texty.poznamkaPredchoziVolba)

                    // Krok P1: zasady ochrany soukromi (Google Play).
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    Poznamka(texty.poznamkaSoukromi)
                }

                Spacer(Modifier.height(8.dp))

                TextButton(
                    onClick = onDismiss,
                    modifier = Modifier.align(Alignment.End),
                ) {
                    Text(texty.zavrit, fontSize = 13.sp)
                }
            }
        }
    }
}

/** Nejvetsi vyska seznamu tahu v okne (i). Do kroku U14/A 320 dp na cely radek. */
private val VYSKA_ZAZNAMU = 260.dp

/** Vodorovne odsazeni bunky s tahem z kazde strany. Do kroku U14/A 6 dp. */
private val ODSAZENI_TAHU = 4.dp

/** Pismo zaznamu: cislo i tahy. Podle nej se meri i sirky bunek. */
private val STYL_ZAZNAMU = TextStyle(fontSize = 13.sp, fontFamily = FontFamily.Monospace)

/**
 * Hloubka a Hodnoceni jako dve radky mrizky (krok U14/A): popisky ve stylu
 * [DialogNadpis], hodnoty zarovnane doprava pod sebou ve spolecnem sloupci
 * hned za delsim popiskem (mezera [MEZERA_HODNOTY]), ne u praveho okraje
 * okna.
 *
 * Sirky obou sloupcu se meri z textu, takze radky sedi pod sebou; popisek
 * a hodnota se v radku rovnaji na uctarnu.
 *
 * Krok U14/D: kdyz se popisek, mezera a hodnota do sirky praveho sloupce
 * nevejdou ([vejdeSeVedle]) - pri vetsim pismu roste i levy sloupec se
 * zaznamem a na pravy zbude malo -, kresli se obe polozky pod sebou jako
 * pred U14: popisek a pod nim hodnota. Hodnota se nikdy nezalamuje.
 */
@Composable
private fun HloubkaAHodnoceni(hloubka: String, hodnoceni: String) {
    val texty = LocalTexty.current
    val stylPopisku = LocalTextStyle.current.merge(
        TextStyle(fontSize = 12.sp, fontWeight = FontWeight.Medium),
    )
    val stylHodnoty = LocalTextStyle.current.merge(TextStyle(fontSize = 13.sp))
    val mereni = rememberTextMeasurer()
    val hustota = LocalDensity.current
    val radky = listOf(texty.hloubka to hloubka, texty.hodnoceni to hodnoceni)
    val popisekPx = radky.maxOf { mereni.measure(it.first, stylPopisku).size.width }
    val hodnotaPx = radky.maxOf { mereni.measure(it.second, stylHodnoty).size.width }
    val sirkaPopisku = with(hustota) { popisekPx.toDp() }
    val sirkaHodnoty = with(hustota) { hodnotaPx.toDp() }
    BoxWithConstraints {
        val vedle = vejdeSeVedle(
            sirkaPopisku = popisekPx,
            mezera = with(hustota) { MEZERA_HODNOTY.roundToPx() },
            sirkaHodnoty = hodnotaPx,
            sirkaSloupce = constraints.maxWidth,
        )
        Column {
            radky.forEachIndexed { i, (popisek, hodnota) ->
                if (vedle) {
                    Row(modifier = Modifier.padding(bottom = 4.dp)) {
                        Text(
                            text = popisek,
                            style = stylPopisku,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            modifier = Modifier.width(sirkaPopisku).alignByBaseline(),
                        )
                        Spacer(Modifier.width(MEZERA_HODNOTY))
                        Text(
                            text = hodnota,
                            style = stylHodnoty,
                            textAlign = TextAlign.End,
                            maxLines = 1,
                            softWrap = false,
                            modifier = Modifier.width(sirkaHodnoty).alignByBaseline(),
                        )
                    }
                } else {
                    if (i > 0) Spacer(Modifier.height(10.dp))
                    DialogNadpis(popisek)
                    Text(hodnota, style = stylHodnoty, maxLines = 1, softWrap = false)
                }
            }
        }
    }
}

/** Mezera mezi popiskem a hodnotou v mrizce Hloubka/Hodnoceni. */
private val MEZERA_HODNOTY = 16.dp

/**
 * Vejde se popisek, mezera a hodnota vedle sebe do sloupce (krok U14/D)?
 * Vse v pixelech. Ciste, aby na hranici sel napsat JVM test.
 */
internal fun vejdeSeVedle(sirkaPopisku: Int, mezera: Int, sirkaHodnoty: Int, sirkaSloupce: Int): Boolean =
    sirkaPopisku + mezera + sirkaHodnoty <= sirkaSloupce

/**
 * Elo motoru do okna (i) (krok U14/A). Prvni radek je hlavicka.
 *
 * Zdroje cisel:
 *  - Light Red, RedMAX a Crafty: docs/MERENI-ELO.md, oddil Souhrn (E1/D,
 *    E1/C; Crafty je hruby prepocet E1 - 214, nemereny);
 *  - Stockfish: app/src/main/cpp/stockfish/search.h r. 251, "Skill 0 .. 19
 *    now covers CCRL Blitz Elo from 1320 to 3190, approximately";
 *  - veta pod tabulkou o 200 slabsim Stockfishi: docs/MERENI-ELO.md, E1/C
 *    (zkraceni hledani ho oslabuje asi o 214 Elo pri N = 2650).
 */
private fun tabulkaElo(texty: Texty) = listOf(
    listOf(texty.eloMotor, texty.eloElo, texty.eloMereni),
    listOf("LightRed", "1569 ± 65", texty.eloZmereno),
    listOf("RedMax", "2415 ± 113", texty.eloZmereno),
    listOf("Crafty", "≈ 2600", texty.eloPrepocet),
    listOf("Stockfish", "1320–3190", texty.eloUdajAutoru),
)

/**
 * Tabulka [tabulkaElo]. Od kroku J2 radek po radku: kazdy radek je Row
 * s pevnymi sirkami prvnich dvou sloupcu, zmerenymi z nejsirsi bunky
 * v temz stylu (vzor: sirky zaznamu z U14/A), posledni sloupec bere zbytek
 * sirky. Zalomi-li se bunka (anglictina s vetsim pismem), zvysi se cely
 * radek a bunky zustanou vedle sebe. Do J2 to byly tri Column vedle sebe
 * a zalomena bunka posunula treti sloupec proti radkum.
 * Tucna je jen hlavicka, cislice jsou tabularni ("tnum").
 */
@Composable
private fun TabulkaElo() {
    val tabulka = tabulkaElo(LocalTexty.current)
    val styl = LocalTextStyle.current.merge(
        TextStyle(fontSize = 12.sp, lineHeight = 16.sp, fontFeatureSettings = "tnum"),
    )
    fun vaha(i: Int) = if (i == 0) FontWeight.Medium else FontWeight.Normal
    val mereni = rememberTextMeasurer()
    val hustota = LocalDensity.current
    // Sirky vsech sloupcu krome posledniho, z nejsirsi bunky.
    val sirky = (0 until tabulka.first().size - 1).map { sloupec ->
        val px = tabulka.indices.maxOf { i ->
            mereni.measure(tabulka[i][sloupec], styl.merge(TextStyle(fontWeight = vaha(i)))).size.width
        }
        with(hustota) { px.toDp() }
    }
    Column {
        tabulka.forEachIndexed { i, radek ->
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                radek.forEachIndexed { sloupec, text ->
                    Text(
                        text = text,
                        style = styl,
                        fontWeight = vaha(i),
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = if (sloupec < sirky.size) Modifier.width(sirky[sloupec]) else Modifier.weight(1f),
                    )
                }
            }
        }
    }
}

/** Vysvetlujici text dole v okne (i). */
@Composable
private fun Poznamka(text: String) {
    Text(
        text,
        fontSize = 12.sp,
        lineHeight = 16.sp,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
    )
}

/**
 * Jeden cislovany radek zaznamu. Cislo je obycejny text, oba pultahy jsou
 * samostatna klikatelna pole - proto se nedaji slozit do jednoho Textu.
 *
 * Tri bunky maji pevnou sirku, ne sirku podle obsahu: jinak by cerny tah
 * zacinal pokazde jinde, protoze cislo je jinak siroke u jednociferneho nez
 * u dvojciferneho a bily tah jinak u "e4" nez u "N5xe6+". Od kroku U14/A se
 * sirky meri z textu ve stejnem stylu ([sirkaCisla] = "999.", [sirkaTahu] =
 * sedm znaku a odsazeni), do te doby to byly podily sirky leveho sloupce.
 *
 * Monospace srovna i pismena uvnitr bunky, takze notace stoji pod sebou i
 * uprostred sloupce.
 */
@Composable
private fun ZaznamRadek(
    radek: RadekZaznamu,
    aktivniIndex: Int,
    styl: TextStyle,
    sirkaCisla: Dp,
    sirkaTahu: Dp,
    onSkok: (Int) -> Unit,
) {
    Row(
        modifier = Modifier.padding(vertical = 1.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            // "12." kdyz radek zacina bilym, "12..." kdyz partie zacala cernym.
            text = radek.cislo.toString() + if (radek.bily == null) "..." else ".",
            modifier = Modifier.width(sirkaCisla),
            textAlign = TextAlign.End,
            style = styl,
            softWrap = false,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
        PulTahBunka(radek.bily, aktivniIndex, styl, onSkok, Modifier.width(sirkaTahu))
        PulTahBunka(radek.cerny, aktivniIndex, styl, onSkok, Modifier.width(sirkaTahu))
    }
}

/**
 * Jeden pultah. Tucne je prave ten, na jehoz pozici uzivatel stoji - ne cely
 * radek: po klepnuti na cerny tah by jinak nebylo poznat, kde se stoji.
 *
 * Klikatelnost i odsazeni sedi na cele bunce, ne na samotnem pismu: dotykova
 * plocha je tim vetsi a stejna u "e4" i u "Qxh8=Q#".
 */
@Composable
private fun PulTahBunka(
    pulTah: PulTah?,
    aktivniIndex: Int,
    styl: TextStyle,
    onSkok: (Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    if (pulTah == null) {
        // Chybejici bily tah (partie zacala cernym) musi misto nechat prazdne,
        // ne ho srazit - jinak by se prvni radek rozjel proti ostatnim.
        Box(modifier)
        return
    }
    Text(
        text = pulTah.san,
        modifier = modifier
            .clickable { onSkok(pulTah.historyIndex) }
            .padding(horizontal = ODSAZENI_TAHU, vertical = 2.dp),
        style = styl,
        softWrap = false,
        fontWeight = if (pulTah.historyIndex == aktivniIndex) FontWeight.Bold
                     else FontWeight.Normal,
    )
}

@Composable
private fun DialogNadpis(text: String, modifier: Modifier = Modifier.padding(bottom = 4.dp)) {
    Text(
        text = text,
        fontSize = 12.sp,
        fontWeight = FontWeight.Medium,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
        modifier = modifier,
    )
}

/**
 * Text hloubky pro dlazdici i okno (i): dosazena hloubka, po tahu
 * z knihovny "kniha" (krok K1/B, rozhodnuti B), jinak "—". Slovo "kniha"
 * je od kroku J1 v tabulce textu ([Texty.hloubkaKniha]).
 */
internal fun textHloubky(hloubka: Int?, zKnihy: Boolean, texty: Texty = TextyCz): String = when {
    zKnihy -> texty.hloubkaKniha
    hloubka != null -> hloubka.toString()
    else -> "—"
}

/**
 * Radek "Zahajeni" nahore v okne (i) pres celou sirku (krok K1/B, predloha
 * docs/navrhy/k1-kniha.png): popisek ve stylu [DialogNadpis], vedle ECO
 * a nazev. Nazev ma zbytek sirky, takze se dlouhy zalomi pod sebe, ne pod
 * ECO. Bez nazvu "—".
 */
@Composable
private fun RadekZahajeni(zahajeni: Zahajeni?) {
    Row(modifier = Modifier.fillMaxWidth()) {
        DialogNadpis(LocalTexty.current.zahajeni, Modifier.alignByBaseline())
        Spacer(Modifier.width(24.dp))
        if (zahajeni == null) {
            Text("—", fontSize = 13.sp, modifier = Modifier.alignByBaseline())
            return@Row
        }
        Text(zahajeni.eco, fontSize = 13.sp, modifier = Modifier.alignByBaseline())
        Spacer(Modifier.width(8.dp))
        Text(
            zahajeni.nazev,
            fontSize = 13.sp,
            lineHeight = 18.sp,
            modifier = Modifier.weight(1f).alignByBaseline(),
        )
    }
}
