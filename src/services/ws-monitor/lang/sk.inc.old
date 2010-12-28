<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- Slovak [sk]
// -- Translation: martin.savko@gmail.com
// -- Author: oxana.smirnova@hep.lu.se


// Some common strings:

$clickable = "ODKAZ";

$str_att = ", Čitateľné človekom (s výnimkou niekoľkých atribútov špecifických pre systém MDS). Po kliknutí sa zobrazia významy tohto atribútu v priestore <i>ARC</i>.";

$str_val = "Hodnota atribútu v Informačnom Systéme.";

$str_que = "Obyčajne sa jednotlivé rady navzájom líšia prípustným trvaním úloh, prípadne rôznymi skupinami užívateľov. Po kliknutí na príslušnú radu získate podrobné informácie vrátane zoznamu bežiacich, čakajúcich a ukončených úloh.";

$str_job = "Názov úlohy zvolený užívateľom.
          <br> <font color=\"red\">N/A</font> znamená, že vlastník nepriradil úlohe žiadne meno.
          <br> <font color=\"red\"><b>X</b></font> znamená, že úloha bola ukončená vlastníkom.
          <br> <font color=\"red\"><b>!</b></font> znamená, že pri plnení úlohy došlo k chybe.
          <br> Kliknutím zobrazíte podrobné informácie o úlohe.";

$str_nam = "Meno užívateľa, podľa jeho osobného certifikátu. Kliknutím získate zoznam všetkých jemu dostupných zdrojov, ako aj zoznam všetkých úloh spustených týmto užívateľom momentálne v systéme.";

$str_sta = "Stav úlohy: podľa Gridového Manažéra (GM) a systému správy lokálnych zdrojov (LRMS). Poradie možných stavov je na  nasledujúce:
   <dl>
     <dt>ACCEPTED &ndash; úloha je prijatá, ale jej vykonávanie ešte nezačalo</dt>
     <dt>PREPARING &ndash; sťahujú sa vstupné súbory</dt>
     <dt>SUBMITTING &ndash; informácie sa posielajú do LRMS</dt>
     <dt>INLRMS &ndash; správa úlohy predaná LRMS; informačný systém zabezpečuje informácie o vnútornom stave úlohy. Možné sú nasledujúce stavy:</dt>
       <dd>: Q &ndash; úloha čaká v rade
       <br>: U &ndash; úloha je pozastavená na preťaženom pracovnom uzle (PBSPro)
       <br>: S &ndash; úloha je pozastavená (Condor)
       <br>: R, run &ndash; úloha sa vykonáva
       <br>: E &ndash; úloha sa ukončuje (PBS)
     </dd>
     <dt>FINISHING &ndash; výstupné súbory sú prenášané na miesto určenia</dt>
     <dt>FINISHED &ndash; úloha je ukončená; čas ukončenia je stanovený informačným systémom</dt>
     <dt>CANCELING &ndash; úloha sa ruší</dt>
     <dt>DELETED &ndash; výstupy úlohy nezmazané užívateľom, ale zmazané GM po expiračnej dobe</dt>
   </dl>
   Ku každému stavu môže byť pridaná predpona \"PENDING:\", ktorá značí,
   že GM práve nemôže prejsť k nasledujúcemu kroku v dôsledku vnútorných obmedzení.";

$str_tim = "CPU čas spotrebovaný úlohou, v minútach.";

$str_mem = "Rozsah operačnej pamäte využívanej úlohou, v KB.";

$str_cpu = "Počet CPU využívaných úlohou.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    V tomto okne sa zobrazujú všetky výpočtové zdroje, ktoré sa registrujú do najvyššieho indexovacieho servisu <i>ARC</i>. Tabuľka je usporiadaná podľa anglického názvu príslušnej krajiny a v sekcii pre danú krajinu podľa názvu hlavného stroja daného zdroja (výpočtového klástra). Pre každý kláster sa uvádzajú nasledujúce parametre: názov, celkové množstvo procesorov, počet obsadených procesorov a tiež počet bežiacich a čakajúcich úloh tak spustených cez Grid ako aj lokálne. Použite utilitu &quot;Vyhľadávanie&quot; pre prezeranie a porovnanie s parametrami ostatných klástrov, rád, úloh atď.
                                                  </dt>
                                                  <dt><b>Krajina</b></dt>
                                                  <dd>".$clickable.". Vlajka a názov krajiny. Názov krajiny určený z dostupného popisu výpočtového zdroja. Kliknite pre zobrazenie zdrojov výlučne z danej krajiny.</dd>
                                                  <dt><b>Zdroj</b></dt>
                                                  <dd>".$clickable.".  Názov zdroja (obyčajne výpočtového klástra) daný jeho majiteľom. Maximálna dĺžka zobrazeného reťazca je 22 znakov. Kliknite na názov pre podrobné informácie o zdroji.</dd>
                                                  <dt><b>CPU</b></dt>
                                                  <dd>Celkový počet CPU zdroja. <b>Pozor!</b> Je možné, že iba časť je využiteľná cez Grid.</dd>
                                                  <dt><b>Záťaž (procesy)</b></dt>
                                                  <dd>".$clickable.". Relatívne využitie zdroja, zodpovedajúce počtu zaťažených CPU. Sivá úsečka zodpovedá počtu procesorov obsadených lokálnymi úlohami, zelená úsečka zodpovedá procesorom vykonávajúcim Gridové úlohy. Kliknite pre podrobné informácie o všetkých Gridových úlohách bežiacich na zdroji, vrátane informácií o počte procesorov na každú úlohu.</dd>
                                                  <dt><b>Čakajúce</b></dt>
                                                  <dd>".$clickable.". Počet všetkých úloh čakajúcich v rade na danom zdroji, uvádzaný ako súčet Gridových a lokálnych úloh. Kliknite na prvé z čísel pre podrobné informácie o všetkých Gridových úlohách čakajúcich v rade.</dd>
                                                </dl>
                                                </div>",
                                      "Krajina"                      => 30,
                                      "Zdroj"                        => 160,
                                      "Počet CPU"                    => 10,
                                      "Záťaž (procesy: Grid+lokálne)" => 210,
                                      "Čakajúce"                     => 10
                                      ),
                  "clusdes"  => array(
                                      "0" => "Opis zdroja",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Atribút</b></dt>
                                                  <dd>".$clickable.". Názov atribútov zdroja".$str_att."</dd>
                                                  <dt><b>Hodnota</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Rada</b></dt>
                                                  <dd>".$clickable.". Názvy (dané majiteľom zdroja) rád, dostupných Gridovým užívateľom. ".$str_que."</dd>
                                                  <dt><b>Stav</b></dt>
                                                  <dd>Stav rady. Fungujúca rada obyčajne udáva stav <i>active</i>.</dd>
                                                  <dt><b>Trvanie (min)</b></dt>
                                                  <dd>Limit trvania úlohy v danej rade - ak je limit stanovený - v minútach procesorového času. Prvá zobrazená hodnota je dolný limit, druhá predstavuje horné ohraničenie tohto parametra. Ak limity nie so stanovené (úlohy ľubovoľného času trvania sú akceptované), zobrazí sa reťazec <b>N/A</b>.</dd>
                                                  <dt><b>Bežiace</b></dt>
                                                  <dd>Počet úloh spracovávaných v rade. Zobrazuje sa celkový počet úloh, pričom počet <b>procesorov</b> obsadených Gridovými úlohami je uvedený v zátvorkách, napr. (Grid: 12). Pozor! Pri paralelných mnohoprocesorových úlohách môže byť číslo v zátvorkách väčšie ako počet úloh.</dd>
                                                  <dt><b>Čakajúce</b></dt>
                                                  <dd>Počet úloh čakajúcich na spustenie v rade. Zobrazuje sa celkový počet úloh, pričom množstvo úloh spustených cez Grid je uvedené v zátvorkách, napr. (Grid: 235)</dd>
                                                </dl>
                                                </div>",
                                      "Rada"                         => 0,
                                      "Stav"                         => 0,
                                      "Trvanie (min)"                => 0,
                                      "Počet CPU"                    => 0,
                                      "Bežiace"                      => 0,
                                      "Čakajúce"                     => 0
                                      ),
                  "jobstat"  => array(
                                      "0" => "Úlohy na:identifikátor úlohy",
                                      "help" => "<div align=\"left\">
                                                <b>ZOZNAM ÚLOH:</b>
                                                <dl>
                                                <dt><b>Názov úlohy</b></dt>
                                                <dd>".$clickable.". Názov úlohy daný užívateľom. &quot;<span
                                                    style={color:red;}>N/A</span>&quot; znamená, že užívateľ úlohe názov nepriradil. Po kliknutí sa zobrazí detailný popis úlohy.</dd>
                                                <dt><b>Užívateľ</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Stav</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU čas (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Rada</b></dt>
                                                <dd>".$clickable.". Názov rady, v ktorej sa úloha vykonáva. ".$str_que."</dd>
                                                <dt><b>Procesory</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>OPIS ÚLOHY:</b>
                                                <dl>
                                                <dt><b>Atribút</b></dt>
                                                <dd>".$clickable.". Názvy atribútov úlohy".$str_att."</dd>
                                                <dt><b>Hodnota</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>",
                                      "Názov úlohy"                  => 0,
                                      "Užívateľ"                     => 0,
                                      "Stav"                         => 0,
                                      "CPU čas (min)"                => 0,
                                      "Rada"                         => 0,
                                      "Počet CPU"                    => 0
                                      ),
                  "volist"   => array(
                                      "0" => "Virtuálne organizácie",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuálne organizácie</b></dt>
                                                   <dd>".$clickable.". Skupina užívateľov, obyčajne zdieľajúca spoločné aktivity a zdroje, autorizovaná na aspoň jednom zdroji zapojenom v <i>ARC</i>. Po kliknutí sa zobrazí zoznam členov skupiny.</dd>
                                                   <dt><b>Členovia</b></dt>
                                                   <dd>Počet členov skupiny.</dd>
                                                   <dt><b>Obsluhuje sa</b></dt>
                                                   <dd>Adresa servera spravujúceho databázu s údajmi o členoch skupiny.</dd>
                                                 </dl>
                                                 </div>",
                                      "Virtuálna organizácia"        => 0,
                                      "Členovia"                     => 0,
                                      "Obsluhuje sa"                 => 0
                                      ),
                  "vousers"  => array(
                                      "0" => "Užívatelia",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Meno</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Pracovisko</b></dt>
                                                   <dd>Pracovisko užívateľa, podľa zápisu v databáze. Nepovinný údaj.</dd>
                                                   <dt><b>Elektronická pošta</b></dt>
                                                   <dd>".$clickable.". Adresa elektronickej poštovej schránky užívateľa, podľa zápisu v databáze. Nepovinný údaj. Kliknite na adresu pre poslanie správy užívateľovi.</dd>
                                                 </dl>
                                                 </div>",
                                      "#"                            => 0,
                                      "Meno"                         => 0,
                                      "Pracovisko"                   => 0,
                                      "Elektronická pošta"           => 0
                                      ),
                  "userlist" => array(
                                      "0" => "Informácia pre",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Zdroj:rada</b></dt>
                                                <dd>".$clickable.". Názvy zdrojov (klástrov) a zodpovedajúcich rád lokálnych systémov správy úloh (LRMS) (oddelené dvojbodkou,&quot;:&quot;), na ktorých je užívateľ oprávnený posielať úlohy. Ak užívateľ nie je oprávnený, objaví sa správa: &quot;<b><font color=\"red\">Not authorised at host ...</font></b>&quot;. Po kliknutí na názov klástra sa zobrazí jeho podrobný opis. Pre získanie podrobností o rade kliknite na jej názov. </dd>
                                                <dt><b>Voľné CPU</b></dt>
                                                <dd>Počet voľných CPU v danej rade, pre daného užívateľa, v danom čase. V prípade, že rada využíva časové ohraničenia na prípustné trvanie behu úlohy, je tento údaj zobrazený za číslom reprezentujúcim počet procesorov (v minútach, oddelený dvojbodkou). Napríklad &quot;3&quot; znamená dostupnosť troch voľných CPU pre úlohy s akoukoľvek dĺžkou trvania; &quot;4:360&quot; označuje dostupnosť štyroch voľných CPU, pre úlohy s dobou trvania behu nie dlhšou ako šesť hodín; &quot;10:180:30&quot; znamená, že je dostupných desať  CPU pre úlohy s trvaním nepresahujúcim 3 hodiny a ďalších tridsať procesorov, ktoré môžu prijať úlohy s neobmedzeným časom behu; &quot;0&quot; znamená, že v danom momente nie sú žiadne voľné CPU a úlohy budú zaradené do príslušnej rady ako čakajúce.</dd>
                                                <dt><b>Úlohy v rade</b></dt>
                                                <dd>Počet úloh užívateľa v zozname čakajúcich úloh pred novou úlohou, zaslanou daným užívateľom. Počet &quot;0&quot; znamená, že úloha by sa mala začať vykonávať okamžite. <b>POZOR!</b> Je to odhad, ktorý nemusí zohľadňovať všetky lokálne nastavenia správcu zdroja.</dd>
                                                <dt><b>Voľný diskový priestor (MB)</b></dt>
                                                <dd>Diskový priestor dostupný pre užívateľa v danej rade (v megabajtoch). <b>POZOR!</b> Ide len o odhad, väčšina zdrojov nepodporuje resp. nevyužíva kvóty na diskový priestor.</dd>
                                                <dt><b>Názov úlohy</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Stav</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>Čas (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Zdroj</b></dt>
                                                <dd>".$clickable.". Názov zdroja (obyčajne klástra), na ktorom sa úloha vykonáva. Po kliknutí sa zobrazia podrobné informácie o zdroji.</dd>
                                                <dt><b>Rada</b></dt>
                                                <dd>".$clickable.". Názov rady v LRMS, v ktorej sa vykonávajúcej úlohu. ".$str_que."</dd>
                                                <dt><b>Počet CPU</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>",
                                      ""                             => 0,
                                      "Názov úlohy"                  => 0,
                                      "Stav"                         => 0,
                                      "Čas (min)"                    => 0,
                                      "Zdroj"                        => 0,
                                      "Rada"                         => 0,
                                      "Počet CPU"                    => 0
                                      ),
                  "attlist"  => array(
                                      "0" => "Attribute values",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objekt</b></dt>
                                                   <dd>".$clickable.". Názov objektu, atribúty ktorého sú zobrazené. Môže ísť o názov rady klástra, názov úlohy, meno užívateľa atď. Po kliknutí sa zobrazia podrobné informácie o objekte.</dd>
                                                   <dt><b>Atribút</b></dt>
                                                   <dd>Pre každý objekt v tabuľke je možné uviesť jeden alebo viacero atribútov. V hlavičke stĺpca je uvedený názov atribútu, upravený do jednoducho čitateľnej formy (s výnimkou niekoľkých atribútov špecifických pre systém MDS), obsah jednotlivých stĺpcov predstavujú význam týchto atribútov podľa toho, ako sú popísané v Informačnom Systéme.</dd>
                                                 </dl>
                                                 </div>",
                                      "Objekt"                       => 0,
                                      "Atribút"                      => 0
                                      ),
                  "quelist"  => array(
                                      "0" => "Popis rady",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Atribút</b></dt>
                                                   <dd>".$clickable.". Názvy atribútov rady".$str_att."</dd>
                                                   <dt><b>Hodnota</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Názov úlohy</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Majiteľ</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Stav</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU čas (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Pamäť (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>Počet CPU</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>",
                                      ""                             => 0,
                                      "Názov úlohy"                  => 0,
                                      "Majiteľ"                      => 0,
                                      "Stav"                         => 0,
                                      "CPU čas (min)"                => 0,
                                      "Operačná pamäť (KB)"          => 0,
                                      "Počet CPU"                    => 0
                                      ),
                  "sestat"   => array(
                                      "0" => "Úložné zariadenia",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Názov</b></dt>
                                                  <dd>Názov úložného zariadenia zaregistrovaný v Informačnom Systéme. Zobrazených maximálne 15 znakov.</dd>
                                                  <dt><b>Celková kapacita</b></dt>
                                                  <dd>Celkový diskový priestor v GB.</dd>
                                                  <dt><dt><b>Voľná kapacita</b></dt>
                                                  <dd>Momentálne dostupný diskový priestor v GB.</dd>
                                                  <dt><b>Názov</b></dt>
                                                  <dd>Názov úložného zariadenia skladajúci sa z názvu logickej jednotky a názvu servera (rozdelených dvojbodkou). Logický názov sa využíva len pre účely Informačného Systému pre zjednodušenie rozoznávania rôznych úložných zariadení, nachádzajúcich sa na jednom a tom istom servere.</dd>
                                                  <dt><b>Bázová URL</b></dt>
                                                  <dd>URL úložného zariadenia, obyčajne využívajúc protokol gsiftp. Použite túto URL ako bázu pre prístup k súborom.</dd>
                                                  <dt><b>Typ</b></dt>
                                                  <dd>Typ úložného zariadenia. Typ &quot;gridftp-based&quot; označuje úložnú jednotku dostupnú cez GridFTP rozhranie.</dd>
                                                </dl>
                                                </div>",
                                      "#"                            => 0,
                                      "Názov"                        => 0,
//                                      "Celková kapacita"                   => 0,
                                      "Voľná/celková kapacita v GB"   => 0,
                                      "Názov"                        => 0,
                                      "Bázová URL"                   => 0,
                                      "Typ"                          => 0
                                      ),
                  "allusers" => array(
                                      "0" => "Autorizovaný užívatelia:Aktívny užívatelia",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Meno</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Pracovisko</b></dt>
                                                   <dd>Pracovisko užívateľa, podľa informácií v jeho osobnom certifikáte</dd>
                                                   <dt><b>Úlohy</b></dt>
                                                   <dd>Počet všetkých užívateľových úloh v systéme (bežiacich, čakajúcich, ukončených a vymazaných)</dd>
                                                   <dt><b>Zdroje</b></dt>
                                                   <dd>Počet klástrov, na ktoré má daný užívateľ prístup</dd>
                                                 </dl>
                                                 </div>",
                                      "#"                            => 0,
                                      "Meno"                         => 0,
                                      "pracovisko"                   => 0,
                                      "Úlohy"                        => 0,
                                      "Zdroje"                       => 0
                                      ),
                  "userres"  => array(
                                      "0" => "",
                                      "Zdroj:rada"                   => 0,
                                      "Voľné CPU"                    => 0,
                                      "Úlohy v rade"                 => 0,
                                      "Voľný diskový priestor (MB)"  => 0
                                      ),
                  "ldapdump" => array(
                                      "0" => "",
                                      "Atribút"                      => 0,
                                      "Hodnota"                      => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Údaje platné od (GMT)",
                                      "Mds-validto"   => "Údaje platné do (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Meno hlavného stroja",
                                      "nordugrid-cluster-aliasname"          => "Názov",
                                      "nordugrid-cluster-contactstring"      => "Kontaktná adresa",
                                      "nordugrid-cluster-interactive-contactstring" => "Interaktívna adresa",
                                      "nordugrid-cluster-comment"            => "Komentár",
                                      "nordugrid-cluster-support"            => "Elektronická adresa zodpovednej osoby",
                                      "nordugrid-cluster-acl"                => "Autorizované VO",
                                      "nordugrid-cluster-lrms-type"          => "typ LRMS",
                                      "nordugrid-cluster-lrms-version"       => "verzia LRMS",
                                      "nordugrid-cluster-lrms-config"        => "podrobnosti o LRMS",
                                      "nordugrid-cluster-architecture"       => "Architektúra",
                                      "nordugrid-cluster-opsys"              => "Operačný systém",
                                      "nordugrid-cluster-homogeneity"        => "Homogenita klástra",
                                      "nordugrid-cluster-nodecpu"            => "Typ procesoru (najslabšieho)",
                                      "nordugrid-cluster-nodememory"         => "Pamäť (MB, najmenšia)",
                                      "nordugrid-cluster-totalcpus"          => "Počet CPU celkovo",
                                      "nordugrid-cluster-cpudistribution"    => "Počet CPU na jeden uzol",
                                      "nordugrid-cluster-benchmark"          => "Etalónový test (Benchmark)",
                                      "nordugrid-cluster-sessiondir-free"    => "Diskový priestor, dostupný (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Diskový priestor, celkový (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Doba života gridovej úlohy (min)",
                                      "nordugrid-cluster-cache-free"         => "Kešová pamäť, dostupná (MB)",
                                      "nordugrid-cluster-cache-total"        => "Kešová pamäť, celková (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Pracovné prostredie",
                                      "nordugrid-cluster-localse"            => "Lokálne úložné zariadenie",
                                      "nordugrid-cluster-middleware"         => "Gridové rozhranie (middleware)",
                                      "nordugrid-clAliasuster-totaljobs"     => "Úlohy, celkovo",
                                      "nordugrid-cluster-usedcpus"           => "CPU, obsadené",
                                      "nordugrid-cluster-queuedjobs"         => "Úlohy v rade (ZASTARANÉ)",
                                      "nordugrid-cluster-prelrmsqueued"      => "Gridové úlohy čakajúce na zaslanie",
                                      "nordugrid-cluster-location"           => "Poštové smerovacie číslo",
                                      "nordugrid-cluster-owner"              => "Majiteľ",
                                      "nordugrid-cluster-issuerca"           => "Certifikačná autorita",
                                      "nordugrid-cluster-issuerca-hash"      => "Hash-kód certifikačnej autority",
                                      "nordugrid-cluster-trustedca"          => "Akceptované certifikačné autority",
                                      "nordugrid-cluster-nodeaccess"         => "IP-konektivita uzlov",
                                      "nordugrid-cluster-gridarea"           => "Priestor gridovej úlohy (ZASTARANÉ)",
                                      "nordugrid-cluster-gridspace"          => "Gridový diskový priestor (ZASTARANÉ)",
                                      "nordugrid-cluster-opsysdistribution"  => "Distribúcia OS (ZASTARANÉ)",
                                      "nordugrid-cluster-runningjobs"        => "Bežiace úlohy (ZASTARANÉ)",
                                      "nordugrid-queue-name"                 => "Názov rady",
                                      "nordugrid-queue-comment"              => "Komentár",
                                      "nordugrid-queue-status"               => "Stav rady",
                                      "nordugrid-queue-running"              => "Všetky obsadené CPU",
                                      "nordugrid-queue-localqueued"          => "Lokálne úlohy v rade",
                                      "nordugrid-queue-prelrmsqueued"        => "Gridové úlohy čakajúce na zaslanie do rady",
                                      "nordugrid-queue-queued"               => "Úlohy v rade (ZASTARANÉ)",
                                      "nordugrid-queue-maxrunning"           => "Bežiace úlohy (max)",
                                      "nordugrid-queue-maxqueuable"          => "Počet úloh v rade (max)",
                                      "nordugrid-queue-maxuserrun"           => "Počet úloh na užívateľa (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU čas, maximum (min.)",
                                      "nordugrid-queue-mincputime"           => "CPU čas, minimum (min.)",
                                      "nordugrid-queue-defaultcputime"       => "CPU čas, bez udania (min.)",
                                      "nordugrid-queue-maxwalltime"          => "Trvanie, maximum (min.)",
                                      "nordugrid-queue-minwalltime"          => "Trvanie, minimum (min.)",
                                      "nordugrid-queue-defaultwalltime"      => "Trvanie, bez udania (min.)",
                                      "nordugrid-queue-schedulingpolicy"     => "Pravidlá rozvrhu úloh",
                                      "nordugrid-queue-totalcpus"            => "Celkový počet CPU",
                                      "nordugrid-queue-nodecpu"              => "Typ CPU",
                                      "nordugrid-queue-nodememory"           => "Operačná pamäť (MB)",
                                      "nordugrid-queue-architecture"         => "Architektúra",
                                      "nordugrid-queue-opsys"                => "Operačný systém",
                                      "nordugrid-queue-homogeneity"          => "Homogenita rady",
                                      "nordugrid-queue-gridrunning"          => "CPU obsadené Gridovými úlohami",
                                      "nordugrid-queue-gridqueued"           => "Gridové úlohy v rade",
                                      "nordugrid-queue-benchmark"            => "Etalónový test - Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "Počet CPU v rade (ZASTARANÉ)",
                                      "nordugrid-queue-assignedcputype"      => "Typ CPU v rade (ZASTARANÉ)",
                                      "nordugrid-job-globalid"               => "Identifikátor",
                                      "nordugrid-job-globalowner"            => "Majiteľ",
                                      "nordugrid-job-execcluster"            => "Vykonávajúci kláster",
                                      "nordugrid-job-execqueue"              => "Vykonávajúca rada",
                                      "nordugrid-job-stdout"                 => "Štandardný výstup",
                                      "nordugrid-job-stderr"                 => "Štandardný chybový výstup",
                                      "nordugrid-job-stdin"                  => "Štandardný vstup",
                                      "nordugrid-job-reqcputime"             => "Požadovaná CPU čas",
                                      "nordugrid-job-reqwalltime"            => "Požadovaný čas trvania",
                                      "nordugrid-job-status"                 => "Stav úlohy",
                                      "nordugrid-job-queuerank"              => "Pozícia úlohy v rade",
                                      "nordugrid-job-comment"                => "LRMS komentár",
                                      "nordugrid-job-submissionui"           => "Stroj, z ktorého bola úloha zaslaná",
                                      "nordugrid-job-submissiontime"         => "Čas zaslania (GMT)",
                                      "nordugrid-job-usedcputime"            => "Použitý CPU čas",
                                      "nordugrid-job-usedwalltime"           => "Doterajšie trvanie úlohy",
                                      "nordugrid-job-completiontime"         => "Čas ukončenia (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Čas vymazania (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Čas vypršania proxy certifikátu (GMT)",
                                      "nordugrid-job-usedmem"                => "Použitá pamäť (KB)",
                                      "nordugrid-job-errors"                 => "Chyby",
                                      "nordugrid-job-exitcode"               => "Návratová hodnota",
                                      "nordugrid-job-jobname"                => "Názov",
                                      "nordugrid-job-runtimeenvironment"     => "Pracovné prostredie",
                                      "nordugrid-job-cpucount"               => "Požadovaný počet CPU",
                                      "nordugrid-job-executionnodes"         => "Vykonávajúce uzly",
                                      "nordugrid-job-gmlog"                  => "GM log súbor",
                                      "nordugrid-job-clientsoftware"         => "Verzia klienta",
                                      "nordugrid-job-rerunable"              => "Znovuspustiteľnosť",
                                      "nordugrid-job-reqcput"                => "Požadovaný čas (ZASTARANÉ)",
                                      "nordugrid-job-gridlog"                => "Gridlog súbor (ZASTARANÉ)",
                                      "nordugrid-job-lrmscomment"            => "LRMS komentár (ZASTARANÉ)",
                                      "nordugrid-authuser-name"              => "Meno",
                                      "nordugrid-authuser-sn"                => "Subjekt",
                                      "nordugrid-authuser-freecpus"          => "Voľné CPU",
                                      "nordugrid-authuser-diskspace"         => "Voľný úložný priestor (MB)",
                                      "nordugrid-authuser-queuelength"       => "Úlohy užívateľa v rade",
                                      "nordugrid-se-name"                    => "Celý názov",
                                      "nordugrid-se-aliasname"               => "Názov",
                                      "nordugrid-se-type"                    => "Typ",
                                      "nordugrid-se-acl"                     => "Autorizované VO",
                                      "nordugrid-se-freespace"               => "Voľný priestor (MB)",
                                      "nordugrid-se-totalspace"              => "Celkový priestor (MB)",
                                      "nordugrid-se-url"                     => "Kontaktná URL adresa",
                                      "nordugrid-se-baseurl"                 => "Kontaktná základná URL adresa (ZASTARANÉ)",
                                      "nordugrid-se-accesscontrol"           => "Kontrola prístupu",
                                      "nordugrid-se-authuser"                => "Autorizovaný užívateľ (DN)",
                                      "nordugrid-se-location"                => "Poštové smerovacie číslo",
                                      "nordugrid-se-owner"                   => "Majiteľ",
                                      "nordugrid-se-middleware"              => "Gridové rozhranie",
                                      "nordugrid-se-issuerca"                => "Certifikačná autorita",
                                      "nordugrid-se-issuerca-hash"           => "Hash-kód certifikačnej autority",
                                      "nordugrid-se-trustedca"               => "Akceptované certifikačné autority",
                                      "nordugrid-se-comment"                 => "Komentár",
                                      "nordugrid-rc-name"                    => "Názov domény",
                                      "nordugrid-rc-aliasname"               => "Názov",
                                      "nordugrid-rc-baseurl"                 => "Kontaktná URL adresa",
                                      "nordugrid-rc-authuser"                => "Autorizovaní užívatelia (DN)",
                                      "nordugrid-rc-location"                => "Poštové smerovacie číslo",
                                      "nordugrid-rc-owner"                   => "Majiteľ",
                                      "nordugrid-rc-issuerca"                => "Certifikačná autorita"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Nemožno prečítať údaje z indexu vyššej úrovne",
                                      "2"   => "Žiaden z lokálnych indexov neodpovedá",
                                      "3"   => " nesprávna konfigurácia alebo uplynul čas požiadavky",
                                      "4"   => "Žiadna gridová úloha",
                                      "5"   => "Žiadna informácia",
                                      "6"   => "Služba je nedostupná",
                                      "7"   => " - pokúste sa obnoviť neskôr",
                                      "8"   => "Informácie o rade nedostupné",
                                      "9"   => "Žiadne údaje",
                                      "11"  => "Žiadny užívatelia",
                                      "11"  => "Neautorizovaný na danom zdroji",
                                      "12"  => "neodpovedá",
                                      "13"  => "Momentálne niet úloh od daného užívateľa ",
                                      // debug messages
                                      "101" => " Časový limit pre spojenie s lokálnym indexom: ",
                                      "102" => " s pre spojenie a ",
                                      "103" => " s pre hľadanie",
                                      "104" => " s strávených hľadaním",
                                      "105" => "Zobrazenie zdrojov výlučne v ",
                                      "106" => "Dopytované indexy vyššej úrovne: ",
                                      "107" => "Prijaté geografické koordináty zdrojov, preskenované zdroje: ",
                                      "108" => " zdrojov usporiadaných podľa geografickej polohy",
                                      "109" => "Vyhľadávanie atribútov klástra",
                                      "110" => "Vyhľadávanie atribútov rady",
                                      "111" => "Niet údajov od ",
                                      "112" => " fungujúcich v krajine:  ",
                                      "113" => " žiadne ponúkané zdroje",
                                      "114" => " Časový limit pre spojenie s globálnym indexom: ",
                                      "115" => "Ignoruje sa zdroj: ",
                                      "116" => "nezodpovedá typu ",
                                      "117" => "Preverovanie spojenia: ",
                                      "118" => "V poriadku",
                                      "119" => "Doteraz objavených zdrojov typu ",
                                      "120" => "Chyba LDAP pri hľadaní ",
                                      "121" => " stav v ",
                                      "122" => "Zablokované: ",
                                      "123" => "Objavený registrant ",
                                      "124" => "Vyhľadávanie atribútov úložných zariadení",
                                      "125" => "Vyhľadávanie užívateľov",
                                      "126" => "Vyhľadávanie úloh",
                                      "127" => " spustil úlohu ",
                                      "128" => " nemajúc autorizáciu",
                                      "129" => "Niet údajov o objekte: chyba ",
                                      // icon titles
                                      "301" => "Obnoviť",
                                      "302" => "Tlač",
                                      "303" => "Pomoc",
                                      "304" => "Zatvoriť",
                                      "305" => "Zelená",
                                      "306" => "Sivá",
                                      "307" => "Všetci užívatelia",
                                      "308" => "Aktívny užívatelia",
                                      "309" => "Vyhľadávanie",
                                      "310" => "Úložiská",
                                      "311" => "Virtuálne organizácie",
                                      "312" => "Vlajka krajiny: ",
                                      "313" => " (gridové procesy), ",
                                      "314" => " (lokálne procesy)",
                                      // auxilliary strings
                                      "401" => "Procesy",
                                      "402" => "Grid",
                                      "403" => "Lokálne",
                                      "404" => "Svet",
                                      "405" => "CELKOVO",
                                      "406" => " zdrojov",
                                      "407" => "mnoho",
                                      "408" => " GB",
                                      "409" => " VŠETKY",
                                      "410" => "Kláster",
                                      "411" => "Rada",
                                      "412" => "Úloha",
                                      "413" => "Užívateľ",
                                      "414" => "Úložisko",
                                      "415" => "Katalóg replík",
                                      "416" => "Zadajte atribúty, ktoré sa majú zobraziť pre vybraný objekt: ",
                                      "417" => "Vyhľadávanie sa vykonáva pre logické A všetkých zadaných výrazov",
                                      "418" => "Ponechajte pravé pole prázdne ak filter nie je potrebný",
                                      "419" => "Prezeranie zdrojov alebo objektov podľa výberu",
                                      "420" => "Plný názov (DN)",
                                      "421" => "Môže použiť celkovo ",
                                      "422" => " zdrojov",
                                      "423" => "Zdroj / objekt:",
                                      "424" => "Počet atribútov (6 automaticky):",
                                      "425" => "Objekt",
                                      "426" => "Ďalší",
                                      "427" => "Vyberte",
                                      "428" => "Znovunačítať",
                                      "429" => "UKÁZAŤ"
                                      ),
                  // Post code conversion: only for [en]!
                  "tlconvert" => array (
                                        "AU"  => "Austrália",
                                        "AT"  => "Rakúsko",
                                        "AM"  => "Arménsko",
                                        "DZ"  => "Alžírsko",
                                        "BE"  => "Belgicko",
                                        "BG"  => "Bulharsko",
                                        "CA"  => "Kanada",
                                        "CN"  => "Čína",
                                        "CZ"  => "Česko",
                                        "DK"  => "Dánsko",
                                        "EE"  => "Estónsko",
                                        "FI"  => "Fínsko",
                                        "FIN" => "Fínsko",
                                        "SF"  => "Fínsko",
                                        "FR"  => "Francúzsko",
                                        "GE"  => "Gruzínsko",
                                        "DE"  => "Nemecko",
                                        "D"   => "Nemecko",
                                        "GR"  => "Grécko",
                                        "HU"  => "Maďarsko",
                                        "IS"  => "Island",
                                        "IR"  => "Írsko",
                                        "IE"  => "Írsko",
                                        "IT"  => "Taliansko",
                                        "JP"  => "Japonsko",
                                        "KEK" => "Japonsko",
                                        "TOKYO" => "Japonsko",
                                        "LV"  => "Lotyšsko",
                                        "LT"  => "Litva",
                                        "MA"  => "Maroko",
                                        "NL"  => "Holandsko",
                                        "NO"  => "Nórsko",
                                        "N"   => "Nórsko",
                                        "PL"  => "Poľsko",
                                        "PT"  => "Portugalsko",
                                        "RO"  => "Rumunsko",
                                        "RU"  => "Rusko",
                                        "LK"  => "Srí Lanka",
                                        "SE"  => "Švédsko",
                                        "SK"  => "Slovensko",
                                        "SI"  => "Slovinsko",
                                        "CH"  => "Švajčiarsko",
                                        "TR"  => "Turecko",
                                        "UK"  => "UK",
                                        "UA"  => "Ukrajina",
                                        "COM" => "USA",
                                        "GOV" => "USA",
                                        "USA" => "USA",
                                        "US"  => "USA",
                                        "Australia"    => "Austrália",
                                        "Austria"      => "Rakúsko",
                                        "Armenia"      => "Arménsko",
                                        "Algeria"      => "Alžírsko",
                                        "Belgium"      => "Belgicko",
                                        "Bulgaria"     => "Bulharsko",
                                        "Canada"       => "Kanada",
                                        "China"        => "Čína",
                                        "Czechia"      => "Česko",
                                        "Denmark"      => "Dánsko",
                                        "Estonia"      => "Estónsko",
                                        "Finland"      => "Fínsko",
                                        "France"       => "Francúzsko",
                                        "Georgia"      => "Gruzínsko",
                                        "Germany"      => "Nemecko",
                                        "Greece"       => "Grécko",
                                        "Hungary"      => "Maďarsko",
                                        "Iceland"      => "Island",
                                        "Ireland"      => "Írsko",
                                        "Italy"        => "Taliansko",
                                        "Japan"        => "Japonsko",
                                        "Latvia"       => "Lotyšsko",
                                        "Lithuania"    => "Litva",
                                        "Morocco"      => "Maroko",
                                        "Netherlands"  => "Holandsko",
                                        "Norway"       => "Nórsko",
                                        "Poland"       => "Poľsko",
                                        "Portugal"     => "Portugalsko",
                                        "Romania"      => "Rumunsko",
                                        "Russia"       => "Rusko",
                                        "SriLanka"     => "Srí Lanka",
                                        "Sweden"       => "Švédsko",
                                        "Slovakia"     => "Slovensko",
                                        "Slovenia"     => "Slovinsko",
                                        "Switzerland"  => "Švajčiarsko",
                                        "Turkey"       => "Turecko",
                                        "UK"           => "Veľká Británia",
                                        "Ukraine"      => "Ukrajina",
                                        "USA"          => "USA",
                                        "World"        => "Svet"
                                        )
                  );

?>
