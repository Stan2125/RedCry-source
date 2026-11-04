package cz.redcry.chess.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowDropDown
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import cz.redcry.chess.engine.EngineFactory
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.texty.LocalTexty

// Ovladani motoru: vyber motoru, sila Elo, meze hledani a jejich zadani cislem.
// Vzniklo v kroku R1 presunem z GameScreen.kt.

/**
 * Levy sloupec ovladani, druhy radek: popisek Engine a vedle nej vyber
 * motoru. Vyber ukazuje jen jmeno motoru - Elo ma od kroku U5 vlastni
 * radek, ktery je videt porad.
 *
 * Krok U15: popisek a vyber jsou v jednom radku (driv dva radky 50 px
 * s mezerami 30 a 46), nad nim pribyl [RadekJazyka]. Radek konci na
 * [Navrh.KRAJ_OVLADANI], tedy na pravem kraji (i), a vyber je u nej
 * zarovnany zprava - viditelna spicka sipky lezi presne na tom kraji.
 *
 * Zamcena i odemcena varianta maji tutez vysku i sirku, takze prechod po
 * prvnim tahu nic neposune - meni se jen sipka.
 */
@Composable
internal fun EngineBlok(
    n: Navrh,
    engineName: String,
    selectable: Boolean,
    onEngineSelected: (EngineId) -> Unit,
) {
    var expanded by remember { mutableStateOf(false) }

    // engineLabel: x = 68, vyska radku jako Plna sila.
    Row(
        modifier = Modifier
            .width(n.px(Navrh.KRAJ_OVLADANI))
            .height(n.px(Navrh.RADEK_OVLADANI))
            .padding(start = n.px(24)),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(LocalTexty.current.engine, fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)

        // Klikatelna je cela vyska radku, ne jen text.
        Box(
            modifier = Modifier
                .fillMaxHeight()
                .clickable(enabled = selectable) { expanded = true },
            contentAlignment = Alignment.CenterEnd,
        ) {
            // ArrowDropDown je trojuhelnik M7 10l5 5 5-5z v ramecku 24 x 24,
            // vpravo od nej je tedy pruhledny okraj 7/24 velikosti ikony.
            // O ten se posune cela skupina jmeno + sipka, aby na kraji radku
            // (pravy kraj (i)) lezela viditelna spicka, ne pruhledny okraj.
            // Posunout jen ikonu nestaci (krok U15/B): offset meni jen
            // kresleni, ne misto v rozvrzeni, takze jmeno zustalo na miste
            // a mezera jmeno-sipka se zvetsila (oprava v kroku U15/D).
            // Posun plati i za hledani, misto sipky je stejne velke, takze
            // se jmeno pri zamceni nepohne.
            Row(
                modifier = Modifier.offset(x = 16.dp * 7 / 24),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = engineName,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onSurface,
                    maxLines = 1,
                )
                Spacer(Modifier.width(4.dp))
                if (selectable) {
                    Icon(
                        imageVector = Icons.Default.ArrowDropDown,
                        contentDescription = null,
                        modifier = Modifier.size(16.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                } else {
                    // Stejne misto jako sipka: jmeno se nepohne.
                    Spacer(Modifier.size(16.dp))
                }
            }

            DropdownMenu(
                expanded = selectable && expanded,
                onDismissRequest = { expanded = false },
            ) {
                // Vychozi Material styl je pro tenhle radek moc velky,
                // proto stejny bodyMedium (13sp) jako popisek vedle.
                val itemStyle = MaterialTheme.typography.bodyMedium
                // Nedostupne motory se ukazuji zesednute, ne skryte -
                // at je videt, co se chysta, a neni to prekvapeni.
                EngineFactory.vse().forEach { id ->
                    DropdownMenuItem(
                        text = { Text(id.label, style = itemStyle) },
                        enabled = id.dostupny,
                        onClick = {
                            expanded = false
                            onEngineSelected(id)
                        },
                    )
                }
            }
        }
    }
}

/**
 * Sila motoru podle Elo (krok S6), jen u motoru s [EngineId.minElo]:
 * prepinac Plna sila a pod nim Elo. Elo se zadava klepnutim na cislo
 * stejnym dialogem jako cas, stepper tu neni - rozsah 1320..3190 by se po
 * krocich proklepaval dlouho.
 *
 * Za plne sily se radek Sila ELO nevykresli vubec (krok U12/A, nahrazuje
 * zesednuti z kroku U5): zesednute Elo za plne sily uzivatele matlo. Box
 * radku s pevnou velikosti ale zustava prazdny, takze se rozvrzeni pod nim
 * pri prepinani neposouva. Po vypnuti Plne sily ViewModel vrati naposledy
 * nastavene Elo a radek ho ukaze.
 *
 * Krok U15: oba radky konci na [Navrh.KRAJ_OVLADANI] (pravy kraj (i))
 * a prepinac je k nemu zarovnany zprava, stejne jako vyber motoru nad nimi.
 *
 * Krok U16 rusi zarovnani cisla Elo zprava z kroku U15 (rozhodnuti
 * uzivatele podle repliky): cislo je zase vystredene na svislou osu
 * prepinace jako od kroku U6/A, a proto se zase meri sirka prepinace.
 *
 * Zamceny je ve stejnych situacich jako [MezeSloupec]: za hledani by zmena
 * platila az pro pristi tah.
 */
@Composable
internal fun SilaBlok(
    n: Navrh,
    elo: Int?,
    odemceno: Boolean,
    onPlnaSila: (Boolean) -> Unit,
    onZadat: () -> Unit,
) {
    // fullPowerLabel x = 68, vyska 88.
    // Sirka prepinace se meri: radek Sila ELO pod nim podle ni stredi cislo.
    val hustota = LocalDensity.current
    var sirkaPrepinace by remember { mutableStateOf(52.dp) }
    Row(
        modifier = Modifier
            .width(n.px(Navrh.KRAJ_OVLADANI))
            .height(n.px(Navrh.RADEK_OVLADANI))
            .padding(start = n.px(24)),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(LocalTexty.current.plnaSila, fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Switch(
            checked = elo == null,
            onCheckedChange = onPlnaSila,
            enabled = odemceno,
            modifier = Modifier.onSizeChanged { sirkaPrepinace = with(hustota) { it.width.toDp() } },
        )
    }
    // Krok U6/A: o pul radku vys nez v navrhu, blize k Plne sile.
    Spacer(Modifier.height(n.px(Navrh.MEZERA_ELO)))

    // eloLabel x = 68, vyska 58. Radek je stejne siroky jako radek prepinace
    // a cislo stoji v plose sirkaCisla, ktera je vystredena na svislou osu
    // prepinace: jeji konec je o (sirkaCisla - sirkaPrepinace) / 2 za koncem
    // radku. Text je v plose vystredeny, takze stred cisla = stred prepinace
    // pri jakemkoli poctu cislic (krok U6/A, znovu od kroku U16).
    val sedy = MaterialTheme.colorScheme.onSurfaceVariant
    val sirkaCisla = n.px(150)
    Box(
        modifier = Modifier
            .width(n.px(Navrh.KRAJ_OVLADANI))
            .height(n.px(Navrh.RADEK_ELO))
            .padding(start = n.px(24)),
    ) {
        // Krok U12/A: za plne sily prazdny Box, jen drzi misto.
        if (elo == null) return@Box
        Text(
            LocalTexty.current.silaElo,
            fontSize = 13.sp,
            color = sedy,
            modifier = Modifier.align(Alignment.CenterStart),
        )
        // Plocha pro klepnuti je cela vyska radku, ne odsazeni kolem textu:
        // radek ma jen 58 px a odsazeny text by se v nem usekl.
        Box(
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .offset(x = (sirkaCisla - sirkaPrepinace) / 2)
                .width(sirkaCisla)
                .fillMaxHeight()
                .clickable(enabled = odemceno, onClick = onZadat),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = elo.toString(),
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium,
                color = if (odemceno) MaterialTheme.colorScheme.onSurface else sedy,
                maxLines = 1,
            )
        }
    }
}

/**
 * Pravy sloupec ovladani: Max hloubka a pod ni Max sekund, kazda s popiskem
 * nad stepperem. Hledani skonci na te z mezi, ktera prijde driv.
 *
 * Tlacitka zustavaji klikatelna i na kraji rozsahu: mez si nese motor sam
 * (hloubka) nebo RedCry (sekundy) a ViewModel na kraji rekne, jaka je.
 * Zamceny sloupec je jen ten za beziciho nebo dobihajiciho hledani.
 *
 * Dlazdice "Hloubka" nahore ukazuje DOSAZENOU hloubku, tenhle sloupec
 * NASTAVENOU mez. Proto se popisek jmenuje "Max hloubka" a ne "Hloubka".
 */
@Composable
internal fun MezeSloupec(
    n: Navrh,
    maxHloubka: Int,
    maxSekund: Int,
    odemceno: Boolean,
    onHloubkaZmena: (Int) -> Unit,
    onSekundyZmena: (Int) -> Unit,
    onZadat: (ZadavanaMez) -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier) {
        // depthLabel y = 1269 (9 pod hornim krajem bloku), stepper y = 1325.
        // Krok U15: blok zacina o JAZYK_NAVIC vys, sloupec zustava na miste.
        Spacer(Modifier.height(n.px(9 + Navrh.JAZYK_NAVIC)))
        MezPopisek(n, LocalTexty.current.maxHloubka)
        Spacer(Modifier.height(n.px(16)))
        MezStepper(
            n = n,
            hodnota = maxHloubka.toString(),
            odemceno = odemceno,
            onZmena = onHloubkaZmena,
            onKlepnuti = { onZadat(ZadavanaMez.HLOUBKA) },
        )
        // secondsLabel y = 1427, stepper y = 1478.
        Spacer(Modifier.height(n.px(34)))
        MezPopisek(n, LocalTexty.current.maxSekund)
        Spacer(Modifier.height(n.px(11)))
        MezStepper(
            n = n,
            // Nula je "bez omezeni". Znak nekonecna rekne totez a vejde se
            // do tehoz mista jako dvojciferne cislo.
            hodnota = if (maxSekund == EngineId.BEZ_OMEZENI) "∞" else maxSekund.toString(),
            odemceno = odemceno,
            onZmena = onSekundyZmena,
            onKlepnuti = { onZadat(ZadavanaMez.SEKUNDY) },
        )
    }
}

@Composable
private fun MezPopisek(n: Navrh, text: String) {
    Box(modifier = Modifier.height(n.px(40)), contentAlignment = Alignment.CenterStart) {
        Text(
            text,
            fontSize = 13.sp,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            maxLines = 1,
        )
    }
}

/** Minus, cislo a plus: 96 + 10 + 88 + 10 + 96 px, vyska 68 px. */
@Composable
private fun MezStepper(
    n: Navrh,
    hodnota: String,
    odemceno: Boolean,
    onZmena: (Int) -> Unit,
    onKlepnuti: () -> Unit,
) {
    Row(
        modifier = Modifier.height(n.px(68)),
        horizontalArrangement = Arrangement.spacedBy(n.px(10)),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        MezButton(n, "−", odemceno) { onZmena(-1) }
        // Klepnuti na cislo otevre dialog s textovym polem - k 900 by
        // se jinak po krocich klepalo dlouho. Zamcene je ve stejnych
        // situacich jako tlacitka vedle.
        // Pevna sirka, aby prechod z jednociferne na trojcifernou
        // neposunul tlacitka. Zaroven je to plocha pro klepnuti, pres celou
        // vysku stepperu.
        Box(
            modifier = Modifier
                .width(n.px(88))
                .fillMaxHeight()
                .clickable(enabled = odemceno, onClick = onKlepnuti),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = hodnota,
                fontSize = 13.sp,
                fontWeight = FontWeight.Bold,
                color = if (odemceno) MaterialTheme.colorScheme.onSurface
                        else MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
        MezButton(n, "+", odemceno) { onZmena(+1) }
    }
}

/** Ktera mez se zadava cislem v [ZadaniMezeDialog]. ELO od kroku S6. */
internal enum class ZadavanaMez { HLOUBKA, SEKUNDY, ELO }

/**
 * Zadani meze cislem. Text se predava ViewModelu tak, jak je - orez do mezi
 * a hlasky o nem dela on, at na to jde napsat JVM test. Dialog se po
 * potvrzeni zavre vzdycky: hlaska o orezu nebo o necislu jde do Snackbaru,
 * ktery by pod otevrenym dialogem nebyl videt.
 *
 * Pole zacina s aktualni hodnotou oznacenou celou, takze prvni cislice ji
 * prepise a Hotovo bez psani nic nezmeni.
 */
@Composable
internal fun ZadaniMezeDialog(
    nadpis: String,
    rozsah: String,
    hodnota: Int,
    onPotvrdit: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    val texty = LocalTexty.current
    val puvodni = hodnota.toString()
    var pole by remember {
        mutableStateOf(TextFieldValue(puvodni, selection = TextRange(0, puvodni.length)))
    }
    val fokus = remember { FocusRequester() }
    LaunchedEffect(Unit) { fokus.requestFocus() }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(nadpis) },
        text = {
            OutlinedTextField(
                value = pole,
                onValueChange = { pole = it },
                label = { Text(rozsah) },
                singleLine = true,
                keyboardOptions = KeyboardOptions(
                    keyboardType = KeyboardType.Number,
                    imeAction = ImeAction.Done,
                ),
                keyboardActions = KeyboardActions(onDone = { onPotvrdit(pole.text) }),
                modifier = Modifier.focusRequester(fokus),
            )
        },
        confirmButton = {
            TextButton(onClick = { onPotvrdit(pole.text) }) { Text(texty.nastavit) }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(texty.zrusit) }
        },
    )
}

@Composable
private fun MezButton(n: Navrh, znak: String, odemceno: Boolean, onClick: () -> Unit) {
    OutlinedButton(
        onClick = onClick,
        enabled = odemceno,
        // depthMinus, depthPlus: 96 x 68 px.
        modifier = Modifier.size(width = n.px(96), height = n.px(68)),
        contentPadding = PaddingValues(0.dp),
    ) {
        Text(znak, fontSize = 15.sp, fontWeight = FontWeight.Medium)
    }
}
