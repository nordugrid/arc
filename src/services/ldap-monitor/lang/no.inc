<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- Norwegian [no]
// -- Translation: Sigve Haug (sigve.haug@fys.uio.no)
// -- Author: oxana.smirnova@hep.lu.se

// Some common strings:

$clickable = "KAN KLIKKES";

$str_att = ", menneskelig lesbart foruten noen MDS-spesifikke attributter. Klikk på attributtnavnet
for å få en liste med attributverdier for hela ARC universet.";

$str_val = "Attributtverdi som lagret i informasjonssystemet.";

$str_que = "Oftest tilsvarer ulike køer forskjellige tillatte eksekveringstider eller forskjellige brukergrupper. Klikk på et kønavn for å få 
detaljert informasjon om køen, inkludert aktive, ventende og avsluttede jobber.";

$str_job = "Jobbnavn som tildelt av jobbeier.
          <br> <font color=\"red\">N/A</font> indikerer at eier ikke har tildelt jobben et navn.
          <br> <font color=\"red\"><b>X</b></font> indikerer at eier har avbrutt jobben.
          <br> <font color=\"red\"><b>!</b></font> indikerer at jobben ikke ble fullført.
          <br> Klikk på et navn for å få en detaljert beskrivelse av jobben.";

$str_nam = "Brukernavn som spesifisert i det personlige sertifikatet. Klikk på et navn for resurser tilgjengelige for denne brukeren og dennes jobber i systemet.";

$str_sta = "Jobbstatus som returnert av gridmanageren (GM) og lokalt resursmanagementsystem LRMS. Kronologisk er tilstandene :
   <dl>
     <dt>ACCEPTED &ndash; jobben er sendt, men ikke behandlet.</dt>
     <dt>PREPARING &ndash; inputfiler hentes.</dt>
     <dt>SUBMITTING &ndash; forhandlinger med LRMS pågår</dt>
     <dt>INLRMS &ndash; jobben er overført til LRMS. Informasjonssystemet lagrer lokal status. Mulige tilstander er :</dt>
       <dd>: Q &ndash; jobben er i køen
       <br>: U &ndash; jobben er satt på vent på en opptatt maskin (PBSPro)
       <br>: S &ndash; jobben er satt på vent (Condor)
       <br>: R, run &ndash; jobben kjøres.
       <br>: E &ndash; jobben avsluttes (PBS)
     </dd>
     <dt>FINISHING &ndash; outputfiler overføres av GM.</dt>
     <dt>FINISHED &ndash; jobben err avsluttet; tidsstempel legges til av informasjonssystemet.</dt>
     <dt>CANCELING &ndash; jobben avbrytes.</dt>
     <dt>DELETED &ndash; jobben er ikke ryddet opp av eier, men slettet av GM på grunn av overgått lagringstid.</dt>
   </dl>
   Alla disse tilstandene kan meldes med prefikset PENDING: som betyr at GM prøver å flytte jobben over i neste tilstand.";

$str_tim = "CPU-tid i minutter brukt av jobben.";

$str_mem = "Minne i KB brukt av jobben.";

$str_cpu = "Antall prosessorer brukt av jobben.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Gridmonitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    Denne siden viser alle klynger (sites) som har 
registrert seg i indekstjenesten til ARC, sortert etter land og deretter maskinnavn. Følgende 
klyngeparametere 
listes : klyngealias, total CPU-kapasitet og antall kjørende og ventende jobber, både lokale jobber og 
gridjobber. Bruk søkefunksjonen hvis annen informasjon om klynger, køer, jobber eller lignende er 
ønsket.
                                                  </dt>
                                                  <dt><b>Land</b></dt>
                                                  <dd>".$clickable.". Flagg og navn på land hentet fra 
tilgjengellige resursbeskrivelser. Klikk for å få opp infomasjon om et land.</dd>
                                                  <dt><b>Klynger</b></dt>
                                                  <dd>".$clickable.". Klyngealias tildelt av eier. Maksimalt 
vises 22 tegn. Klikk på aliaset for en detaljert klyngebeskrivelse.</dd>
                                                  <dt><b>CPU-er</b></dt>
                                                  <dd>Totalt antall CPU-er i en klynge. <b>OBS!</b> Muligens 
er bare noen av disse tilgjengelige for gridbrukere.</dd>
                                                  <dt><b>Belastning (prosesser: grid + lokalt)</b></dt>
                                                  <dd>".$clickable.". Relativ klyngebelastning som 
tilsvarer
antall opptatte CPU-er. Grå felt viser antall prosessorer som kjører lokale jobbber, røde felt 
viser antall CPU-er som kjører gridjobber. Klikk på feltet for en detaljert liste over alle gridjobber som
kjøres på klyngen, inklusive antall prosessorer per jobb.</dd>
                                                  <dt><b>Ventende</b></dt>
                                                  <dd>".$clickable.". Totalt antall jobber som venter på 
klyngen, vises som antall ventende gridjobber pluss antall ventende lokale jobber. Klikk på det første 
sifferet for å liste ventende gridjobber på klyngen.</dd>
                                                </dl>
                                                </div>",
                                      "Land"                                  => 30,
                                      "Klynge"                               => 160,
                                      "CPU-er"                                => 10,
                                      "Belastning (prosesser: grid + lokalt)" => 210,
                                      "Ventende"                                => 10
                                      ),
                  "clusdes"  => array("0" => "Resursinformasjon for",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attributt</b></dt>
                                                  <dd>".$clickable.". Klyngeattributtnavn".$str_att."</dd>
                                                  <dt><b>Verdi</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Kø</b></dt>
                                                  <dd>".$clickable.". Klyngeeiers navn på batchkøene som er tilgjengelige for ARC 
brukere.".$str_que."</dd>
                                                  <dt><b>Status</b></dt>
                                                  <dd>Køstatus. Fungerende køer viser normalt status <i>active</i>.</dd>
                                                  <dt><b>Tidsgrenser (min)</b></dt>
                                                  <dd>Tidsgrense for jobblengde per kø, hvis definert, i CPU-minutter. Den første verdien er den 
nedre grensen, den andre den øvre. Hvis ingen grenser er definert, dvs. alle jobber er tillatt, vises <b>N/A</b></dd>
                                                  <dt><b>Kjøres</b></dt>
                                                  <dd>Antall kjørende jobber. Totalt antall jobber vises med antall
                                                    <b>prosessorer</b> med gridjobber i parentes, f.eks. (Grid:
                                                    12). OBS! For parallelle multiprosessorjobber kan nummeret i
                                                    parentes være større enn antall jobber.</dd>
                                                  <dt><b>Køer</b></dt>
                                                  <dd>Antall jobber i køen. Totalt antall jobber vises
                                                    med gridjobber i parentes, f.eks. (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "Kø"                             => 0,
                                      "Status"                         => 0,
                                      "Tidsgrenser (min)"              => 0,
                                      "CPU-er"                         => 0,
                                      "Kjøres"                      => 0,
                                      "Køer"                           => 0
                                      ),
                  "jobstat"  => array("0" => "Jobber på:Jobb-ID",
                                      "help" => "<div align=\"left\">
                                                <b>JOBBLISTE:</b>
                                                <dl>
                                                <dt><b>Jobbnavn</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Eier</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Kø</b></dt>
                                                <dd>".$clickable.". Navn på batchkø hvor jobben kjøres. ".$str_que."</dd>
                                                <dt><b>CPU-er</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>JOBBINFORMASJON:</b>
                                                <dl>
                                                <dt><b>Attributt</b></dt>
                                                <dd>".$clickable.". Jobbattributtnavn".$str_att."</dd>
                                                <dt><b>Verdi</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Jobbnavn"                     => 0,
                                      "Eier"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Kø"                           => 0,
                                      "CPU-er"                       => 0
                                      ),
                  "volist"   => array("0" => "Virtuelle organisasjoner",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuell organisasjon VO</b></dt>
                                                   <dd>".$clickable.". En gruppe brukere som ofte arbeider 
med det samme og bruker de samme resursene. En VO er autorisert på minst en ARC klynge. Klikk på navnet
                                                     for å få en liste over medlemmene.</dd>
                                                   <dt><b>Medlemmer</b></dt>
                                                   <dd>Antall medlemmer.</dd>
                                                   <dt><b>Tjener</b></dt>
                                                   <dd>LDAP-tjener som huser databasen med medlemskapene.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtuell organisasjon"        => 0,
                                      "Medlemmer"                    => 0,
                                      "Tjener"                       => 0
                                      ),
                  "vousers"  => array("0" => "Gridbrukerbase",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Navn</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Tilknytning</b></dt>
                                                   <dd>Brukerens hjemmeinstitutt registrert av en VO manager. Kan være
                                                     tomt.</dd>
                                                   <dt><b>E-post</b></dt>
                                                   <dd>".$clickable.". Brukerens e-post registrert av en VO-manager. Kan være
                                                     tomt. Klikk på adressen for å sende en e-post til brukeren.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Navn"                         => 0,
                                      "Tilknytning"                   => 0,
                                      "E-post"                       => 0
                                      ),
                  "userlist" => array("0" => "Informasjon om",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Klynge:kø</b></dt>
                                                <dd>".$clickable.". Navn på klynge og dens respektive køer
                                                  (separert med et kolon, &quot;:&quot;) som brukerern er
                                                  autorisert til å sende jobber til. Hvis brukeren ikke er autorisert
                                                  vises meldingen &quot;<b><font color=\"red\">Not authorised at host
                                                      ...</font></b>&quot;. Klikk på klyngenavn
                                                  for å få detaljert klyngebeskrivelse. Klikk på kønavn
                                                  for å få detaljert købeskrivelse.</dd>
                                                <dt><b>Ledige CPU-er</b></dt>
                                                <dd>Antall ledige CPU-er i køen for denne brukeren
                                                  i øyeblikket, iblant med en øvre tidsgrense i minutter. F.eks.
                                                  &quot;3&quot; betyr tre CPU-er tilgjengelige for en jobb med 
ubegrenset
                                                  kjøringstid; &quot;4:360&quot; indikerer at det finnes fire 
CPU-er
                                                  tilgjengelige for jobber kortere enn seks timer; 
&quot;10:180
                                                  30&quot; betyr at det finnes ti CPU-er tilgjengelige for jobber som ikke
                                                  overgår tre timer, pluss 30 CPU-er tilgjengelige for jobber av valgfri
                                                  lengde; &quot;0&quot; betyr at det ikke finnes noen CPU-er tilgjenglige for
                                                  øyeblikket og at jobben kommer til å bli satt i kø.</dd>
                                                <dt><b>Ventende jobber</b></dt>
                                                <dd>Antall brukerens forventede jobber foran i køen for denne brukeren.
                                                  &quot;0&quot; betyr at jobben forventes å kjøres umiddelbart.
                                                  <b>OBS!</b> Dette er kun et estimat som
                                                  kan overkjøres av lokale regler.</dd>
                                                <dt><b>Ledig disk (MB)</b></dt>
                                                <dd>Diskplass tilgjengelig for brukeren i en gitt kø (i
                                                  megabyte). <b>OBS!</b> Dette er kun et estimat da de fleste
                                                  klynger ikke tilbyr faste diskkvoter.</dd>
                                                <dt><b>Jobbnavn</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Klynge</b></dt>
                                                <dd>".$clickable.". Navn på klynge hvor jobben
                                                  kjøres / ble kjørt. Klikk på klyngenavn for detaljert
                                                  informasjon om klyngen.</dd>
                                                <dt><b>Kø</b></dt>
                                                <dd>".$clickable.". Navn på batchkøen hvor jobben
                                                  kjøres / ble kjørt. ".$str_que."</dd>
                                                <dt><b>CPU-er</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Jobbnavn"                     => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Klynger"                      => 0,
                                      "Kø"                           => 0,
                                      "CPU-er"                       => 0
                                      ),
                  "attlist"  => array("0" => "Attributtverdi",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objekt</b></dt>
                                                   <dd>".$clickable.". Namn på det objekt vars attribut
                                                     visas. Det kan vara ett klusternamn, ett klusters könamn,
                                                     ett jobbnamn, ett användarnamn etc. Klicka på namnet för att få en
                                                     detaljerad beskrivning av objektet.</dd>
                                                   <dt><b>Attributt</b></dt>
                                                   <dd>För varje objekt, ett eller flera attributtvärden 
kan
                                                     listas. Kolumntiteln är det human-readable 
attributtnamnet
                                                     (förutom för några MDS-specifika attributt), och 
Kolumnens
                                                     innehåll är attributtvärden per objekt inmatade i
                                                     informationssystemet.</dd>
                                                 </dl>
                                                 </div>",
                                      "Objekt"                       => 0,
                                      "Attributt"                     => 0
                                      ),
                  "quelist"  => array("0" => "Kø",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attributt</b></dt>
                                                   <dd>".$clickable.". Køattributtnavn".$str_att."</dd>
                                                   <dt><b>Verdi</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Jobbnavn</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Eier</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Status</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Minne (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPU-er</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Jobbnavn"                     => 0,
                                      "Eier"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Minne (KB)"                   => 0,
                                      "CPU-er"                       => 0
                                      ),
                  "sestat"   => array("0" => "Lagringselementer",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Lagringselementets alias som angitt i 
informasjonssystemet.
                                                    Det vises maksimalt 15 tegn.</dd>
                                                  <dt><b>Total plass</b></dt>
                                                  <dd>Total diskplass (GB).</dd>
                                                  <dt><dt><b>Ledig plass</b></dt>
                                                  <dd>Diskplass tilgjengelig for øyeblikket (GB).</dd>
                                                  <dt><b>Navn</b></dt>
                                                  <dd>Lagringselementets navn. Både logisk
navn og
                                                    maskinnavn (separert med et kolon, &quot;:&quot;) angis. Det logiske
                                                    navnet brukes av informasjonssystemet for å
                                                    skille mellom ulike lagringselementer på samme maskin.</dd>
                                                  <dt><b>Base-URL</b></dt>
                                                  <dd>Lagringselementets URL, oftest en gsiftp://
                                                    protokoll. Bruk URL som basis for tilgang til filer.</dd>
                                                  <dt><b>Type</b></dt>
                                                  <dd>Lagringselementets type. &quot;gridftp-based&quot; indikerer
                                                    disklagring med gridftp-grensesnitt.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
				      //                                      "Total plass"               => 0,
                                      "Ledig/total plass"                => 0,
                                      "Navn"                         => 0,
                                      "Base-URL"                      => 0,
                                      "Type"                          => 0
                                      ),
                  "allusers" => array("0" => "Autoriserte gridbrukere:Aktive gridbrukere",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Navn</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Tilknytning</b></dt>
                                                   <dd>Brukerens tilknytning som spesifisert i det
personlige sertifikatet.</dd>
                                                   <dt><b>Jobber</b></dt>
                                                   <dd>Totalt antall jobber som denne brukeren har i 
systemet (kjørende, ventende,
                                                     ferdige eller slettede).</dd>
                                                   <dt><b>Klynger</b></dt>
                                                   <dd>Viser antall klynger som denne brukeren er 
autorisert på.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Navn"                         => 0,
                                      "Tilknytning"                   => 0,
                                      "Jobber"                         => 0,
                                      "Klynger"                      => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Klynge:kø"                   => 0,
                                      "Ledige CPU-er"                  => 0,
                                      "Ventende jobber"            => 0,
                                      "Ledig disk (MB)"                => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attributt"                     => 0,
                                      "Verdi"                        => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info gyldig f.o.m. (GMT)",
                                      "Mds-validto"   => "Info gyldig t.o.m. (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Front-end domenenavn",
                                      "nordugrid-cluster-aliasname"          => "Klyngealias",
                                      "nordugrid-cluster-contactstring"      => "Kontaktstreng",
                                      "nordugrid-cluster-interactive-contactstring" => "Interaktiv kontakt",
                                      "nordugrid-cluster-comment"            => "Kommentar",
                                      "nordugrid-cluster-support"            => "E-postkontakt",
				      "nordugrid-cluster-acl"                => "Autoriserte VO-er",
                                      "nordugrid-cluster-lrms-type"          => "LRMS-type",
                                      "nordugrid-cluster-lrms-version"       => "LRMS-versjon",
                                      "nordugrid-cluster-lrms-config"        => "LRMS-detaljer",
                                      "nordugrid-cluster-architecture"       => "Arkitektur",
                                      "nordugrid-cluster-opsys"              => "Operativsystem",
                                      "nordugrid-cluster-homogeneity"        => "Homogen klynge",
                                      "nordugrid-cluster-nodecpu"            => "CPU-type (langsomste)",
                                      "nordugrid-cluster-nodememory"         => "Minne (MB, minste)",
                                      "nordugrid-cluster-totalcpus"          => "CPU-er, totalt",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:maskiner",
                                      "nordugrid-cluster-benchmark"          => "Benchmark",
                                      "nordugrid-cluster-sessiondir-free"    => "Diskplass tilgjengelig 
(MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Diskplass totalt (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Gridsesjonens levetid (min)",
                                      "nordugrid-cluster-cache-free"         => "Cachestørrelse tilgjengelig (MB)",
                                      "nordugrid-cluster-cache-total"        => "Cachestørrelse totalt (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Runtimemiljø",
                                      "nordugrid-cluster-localse"            => "Lagringselement, lokalt",
                                      "nordugrid-cluster-middleware"         => "Grid-middleware",
                                      "nordugrid-cluster-totaljobs"          => "Jobber, totalt antall",
                                      "nordugrid-cluster-usedcpus"           => "CPU-er, opptatte",
                                      "nordugrid-cluster-queuedjobs"         => "Jobber, ventende",
				      "nordugrid-cluster-prelrmsqueued"      => "Grid jobs, awaiting submission",
                                      "nordugrid-cluster-location"           => "Postnummer",
                                      "nordugrid-cluster-owner"              => "Eier",
                                      "nordugrid-cluster-issuerca"           => "Sertifikatutstedere",
                                      "nordugrid-cluster-issuerca-hash"      => "Certificate issuer's hash",
                                      "nordugrid-cluster-trustedca"          => "Trusted certificate issuers",
                                      "nordugrid-cluster-nodeaccess"         => "Node-IP-Oppkobling",
                                      "nordugrid-cluster-gridarea"           => "Sesjonsområde (Utgått)",
                                      "nordugrid-cluster-gridspace"          => "Griddiskplass (Utgått)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS-distribusjon (Utgått)",
                                      "nordugrid-cluster-runningjobs"        => "Kjørende jobber (Utgått)",
				      "nordugrid-cluster-credentialexpirationtime" => "Credential expiration time",
                                      "nordugrid-queue-name"                 => "Kønavn",
                                      "nordugrid-queue-comment"              => "Kommentar",
                                      "nordugrid-queue-status"               => "Køstatus",
                                      "nordugrid-queue-running"              => "CPU-er, opptatte",
				      "nordugrid-queue-localqueued"          => "Local jobs, queued",
				      "nordugrid-queue-prelrmsqueued"        => "Grid jobs, awaiting submission",
                                      "nordugrid-queue-queued"               => "Jobber, ventende (Utgått)",
                                      "nordugrid-queue-maxrunning"           => "Jobber, kjørende (max)",
                                      "nordugrid-queue-maxqueuable"          => "Jobber, ventende (max)",
                                      "nordugrid-queue-maxuserrun"           => "Jobber per unixbruker (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU-tid, max. (min)",
                                      "nordugrid-queue-mincputime"           => "CPU-tid, min. (min)",
                                      "nordugrid-queue-defaultcputime"       => "CPU-tid, spesifisert (min)",
                                      "nordugrid-queue-maxwalltime"          => "Klokketid, max. (min)",
                                      "nordugrid-queue-minwalltime"          => "Klokketid, min. (min)",
                                      "nordugrid-queue-defaultwalltime"      => "Klokketid, spesifisert (min)",
                                      "nordugrid-queue-schedulingpolicy"     => "Scheduleringspolicy",
                                      "nordugrid-queue-totalcpus"            => "CPU-er, totalt",
                                      "nordugrid-queue-nodecpu"              => "CPU-type",
                                      "nordugrid-queue-nodememory"           => "Minne (MB)",
                                      "nordugrid-queue-architecture"         => "Arkitektur",
                                      "nordugrid-queue-opsys"                => "Operativsystem",
                                      "nordugrid-queue-homogeneity"          => "Homogen kø",
                                      "nordugrid-queue-gridrunning"          => "CPUs occupied by Grid jobs",
                                      "nordugrid-queue-gridqueued"           => "Gridjobber, ventende",
                                      "nordugrid-queue-benchmark"            => "Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "CPU-er per kø 
(FÖRLEGAD)",
                                      "nordugrid-queue-assignedcputype"      => "CPU-type (Utgått)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Eier",
                                      "nordugrid-job-execcluster"            => "Eksekveringsklynge",
                                      "nordugrid-job-execqueue"              => "Eksekveringskø",
                                      "nordugrid-job-stdout"                 => "Standard outputfil",
                                      "nordugrid-job-stderr"                 => "Standard errorfil",
                                      "nordugrid-job-stdin"                  => "Standard inputfil",
                                      "nordugrid-job-reqcputime"             => "Forlangt CPU-tid",
                                      "nordugrid-job-reqwalltime"            => "Forlangt klokketid",
                                      "nordugrid-job-status"                 => "Status",
                                      "nordugrid-job-queuerank"              => "Plass i køen",
                                      "nordugrid-job-comment"                => "LRMS-kommentar",
                                      "nordugrid-job-submissionui"           => "Innsendingsmaskin",
                                      "nordugrid-job-submissiontime"         => "Innsendingstid (GMT)",
                                      "nordugrid-job-usedcputime"            => "Brukt CPU-tid",
                                      "nordugrid-job-usedwalltime"           => "Brukt klokketid",
                                      "nordugrid-job-completiontime"         => "Avslutningstid (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Slettetid (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxy forfallstid (GMT)",
                                      "nordugrid-job-usedmem"                => "Brukt minne (KB)",
                                      "nordugrid-job-errors"                 => "Feil",
                                      "nordugrid-job-exitcode"               => "Returkode",
                                      "nordugrid-job-jobname"                => "Navn",
                                      "nordugrid-job-runtimeenvironment"     => "Runtimemiljø",
                                      "nordugrid-job-cpucount"               => "Forlangte CPU-er",
                                      "nordugrid-job-executionnodes"         => "Ekseekveringsnoder",
                                      "nordugrid-job-gmlog"                  => "GM loggfil",
                                      "nordugrid-job-clientsoftware"         => "Klientversjon",
                                      "nordugrid-job-rerunable"              => "Omkjørbar",
                                      "nordugrid-job-reqcput"                => "Forlangt tid (Utgått)",
                                      "nordugrid-job-gridlog"                => "Gridloggfil (Utgått)",
                                      "nordugrid-job-lrmscomment"            => "LRMS-kommentar (Utgått)",
                                      "nordugrid-authuser-name"              => "Navn",
                                      "nordugrid-authuser-sn"                => "Subjektnavn",
                                      "nordugrid-authuser-freecpus"          => "Ledige CPU-er",
                                      "nordugrid-authuser-diskspace"         => "Ledig diskplass (MB)",
                                      "nordugrid-authuser-queuelength"       => "User's queued jobs",
                                      "nordugrid-se-name"                    => "Navn",
                                      "nordugrid-se-aliasname"               => "Lagringselementalias",
                                      "nordugrid-se-type"                    => "Lagringselementtype",
				      "nordugrid-se-acl"                     => "Autoriserte VO-er",
                                      "nordugrid-se-freespace"               => "Ledig plass (MB)",
                                      "nordugrid-se-totalspace"              => "Totalt utrymme (MB)",
                                      "nordugrid-se-url"                     => "Kontakt-URL",
                                      "nordugrid-se-baseurl"                 => "Kontakt-URL (Utgått)",
                                      "nordugrid-se-accesscontrol"           => "Tilgangskontroll",
                                      "nordugrid-se-authuser"                => "Autorisert bruker", 
				      "nordugrid-se-location"                => "Postnummer",
                                      "nordugrid-se-owner"                   => "Eier",
                                      "nordugrid-se-middleware"              => "Grid-middleware",
                                      "nordugrid-se-issuerca"                => "Sertifikatutsteder",
                                      "nordugrid-se-issuerca-hash"           => "Certificate issuer's hash",
                                      "nordugrid-se-trustedca"               => "Trusted certificate issuers",
                                      "nordugrid-se-comment"                 => "Kommentar",
                                      "nordugrid-rc-name"                    => "Domenenavn",
                                      "nordugrid-rc-aliasname"               => "Replikkakatalog-Alias",
                                      "nordugrid-rc-baseurl"                 => "Kontakt-URL",
                                      "nordugrid-rc-authuser"                => "Autorisert bruker (DN)",
                                      "nordugrid-rc-location"                => "Postnummer",
                                      "nordugrid-rc-owner"                   => "Eier",
                                      "nordugrid-rc-issuerca"                => "Sertifikatutsteder"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Kan ikke lese toppnivå indekstjenere",
                                      "2"   => "Ingen av de lokale indekstjenerne returnerte 
oppkoblingen",
                                      "3"   => " dålig konfigurering eller begäran drog över tiden",
                                      "4"   => "Ingen gridjobber funnet",
                                      "5"   => "Ingen informasjon funnet",
                                      "6"   => "Tjener utilgjengelig",
                                      "7"   => " - reload senere",
                                      "8"   => "Ingen køinformasjon funnet",
                                      "9"   => "Ingen poster funnet",
                                      "10"  => "Ingen brukere funnet",
                                      "11"  => "Ikke autorisert på ",
                                      "12"  => "svarer ikke",
                                      "13"  => "Ingen nye jobber funnet for ",
                                      // debug messages
                                      "101" => " Monitor timeout for GRIS: ",
                                      "102" => " sek for oppkobling og ",
                                      "103" => " sek for søk",
                                      "104" => " sek brukere for søk",
                                      "105" => "Viser resurser også i ",
                                      "106" => "Spurte toppnivå indekstjenere: ",
                                      "107" => "Fikk geografiske data, skannede klynger: ",
                                      "108" => " klynger sortert etter geografiske data",
                                      "109" => "Søk etter klyngeattributter",
                                      "110" => "Søk etter køattributter",
                                      "111" => "Ingen data fra ",
                                      "112" => " Er oppe i ",
                                      "113" => " har ingen resurser å tilby",
                                      "114" => " Monitor timeout for GIIS: ",
                                      "115" => "Hopper over GRIS: ",
                                      "116" => "ikke en ",
                                      "117" => "Verifiserer oppkobling: ",
                                      "118" => "OK",
                                      "119" => "Hittil, detekterte resurser av slag ",
                                      "120" => "LDAP-feil ved søk etter ",
                                      "121" => " status ved ",
                                      "122" => "Svartelistede: ",
                                      "123" => "Registrert funnet for ",
                                      "124" => "Søk etter lagringselementattributter",
                                      "125" => "Søk etter brukere",
                                      "126" => "Søk etter jobb",
                                      "127" => " har jobb ",
                                      "128" => " uten  være autorisert",
                                      "129" => "Kan ikke lade objektdata: feil ",
                                      // icon titles
                                      "301" => "Reload",
                                      "302" => "Skriv ut",
                                      "303" => "Hjelp",
                                      "304" => "Lukk",
                                      "305" => "Rød",
                                      "306" => "Grå",
                                      "307" => "Alle brukere",
                                      "308" => "Aktive brukere",
                                      "309" => "Søk",
                                      "310" => "Lagringsenheter",
                                      "311" => "VO-er",
                                      "312" => "Flagg for ",
                                      "313" => " gridprosesser og ",
                                      "314" => " lokale prosesser",
                                      // auxilliary strings
                                      "401" => "Prosesser",
                                      "402" => "Grid",
                                      "403" => "Lokalt",
                                      "404" => "Verden",
                                      "405" => "TOTALT",
                                      "406" => " klynger",
                                      "407" => "en masse",
                                      "408" => " GB",
                                      "409" => " ALLE",
                                      "410" => "Klynge",
                                      "411" => "Kø",
                                      "412" => "Jobb",
                                      "413" => "Bruker",
                                      "414" => "Lagringsenhet",
                                      "415" => "Replikkakatalog",
                                      "416" => "Definer søkeattributter for objekt : ",
                                      "417" => "Det søkes logisk OG av alle uttrykkene.",
                                      "418" => "La det høyre feltet stå tomt for å vise alt.",
                                      "419" => "Vis resurser eller objekt som samsvarer med ditt valg",
                                      "420" => "Særskilt navn",
                                      "421" => "Kan bruke totalt ",
                                      "422" => " klynger",
                                      "423" => "Resurs / objekt:",
                                      "424" => "Antall attributter (standard er 6):",
                                      "425" => "Objekt",
                                      "426" => "Neste",
                                      "427" => "Velg",
                                      "428" => "Gjenopprett",
                                      "429" => "VIS"
                                      ),
                  // Post code conversion
                  "tlconvert" => array (
                                        "Australia"    => "Australia",
					"Austria"      => "&Ouml;sterrike",
					"Armenia"      => "Armenia",
					"Algeria"      => "Algerie",
					"Belgium"      => "Belgia",
					"Bulgaria"     => "Bulgaria",
                                        "Canada"       => "Canada",
                                        "China"        => "Kina",
					"Czechia"      => "Tsjekkia",
                                        "Denmark"      => "Danmark", 
                                        "Estonia"      => "Estland", 
                                        "Finland"      => "Finland", 
					"France"       => "Frankrike",
					"Georgia"      => "Georgia",
                                        "Germany"      => "Tyskland", 
					"Greece"       => "Hellas",
					"Hungary"      => "Ungarn",
                                        "Iceland"      => "Island",
					"Ireland"      => "Irland",
					"Italy"        => "Italia",
                                        "Japan"        => "Japan", 
                                        "Latvia"       => "Lettland", 
                                        "Lithuania"    => "Litauen", 
					"Morocco"      => "Marokko",
					"Netherlands"  => "Nederland",
                                        "Norway"       => "Norge", 
					"Poland"       => "Polen",
					"Portugal"     => "Portugal",
					"Romania"      => "Romania",
					"Russia"       => "Russland", 
					"SriLanka"     => "Sri Lanka",
                                        "Sweden"       => "Sverige", 
                                        "Slovakia"     => "Slovakia", 
                                        "Slovenia"     => "Slovenia", 
                                        "Switzerland"  => "Sveits", 
					"Turkey"       => "Tyrkia",
					"UK"           => "Storbritannia",
					"Ukraine"      => "Ukraina",
					"USA"          => "USA", 
					"World"        => "Verden"
                                        )
                  );

?>
