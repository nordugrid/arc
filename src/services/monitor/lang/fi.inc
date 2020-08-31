<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- Finnish [fi]
// -- Translation: Marko Niinimäki (marko.niinimaki@cern.ch), Olli Tourunen (olli.tourunen@csc.fi)
// -- Author: oxana.smirnova@hep.lu.se

// Some common strings:

$clickable = "CLICKABLE";

$str_att = ", lukukelpoista tekstiä paitsi jotkin MDS-ominaisuudet. Valitse 
ominaisuuden nimi jos haluat listan arvoista siten kuin ne esiintyvät 
ARCissa.";

$str_val = "Ominaisuuksien arvot (tietojärjestelmässä).";

$str_que = "Yleensä jono on viritetty siten että siinä on kestoltaan samanlaisia töita tai samanlaisten käyttäjien töitä. Valitse jonon nimi jos haluat tietoa jonosta, kuten ajossa olevat, jonottavat ja loppuun suoritetut työt.";

$str_job = "Tyon omistajan tyolleen antama nimi.
          <br> <font color=\"red\">N/A</font> tarkoittaa: käyttäjä ei antanut tyolle nimeä.
          <br> <font color=\"red\"><b>X</b></font> tarkoittaa: käyttäjä tappoi tyonsä.
          <br> <font color=\"red\"><b>!</b></font> tarkoittaa: tyon suoritus epäonnistui.
          <br> Valitse tyon nimi jos haluat tyon tarkemmat tiedot.";

$str_nam = "Käyttäjän nimi, siten kuin se on käyttäjän varmenteessa. Valitse käyttäjän nimi jos haluat tietoa resursseista jotka ovat hänen käytettävissään ja käyttäjän ajossa olevista toistä.";

$str_sta = "Työn tila, siten kuin Grid Manager (GM) ja
   jonosuoritusohjelma (LRMS) sen kertoivat. Tilat ovat:
   <dl>
     <dt>ACCEPTED &ndash; tyo lähetetty</dt>
     <dt>PREPARING &ndash; haetaan syötetiedostoja</dt>
     <dt>SUBMITTING &ndash; lähetys jononsuoritusohjelmaan (LRMS) menossa</dt>
     <dt>INLRMS &ndash; tyo on jononsuoritusohjelman armoilla; 
	Tietojärjestelmä lisää seuraavat LRMSn sisäiset tilat:
       </dt>
       <dd>: Q &ndash; jonossa,
       <br>: U &ndash; jono on jaädytetty väliaikaisesti koska tietokone on kuormitettu (PBSPro)
       <br>: S &ndash; jono on jäädytty (Condor)
       <br>: R, run &ndash; työtä suoritetaan
       <br>: E &ndash; tyo on loppuvaiheessa (PBS)
     </dd>
     <dt>FINISHING &ndash; GM siirtää tyon tulostiedostoja</dt>
     <dt>FINISHED &ndash; tyo suoritettu loppuun; tietojärjestelmä lisää aikaleimaa</dt>
     <dt>CANCELING &ndash; tyo peruutetaan</dt>
     <dt>DELETED &ndash; käyttäjä ei siirtanyt tulosteita, GM poisti ne
       koska maksimiaika ylittyi</dt>
   </dl>
   Kaikkiin tiloihin voi liittya PENDING: -etuliite, joka
   tarkoittaa etta GM yrittää siirtää työtä seuraavaan tilaan";

$str_tim = "Tyon käyttämä prosessoriaika minuutteina.";

$str_mem = "Tyon käyttämä muisti, KB";

$str_cpu = "Tyon käyttämien prosessorien lukumäärä.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    Kohteet jotka rekisteroityvät ARCin luettelopalveluun lajiteltuna maan ja tietokoneen nimen mukaisesti.
                                                    Kohteista rekisteroidään seuraavat ominaisuudet: klusterin alias-nimi, prosessorikapasiteetti, ajossa olevat ja jonottavat työt (sekä Grid-toiminnoilla lahetetyt etta paikalliset).
                                                    Käytä &quot;Search&quot;
                                                    toimintoa jos haluat vertailla muita klusterin, jonon tai tyon ominaisuuksia
                                                  </dt>
                                                  <dt><b>Maa</b></dt>
                                                  <dd>".$clickable.". Maa (lippu ja nimi) kuten annettu resurssien kuvauksessa. Valitse maa jos haluat näyttää vain taman maan tiedot.</dd>
                                                  <dt><b>Klusteri</b></dt>
                                                  <dd>".$clickable.". Klusterin alias-nimi kuten omistaja on sen antanut. Naytetään max 22 merkkia. Valitse alias jos haluat tarkemman kuvauksen klusterista.</dd>
                                                  <dt><b>Prosessoreita</b></dt>
                                                  <dd>Klusterin prosessorien kokonaismäärä. <b>Huom!</b> Grid-käyttäjien saatavilla voi olla näistä vai osa.</dd>
                                                  <dt><b>Kuorma (prosesseja:Grid&#43paikallinen)</b></dt>
                                                  <dd>".$clickable.". Klusterin suhteelinen kuorma, eli kuormitettujen prosessorien määrä. Harmaat palkit vastaavat prosessoreita jotka suorittavat paikallisesti lahetettyha töitä, punaiset palkit Grid-töitä. Valitse palkki jos haluat tarkempaa tietoa Grid-toistä joita suoritetaan klusterissa.</dd>
                                                  <dt><b>Jonottamassa</b></dt>
                                                  <dd>".$clickable.". Klusterissa jonottavien toiden lukumäärä, Grid työt sekä paikallisesti lähetetyt työt. Valitse ensimmainen numero jos haluat tarkempaa jonottavista Grid-toista.</dd>
                                                </dl>
                                                </div>",
                                      "Maa"                      => 30,
                                      "Kohde"                         => 160,
                                      "Prosesseja"                         => 10,
                                      "Kuorma (prosesseja: Grid+paikall.)" => 210,
                                      "Jonottamassa"                     => 10
                                      ),
                  "clusdes"  => array("0" => "Resource Details for",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Ominaisuus</b></dt>
                                                  <dd>".$clickable." Klusterin ominaisuuden nimi".$str_att."</dd>
                                                  <dt><b>Arvo</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Jono</b></dt>
                                                  <dd>".$clickable.". Jonon nimi siten kuin jonon omistaja on sen maarittanyt. ".$str_que."</dd>
                                                  <dt><b>Tila</b></dt>
                                                  <dd>Jonon tila. Toiminnassa oleva jono ilmoittaa yleensa tilan
                                                    <i>active</i>.</dd>
                                                  <dt><b>Prosessorit (min)</b></dt>
                                                  <dd>Jonon toiden aikarajoitus (jos annettu) prosessoriminuutteina. Naytetään ala- ja yläraja. N/A näytetään ios rajoituksia ei ole (kaikenkestoiset työt sallitaan).</dd>
                                                  <dt><b>Ajossa</b></dt>
                                                  <dd>Ajossa olevat jonon työt. Toiden kokonaismäärä, suluissa Grid-töitä suorittavien prosessorien kokonaismäärä. Huom: rinnakkaisille multiprosessoritoille suluissa oleva numero voi olla suurempi kuin toiden määrä.</dd>
                                                  <dt><b>Jonottamassa</b></dt>
                                                  <dd>TyöT jotka odottavat suoritukseen paäsyä jonossa. Toiden kokonaismäärä ja Grid-toiminnoilla lähetetyt
                                                    suluissa esim. (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "Jono"                        => 0,
                                      "Mapping Queue"                => 0,
                                      "Tila"                       => 0,
                                      "Rajoitukset (min)"                 => 0,
                                      "Prosessoreita"                         => 0,
                                      "Ajossa"                      => 0,
                                      "Jonottamassa"                     => 0
                                      ),
                  "jobstat"  => array("0" => "Jobs at:Job ID",
                                      "help" => "<div align=\"left\">
                                                <b>TYÖT:</b>
                                                <dl>
                                                <dt><b>Tyon nimi</b></dt>
                                                <dd>".$clickable.". Tyon nimi (omistajan antama). Jos omistaja ei antanut tyolle nimea, näytetään
                                                  &quot;<span>
                                                    style={color:red;}>N/A</span>&quot; . Valitse nimi jos haluat kuvauksen tyostä.</dd>
                                                <dt><b>Omistaja</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Tila</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>Prosessoreita (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Jono</b></dt>
                                                <dd>".$clickable.". Eräajojono, jossa työtä suoritetaan. ".$str_que."</dd>
                                                <dt><b>Prosessoreita</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>TYÖT (YKSITYISKOHTAISESTI):</b>
                                                <dl>
                                                <dt><b>Ominaisuus</b></dt>
                                                <dd>".$clickable.". Tyon ominaisuus".$str_att."</dd>
                                                <dt><b>Arvo</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Tyon nimi"                     => 0,
                                      "Omistaja"                        => 0,
                                      "Tila"                       => 0,
                                      "Prosessoreita (min)"                    => 0,
                                      "Jono"                        => 0,
                                      "Prosessoreita"                         => 0
                                      ),
                  "volist"   => array("0" => "Virtual Organisations",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuaaliorganisaatio (VO)</b></dt>
                                                   <dd>".$clickable.". Ryhmä käyttäjiä jotka käyttävät samanlaisia resursseja ARC-tietkoneissa. Valitse ryhmän nimi jos haluta listan ryhmän jäsenistä.</dd>
                                                   <dt><b>Jäseniä</b></dt>
                                                   <dd>Ryhmän jäsenten määrä.</dd>
                                                   <dt><b>Served by</b></dt>
                                                   <dd>LDAP palvelin johon ryhmä/jäsenyystiedot talletetaan.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtuaaliorganisaatio (VO)"         => 0,
                                      "Jäsenet"                      => 0,
                                      "Palvelin"                    => 0
                                      ),
                  "vousers"  => array("0" => "Grid User Base",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Nimi</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Organisaatio</b></dt>
                                                   <dd>".$clickable.". Käyttäjän organisaatio siinä muodossa kuin VO'n hallinnoija on sen antanut (voi olla myos tyhjä).</dd>
                                                   <dt><b>Sähkopostiosoite</b></dt>
							
                                                   <dd>".$clickable.". Käyttäjän sähkopostiosoite siinä muodossa kuin VO'n hallinnoija on sen antanut (voi olla myos tyhjä). Valitsemalla sähkopostiosoitteen voit lähettää käyttäjälle sähkopostia.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Nimi"                         => 0,
                                      "Organisaatio"                  => 0,
                                      "Sähkopostiosoite"                       => 0
                                      ),
                  "userlist" => array("0" => "Information for",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Klusteri:jono</b></dt>
                                                <dd>".$clickable.". Klusterit ja niiden jonot
                                                  (kaksoispisteella erotettuina, &quot;:&quot;) joihin käyttäja voi lähettää töitä.
                                                  Jos käyttäjällä ei ole oikeutta lähettää työtä,
                                                  tuloste on &quot;<b><font color=\"red\">Not authorised at host
                                                      ...</font></b>&quot; Valitse klusterin nimi jos haluat yksityiskohtaisen kuvauksen klusterista.
						Valitse jonon nimi jos haluat yksityiskohtaisen kuvauksen jonosta.</dd>
                                                <dt><b>Vapaita prosessoreita</b></dt>
                                                <dd>Tälle käyttäjälle, annetussa jonossa saatavilla olevien prosessorien maara. Tämän jälkeen saattaa ilmetä 
myos maksimiarvo joka kertoo kuinka monta minuuttia prosessori on käytettävissä. &quot;3&quot; tarkoittaa: 3 prosessoria käytettävissä ilman aikarjaa.
&quot;4:360&quot; tarkoittaa: 4 prosessoria korkeintaan kuudeksi tunniksi.
&quot;0&quot; tarkoittaa: ei prosessoreita saatavilla ja työt jonottavat kunnes
niita vapautuu.</dd>
                                                <dt><b>Jonossa olevia töitä</b></dt>
                                                <dd>TyöT jotka todennäkoisesti suoritetaan ennen uutta jonoon tulevaa. 

                                                  &quot;0&quot; tarkoittaa: tyo
suoritetaan heti.
                                                   <b>Huom!</b> Tämä on arvio,
jonon paikalliskaytanto saattaa muuttaa prioriteetteja.</dd>
                                                <dt><b>Vapaa tila (MB)</b></dt>
                                                <dd>Käyttäjälle tarjolla oleva levytila tässä jonossa (megatavuina).
                                                  <b>Huom!</b> 
                                                  Tämä on arvio, koska klusterit eivät tarjoa levykiintioitä.</dd>
                                                <dt><b>Tyon nimi</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Tila</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Klusteri</b></dt>
                                                <dd>".$clickable.". Klusteri kossa tyo suoritetaan/suoritettiin.</dd>
                                                <dt><b>Queue</b></dt>
                                                <dd>".$clickable.". Jono jossa tyo suoritetaan/suoritettiin. ".$str_que."</dd>
                                                <dt><b>Prosessoreita</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Tyon nimi"                     => 0,
                                      "Tila"                       => 0,
                                      "Prosessoreita (min)"                    => 0,
                                      "Klusteri"                      => 0,
                                      "Jono"                        => 0,
                                      "Prosessoreita"                         => 0
                                      ),
                  "attlist"  => array("0" => "Attribute values",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Object</b></dt>
                                                   <dd>".$clickable.". Objekti
							jonka ominaisuuksia
							tarkastellaan. Objekti
							voi olla klusteri, jono, tyo, käyttäjä jne. Valitsemalla objektin sen kuvauksen.</dd>
                                                   <dt><b>Ominaisuus</b></dt>
                                                   <dd>Ominaisuuksia ja niiden arvoja tulostetaan yksi tai usempia per kohde. Sarakkeen otsikko on ihmiselle ymmarrettävässä muodossa (poislukien jotkin MDS-spesifit ominaisuudet). Sarake sisältää vastaavan ominaisuuden arvot tälle kohteelle (arvot saadaan tietojärjestelmästä).
                                                    </dd>
                                                 </dl>
                                                 </div>",
                                      "Objekti"                       => 0,
                                      "Ominaisuus"                    => 0
                                      ),
                  "quelist"  => array("0" => "Jono",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Ominaisuus</b></dt>
                                                   <dd>".$clickable.". Name of a queue attribute".$str_att."</dd>
                                                   <dt><b>Arvo</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Tyon nimi</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Omistaja</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Tila</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>Prosessoreita (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Muisti (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPUs</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Tyon nimi"                     => 0,
                                      "Omistaja"                        => 0,
                                      "Tila"                       => 0,
                                      "Prosessoreita (min)"                    => 0,
                                      "Muisti (KB)"                  => 0,
                                      "Prosessoreita"                         => 0
                                      ),
                  "sestat"   => array("0" => "Storage Elements",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias-nimi</b></dt>
                                                  <dd>Talletuselementin nimi siinä muodossa kuin se on tietojärjestelmässä (IS), max. 15 merkkiä näytetään.</dd>
                                                  <dt><b>Tilaa kaikkiaan</b></dt>
                                                  <dd>Kokonaislevytila, GB.</dd>
                                                  <dt><dt><b>Vapaa tila</b></dt>
                                                  <dd>Tälle hetkellä vapaana oleva levytila, GB.</dd>
                                                  <dt><b>Name</b></dt>
                                                  <dd>Talletuselementin nimi, looginen nimi ja tietokoneen nimi kaksoispisteella eroteltuna. Loogista nimeä käyttää vain tietojärjestelmä (IS), jotta voidaan erottaa eri talletuselementit samalla koneella.</dd>
                                                  <dt><b>URLin alku</b></dt>
                                                  <dd>Talletuselementin URL, usein gsiftp://..
                                                     Tama URL on edeltää yksittäisiä tiedostoja tai hakemistoja.</dd>
                                                  <dt><b>Type</b></dt>
                                                  <dd>Talletuselementin tyyppi. &quot;gridftp-based&quot; tarkoittaa tietovarantoa jossa GridFTP liittymä.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias-nimi"                   => 0,
				      //                                      "Tilaa kaikkiaan"                   => 0,
                                      "Vapaa/kaikkiaan tila, GB"     => 0,
                                      "Nimi"                         => 0,
                                      "URLin alku"                   => 0,
                                      "Typpi"                        => 0
                                      ),
                  "allusers" => array("0" => "Grid kayttäjät joille käytto sallittu:Aktiiviset Grid käyttäjät",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Nimi</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Organisaatio</b></dt>
                                                   <dd>Käyttäjän organisaatio, tieto saatu varmenteesta</dd>
                                                   <dt><b>TyöT</b></dt>
                                                   <dd>Kaikki käyttäjien työt (ajossa, odottamassa,
                                                     suoritettu tai poistettu)</dd>
                                                   <dt><b>Kohteet</b></dt>
                                                   <dd>Kuinka monta kohdetta tämä käyttäjä voi käyttää</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Nimi"                         => 0,
                                      "Organisaatio"                   => 0,
                                      "TöIta"                         => 0,
                                      "Kohteita"                        => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Klusteri:jono"                => 0,
                                      "Vapaita prosessoreita"                    => 0,
                                      "Jonossa olevia töitä"            => 0,
                                      "Vapaata levytilaa (MB)"               => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Ominaisuus"                    => 0,
                                      "Arvo"                        => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info valid from (GMT)",
                                      "Mds-validto"   => "Info valid to (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Edustakoneen domain",
                                      "nordugrid-cluster-aliasname"          => "Klusterin alias-nimi",
                                      "nordugrid-cluster-contactstring"      => "Kontakti",
                                      "nordugrid-cluster-interactive-contactstring" => "Interaktiivisten toiden kontakti",
                                      "nordugrid-cluster-comment"            => "Kommentti",
                                      "nordugrid-cluster-support"            => "Tukipalvelun sähkoposti",
                                      "nordugrid-cluster-acl"                => "Sallitut VO't",
                                      "nordugrid-cluster-lrms-type"          => "Jononhallintaohjelmiston tyyppi",
                                      "nordugrid-cluster-lrms-version"       => "Jononhallintaohjelmiston versio",
                                      "nordugrid-cluster-lrms-config"        => "Jononhallintaohjelmisto, tarkemmin",
                                      "nordugrid-cluster-architecture"       => "Arkkitehtuuri",
                                      "nordugrid-cluster-opsys"              => "Käyttojärjestelmä",
                                      "nordugrid-cluster-homogeneity"        => "Klusterin arkkitehtuuri yhtenainen ",
                                      "nordugrid-cluster-nodecpu"            => "Prosessorin tyyppi (hitain)",
                                      "nordugrid-cluster-nodememory"         => "Muisti (MB, pienin määrä)",
                                      "nordugrid-cluster-totalcpus"          => "Prosessoreita kaikkiaan",
                                      "nordugrid-cluster-cpudistribution"    => "Prosessoreita tietokonetta kohti",
                                      "nordugrid-cluster-benchmark"          => "Suoritustesti",
                                      "nordugrid-cluster-sessiondir-free"    => "Levytila, saatavilla (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Levytila kaikkiaan (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Sessiohakemiston elinaika (min)",
                                      "nordugrid-cluster-cache-free"         => "Valimuistin koko, saatavilla (MB)",
                                      "nordugrid-cluster-cache-total"        => "Valimuistin koko kaikkiaan (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Ajoaikainen ymparisto",
                                      "nordugrid-cluster-localse"            => "Paikallinen talletuselementti (SE)",
                                      "nordugrid-cluster-middleware"         => "Väliohjelmisto",
                                      "nordugrid-cluster-totaljobs"          => "Töiden kokonaismäärä",
                                      "nordugrid-cluster-usedcpus"           => "Prosessoreita varattu",
                                      "nordugrid-cluster-queuedjobs"         => "TöItä jonossa",
				                      "nordugrid-cluster-prelrmsqueued"      => "Grid&ndash;töitä odottamassa",
                                      "nordugrid-cluster-location"           => "Postinumero",
                                      "nordugrid-cluster-owner"              => "Omistaja",
                                      "nordugrid-cluster-issuerca"           => "Varmenteen myontäjä",
                                      "nordugrid-cluster-issuerca-hash"      => "Varmenteen myontäjän hajakoodi",
                                      "nordugrid-cluster-trustedca"          => "Luotetut varmenteen myontäjä",
                                      "nordugrid-cluster-nodeaccess"         => "Laskentasolmun internet-yhteys",
                                      "nordugrid-cluster-gridarea"           => "Sessiotila (VANHENTUNUT)",
                                      "nordugrid-cluster-gridspace"          => "Grid levytila (VANHENTUNUT)",
                                      "nordugrid-cluster-opsysdistribution"  => "Käyttojärjestelmän jakelunimi (VANHENTUNUT)",
                                      "nordugrid-cluster-runningjobs"        => "TöItä, ajossa (VANHENTUNUT)",
				                      "nordugrid-cluster-credentialexpirationtime" => "Varmenne voimassa",
                                      "nordugrid-queue-name"                 => "Jonon nimi",
                                      "nordugrid-queue-comment"              => "Kommentti",
                                      "nordugrid-queue-status"               => "Jonon tila",
                                      "nordugrid-queue-running"              => "Prosessoreita varattu",
                                      "nordugrid-queue-localqueued"          => "Paikallisia töitä jonossa",
                                      "nordugrid-queue-prelrmsqueued"        => "Grid&ndash;töitä odottamassa",
                                      "nordugrid-queue-queued"               => "Jonossa olevia töitä (VANHENTUNUT)",
                                      "nordugrid-queue-maxrunning"           => "Ajossa olevia töitä (max)",
                                      "nordugrid-queue-maxqueuable"          => "TöItä jotka voivat jonottaa (max)",
                                      "nordugrid-queue-maxuserrun"           => "TöItä käyttäjää kohti (max)",
                                      "nordugrid-queue-maxcputime"           => "Prosessoriaika, max. (minuutteja)",
                                      "nordugrid-queue-mincputime"           => "Prosessoriaika, min. (minuutteja)",
                                      "nordugrid-queue-defaultcputime"       => "Prosessoriaika, oletusarvo (minuutteja)",
                                      "nordugrid-queue-maxwalltime"          => "Kokonaisaika, max. (minuutteja)",
                                      "nordugrid-queue-minwalltime"          => "Kokonaisaika, min. (minuutteja)",
                                      "nordugrid-queue-defaultwalltime"      => "Kokonaisaika, oletusarvo (minuutteja)",
                                      "nordugrid-queue-schedulingpolicy"     => "Schedulointipolitiikka",
                                      "nordugrid-queue-totalcpus"            => "Prosessoreita kaikkiaan",
                                      "nordugrid-queue-nodecpu"              => "Prosessrin tyyppi",
                                      "nordugrid-queue-nodememory"           => "Muistia (MB)",
                                      "nordugrid-queue-architecture"         => "Arkkitehtuuri",
                                      "nordugrid-queue-opsys"                => "Käyttojärjestelmä",
                                      "nordugrid-queue-homogeneity"          => "Jonon arkkitehtuuri yhtenäinen",
                                      "nordugrid-queue-gridrunning"          => "Grid&ndash;töiden käyttämät prosessorit",
                                      "nordugrid-queue-gridqueued"           => "Grid työt, jonossa",
                                      "nordugrid-queue-benchmark"            => "Mitattu suorituskyky",
                                      "nordugrid-queue-assignedcpunumber"    => "Prosessoreita jonoa kohti (VANHENTUNUT)",
                                      "nordugrid-queue-assignedcputype"      => "Prosessorin tyyppi (VANHENTUNUT)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Omistaja",
                                      "nordugrid-job-execcluster"            => "Suoritusklusteri",
                                      "nordugrid-job-execqueue"              => "Suoritusjono",
                                      "nordugrid-job-stdout"                 => "Standardi tulostiedosto",
                                      "nordugrid-job-stderr"                 => "Standardi virhetiedosto ",
                                      "nordugrid-job-stdin"                  => "Standardi syotetiedosto",
                                      "nordugrid-job-reqcputime"             => "Pyydetty prosessoriaika",
                                      "nordugrid-job-reqwalltime"            => "Pyydetty kokonaisaika",
                                      "nordugrid-job-status"                 => "Tyon tila",
                                      "nordugrid-job-queuerank"              => "Paikka jonossa",
                                      "nordugrid-job-comment"                => "Jonosuoritusohjelman kommentti",
                                      "nordugrid-job-submissionui"           => "Lähetetty koneesta",
                                      "nordugrid-job-submissiontime"         => "Lähetysaika (GMT)",
                                      "nordugrid-job-usedcputime"            => "Käytetty prosessoriaika",
                                      "nordugrid-job-usedwalltime"           => "Käytetty kokonaisaika",
                                      "nordugrid-job-completiontime"         => "Saatu suoritettua (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Poistamisaika (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxyn käyttoaika loppuu (GMT)",
                                      "nordugrid-job-usedmem"                => "Käytetty muisti (KB)",
                                      "nordugrid-job-errors"                 => "Virheet",
                                      "nordugrid-job-exitcode"               => "Poistumiskoodi",
                                      "nordugrid-job-jobname"                => "Nimi",
                                      "nordugrid-job-runtimeenvironment"     => "Ajoaikainen ympäristo",
                                      "nordugrid-job-cpucount"               => "Pyydetyt prosessorit",
                                      "nordugrid-job-executionnodes"         => "Suoritusnoodi",
                                      "nordugrid-job-gmlog"                  => "GM log -tiedosto",
                                      "nordugrid-job-clientsoftware"         => "Asiakasohjelmiston nimi",
                                      "nordugrid-job-rerunable"              => "Uudelleen ajettavissa",
                                      "nordugrid-job-reqcput"                => "Pyydetty suoritusaika (VANHENTUNUT)",
                                      "nordugrid-job-lrmscomment"            => "Jonosuoritusohjelman kommentti (VANHENTUNUT)",
                                      "nordugrid-job-gridlog"                => "Gridlog tiedosto (VANHENTUNUT)",
                                      "nordugrid-authuser-name"              => "Nimi",
                                      "nordugrid-authuser-sn"                => "Subject-nimi",
                                      "nordugrid-authuser-freecpus"          => "Vapaita prosessoreita",
                                      "nordugrid-authuser-diskspace"         => "Vapaa levytila (MB)",
                                      "nordugrid-authuser-queuelength"       => "KДyttДjДn tЖitД jonossa",
                                      "nordugrid-se-name"                    => "Nimi",
                                      "nordugrid-se-aliasname"               => "Talletuselementin alias-nimi",
                                      "nordugrid-se-type"                    => "Talletuselementin tyyppi",
				      "nordugrid-se-acl"                     => "Autorisoidut VOt",
                                      "nordugrid-se-freespace"               => "Vapaa tila (MB)",
                                      "nordugrid-se-totalspace"              => "Kokonaistila (MB)",
                                      "nordugrid-se-url"                     => "Yhteys-URL",
                                      "nordugrid-se-baseurl"                 => "Yhteys-URL (VANHENTUNUT)",
                                      "nordugrid-se-accesscontrol"           => "Kayttokontrolli",
                                      "nordugrid-se-authuser"                => "Auktorisoitu käyttäjä (DN)",
                                      "nordugrid-se-location"                => "Postinumero",
                                      "nordugrid-se-owner"                   => "Omistaja",
                                      "nordugrid-se-middleware"              => "VДliohjelmisto",
                                      "nordugrid-se-issuerca"                => "Varmenteen myontäjä",
                                      "nordugrid-se-issuerca-hash"           => "Varmenteen myontäjän hajakoodi",
                                      "nordugrid-se-trustedca"               => "Luotetut varmenteen myontäjä",
                                      "nordugrid-se-comment"                 => "Kommentteja",
                                      "nordugrid-rc-name"                    => "Domainin nimi",
                                      "nordugrid-rc-aliasname"               => "Replica Catalog alias",
                                      "nordugrid-rc-baseurl"                 => "Yhteys-URL",
                                      "nordugrid-rc-authuser"                => "Auktorisoitu käyttäjä (DN)",
                                      "nordugrid-rc-location"                => "Postinumero",
                                      "nordugrid-rc-owner"                   => "Omistaja",
                                      "nordugrid-rc-issuerca"                => "Varmenteen myontäjä (CA)"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Ei voitu lukea ylätason indeksejä",
                                      "2"   => "Ei saatu yhteyttä paikallisiin indeksipalveluihin",
                                      "3"   => " viallinen konfiguraatio tai pyyynnolle annettu aika ylittyi",
                                      "4"   => "Ei Grid-töitä",
                                      "5"   => "Ei loytynyt tietoa",
                                      "6"   => "Tietokone ei saavutettavissa",
                                      "7"   => " - hae uudestaan myohemmin",
                                      "8"   => "Ei jonotietoa",
                                      "9"   => "Ei kohderiveja",
                                      "10"  => "Ei käyttäjiä",
                                      "11"  => "Ei oikeutta käyttää tietokonetta",
                                      "12"  => "ei vastaa",
                                      "13"  => "Ei töitä ",
                                      // debug messages
                                      "101" => " Monitoriprosesille annettu aika:  GRIS: ",
                                      "102" => " sekuntia yhteyksien luomiseen ",
                                      "103" => " sekuntia käytetty etsimisprosessissa",
                                      "104" => " sekuntia käytetty etsimiseen",
                                      "105" => "Näytetään vain resurssit: ",
                                      "106" => "Tutkittu ylimman tason indeksit: ",
                                      "107" => "Maantieteelliset kohteet haettu, lisataan tietoa: ",
                                      "108" => " kohteet jarjestetty maantieteellisesti",
                                      "109" => "Etsi klusterin ominaisuuksilla",
                                      "110" => "Etsi jonon ominaisuuksilla",
                                      "111" => "Ei dataa kohteesta ",
                                      "112" => " on toiminnassa: ",
                                      "113" => " ei resursseja tarjolla",
                                      "114" => " Monitoriprosessille annettu aika ylittyi, GIIS: ",
                                      "115" => "Jätetään valiin GRIS: ",
                                      "116" => "ei ole ",
                                      "117" => "Tarkintan yhteyttä: ",
                                      "118" => "OK",
                                      "119" => "Siihen mennessä loytynyt seuraavanlaisia resursseja ",
                                      "120" => "LDAP etsinnässä virhe ",
                                      "121" => " status  ",
                                      "122" => "Mustalla listalla: ",
                                      "123" => "Rekisteroitynyt ",
                                      "124" => "Etsi tallennuselementin (SE) ominaisuuskai",
                                      "125" => "Etsi käyttäjiä",
                                      "126" => "Etsi töitä",
                                      "127" => " tyo ",
                                      "128" => " ei käyttooikeutta",
                                      "129" => "Virhe: ei tietoa kohteesta ",
                                      "130" => " Monitoriprosessille annettu aika ylittyi, EMIR: ",
                                      // icon titles
                                      "301" => "Lataa uudestaan",
                                      "302" => "Tulosta",
                                      "303" => "Ohjeet",
                                      "304" => "Sulje",
                                      "305" => "Punainen",
                                      "306" => "Harmaa",
                                      "307" => "Kaikki käyttäjät",
                                      "308" => "Aktiiviset käyttäjät",
                                      "309" => "Hae",
                                      "310" => "Tietovarannot",
                                      "311" => "Virtuaaliorganisaatiot",
                                      "312" => "Lippu: ",
                                      "313" => " Grid prosessit ja ",
                                      "314" => " paikalliset prosessit",
                                      // auxilliary strings
                                      "401" => "Prosessit",
                                      "402" => "Grid",
                                      "403" => "Paikallinen",
                                      "404" => "Maailma",
                                      "405" => "TOTAL",
                                      "406" => " kohdetta ",
                                      "407" => "paljon",
                                      "408" => " GB",
                                      "409" => " KAIKKI",
                                      "410" => "Klusteri",
                                      "411" => "Jono",
                                      "412" => "Tyo",
                                      "413" => "Kayttäjä",
                                      "414" => "Tietovaranto",
                                      "415" => "Replica Cat.",
                                      "416" => "Valitse ominaisuudet jotka näytetään: ",
                                      "417" => "Kaikkien valintojen kombinaation näytetään",
                                      "418" => "Jata oikeanpuoleinen kenttä tyhjäksi jos haluat kaikki tulokset näyttoon",
                                      "419" => "Näyta valitut resurssit tai kohteet",
                                      "420" => "Distinguished name",
                                      "421" => "käytettävissä ",
                                      "422" => " kohdetta",
                                      "423" => "Resurssi / objekti:",
                                      "424" => "Ominaisuuksia (def. 6):",
                                      "425" => "Objekti",
                                      "426" => "Seuraava",
                                      "427" => "Valise yksi",
                                      "428" => "Tyhjennä valinnat",
                                      "429" => "NÄYTÄ"
                                      ),
                  // Post code conversion
                  "tlconvert" => array (
                                        "Australia" => "Australia",	   
					"Austria" => "Itävalta",	   
					"Armenia" => "Armenia",	   
					"Algeria" => "Algeria",	   
					"Belgium" => "Belgia",	   
					"Bulgaria" => "Bulgaria",	   
                                        "Canada" => "Kanada",	   
					"Czechia" => "Tsekki",	   
                                        "China"        => "Kiina",
                                        "Denmark" => "Tanska",	   
                                        "Estonia" => "Eesti",	   
                                        "Finland" => "Suomi",	   
					"France" => "Ranska",	   
					"Georgia" => "Georgia",	   
                                        "Germany" => "Saksa",	   
					"Greece" => "Kreikka",	   
					"Hungary" => "Unkari",	   
                                        "Iceland" => "Islanti",	   
					"Ireland" => "Irlanti",	   
					"Italy" => "Italia",	   
                                        "Japan" => "Japani",	   
                                        "Latvia" => "Latvia",	   
                                        "Lithuania" => "Liettua",	   
					"Morocco" => "Marokko",	   
					"Netherlands" => "Alankomaat",	   
                                        "Norway" => "Norja",	   
					"Poland" => "Puola",	   
					"Portugal" => "Portugali",	   
					"Romania" => "Romania",	   
					"Russia" => "Venäjä",	   
					"SriLanka" => "Sri&nbsp;Lanka",	   
                                        "Sweden" => "Ruotsi",	   
                                        "Slovakia" => "Slovakia",	   
                                        "Slovenia" => "Slovenia",	   
                                        "Switzerland" => "Sveitsi",	   
                                        "Turkey" => "Turkki",	   
					"UK" => "Iso-Britannia",	   
					"Ukraine" => "Ukraina",	   
					"USA" => "USA"             
                                        )
                  );

?>
