<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- Swedish [sv]
// -- Translation: Mattias Ellert (mattias.ellert@tsl.uu.se)
// -- Author: oxana.smirnova@hep.lu.se

// Some common strings:

$clickable = "KLICKBAR";

$str_att = ", human-readable förutom för några MDS-specifika attribut. Klicka på attributnamnet
för att få en lista med attributvärden för hela ARCs universum.";

$str_val = "Attributvärde inmatat i informationssystemet.";

$str_que = "Oftast motsvarar olika köer olika tillåtna exekveringstider eller olika användargrupper. Klicka på ett könamn för att få detaljerad information om kön, inklusive aktiva, köade och avslutade jobb.";

$str_job = "Jobnamn tilldelat av jobbägaren.
          <br> <font color=\"red\">N/A</font> betyder att ägaren inte tilldelat ett jobbnamn
          <br> <font color=\"red\"><b>X</b></font> betyder att jobbet dödats av ägaren
          <br> <font color=\"red\"><b>!</b></font> betyder att jobbet inte fullbordades i systemet
          <br> Klicka på ett namn för att få en detaljerad beskrivning av jobbet.";

$str_nam = "Användarens namn såsom specificerat i det personliga cerifikatet. Klicka på ett namn för att få en lista över alla resurser som är tillgängliga för denna användare och denna användares alla jobb som för närvarande finns i systemet.";

$str_sta = "Jobbstatus returnerad av gridmanagern (GM) och
   LRMS. Tillstånden är i tidsordning:
   <dl>
     <dt>ACCEPTED &ndash; jobbet har skickats in men är ännu ej behandlat</dt>
     <dt>PREPARING &ndash; indatafiler hämtas</dt>
     <dt>SUBMITTING &ndash; växelverkan med LRMS pågår</dt>
     <dt>INLRMS &ndash; jobbet har överförts till LRMS; intern
       status läggs till av informationsstystemet. Möjliga tillstånd är:</dt>
       <dd>: Q &ndash; jobbet är köat
       <br>: U &ndash; jobbet är i ett uppskjutet tillstånd på en upptagen nod (PBSPro)
       <br>: S &ndash; jobbet är i ett uppskjutet tillstånd (Condor)
       <br>: R, run &ndash; jobbet exekveras
       <br>: E &ndash; jobbet avslutas (PBS)
     </dd>
     <dt>FINISHING &ndash; utdatafiler överförs av GM</dt>
     <dt>FINISHED &ndash; jobbet är avslutat; tidsstämpel läggs till av informationssystemet</dt>
     <dt>CANCELING &ndash; jobbet håller på att avbrytas</dt>
     <dt>DELETED &ndash; jobbet har inte tagits bort på begäran av användaren utan
       av GM p.g.a. att maximala lagringstiden har passerat</dt>
   </dl>
   Alla dessa tillstånd kan rapporteras med prefixet PENDING:,
   vilket betyder att GM försöker a flytta jobbet till nästa tillstånd";

$str_tim = "CPU-tid som jobbet använt, minuter.";

$str_mem = "Minne som jobbet använt, KB.";

$str_cpu = "Antal processorer som jobbet använt.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Gridmonitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    Denna sida visar alla kluster som registrerar sig till ARCs indexservice,
                                                    sorterade efter land och därefter värdnamn. Utvalda klusterparametrar
                                                    monitoreras: klusteralias, total CPU-kapacitet och antal jobb som exekveras och
                                                    köar på klustret, såväl gridjobb som lokala jobb. Använd sökfuntionen om du
                                                    vill jämföra annan kluster-, kö- och jobbinformation.
                                                  </dt>
                                                  <dt><b>Land</b></dt>
                                                  <dd>".$clickable.". Landslagga och landsnamn härledda från tillgängliga
                                                      resursbeskrivningar. Klicka för att visa endast detta lands information.</dd>
                                                  <dt><b>Kluster</b></dt>
                                                  <dd>".$clickable.". Klusteralias tilldelat av ägaren. Maximal visad längd är 22
                                                      tecken. Klicka på detta alias för att få en detaljerad
                                                      klusterbeskrivning.</dd>
                                                  <dt><b>CPU:er</b></dt>
                                                  <dd>Totalt antal CPU:er i ett kluster. <b>OBS!</b> Endast en del av dessa kan
                                                      vara tillgängliga för gridanvändare.</dd>
                                                  <dt><b>Belastning (processer: grid + lokala)</b></dt>
                                                  <dd>".$clickable.". Relativ klusterbelastning, motsvarande antalet upptagna
                                                      CPU:er. Grå fält markerar processorer upptagna av de lokalt inskickade
                                                      jobben, medan röda fält visar CPU:er upptagna av jobb som skickats in via
                                                      grid. Klicka på fältet för att få en detaljerad lista av alla gridjobb som
                                                      exekveras på klustret, inklusive antalet processorer per job.</dd>
                                                  <dt><b>Köande</b></dt>
                                                  <dd>".$clickable.". Totalt antal jobb som köar på klustret, visat som antalet
                                                      köande gridjobb plus antalet lokalt inskickade köande jobb. Klicka på den
                                                      första siffran för att få en lista av köande gridjob på klustret.</dd>
                                                </dl>
                                                </div>",
                                      "Land"                                  => 30,
                                      "Kluster"                               => 160,
                                      "CPU:er"                                => 10,
                                      "Belastning (processer: grid + lokala)" => 210,
                                      "Köande"                                => 10
                                      ),
                  "clusdes"  => array("0" => "Resursinformation för",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attribut</b></dt>
                                                  <dd>".$clickable.". Klusterattributnamn".$str_att."</dd>
                                                  <dt><b>Värde</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Kö</b></dt>
                                                  <dd>".$clickable.". Namn på batchköer tillgängliga för
                                                    ARCanvändarna uppsatta av klusterägarna. ".$str_que."</dd>
                                                  <dt><b>Status</b></dt>
                                                  <dd>Köstatus. Fungerande köer visar normalt status <i>active</i>.</dd>
                                                  <dt><b>Tidsgränser (min)</b></dt>
                                                  <dd>Tidsgräns för jobblängd per kö, om definierad, i CPU-minuter. Det första
                                                    visade värdet är den nedre gränsen, det andra den övre. Om inga gränser är
                                                    definierade (jobb med alla längder är tillåtna), visas <b>N/A</b></dd>
                                                  <dt><b>Exekveras</b></dt>
                                                  <dd>Antal jobb som exekveras i kön. Det totala antalet jobb visas, med antalet
                                                    <b>processorer</b> upptagna av gridjobb i parentes, t.ex. (Grid:
                                                    12). OBS! För parallella multiprocessorjobb kan numret i
                                                    parentes vara större än antalet jobb.</dd>
                                                  <dt><b>Köar</b></dt>
                                                  <dd>Antal jobb som väntar på att exekveras i kön. Det totala antalet jobb visas,
                                                    med gridjobb visade i parentes, t.ex. (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "Kö"                             => 0,
                                      "Mappningskö"                    => 0,
                                      "Status"                         => 0,
                                      "Tidsgränser (min)"              => 0,
                                      "CPU:er"                         => 0,
                                      "Exekveras"                      => 0,
                                      "Köar"                           => 0
                                      ),
                  "jobstat"  => array("0" => "Jobb på:Jobb-ID",
                                      "help" => "<div align=\"left\">
                                                <b>JOBBLISTA:</b>
                                                <dl>
                                                <dt><b>Jobbnamn</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Ägare</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Kö</b></dt>
                                                <dd>".$clickable.". Namn på batchkön i vilken jobbet exekveras. ".$str_que."</dd>
                                                <dt><b>CPU:er</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>JOBBINFORMATION:</b>
                                                <dl>
                                                <dt><b>Attribut</b></dt>
                                                <dd>".$clickable.". Jobbattributnamn".$str_att."</dd>
                                                <dt><b>Värde</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Jobbnamn"                     => 0,
                                      "Ägare"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Kö"                           => 0,
                                      "CPU:er"                       => 0
                                      ),
                  "volist"   => array("0" => "Virtuella organisationer",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuell organisation</b></dt>
                                                   <dd>".$clickable.". Användargrupp, delar oftast gemensamma activiteter och
                                                     resurser, autoriserad på åtminstone ett ARC-kluster. Klicka på namnet
                                                     för att få en lista med gruppmedlemmar.</dd>
                                                   <dt><b>Medlemmar</b></dt>
                                                   <dd>Antal gruppmedlemmar.</dd>
                                                   <dt><b>Server</b></dt>
                                                   <dd>LDAP-server som huserar gruppmedlemsskapsdatabasen.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtuell organisation"        => 0,
                                      "Medlemmar"                    => 0,
                                      "Server"                       => 0
                                      ),
                  "vousers"  => array("0" => "Gridanvändarbas",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Namn</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Anknytning</b></dt>
                                                   <dd>Användarens heminstitut inmatat av VO-managern. Kan vara
                                                     tomt.</dd>
                                                   <dt><b>E-post</b></dt>
                                                   <dd>".$clickable.". Användarens e-post inmatad av en VO-manager. Kan vara
                                                     tomt. Klicka på adressen för att sända ett e-brev till användaren.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Namn"                         => 0,
                                      "Anknytning"                   => 0,
                                      "E-post"                       => 0
                                      ),
                  "userlist" => array("0" => "Information för",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Kluster:kö</b></dt>
                                                <dd>".$clickable.". Namn på kluster och dess respektive köer
                                                  (separade med ett kolon, &quot;:&quot;) där en användare är
                                                  autoriserad att skicka in jobb. Om en avändare inte är autoriserad
                                                  visas meddelendet &quot;<b><font color=\"red\">Not authorised at host
                                                      ...</font></b>&quot;. Klicka på ett klusternamn
                                                  för att få en detaljerad klusterbeskrivning. Klicka på ett könamn
                                                  föt att få en detaljerad köbeskrivning.</dd>
                                                <dt><b>Fria CPU:er</b></dt>
                                                <dd>Antal fria CPU:er tillgängliga i en given kö för denna användare
                                                  vid detta tillfälle, ibland med en övre tidsgräns (i minuter) bifogad. T.ex.
                                                  &quot;3&quot; betyder 3 CPU:er tillgängliga för ett jobb med obegränsad
                                                  exekveringstid; &quot;4:360&quot; indikerar att det finns 4 CPU:er
                                                  tillgängliga för jobb som inte är längre än 6 timmar; &quot;10:180
                                                  30&quot; betyder att det finns 10 CPU:er tillgängliga för jobb som inte
                                                  övergår 3 timmar, plus 30 CPU:er tillgängliga för jobb av valfri
                                                  längd; &quot;0&quot; betyder att det inte finns några CPU:er tillgängliga för
                                                  tillfället, och att jobben kommer att placeras i kö.</dd>
                                                <dt><b>Köade jobb</b></dt>
                                                <dd>Antal användarens jobb som förväntas stå före ett nytt inskickat
                                                  jobb (för denna användare) i en kö.
                                                  &quot;0&quot; betyder att jobbet förväntas exekveras
                                                  omedelbart. <b>OBS!</b> Detta är endast en uppskattning, som
                                                  kan åsidosättas av lokala regler.</dd>
                                                <dt><b>Fri disk (MB)</b></dt>
                                                <dd>Diskutrymme gillgängligt för användaren i en given kö (i
                                                  megabyte). <b>OBS!</b> Detta är endast en uppskattning, då de flesta
                                                  kluster inte erbjuder fasta diskkvoter.</dd>
                                                <dt><b>Jobbnamn</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Kluster</b></dt>
                                                <dd>".$clickable.". Namn på det kluster på vilket jobbet
                                                  exekvera(de)s. Klicka på ett klusternamn för att få detaljerad
                                                  information om klustret.</dd>
                                                <dt><b>Kö</b></dt>
                                                <dd>".$clickable.". Name på den batchkö i vilken jobbet
                                                  exekvera(de)s. ".$str_que."</dd>
                                                <dt><b>CPU:er</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Jobbnamn"                     => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Kluster"                      => 0,
                                      "Kö"                           => 0,
                                      "CPU:er"                       => 0
                                      ),
                  "attlist"  => array("0" => "Attributvärden",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objekt</b></dt>
                                                   <dd>".$clickable.". Namn på det objekt vars attribut
                                                     visas. Det kan vara ett klusternamn, ett klusters könamn,
                                                     ett jobbnamn, ett användarnamn etc. Klicka på namnet för att få en
                                                     detaljerad beskrivning av objektet.</dd>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>För varje objekt, ett eller flera attributvärden kan
                                                     listas. Kolumntiteln är det human-readable attributnamnet
                                                     (förutom för några MDS-specifika attribut), och Kolumnens
                                                     innehåll är attributvärden per objekt inmatade i
                                                     informationssystemet.</dd>
                                                 </dl>
                                                 </div>",
                                      "Objekt"                       => 0,
                                      "Attribut"                     => 0
                                      ),
                  "quelist"  => array("0" => "Kö",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>".$clickable.". Köattributnamn".$str_att."</dd>
                                                   <dt><b>Värde</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Jobbnamn</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Ägare</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Status</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Minne (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPU:er</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Jobbnamn"                     => 0,
                                      "Ägare"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Minne (KB)"                   => 0,
                                      "CPU:er"                       => 0
                                      ),
                  "sestat"   => array("0" => "Lagringselement",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Lagringselementalias specificerat i informationssytemet.
                                                    Maximal visad längd är 15 tecken.</dd>
                                                  <dt><b>Totalt utrymme</b></dt>
                                                  <dd>Totalt diskutrymme, GB.</dd>
                                                  <dt><dt><b>Fritt utrymme</b></dt>
                                                  <dd>Diskutrymme tillgängligt för tillfället, GB.</dd>
                                                  <dt><b>Namn</b></dt>
                                                  <dd>Lagringselementnamn, bestående av ett logiskt namn och
                                                    värdnamn (separerade av ett kolon, &quot;:&quot;). Det logiska
                                                    namnet används endast för informationssystemsyften, för att
                                                    särskilja olika lagringselement som huserar på
                                                    samma maskin.</dd>
                                                  <dt><b>Bas-URL</b></dt>
                                                  <dd>Lagringselementats URL, oftast ett gsiftp://
                                                    protocol. Använd denna URL som bas för att komma åt filer.</dd>
                                                  <dt><b>Typ</b></dt>
                                                  <dd>Lagringselementtyp. &quot;gridftp-based&quot; indikerar
                                                    disklagring med gridftp-gränssnitt.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
				      //                                      "Totalt utrymme"               => 0,
                                      "Fritt/totalt utrymme, GB"     => 0,
                                      "Namn"                         => 0,
                                      "Bas-URL"                      => 0,
                                      "Typ"                          => 0
                                      ),
                  "allusers" => array("0" => "Autoriserade gridanvändare:Aktiva gridanvändare",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Namn</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Anknytning</b></dt>
                                                   <dd>Användarens anknytning, härledd från det personliga certifikatet</dd>
                                                   <dt><b>Jobb</b></dt>
                                                   <dd>Totalt antal jobb från denna användarens i systemet (exekveras, avvaktande,
                                                     avslutade eller borttagna)</dd>
                                                   <dt><b>Kluster</b></dt>
                                                   <dd>Visar hur många kluster som autoriserar denna användare</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Namn"                         => 0,
                                      "Anknytning"                   => 0,
                                      "Jobb"                         => 0,
                                      "Kluster"                      => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Kluster:kö"                   => 0,
                                      "Fria CPU:er"                  => 0,
                                      "Köade jobb"                   => 0,
                                      "Fri disk (MB)"                => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attribut"                     => 0,
                                      "Värde"                        => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info giltig fr.o.m. (GMT)",
                                      "Mds-validto"   => "Info giltig t.o.m. (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Front-end domännamn",
                                      "nordugrid-cluster-aliasname"          => "Klusteralias",
                                      "nordugrid-cluster-contactstring"      => "Kontaktsträng",
                                      "nordugrid-cluster-interactive-contactstring" => "Interaktiv kontakt",
                                      "nordugrid-cluster-comment"            => "Kommentar",
                                      "nordugrid-cluster-support"            => "E-postkontakt",
				      "nordugrid-cluster-acl"                => "Auktoriserade VO:er",
                                      "nordugrid-cluster-lrms-type"          => "LRMS-typ",
                                      "nordugrid-cluster-lrms-version"       => "LRMS-version",
                                      "nordugrid-cluster-lrms-config"        => "LRMS-detaljer",
                                      "nordugrid-cluster-architecture"       => "Arkitektur",
                                      "nordugrid-cluster-opsys"              => "Operativsystem",
                                      "nordugrid-cluster-homogeneity"        => "Homogent kluster",
                                      "nordugrid-cluster-nodecpu"            => "CPU-typ (långsammast)",
                                      "nordugrid-cluster-nodememory"         => "Minne (MB, minsta)",
                                      "nordugrid-cluster-totalcpus"          => "CPU:er, totalt",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:er, per maskin",
                                      "nordugrid-cluster-benchmark"          => "Benchmark",
                                      "nordugrid-cluster-sessiondir-free"    => "Diskutrymme, tillgängligt (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Diskutrymme, totalt (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Gridsessionens livstid (min)",
                                      "nordugrid-cluster-cache-free"         => "Cachestorlek, tillgängligt (MB)",
                                      "nordugrid-cluster-cache-total"        => "Cachestorlek, totalt (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Runtime-miljö",
                                      "nordugrid-cluster-localse"            => "Lagringselement, lokalt",
                                      "nordugrid-cluster-middleware"         => "Grid-middleware",
                                      "nordugrid-cluster-totaljobs"          => "Jobb, totalt antal",
                                      "nordugrid-cluster-usedcpus"           => "CPU:er, upptagna",
                                      "nordugrid-cluster-queuedjobs"         => "Jobb, köade",
				      "nordugrid-cluster-prelrmsqueued"      => "Gridjobb, köade före LRMS",
                                      "nordugrid-cluster-location"           => "Postnummer",
                                      "nordugrid-cluster-owner"              => "Ägare",
                                      "nordugrid-cluster-issuerca"           => "Certifikatutfärdare",
                                      "nordugrid-cluster-issuerca-hash"      => "Certifikatutfärdares hashsumma",
                                      "nordugrid-cluster-trustedca"          => "Betrodd certifikatutfärdare",
                                      "nordugrid-cluster-nodeaccess"         => "Nod-IP-uppkoppling",
                                      "nordugrid-cluster-gridarea"           => "Sessionsarea (FÖRLEGAD)",
                                      "nordugrid-cluster-gridspace"          => "Griddiskutrymme (FÖRLEGAD)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS-distribution (FÖRLEGAD)",
                                      "nordugrid-cluster-runningjobs"        => "Jobb, exekveras (FÖRLEGAD)",
				      "nordugrid-cluster-credentialexpirationtime" => "Certifikat-förfallotid",
                                      "nordugrid-queue-name"                 => "Könamn",
                                      "nordugrid-queue-comment"              => "Kommentar",
                                      "nordugrid-queue-status"               => "Köstatus",
                                      "nordugrid-queue-running"              => "CPU:er, upptagna",
				      "nordugrid-queue-localqueued"          => "Lokala jobb, köade",
				      "nordugrid-queue-prelrmsqueued"        => "Grid jobb, köade före LRMS",
                                      "nordugrid-queue-queued"               => "Jobb, köade (FÖRLEGAD)",
                                      "nordugrid-queue-maxrunning"           => "Jobb, exekveras (max)",
                                      "nordugrid-queue-maxqueuable"          => "Jobb, köbara (max)",
                                      "nordugrid-queue-maxuserrun"           => "Jobb per unixanvändare (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU-tid, max. (minuter)",
                                      "nordugrid-queue-mincputime"           => "CPU-tid, min. (minuter)",
                                      "nordugrid-queue-defaultcputime"       => "CPU-tid, förvald (minuter)",
                                      "nordugrid-queue-maxwalltime"          => "Klocktid, max. (minuter)",
                                      "nordugrid-queue-minwalltime"          => "Klocktid, min. (minuter)",
                                      "nordugrid-queue-defaultwalltime"      => "Klocktid, förvald (minuter)",
                                      "nordugrid-queue-schedulingpolicy"     => "Scheduleringspolicy",
                                      "nordugrid-queue-totalcpus"            => "CPU:er, totalt",
                                      "nordugrid-queue-nodecpu"              => "CPU-typ",
                                      "nordugrid-queue-nodememory"           => "Minne (MB)",
                                      "nordugrid-queue-architecture"         => "Arkitektur",
                                      "nordugrid-queue-opsys"                => "Operativsystem",
                                      "nordugrid-queue-homogeneity"          => "Homogen kö",
                                      "nordugrid-queue-gridrunning"          => "CPU:er, upptagna av gridjobb",
                                      "nordugrid-queue-gridqueued"           => "Gridjobb, köade",
                                      "nordugrid-queue-benchmark"            => "Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "CPU:er per kö (FÖRLEGAD)",
                                      "nordugrid-queue-assignedcputype"      => "CPU-typ (FÖRLEGAD)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Ägare",
                                      "nordugrid-job-execcluster"            => "Exekveringskluster",
                                      "nordugrid-job-execqueue"              => "Exekveringskö",
                                      "nordugrid-job-stdout"                 => "Standard output-fil",
                                      "nordugrid-job-stderr"                 => "Standard error-fil",
                                      "nordugrid-job-stdin"                  => "Standard input-fil",
                                      "nordugrid-job-reqcputime"             => "Begärd CPU-tid",
                                      "nordugrid-job-reqwalltime"            => "Begärd klocktid",
                                      "nordugrid-job-status"                 => "Status",
                                      "nordugrid-job-queuerank"              => "Plats i kön",
                                      "nordugrid-job-comment"                => "LRMS-kommentar",
                                      "nordugrid-job-submissionui"           => "Inskickningsmaskin",
                                      "nordugrid-job-submissiontime"         => "Inskickningstid (GMT)",
                                      "nordugrid-job-usedcputime"            => "Använd CPU-tid",
                                      "nordugrid-job-usedwalltime"           => "Använd klocktid",
                                      "nordugrid-job-completiontime"         => "Avslutningstid (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Raderingstid (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxyförfallotid (GMT)",
                                      "nordugrid-job-usedmem"                => "Använt minne (KB)",
                                      "nordugrid-job-errors"                 => "Fel",
                                      "nordugrid-job-exitcode"               => "Returkod",
                                      "nordugrid-job-jobname"                => "Namn",
                                      "nordugrid-job-runtimeenvironment"     => "Runtimemiljö",
                                      "nordugrid-job-cpucount"               => "Begärda CPU:er",
                                      "nordugrid-job-executionnodes"         => "Exekveringsnoder",
                                      "nordugrid-job-gmlog"                  => "GM loggfil",
                                      "nordugrid-job-clientsoftware"         => "Klientversion",
                                      "nordugrid-job-rerunable"              => "Omkörbart",
                                      "nordugrid-job-reqcput"                => "Begärd tid (FÖRLEGAD)",
                                      "nordugrid-job-gridlog"                => "Gridloggfil (FÖRLEGAD)",
                                      "nordugrid-job-lrmscomment"            => "LRMS-kommentar (FÖRLEGAD)",
                                      "nordugrid-authuser-name"              => "Namn",
                                      "nordugrid-authuser-sn"                => "Subjektnamn",
                                      "nordugrid-authuser-freecpus"          => "Fria CPU:er",
                                      "nordugrid-authuser-diskspace"         => "Fritt diskutrymme (MB)",
                                      "nordugrid-authuser-queuelength"       => "Användarens kölängd",
                                      "nordugrid-se-name"                    => "Namn",
                                      "nordugrid-se-aliasname"               => "Lagringselementalias",
                                      "nordugrid-se-type"                    => "Lagringselementtyp",
				      "nordugrid-se-acl"                     => "Auktoriserade VO:er",
                                      "nordugrid-se-freespace"               => "Fritt utrymme (MB)",
                                      "nordugrid-se-totalspace"              => "Totalt utrymme (MB)",
                                      "nordugrid-se-url"                     => "Kontakt-URL",
                                      "nordugrid-se-baseurl"                 => "Kontakt-URL (FÖRLEGAD)",
                                      "nordugrid-se-accesscontrol"           => "Access kontroll",
                                      "nordugrid-se-authuser"                => "Auktoriserad användare (DN)",
                                      "nordugrid-se-location"                => "Postnummer",
                                      "nordugrid-se-owner"                   => "Ägare",
                                      "nordugrid-se-middleware"              => "Grid-middleware",
                                      "nordugrid-se-issuerca"                => "Certifikatutfärdare",
                                      "nordugrid-se-issuerca-hash"           => "Certifikatutfärdares hashsumma",
                                      "nordugrid-se-trustedca"               => "Betrodd certifikatutfärdare",
                                      "nordugrid-se-comment"                 => "Kommentar",
                                      "nordugrid-rc-name"                    => "Domännamn",
                                      "nordugrid-rc-aliasname"               => "Replica Catalog-alias",
                                      "nordugrid-rc-baseurl"                 => "Kontakt-URL",
                                      "nordugrid-rc-authuser"                => "Auktoriserad användare (DN)",
                                      "nordugrid-rc-location"                => "Postnummer",
                                      "nordugrid-rc-owner"                   => "Ägare",
                                      "nordugrid-rc-issuerca"                => "Certifikatutfärdare"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Kan inte läsa topp-nivå-indexservrarna",
                                      "2"   => "Ingen av de lokala indexservrarna returnerade uppkopplingen",
                                      "3"   => " dålig konfigurering eller begäran drog över tiden",
                                      "4"   => "Inga gridjobb funna",
                                      "5"   => "Ingen information funnen",
                                      "6"   => "Server otillgänglig",
                                      "7"   => " - ladda om senare",
                                      "8"   => "Ingen köinformation funnen",
                                      "9"   => "Inga poster funna",
                                      "10"  => "Inga användare funna",
                                      "11"  => "Inte autoriserad på värden",
                                      "12"  => "svarar inte",
                                      "13"  => "Inga nya jobb funna för ",
                                      // debug messages
                                      "101" => " Monitor-time-out för GRIS: ",
                                      "102" => " sek för uppkoppling och ",
                                      "103" => " sek för sökning",
                                      "104" => " sek använda för sökning",
                                      "105" => "Visar resurser endast i ",
                                      "106" => "Frågade topp-nivå-indexservrar: ",
                                      "107" => "Fick geografiska data, skannade kluster: ",
                                      "108" => " kluster sorterade efter geografiska data",
                                      "109" => "Sökning efter klusterattribut",
                                      "110" => "Sökning efter köattribut",
                                      "111" => "Inga data från ",
                                      "112" => " är uppe i ",
                                      "113" => " har inga resurser att erbjuda",
                                      "114" => " Monitor-time-out för GIIS: ",
                                      "115" => "Hoppar över GRIS: ",
                                      "116" => "inte en ",
                                      "117" => "Verifierar uppkoppling: ",
                                      "118" => "OK",
                                      "119" => "Hittills, detekterade resurser av slag ",
                                      "120" => "LDAP-fel vid sökning efter ",
                                      "121" => " status vid ",
                                      "122" => "Svartlistad: ",
                                      "123" => "Registrant funnen för ",
                                      "124" => "Sökning efter lagringselementattribut",
                                      "125" => "Sökning efter användare",
                                      "126" => "Sökning efter jobb",
                                      "127" => " har jobb ",
                                      "128" => " utan att vara auktoriserad",
                                      "129" => "Kan inte hämta objektdata: fel ",
                                      "130" => " Monitor-time-out för EMIR: ",
                                      // icon titles
                                      "301" => "Ladda om",
                                      "302" => "Skriv ut",
                                      "303" => "Hjälp",
                                      "304" => "Stäng",
                                      "305" => "Röd",
                                      "306" => "Grå",
                                      "307" => "Alla användare",
                                      "308" => "Aktiva användare",
                                      "309" => "Sök",
                                      "310" => "Lagring",
                                      "311" => "VO:ar",
                                      "312" => "Flagga för ",
                                      "313" => " Gridprocesser and ",
                                      "314" => " lokala processer",
                                      // auxilliary strings
                                      "401" => "Processer",
                                      "402" => "Grid",
                                      "403" => "Lokala",
                                      "404" => "Världen",
                                      "405" => "TOTALT",
                                      "406" => " kluster",
                                      "407" => "en massa",
                                      "408" => " GB",
                                      "409" => " ALLA",
                                      "410" => "Kluster",
                                      "411" => "Kö",
                                      "412" => "Jobb",
                                      "413" => "Användare",
                                      "414" => "Lagring",
                                      "415" => "Replikakatalog",
                                      "416" => "Definera attribut att visa för objektet: ",
                                      "417" => "logiskt OCH av alla uttryck kommer att hittas",
                                      "418" => "Lämna det högra fältet tomt för att visa allt",
                                      "419" => "Visa resurser eller objekt enligt ditt val",
                                      "420" => "Särskijlande namn",
                                      "421" => "Kan använda totalt ",
                                      "422" => " kluster",
                                      "423" => "Resurs / objekt:",
                                      "424" => "Antal attribut (förval 6):",
                                      "425" => "Objekt",
                                      "426" => "Nästa",
                                      "427" => "Välj ett",
                                      "428" => "Återställ",
                                      "429" => "VISA"
                                      ),
                  // Post code conversion
                  "tlconvert" => array (
                                        "Australia"    => "Australien",
					"Austria"      => "&Ouml;sterrike",
					"Armenia"      => "Armenien",
					"Algeria"      => "Algeriet",
					"Belgium"      => "Belgien",
					"Bulgaria"     => "Bulgarien",
                                        "Canada"       => "Canada",
                                        "Chile"        => "Chile",
                                        "China"        => "Kina",
					"Czechia"      => "Tjeckien",
                                        "Denmark"      => "Danmark", 
                                        "Estonia"      => "Estland", 
                                        "Finland"      => "Finland", 
					"France"       => "Frankrike",
					"Georgia"      => "Georgien",
                                        "Germany"      => "Tyskland", 
					"Greece"       => "Grekland",
					"Hungary"      => "Ungern",
                                        "Iceland"      => "Island",
					"Ireland"      => "Irland",
					"Italy"        => "Italien",
                                        "Japan"        => "Japan", 
                                        "Latvia"       => "Lettland", 
                                        "Lithuania"    => "Litauen", 
					"Morocco"      => "Marocko",
					"Netherlands"  => "Nederl&auml;nderna",
                                        "Norway"       => "Norge", 
					"Poland"       => "Polen",
					"Portugal"     => "Portugal",
					"Romania"      => "Rum&auml;nien",
					"Russia"       => "Ryssland", 
					"SriLanka"     => "Sri&nbsp;Lanka",
                                        "Sweden"       => "Sverige", 
                                        "Slovakia"     => "Slovakien", 
                                        "Slovenia"     => "Slovenien", 
                                        "Switzerland"  => "Schweiz", 
                                        "Turkey"       => "Turkiet", 
					"UK"           => "Storbritannien",
					"Ukraine"      => "Ukraina",
					"USA"          => "USA", 
					"World"        => "V&auml;rlden"
                                        )
                  );

?>
