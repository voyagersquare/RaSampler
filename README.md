# RaSampler

Windows x64 VST3 sampler instrument, by **Independent Audio**. Native C++17 / JUCE 8.0.6.

Jedan učitani sample, šest kartica, bez zona. Tamno metalno sučelje s narančastim kontrolama prema dogovorenim referentnim slikama. Svi gumbi i parametri donjih panela povezani su s instrumentom.

![RaSampler ADSR](docs/screenshots/ADSR.png)

Prikaz nove ADSR ovojnice s primjerom postavki Attack 60 ms, Decay 90 ms, Sustain 0.8 i Release 80 ms. Početne zvučne postavke instrumenta ostaju iste.

[Pregled svih šest kartica](docs/screenshots/) · [Novi LOOP markeri](docs/screenshots/Loop.png) · [Root Note C1–C4](docs/screenshots/Root.png) · [Sustain / Release / Link](docs/screenshots/LoopModes.png)

## Preuzimanje i instalacija

[**Preuzmi Windows installer RaSampler 0.6.0**](https://github.com/voyagersquare/RaSampler/actions/runs/43424842408/artifacts/14036781625) — paket **RaSampler-Windows-x64-Installer**. Raspakirajte ZIP i pokrenite **RaSampler-0.6.0-Windows-x64-Setup.exe**. Standardni Windows čarobnjak nudi VST3 instrument i opcionalnu samostalnu aplikaciju. Instalacija traži administratorska prava i podržava Windows 10/11 x64.

- **Proizvođač:** Independent Audio, u DAW-u, Windows svojstvima datoteka i popisu instaliranih aplikacija.
- **VST3:** `C:\Program Files\Common Files\VST3\RaSampler.vst3` — standardna mapa za skeniranje VST3 instrumenata.
- **Samostalna aplikacija:** `C:\Program Files\Independent Audio\RaSampler\RaSampler.exe`; izborni Start menu / desktop prečaci. Audio i MIDI postavke nalaze se u izborniku Options.
- **Uklanjanje:** Windows Settings → Apps → Installed apps → RaSampler → Uninstall. Korisnički preseti, sampleovi i DAW projekti ostaju sačuvani.

Prije instalacije zatvorite DAW i RaSampler. Nakon instalacije ponovno skenirajte VST3 instrumente. Sačuvani su izvorni VST3 identifikatori radi kompatibilnosti s postojećim projektima. Ako je prethodna verzija ručno kopirana u drugu mapu za skeniranje (uključujući `%LOCALAPPDATA%\Programs\Common\VST3` iz starije PowerShell instalacije), uklonite tu staru kopiju `RaSampler.vst3` kako biste izbjegli duplikate. Installer nadograđuje postojeću kopiju u standardnoj sistemskoj mapi.

Uz setup dolaze SHA-256 kontrolni zbroj i izvještaj provjere instalacije. Installer trenutačno nema Authenticode digitalni potpis. Zaseban **RaSampler-Windows-x64** ZIP i dalje sadrži VST3, samostalni EXE i slike svih kartica za ručnu instalaciju.

## Rad

Prozor se otvara na **75% (1170 × 555 px)**. Izbornik **Size** u donjoj traci nudi 60%, 75%, 90%, 100%, 125% i 150%. Veličinu možete mijenjati i povlačenjem donjeg desnog kuta, uz očuvanje proporcija. Nove instance i ponovno otvoreni editor počinju na 75%.

- Klik na naziv datoteke ili drag-and-drop učitava WAV, AIFF, FLAC, OGG ili MP3. Strelice pretražuju audio datoteke u istoj mapi. Maksimum je 128 MB i 10 minuta.
- Jedan sample svira preko klavijature. **Root Note** sada je kromatski odabir osnovne visine od **C1 (MIDI 36) do C4 (MIDI 72)**, ukupno 37 položaja. Redoslijed je C, C♯, D, D♯, E, F, F♯, G, G♯, A, A♯, B, zatim C sljedeće oktave. Kotačić miša mijenja jednu notu po koraku; moguće je upisati i enharmonski naziv poput D♭2/Db2. C3 (MIDI 60) je neutralna postavka: pri sviranju MIDI C3 reproducira izvorni sample bez transpozicije. Pomicanje Root Note naviše podiže zvuk po polutonovima, uključujući već aktivan glas i dodatni oscilator; MIDI tipke dodaju svoj interval u odnosu na C3. Kontrola ne analizira niti automatski prepoznaje temeljnu frekvenciju snimke. Cijela odabrana tipka, bijela ili crna, označena je žutom bojom; ostale odsvirane tipke označene su narančasto. Odabrani root automatski se dovodi u vidljivi dio klavijature. Starije vrijednosti izvan novog raspona pri učitavanju preseta ograničavaju se na C1/C4.
- Start, End, Loop, Reverse i Snap nalaze se na donjoj kartici **Sample**, bez duplikata u gornjem redu. Fade In, Fade Out i Gain ostaju dostupni uz waveform.
- Granice Start/End i Loop Start/End prikazane su u sampleovima. Povucite bijele markere za Start/End; Tirkizni LOOP IN / LOOP OUT markeri imaju debele linije i velike označene ručke; povucite ručku izravno ili koristite Shift-povlačenje u waveformu. Kratke petlje dobivaju dvije odvojene razine oznaka radi lakšeg odabira. Snap poravnava na lokalni tranzijent.
- **Sustain Mode / Release Mode / Link:** kartica Sample → LOOP. Sustain bira jednokratnu reprodukciju, ponavljanje naprijed ili ping-pong unutar LOOP IN/OUT. Release OFF nastavlja Sustain petlju tijekom amplitudnog ADSR otpuštanja. Release One-shot izlazi prema kraju sampla; Forward i Ping-pong zatim koriste zaseban početak **Release Loop** i kraj sampla. Release Loop možete upisati ili povlačiti ljubičasti REL LOOP marker na waveformu. **Link povezuje Sample Start s LOOP IN**, uz očuvanje izvornog Start položaja koji se vraća nakon isključivanja Linka. Kod Reverse reprodukcije smjerovi i rubovi su obrnuti; povezani početak reprodukcije je LOOP OUT. Release radi uz aktivnu Sustain petlju u Gate načinu i poštuje sustain pedalu; u Playback One-shot načinu red je zasivljen. Sve postavke spremaju se s projektom/presetom i vraćaju preko Global Reset. Stariji preseti zadržavaju prethodno ponašanje petlje.
- **ADSR na waveformu:** gumb ADSR uz naziv datoteke prikazuje bijelu amplitudnu ovojnicu s narančastim točkama A, D, S i R. A se povlači vodoravno za Attack; D vodoravno za Decay i okomito za Sustain; S okomito za Sustain; R (sredina silazne linije) vodoravno za Release — ulijevo za dulji release. Kontrole su povezane s amplitudnim ADSR-om na Filter/Global. Prikaz automatski prilagođava raspon da sve faze ostanu vidljive; navedeno trajanje je pregled ovojnice, ne položaj MIDI Note Off u sampleu. Vrlo kratke faze imaju minimalni vizualni razmak radi odabira. Shift-povlačenje i dalje uređuje petlju. Skriveni prikaz ne isključuje zvučnu ovojnicu. Parametri i vidljivost spremaju se s projektom/presetom.
- Gate slijedi Note Off i ADSR. One-shot ignorira obični Note Off; bez ponavljanja svira do završetka uzorka, a uz aktivnu ponavljajuću Sustain petlju do Panic/All Notes Off. Petlja koristi zasebne granice. Fade In/Out odnose se na rubove izrezanog uzorka.
- Effects otvara svih pet panela istovremeno. Kvačice uključuju efekte. Povucite ručku desno od naziva u Effects Chainu da promijenite redoslijed. Ploče efekata ostaju na stalnim mjestima.
- Pitch/Osc ima transpose, detune, pitch ADSR, dodatni oscilator i vremensku obradu. Oscilator dijeli amplitudnu ovojnicu i filter sa sampleom.
- **Time-stretch** radi offline WSOLA obradom u pozadinskoj niti, čuva visinu pri promjeni trajanja od 0.25× do 4× (najdulji obrađeni uzorak: 10 minuta). Nove note koriste obrađeni sample kad obrada završi; već pokrenuti glasovi završavaju sa svojom verzijom. To nije sustav beat/warp markera. Standardno MIDI transponiranje mijenja brzinu i trajanje reprodukcije.
- Filter je po glasu, prije efekata. LP/HP 12 i 24 dB, BP, Notch i PEQ. Resonance je Q; EQ Gain upravlja PEQ pojačanjem. Global Volume/Pan djeluju prije FX lanca. Gain uz sample podešava samo sample prije zbrajanja s oscilatorom.
- Modulation: 2 LFO-a, filterska ovojnica i 8 modulacijskih veza. Amp/Pitch ovojnice uređuju se na njihovim karticama. Sync koristi tempo DAW-a (120 BPM bez host tempa).
- MIDI: kanal, velocity krivulja, pitch bend, aftertouch, sustain i CC Learn. Za CC Learn odaberite parametar, kliknite Learn CC i pomaknite kontroler. Mapiranje parametra primjenjuje se preko message threada; nije namijenjeno audio-rate modulaciji. Standardni host automation dostupan je za parametre.
- Save/Load sprema `.rasampler` preset, uključujući izvorni audio. DAW state također ugrađuje audio, postavke i MIDI mapiranja. Vanjska datoteka nije potrebna pri ponovnom otvaranju projekta. **Global Reset** u donjoj traci vraća sve zvučne parametre i redoslijed efekata na tvorničke vrijednosti, briše MIDI mapiranja, zaustavlja glasove i repove efekata te vraća izvorno trajanje uzorka. Učitani sample i odabrana veličina prozora ostaju sačuvani. Panic prekida glasove i čisti repove efekata.

## Izgradnja

Windows: Visual Studio 2022, workload **Desktop development with C++**, CMake 3.22+, Git.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
build/RaSamplerCheck_artefacts/Release/RaSamplerCheck.exe --self-test
build/RaSamplerCheck_artefacts/Release/RaSamplerCheck.exe --screenshots build/screenshots
```

Za izradu standardnog installera instalirajte i **Inno Setup 6.7.4**, zatim pokrenite `Build-Installer.cmd`. Ako je Release build već izrađen: `powershell -NoProfile -ExecutionPolicy Bypass -File Installer/Build-Installer.ps1 -SkipBuild`. Setup i SHA-256 zapis nastaju u `build/installer/`. MSVC runtime ugrađen je statički, bez dodatne instalacije Visual C++ redistributable paketa.

GitHub Actions dodatno provjerava stvarnu instalaciju, identičnost instaliranih datoteka, proizvođača, Windows registraciju, VST3 validaciju iz instalacijske mape, pokretanje samostalne aplikacije, prečace, nadogradnju/popravak, VST3-only instalaciju i deinstalaciju uz očuvanje preseta. Svih 13 stavki instalacijskog izvještaja prolazi, uključujući očuvanje dodatnih korisničkih datoteka i čišćenje praznih mapa prethodno ručno kopiranog VST3 paketa. Testovi instalacije izvode se samo na privremenom Windows runneru.

CMake preuzima JUCE s fiksnog commita. Gotovi plugin: `build/RaSampler_artefacts/Release/VST3/RaSampler.vst3`. Izgradnja na Linuxu također je podržana za provjeru; potrebni su standardni JUCE X11, ALSA i font development paketi.

## Provjera i granice verzije

`RaSamplerCheck --self-test` provjerava audio izlaz, točan MIDI offset, Panic, reverse petlju, oscilator, filtere/efekte, redoslijed efekata, state restore, WSOLA trajanje, ugrađivanje audio datoteke i Note Off. `--screenshots <mapa>` renderira stvarni native editor za svih šest kartica. Verzija 0.6.0 prolazi svih 154 provjere, uključujući Sustain/Release transport, Reverse, Link, povlačenje Release Loop markera i kompatibilnost preseta, te Windows VST3 pluginval validaciju na razini 5. `--loop-modes-demo <mapa>` prikazuje nove kontrole i markere.

Trenutna verzija: 0.6.0. Parametri Reverba Size i Decay zajedno se preslikavaju na JUCE reverb; Decay nije kalibrirano RT60 mjerenje. Saturation i square/saw oscilatori nemaju oversampling/antialiasing. WSOLA može dati artefakte na velikim omjerima ili složenim uzorcima. Nema crossfadea petlje; za diskontinuitet na spoju trebaju odgovarajuće granice. Potrebna je dodatna provjera u stvarnim Windows DAW-ovima prije produkcijske uporabe.

Vidi `THIRD_PARTY.md` za uvjete JUCE licence prije distribucije.
