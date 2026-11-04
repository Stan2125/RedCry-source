package cz.redcry.chess.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Snackbar
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import cz.redcry.chess.analyza.Cislovani
import cz.redcry.chess.analyza.DuvodPauzy
import cz.redcry.chess.analyza.MAX_SEKUND_ANALYZY
import cz.redcry.chess.analyza.MIN_SEKUND_ANALYZY
import cz.redcry.chess.analyza.OknoAnalyzy
import cz.redcry.chess.analyza.VYCHOZI_SEKUNDY_ANALYZY
import cz.redcry.chess.analyza.odhadText
import cz.redcry.chess.analyza.rozsahPultahu
import cz.redcry.chess.engine.EngineFactory
import cz.redcry.chess.engine.EngineId
import cz.redcry.chess.texty.LocalTexty

/**
 * Obrazovkova cast analyzy partie (krok A1/B): okno pred spustenim, pruh
 * dole, dialog preruseni a platformni veci (zivotni cyklus, displej,
 * gesto Zpet). Rozhodovani je ve ViewModelu, tady jen napojeni.
 */

/**
 * Pruh dole za analyzy. Vypada jako hlaska (tentyz Snackbar), ale neni to
 * hlaska ze SnackbarHost: tu rusi klepnuti i dalsi hlaska (collectLatest
 * z U11), pruh zustava, dokud analyza bezi. Klepnuti otevre dialog
 * preruseni.
 */
@Composable
fun PruhAnalyzy(text: String, onKlepnuti: () -> Unit) {
    Snackbar(
        modifier = Modifier
            .padding(12.dp)
            .clickable(onClickLabel = LocalTexty.current.prerusitAnalyzu, onClick = onKlepnuti),
    ) {
        Text(text)
    }
}

@Composable
fun PrerusitAnalyzuDialog(onPrerusit: () -> Unit, onPokracovat: () -> Unit) {
    val texty = LocalTexty.current
    AlertDialog(
        onDismissRequest = onPokracovat,
        title = { Text(texty.prerusitAnalyzuDotaz) },
        text = { Text(texty.prerusitAnalyzuText) },
        confirmButton = { TextButton(onClick = onPrerusit) { Text(texty.prerusit) } },
        dismissButton = { TextButton(onClick = onPokracovat) { Text(texty.pokracovat) } },
    )
}

/**
 * Vsechno platformni kolem analyzy. Posluchace bezi porad, ViewModel je
 * bez bezici analyzy ignoruje.
 *
 * - ON_STOP / ON_START: pauza a pokracovani (analyza jen s otevrenou
 *   aplikaci - rozhodnuti uzivatele, misto systemove notifikace pruh).
 * - keepScreenOn jen po dobu analyzy.
 * - Teplo neresi: ochranu pred prehratim nechava na systemu (krok A1/D).
 * - Zpet za analyzy: dialog preruseni.
 */
@Composable
fun AnalyzaEfekty(viewModel: GameViewModel, bezi: Boolean) {
    val vlastnik = LocalLifecycleOwner.current
    DisposableEffect(vlastnik, viewModel) {
        val pozorovatel = LifecycleEventObserver { _, udalost ->
            when (udalost) {
                Lifecycle.Event.ON_STOP -> viewModel.analyza.onPozastav(DuvodPauzy.POZADI)
                Lifecycle.Event.ON_START -> viewModel.analyza.onPokracuj(DuvodPauzy.POZADI)
                else -> Unit
            }
        }
        vlastnik.lifecycle.addObserver(pozorovatel)
        onDispose { vlastnik.lifecycle.removeObserver(pozorovatel) }
    }

    val view = LocalView.current
    DisposableEffect(bezi) {
        view.keepScreenOn = bezi
        onDispose { view.keepScreenOn = false }
    }

    BackHandler(enabled = bezi) { viewModel.analyza.onPrerusDotaz() }
}

/** Parametry Spustit, ktere cekaji na vyber slozky. */
data class SpusteniAnalyzy(val motor: EngineId, val sekundy: Int, val odTahu: Int, val doTahu: Int)

/** Ktere cislo se v okne prave zadava klavesnici. */
private enum class PoleOkna { SEKUNDY, OD, DO }

/**
 * Okno "Analýza partie". Motor, cas a rozsah drzi okno samo, do ViewModelu
 * jdou az pri Spustit. U nedokoncene analyzy teze partie jsou prevzate
 * a needitovatelne.
 */
@Composable
fun OknoAnalyzyDialog(
    okno: OknoAnalyzy,
    cislovani: Cislovani,
    pocetPultahu: Int,
    onSpustit: (EngineId, Int, Int, Int) -> Unit,
    onPokracovat: () -> Unit,
    onZacitZnovu: () -> Unit,
    onZmenitSlozku: () -> Unit,
    onZavrit: () -> Unit,
) {
    val texty = LocalTexty.current
    var motor by rememberSaveable { mutableStateOf(EngineId.STOCKFISH) }
    var sekundy by rememberSaveable { mutableIntStateOf(VYCHOZI_SEKUNDY_ANALYZY) }
    var odTahu by rememberSaveable { mutableIntStateOf(okno.minTah) }
    var doTahu by rememberSaveable { mutableIntStateOf(okno.maxTah) }
    var zadavane by remember { mutableStateOf<PoleOkna?>(null) }
    val pokracovani = okno.pokracovani

    AlertDialog(
        onDismissRequest = onZavrit,
        title = { Text(texty.analyzaPartie) },
        text = {
            Column(
                modifier = Modifier.verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                if (pokracovani != null) {
                    Text(texty.nedokoncenaAnalyza)
                    Hodnota(texty.motor, pokracovani.motor.label)
                    Hodnota(texty.casNaPozici, texty.sekund(pokracovani.sekundy))
                    Hodnota(texty.tahy, "${pokracovani.odTahu}–${pokracovani.doTahu}")
                } else {
                    Text(texty.motor, fontWeight = FontWeight.Medium)
                    EngineFactory.dostupne().forEach { id ->
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            modifier = Modifier
                                .fillMaxWidth()
                                .selectable(selected = motor == id, role = Role.RadioButton) { motor = id },
                        ) {
                            RadioButton(selected = motor == id, onClick = null)
                            Spacer(Modifier.width(8.dp))
                            Text(id.label)
                        }
                    }
                    Krokovac(texty.casNaPoziciSekundy, sekundy, MIN_SEKUND_ANALYZY..MAX_SEKUND_ANALYZY,
                        { sekundy = it }, { zadavane = PoleOkna.SEKUNDY })
                    Krokovac(texty.odTahu, odTahu, okno.minTah..doTahu,
                        { odTahu = it }, { zadavane = PoleOkna.OD })
                    Krokovac(texty.doTahu, doTahu, odTahu..okno.maxTah,
                        { doTahu = it }, { zadavane = PoleOkna.DO })
                    HorizontalDivider()
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Column(Modifier.weight(1f)) {
                            Text(texty.slozkaProVysledek, fontWeight = FontWeight.Medium)
                            Text(okno.slozka ?: texty.slozkaNevybrana)
                        }
                        TextButton(onClick = onZmenitSlozku) { Text(texty.zmenitSlozku) }
                    }
                    Text(
                        texty.napovedaSlozky,
                        fontSize = 12.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                    HorizontalDivider()
                    val rozsah = rozsahPultahu(odTahu, doTahu, pocetPultahu, cislovani)
                    Text(rozsah?.let { odhadText(it.pocetPozic, sekundy, texty) } ?: texty.vRozsahuNeniTah)
                }
            }
        },
        confirmButton = {
            if (pokracovani != null) {
                TextButton(onClick = onPokracovat) { Text(texty.pokracovatOdTahu(pokracovani.odTahuPokracovani)) }
            } else {
                TextButton(onClick = { onSpustit(motor, sekundy, odTahu, doTahu) }) { Text(texty.spustit) }
            }
        },
        dismissButton = {
            Row {
                if (pokracovani != null) TextButton(onClick = onZacitZnovu) { Text(texty.zacitZnovu) }
                TextButton(onClick = onZavrit) { Text(texty.zrusit) }
            }
        },
    )

    zadavane?.let { pole ->
        val zavri = { zadavane = null }
        val (nadpis, meze, hodnota) = when (pole) {
            PoleOkna.SEKUNDY -> Triple(texty.casNaPozici, MIN_SEKUND_ANALYZY..MAX_SEKUND_ANALYZY, sekundy)
            PoleOkna.OD -> Triple(texty.odTahu, okno.minTah..doTahu, odTahu)
            PoleOkna.DO -> Triple(texty.doTahu, odTahu..okno.maxTah, doTahu)
        }
        ZadaniMezeDialog(
            nadpis = nadpis,
            rozsah = "" + meze.first + "–" + meze.last,
            hodnota = hodnota,
            onPotvrdit = { text ->
                // Necislo nebo prazdne pole nic nezmeni, cislo mimo meze
                // sedne na mez - v okne je videt hned, hlaska netreba.
                (ctiZadani(text, meze.first, meze.last) as? Zadani.Hodnota)?.let {
                    when (pole) {
                        PoleOkna.SEKUNDY -> sekundy = it.hodnota
                        PoleOkna.OD -> odTahu = it.hodnota
                        PoleOkna.DO -> doTahu = it.hodnota
                    }
                }
                zavri()
            },
            onDismiss = zavri,
        )
    }
}

@Composable
private fun Hodnota(popisek: String, hodnota: String) {
    Row {
        Text(popisek, modifier = Modifier.weight(1f))
        Text(hodnota, fontWeight = FontWeight.Medium)
    }
}

/** − cislo +, klepnuti na cislo otevre zadani klavesnici (jako u Max sekund). */
@Composable
private fun Krokovac(
    popisek: String,
    hodnota: Int,
    meze: IntRange,
    onZmena: (Int) -> Unit,
    onZadat: () -> Unit,
) {
    val zmena by rememberUpdatedState(onZmena)
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(popisek, modifier = Modifier.weight(1f))
        OutlinedButton(
            onClick = { zmena((hodnota - 1).coerceIn(meze)) },
            enabled = hodnota > meze.first,
            modifier = Modifier.size(40.dp),
            contentPadding = PaddingValues(0.dp),
        ) { Text("−") }
        Text(
            hodnota.toString(),
            fontWeight = FontWeight.Bold,
            modifier = Modifier
                .width(48.dp)
                .clickable(onClickLabel = LocalTexty.current.zadatCislo, onClick = onZadat),
            textAlign = TextAlign.Center,
        )
        OutlinedButton(
            onClick = { zmena((hodnota + 1).coerceIn(meze)) },
            enabled = hodnota < meze.last,
            modifier = Modifier.size(40.dp),
            contentPadding = PaddingValues(0.dp),
        ) { Text("+") }
    }
}
