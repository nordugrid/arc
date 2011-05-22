<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- Hungarian [hu]
// -- Translation: Gabor Roczei (roczei@niif.hu)
//                 Peter Stefan (stefan@niif.hu)
// -- Author: oxana.smirnova@hep.lu.se

// Some common strings:

$clickable = "KATTINTHATO";

$str_att = ", emberi olvasásra szánt kivéve néhány MDS specifikus attribútumot. Kattintson az attribútum nevére azért, hogy megkapja az attribútumok értékének a listáját az ARC univerzum-on keresztül.";

$str_val = "Az attribútum érték az információs rendszerből származik.";

$str_que = "Az egyes várakozási sorok tipikusan különböző megengedett maximális futási időkre, vagy felhasználói csoportokra illeszkednek. Kattintson a várakozási sor nevére, hogy bővebb információt kapjon a várakozási sorról, beleértve a &quot;running&quot; állapotot";

$str_job = "A job neve, amit a tulajdonosa adott meg.
          <br> <font color=\"red\">N/A</font>:&nbsp; ez utóbbi azt jelenti, hogy a felhasználó nem adott meg neki nevet.
          <br> <font color=\"red\"><b>X</b></font>:&nbsp; ez azt jelenti, hogy a job-ot a tulajdonosa &quot;megölte&quot;
          <br> <font color=\"red\"><b>!</b></font>:&nbsp; ez azt jelenti, hogy a job futása közben hiba lépett fel a rendszerben
          <br> Kattintson a névre, hogy bővebb információt kapjon a job-ról.";

$str_nam = "A felhasználó neve, ez van megadva a személyes tanusítványban. Kattintson a névre azért, hogy megkapja a felhasználó összes elérhető erőforrásoknak a listáját és a hozzá tartozó összes job-ot, ami a rendszerben éppen jelen van.";

$str_sta = "A job állapota, amit a Grid Menedzser (GM) és az
   LRMS ad vissza. Szekvenciális sorrendben az állapotok a következők:
   <dl>
     <dt>ACCEPTED &ndash; a job elküldve, de még nincs feldolgozás alatt</dt>
     <dt>PREPARING &ndash; bemeneti állományok kinyerése. </dt>
     <dt>SUBMITTING &ndash; az interrakció az LRMS-el folyamatban</dt>
     <dt>INLRMS &ndash; a job átküldve az LRMS-nek; a belső
       állapot hozzá lett adva az információs rendszer segítségével. A lehetséges állapotok:</dt>
       <dd> Q &ndash; a job a várakozósorban van
       <br> U &ndash; a job egy felfüggesztett állapotban van egy elfoglalt csomópontban (PBSPro)
       <br> S &ndash; a job egy felfüggesztett állapotban van (Condor)
       <br> R &ndash; a job fut
       <br> E &ndash; a job véget ért (PBS)
     </dd>
     <dt>FINISHING &ndash; a kimeneti fájlok átvitelre megtötént a GM segítségével</dt>
     <dt>FINISHED &ndash; a job véget ért; az idő pecsétet hozzá adta az információs rendszer</dt>
     <dt>CANCELING &ndash; a job érvénytelenítve lett</dt>
     <dt>DELETED &ndash; a job nem lett kitörölve a felhasználó kérésére, viszont a GM eltávolította, mert lejárt a határideje</dt>
   </dl>
   Minden állapotot jelenteni lehet a PENDING állapottal,
   ez azt jelenti a GM számára, hogy a job-ot a következő állapotba próbálja meg átbillenteni";

$str_tim = "A job által lefoglalt CPU idő (perc).";

$str_mem = "A job által lefoglalt memória (KB).";

$str_cpu = "Azon processzorok száma, amit a job használ.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitorozó",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>Ez a képernyő mutatja meg az összes site regisztrációt, ami a legfelső ARC indexelő szolgáltatás esetén előfordul, először az ország, majd a hoszt név szerint van rendezve a lista. A következő site paraméterek vannak monitorozva: klaszter alias, teljes CPU kapacitás és a futó ill. várakozósoros jobok száma (Grid-es és helyi együttesen). Használja a &quot;Keresés&quot; segédeszközt, ha szeretne megosztani egyéb klaszter, várakozósor, job stb. jellemzőt.</dt>
                                                  <dt><b>Ország</b></dt>
                                                  <dd>".$clickable.". Ország zászló és név, ez az elérhető leírásokból száramzik. Kattintson ide, hogy csak ennek az országnak az információit lássa.</dd>
                                                  <dt><b>Klaszter</b></dt>
                                                  <dd>".$clickable.". A klaszter álnevét a tulajdonos jelöli ki. Maximum 22 karakter hosszúságú lehet. Kattintson az aliasre, hogy részletesebb információt kapjon a klaszterről.</dd>
                                                  <dt><b>CPU-k</b></dt>
                                                  <dd>A klaszterben lévő összes CPU száma. Csak ezek töredékét tudják éppen elérni a grid-es felhasználók.</dd>
                                                  <dt><b>Betöltés (feldolgoz:Grid&#43helyi)</b></dt>
                                                  <dd>".$clickable.". Relatív klaszter betöltés, megfelelően a foglalt CPU-k számához. A szürke sáv azt mutatja, hogy a processzorokat helyileg elküldött job-ok foglalják le, a piros sáv pedig azt, hogy a CPU-kat a grid-ről küldött job-ok foglalják le. Kattintson a sávra, hogy részletes információt kapjon a klaszteren futó Grid-es job-okról, ebben benne foglaltatik az is, hogy egy job-hoz hány darab processzor tartozik.</dd>
                                                  <dt><b>Várakozólistában</b></dt>
                                                  <dd>".$clickable.". A klaszterben lévő összes várakozósoros job száma, megmutatja a várakozósoros grid-es job-okat plusz a helyileg elküldött várakozósoros job-okat. Kattintson az első számra ahhoz, hogy a klaszterben lévő várakozósoros grid-es jobok listáját megkapja.</dd>
                                                </dl>
                                                </div>",
                                      "Ország"                      => 30,
                                      "Site"                         => 160,
                                      "CPU-k"                         => 10,
                                      "Betöltés (feldolgoz: Grid+helyi)" => 210,
                                      "Várólistán"                     => 10
                                      ),
                  "clusdes"  => array("0" => "Erőforrás részletek a következőkről:",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attribútum</b></dt>
                                                  <dd>".$clickable.". Klaszter attribútum név".$str_att."</dd>
                                                  <dt><b>Érték</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Várakozósor</b></dt>
                                                  <dd>".$clickable.". Azon kötegelt várakozósoroknak a nevei, amik az ARC felhasználók számára elérhetőek, ezt a klaszter tulajdonosa állítja be. ".$str_que."</dd>
                                                  <dt><b>Állapot</b></dt>
                                                  <dd>A várakozósor állapota. A működő lista tipikusan <i>aktív</i> állapotot jelez.</dd>
                                                  <dt><b>CPU (min)</b></dt>
                                                  <dd>Idő korlát a várakozósoronkénti job-ok időtartamára, ha meg van adva, akkor az CPU percben értendő. Az első megjelenő érték az alsó korlát, a második a felső korlát. Ha a korlátok nincsenek beállítva (a job-ok bármikor elfogadásra kerülnek), ez így van jelölve: <b>N/A</b> .</dd>
                                                  <dt><b>Futás</b></dt>
                                                  <dd>Azon job-ok száma, amik a várakozósorban futnak. Az összes job számát megmutatja, a <b>processzorok</b> számával illetve a zárójelben jelzett grid-feladatokkal együtt, e.g. (Grid: 12). Párhuzamos, többprocesszoros feladatok esetén a zárójelek közötti szám nagyobb is lehet, mint a feladatok száma</dd>
                                                  <dt><b>Várakozólistán</b></dt>
                                                  <dd>Azon job-ok száma, melyek a várakozósorban a futtatásra várnak. Az összes job száma látható, a zárójelben jelzett grid-feladatokkal együtt, például (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "Várakozósor"                        => 0,
                                      "Állapot"                       => 0,
                                      "Korlátok (min)"                 => 0,
                                      "CPU-k"                         => 0,
                                      "Futás"                      => 0,
                                      "Várólistán"                     => 0
                                      ),
                  "jobstat"  => array("0" => "Job helye:Job ID",
                                      "help" => "<div align=\"left\">
                                                <b>JOB LISTA</b>
                                                <dl>
                                                <dt><b>Job név</b></dt>
                                                <dd>".$clickable.". A job neve, amit a tulajdonos jelöl ki. Ha nincsen név kijelölve, akkor a következőt látjuk: &quot;<span style={color:red;}>N/A</span>&quot;. Kattintson a névre, hogy megkapja a job részletes leírását.</dd>
                                                <dt><b>Tulajdonos</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Állapot</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Várakozósor</b></dt>
                                                <dd>".$clickable.". A kötegelt várakozósor neve, amiben a job végrehajtódott. ".$str_que."</dd>
                                                <dt><b>CPU-k</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>JOB RÉSZLETEK</b>
                                                <dl>
                                                <dt><b>Attribútum</b></dt>
                                                <dd>".$clickable.". Job attribútum név".$str_att."</dd>
                                                <dt><b>Érték</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Job név"                     => 0,
                                      "Tulajdonos"                        => 0,
                                      "Állapot"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Várakozósor"                        => 0,
                                      "CPU-k"                         => 0
                                      ),
                  "volist"   => array("0" => "Virtuális szervezetek",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuális szervezet</b></dt>
                                                   <dd>".$clickable.". Felhasználók csoportja, tipikusan közös feladatokat és erőforrásokat osztanak meg egymással, az engedélyezés legalább egy ARC-os site-on megtörténik. Kattintson a névre, hogy megkapja a csoport tajainak a listáját.</dd>
                                                   <dt><b>Tagok</b></dt>
                                                   <dd>A csoport tagjainak a száma.</dd>
                                                   <dt><b>Kiszolgáló</b></dt>
                                                   <dd>LDAP szerver, ami támogatja a csoportos tagsági adatbázist.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtuális szervezet"         => 0,
                                      "Tagok"                      => 0,
                                      "Kiszolgáló"                    => 0
                                      ),
                  "vousers"  => array("0" => "Grid User Base",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Név</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Kapcsolatok</b></dt>
                                                   <dd>A felhasználók saját intézménye, amit egy VO menedzser szolgáltat. üres is lehet.</dd>
                                                   <dt><b>E-mail</b></dt>
                                                   <dd>".$clickable.". Felhasználók E-mail címe, amit egy VO menedzser ad meg. üres is lehet. Kattintson a címre, hogy levelet küldhessen a felhasználó E-mail címére.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Név"                         => 0,
                                      "Kapcsolatok"                  => 0,
                                      "E-mail"                       => 0
                                      ),
                  "userlist" => array("0" => "Információszerzés",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Klaszter:várakozósor</b></dt>
                                                <dd>".$clickable.". A klaszterek nevei és a megfelelő várakozósorok(oszlopokkal elválasztva, &quot;:&quot;), ahol a felhasználó job küldésekre jogosult. Ha a felhasználó nem jogosult, akkor a következő üzenet fog megjelenni: &quot;<b><font color=\"red\">Nincs megfelelő jogosultsága ennél a hosztnál</font></b>&quot;. Kattintson a klaszter nevére, hogy egy részletesebb leírást kapjon a klaszterről. Kattintson a várakozósor nevére, hogy egy részletesebb leírást kapjon a várakozósorról.</dd>
                                                <dt><b>Szabad CPU-k</b></dt>
                                                <dd>Ebben a pillanatban a felhasználó számára elérhető szabad CPU-k száma, az adott várakozósorban, feltételesen ki van egészítve a felső idő korláttal (ez percben értendő). Például a &quot;3&quot; azt jelenti, hogy 3 CPU használható fel a job számára korlátlan ideig; &quot;4:360&quot; ez azt mutatja, hogy 4 CPU-t tud felhasználni a job, de csak 6 órán keresztül; &quot;10:180 30&quot; ez azt jelenti, hogy 10 CPU áll a job-ok rendelkezésére 3 órán keresztül, és ezen kívül 30 CPU van még pluszba korlátlan időre; &quot;0&quot; ez azt jelenti, hogy nem áll rendelkezésre CPU, ebben a pillanatban, és ekkor a job-ok várakozólistára kerülnek.</dd>
                                                <dt><b>Várakosósorban elhelyezett job-ok</b></dt>
                                                <dd>A felhasználó azon job-jainak a száma, amiknek várhatóan várakoznia kell egy újonnan elküldött job előtt a várakozási sorban. A &quot;0&quot; száma azt jelenti, hogy a job remélhetőleg azonnal lefuthat. Ez csupán egy becslés, amit a helyi irányelvek felülbírálhatnak.</dd>
                                                <dt><b>Szabad lemez terület (MB)</b></dt>
                                                <dd>A felhasználó számára elérhető szabad lemezterület egy adott várakozósorban (MB). Ez csupán egy értékelés, a legtöbb klaszter nem nyújt fix lemez kvótákat.</dd>
                                                <dt><b>Job név</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Állapot</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Klaszter</b></dt>
                                                <dd>".$clickable.". A klaszter neve, amelyben a feladat éppen fut. Kattintson a klaszter nevére, hogy egy részletesebb leírást kapjon.</dd>
                                                <dt><b>Várakozósor</b></dt>
                                                <dd>".$clickable.". Azon várakozósor neve, amiben a job lefuttot, vagy le fog futni. ".$str_que."</dd>
                                                <dt><b>CPU-k</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Job név"                     => 0,
                                      "Állapot"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Klaszter"                      => 0,
                                      "Várakozósor"                        => 0,
                                      "CPU-k"                         => 0
                                      ),
                  "attlist"  => array("0" => "Attribútum értékek",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objektum</b></dt>
                                                   <dd>".$clickable.". Az objektumok neve, ezek lesznek megjelenítve. Ez lehet klaszter név, egy klaszter várakozósorának a neve, egy job név, egy felhasználói név stb. Kattintson a szövegre, hogy egy részletesebb leírást kapjon az objektumról.</dd>
                                                   <dt><b>Attribútumok</b></dt>
                                                   <dd>Minden objektum számára egy vagy több attribútum értéket lehet kilistáztatni. Az oszlop címe egy emberi olvasásra szánt név (kivéve néhány MDS specifikus attribútumot), az oszlopok attribútum értékeket tartalmaznak az adott objektumról, ahogy az az információs rendszerbe be lett írva .</dd>
                                                 </dl>
                                                 </div>",
                                      "Objektum"                       => 0,
                                      "Attribútum"                    => 0
                                      ),
                  "quelist"  => array("0" => "Várakozósor",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attribútum</b></dt>
                                                   <dd>".$clickable.". Egy várakozósor attribútumának a neve".$str_att."</dd>
                                                   <dt><b>Érték</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Job név</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Tulajdonos</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Állapot</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Memória (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPU-k</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Job név"                     => 0,
                                      "Tulajdonos"                        => 0,
                                      "Állapot"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Memória (KB)"                  => 0,
                                      "CPU-k"                         => 0
                                      ),
                  "sestat"   => array("0" => "Adattároló elemek",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Az adattároló elem álneve az információs rendszerben van meghatározva. Maximális megjeleníthető hosszúság: 15 karakter</dd>
                                                  <dt><b>Összes lemezterület</b></dt>
                                                  <dd>Összes lemezterület (GB).</dd>
                                                  <dt><dt><b>Szabad terület</b></dt>
                                                  <dd>Pillanatnyilag ennyi szabad terület van, (GB)</dd>
                                                  <dt><b>Név</b></dt>
                                                  <dd>Adattároló elem neve, egy logikai névből és egy hoszt névből áll (egy oszloppal van elválasztva, &quot;:&quot;). A logikai nevet az információs rendszer használja azért, hogy megkülönböztesse a különbözö adattároló elemeket ugyanazon a gépen.</dd>
                                                  <dt><b>Alap URL</b></dt>
                                                  <dd>Az adattároló elem URL-je tipikusan egy gsiftp:// protokoll. Ezt használja alapból ahhoz, hogy elérje a fájlokat.</dd>
                                                  <dt><b>Típus</b></dt>
                                                  <dd>Az adattároló elem típusa. &quot;gridftp-based&quot; jelzi a GridFTP interfészen  keresztül az adattároló lemezt.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
                                      "Szabad/összes hely, GB"          => 0,
                                      "Név"                         => 0,
                                      "Alap URL"                     => 0,
                                      "Típus"                         => 0
                                      ),
                  "allusers" => array("0" => "Megbízható Grid felhasználók:Aktív Grid felhasználók",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Név</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Kapcsolat</b></dt>
                                                   <dd>A felhasználó kapcsolódás-rendszere, amely a személyes tanúsítványából származik.</dd>
                                                   <dt><b>Job-ok</b></dt>
                                                   <dd>Az összes felhasználó rendszerben lévő job-jainak a száma (futás, függőben lévő, befejezett vagy törölt)</dd>
                                                   <dt><b>Site-ok</b></dt>
                                                   <dd>Megmutatja, hogy hány darab site engedélyezi ezt a felhasználót</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Név"                         => 0,
                                      "Kapcsolatok"                   => 0,
                                      "Job-ok"                         => 0,
                                      "Site-ok"                        => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Klaszter:várakozósor"                => 0,
                                      "Szabad CPU-k"                    => 0,
                                      "Várakozólistára helyezett job-ok"                  => 0,
                                      "Szabad lemezterület (MB)"               => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attribútum"                    => 0,
                                      "Érték"                        => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "érvényes információ tőle (GMT)",
                                      "Mds-validto"   => "érvényes információ neki (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Front-end tartomány név",
                                      "nordugrid-cluster-aliasname"          => "Klaszter alias",
                                      "nordugrid-cluster-contactstring"      => "Elérhetőségi szöveg",
                                      "nordugrid-cluster-interactive-contactstring" => "Interaktív elérhetőség",
                                      "nordugrid-cluster-comment"            => "Megjegyzés",
                                      "nordugrid-cluster-support"            => "E-mail cím",
				      "nordugrid-cluster-acl"                => "Engedélyezett VO-k",
                                      "nordugrid-cluster-lrms-type"          => "LRMS típus",
                                      "nordugrid-cluster-lrms-version"       => "LRMS verzió",
                                      "nordugrid-cluster-lrms-config"        => "LRMS részletek",
                                      "nordugrid-cluster-architecture"       => "Architektúra",
                                      "nordugrid-cluster-opsys"              => "Operációs rendszer",
                                      "nordugrid-cluster-homogeneity"        => "Homogén klaszter",
                                      "nordugrid-cluster-nodecpu"            => "CPU típus (leglassabb)",
                                      "nordugrid-cluster-nodememory"         => "Memória (MB, legkisebb)",
                                      "nordugrid-cluster-totalcpus"          => "CPU-k, összesen",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:gépek",
                                      "nordugrid-cluster-benchmark"          => "Teljesítmény értékelés",
                                      "nordugrid-cluster-sessiondir-free"    => "Lemez terület, elérhető (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Lemez terület, összesen (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Grid session élettartam (min)",
                                      "nordugrid-cluster-cache-free"         => "Cache méret, elérhető (MB)",
                                      "nordugrid-cluster-cache-total"        => "Cache méret, összesen (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Futásidejű környezet",
                                      "nordugrid-cluster-localse"            => "Adattárolási elem, helyi",
                                      "nordugrid-cluster-middleware"         => "Grid köztesréteg",
                                      "nordugrid-cluster-totaljobs"          => "Job-ok, teljes összeg",
                                      "nordugrid-cluster-usedcpus"           => "CPU-k, foglalt",
                                      "nordugrid-cluster-queuedjobs"         => "Job-ok, várakozólistás",
				      "nordugrid-cluster-prelrmsqueued"      => "Grid job-ok, várakozó részfeladat",
                                      "nordugrid-cluster-location"           => "Irányítószám",
                                      "nordugrid-cluster-owner"              => "Tulajdonos",
                                      "nordugrid-cluster-issuerca"           => "Tanúsítvány kibocsájtó",
                                      "nordugrid-cluster-issuerca-hash"      => "Tanúsítvány kibocsájtó-s hash",
                                      "nordugrid-cluster-trustedca"          => "Megbízható tanúsítvány kibocsájtók",
                                      "nordugrid-cluster-nodeaccess"         => "Csomópont IP összekapcsolhatóság",
                                      "nordugrid-cluster-gridarea"           => "Session terület (ELAVULT)",
                                      "nordugrid-cluster-gridspace"          => "Grid lemez terület (ELAVULT)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS disztribúció (ELAVULT)",
                                      "nordugrid-cluster-runningjobs"        => "Job-ok, futás (ELAVULT)",
				      "nordugrid-cluster-credentialexpirationtime" => "Credential expiration time",
                                      "nordugrid-queue-name"                 => "A várakozósor neve",
                                      "nordugrid-queue-comment"              => "Megjegyzés",
                                      "nordugrid-queue-status"               => "A várakozósor állapota",
                                      "nordugrid-queue-running"              => "CPU-k, foglalt",
				      "nordugrid-queue-localqueued"          => "Helyi job-ok, várakozólistás",
				      "nordugrid-queue-prelrmsqueued"        => "Grid job-ok, várakozó részfeladat",
                                      "nordugrid-queue-queued"               => "Job-ok, várakozólistás (ELAVULT)",
                                      "nordugrid-queue-maxrunning"           => "Job-ok, futás(max)",
                                      "nordugrid-queue-maxqueuable"          => "Job-ok, várakozólistába tehető (max)",
                                      "nordugrid-queue-maxuserrun"           => "Unix felhasználókénti job-ok (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU idő, max. (perc)",	
                                      "nordugrid-queue-mincputime"           => "CPU idő, min. (perc)",	
                                      "nordugrid-queue-defaultcputime"       => "CPU idő, alap. (perc)",
                                      "nordugrid-queue-maxwalltime"          => "&quot;Wall clock&quot; idő, max. (perc)",
                                      "nordugrid-queue-minwalltime"          => "&quot;Wall clock&quot; idő, min. (perc)",
                                      "nordugrid-queue-defaultwalltime"      => "&quot;Wall clock&quot; idő, alap. (perc)",
                                      "nordugrid-queue-schedulingpolicy"     => "Ütemezési politika",
                                      "nordugrid-queue-totalcpus"            => "CPU-k, összesen",
                                      "nordugrid-queue-nodecpu"              => "CPU típusa",
                                      "nordugrid-queue-nodememory"           => "Memória (MB)",
                                      "nordugrid-queue-architecture"         => "Architektúra",
                                      "nordugrid-queue-opsys"                => "Operációs rendszer",
                                      "nordugrid-queue-homogeneity"          => "Homogén várakozósor",
                                      "nordugrid-queue-gridrunning"          => "CPUs occupied by Grid jobs",
                                      "nordugrid-queue-gridqueued"           => "Grid job-ok, várakozólistás",
                                      "nordugrid-queue-benchmark"            => "Teljesítmény értékelés",
                                      "nordugrid-queue-assignedcpunumber"    => "Várakozósoronkénti CPU-k (ELAVULT)",
                                      "nordugrid-queue-assignedcputype"      => "CPU típus (ELAVULT)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Tulajdonos",
                                      "nordugrid-job-execcluster"            => "Végrehajtási klaszter",
                                      "nordugrid-job-execqueue"              => "Végrehajtási várakozósor",
                                      "nordugrid-job-stdout"                 => "Szabványos kimeneti fájl",
                                      "nordugrid-job-stderr"                 => "Szabványos hiba fájl",
                                      "nordugrid-job-stdin"                  => "Szabványos bemeneti fájl",
                                      "nordugrid-job-reqcputime"             => "Kért CPU idő",
                                      "nordugrid-job-reqwalltime"            => "Kért $quot;wall clock$quot; idő",
                                      "nordugrid-job-status"                 => "Állapot",
                                      "nordugrid-job-queuerank"              => "A városkozási sorban lévő pozíciója",
                                      "nordugrid-job-comment"                => "LRMS megjegyzés",
                                      "nordugrid-job-submissionui"           => "Submission machine",
                                      "nordugrid-job-submissiontime"         => "Részfeladat idő (GMT)",
                                      "nordugrid-job-usedcputime"            => "Felhasznált CPU idő",
                                      "nordugrid-job-usedwalltime"           => "Felhasznált &quot;wall clock&quot; idő",
                                      "nordugrid-job-completiontime"         => "Elkészítési idő (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Törlési idő (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxy lejárati idő (GMT)",
                                      "nordugrid-job-usedmem"                => "Felhasznált memória (KB)",
                                      "nordugrid-job-errors"                 => "Hibák",
                                      "nordugrid-job-exitcode"               => "Kilépési kód",
                                      "nordugrid-job-jobname"                => "Név",
                                      "nordugrid-job-runtimeenvironment"     => "Futásidejű környezet",
                                      "nordugrid-job-cpucount"               => "Kért CPU-k",
                                      "nordugrid-job-executionnodes"         => "Végrehajtási csomópontok",
                                      "nordugrid-job-gmlog"                  => "GM napló fájl",
                                      "nordugrid-job-clientsoftware"         => "Kliens verzió",
                                      "nordugrid-job-rerunable"              => "újra futtatható",
                                      "nordugrid-job-reqcput"                => "Kért idő (ELAVULT)",
                                      "nordugrid-job-gridlog"                => "Grid napló fájl (ELAVULT)",
                                      "nordugrid-job-lrmscomment"            => "LRMS megjegyzés (ELAVULT)",
                                      "nordugrid-authuser-name"              => "Név",
                                      "nordugrid-authuser-sn"                => "Téma neve",
                                      "nordugrid-authuser-freecpus"          => "Szabad CPU-k",
                                      "nordugrid-authuser-diskspace"         => "Szabad hely (MB)",
                                      "nordugrid-authuser-queuelength"       => "A felhasználó várakozósoros job-jai",
                                      "nordugrid-se-name"                    => "Név",
                                      "nordugrid-se-aliasname"               => "Az adattároló elem álneve",
                                      "nordugrid-se-type"                    => "Az adattároló elem típusa",
				      "nordugrid-se-acl"                     => "Engedélyezett VO-k",
                                      "nordugrid-se-freespace"               => "Szabad hely (MB)",
                                      "nordugrid-se-totalspace"              => "Összes lemezterület (MB)",
                                      "nordugrid-se-url"                     => "URL elérhetőség",
                                      "nordugrid-se-baseurl"                 => "URL elérhetőség (ELAVULT)",
                                      "nordugrid-se-accesscontrol"           => "Hozzáférés ellenőrzése",
                                      "nordugrid-se-authuser"                => "Engedélyezett felhasználó (DN)",
                                      "nordugrid-se-location"                => "Irányítószám",
                                      "nordugrid-se-owner"                   => "Tulajdonos",
                                      "nordugrid-se-middleware"              => "Köztesréteg",
                                      "nordugrid-se-issuerca"                => "Tanúsítvány kibocsátó",
                                      "nordugrid-se-issuerca-hash"           => "Tanúsítvány kibocsátó hash-e",
                                      "nordugrid-se-trustedca"               => "Megbízható tanúsítvány kibocsájtók",
                                      "nordugrid-se-comment"                 => "Megjegyzés",
                                      "nordugrid-rc-name"                    => "Tartomány név",
                                      "nordugrid-rc-aliasname"               => "Replika katalógus alias",
                                      "nordugrid-rc-baseurl"                 => "URL elérhetőség",
                                      "nordugrid-rc-authuser"                => "Engedélyezett felhasználó (DN)",
                                      "nordugrid-rc-location"                => "Irányítószám",
                                      "nordugrid-rc-owner"                   => "Tulajdonos",
                                      "nordugrid-rc-issuerca"                => "Tanúsítvány kibocsátó"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Nem tudom olvasni a top level GIS szerverek index szolgáltatásait",
                                      "2"   => "Egyik helyi index sem jelzett vissza kapcsolatot",
                                      "3"   => " rossz konfiguráció, vagy kérési időtúllépés",
                                      "4"   => "Grid-es job nem található",
                                      "5"   => "Nincs információ",
                                      "6"   => "A szervert nem lehet elérni",
                                      "7"   => " - frissítés később",
                                      "8"   => "Nincs információ a várakozási sorról",
                                      "9"   => "Nem található bejegyzés",
                                      "10"  => "Nincs felhasználó",
                                      "11"  => "Nincs megfelelő jogosultsága ennél a hosztnál ",
                                      "12"  => "nincs válasz",
                                      "13"  => "Nincsenek nemrégi feladatok ",
                                      // debug messages
                                      "101" => " Monitorozási időtúllépések a GRIS esetén: ",
                                      "102" => " mp kapcsolódáskor és ",
                                      "103" => " mp kereséskor",
                                      "104" => " mp (keresésre szánt idő)",
                                      "105" => "Az erőforrásokat csupán a következőben mutatja meg ",
                                      "106" => "Lekérdezett felsőszintű indexek: ",
                                      "107" => "Kapott földrajzi helyek, átvizsgált site-ok:  ",
                                      "108" => " site-ok intézése földrajzi helyek szerint",
                                      "109" => "Klaszter attribútumok keresése",
                                      "110" => "A várakozási sor attribútumainak a keresése",
                                      "111" => "Nincs adat erről ",
                                      "112" => " működőképes ",
                                      "113" => " nincs erőforrása, amit felkínálhat",
                                      "114" => " Monitorozási időtúllépések a GIIS esetén: ",
                                      "115" => "GRIS kihagyása: ",
                                      "116" => "nem egy ",
                                      "117" => "Kapcsolat ellenőrzése: ",
                                      "118" => "OK",
                                      "119" => "Eddig, ebből a fajta erőforrásból ",
                                      "120" => "LDAP hiba keresése ",
                                      "121" => " állapot ",
                                      "122" => "Fekete listára került: ",
                                      "123" => "Regisztálót találtam a következő számára ",
                                      "124" => "SE-s attribútumok keresése",
                                      "125" => "Felhasználók keresése",
                                      "126" => "Jobok keresése",
                                      "127" => " van job-ja ",
                                      "128" => " amíg nincsen engedélyezve ",
                                      "129" => "Nem lehet elérni az objektum adatait: hiba ",
                                      // icon titles
                                      "301" => "Frissítés",
                                      "302" => "Nyomtatás",
                                      "303" => "Súgó",
                                      "304" => "Bezár",
                                      "305" => "Piros",
                                      "306" => "Szürke",
                                      "307" => "Minden felhasználó",
                                      "308" => "Aktív felhasználók",
                                      "309" => "Keresés",
                                      "310" => "Adattároló",
                                      "311" => "VO-k",
                                      "312" => "Zászlaja ",
                                      "313" => " Grid-es feldolgozás és ",
                                      "314" => " helyi feldolgozás",
                                      // auxiliary strings
                                      "401" => "Feldolgoz",
                                      "402" => "Grid",
                                      "403" => "Helyi",
                                      "404" => "Világ",
                                      "405" => "TELJES",
                                      "406" => " site-ok",
                                      "407" => "rengeteg",
                                      "408" => " GB",
                                      "409" => " MIND",
                                      "410" => "Klaszter",
                                      "411" => "Várakozási sor",
                                      "412" => "Job",
                                      "413" => "Felhasználó",
                                      "414" => "Adattároló",
                                      "415" => "Replika katalógus",
                                      "416" => "Attribútumok megadása az objektum megjelenítése miatt: ",
                                      "417" => "Minden kifejezés összevetésre fog kerülni",
                                      "418" => "A jobb szélső mezőt hagyja üresen azért, hogy mindent lásson",
                                      "419" => "A kiválasztott erőforrások vagy objektumok megjelenítése",
                                      "420" => "Megkülönböztető név",
                                      "421" => "Összesen használni tud ",
                                      "422" => " site-ot",
                                      "423" => "Erőforrás / objektum:",
                                      "424" => "Attribútumok száma (alap.: 6):",
                                      "425" => "Objektum",
                                      "426" => "Következő",
                                      "427" => "Válassz ki egyet",
                                      "428" => "Törlés",
                                      "429" => "Mutat"
                                      ),
                  // Post code conversion
                  "tlconvert" => array (
                                        "Australia" => "Ausztrália",	   
					"Austria" => "Ausztria",	   
					"Armenia" => "Örményország",	   
					"Algeria" => "Algéria",	   
					"Belgium" => "Belgium",	   
					"Bulgaria" => "Bulgária",	   
                                        "Canada" => "Kanada",	   
                                        "China" => "Kína",	   
					"Czechia" => "Cseszlovákia",	   
                                        "Denmark" => "Dánia",	   
                                        "Estonia" => "észtország",	   
                                        "Finland" => "Finnország",	   
					"France" => "Franciaország",	   
					"Georgia" => "GrúÉzia",	   
                                        "Germany" => "Németország",	   
					"Greece" => "Görögország",	   
					"Hungary" => "Magyarország",	   
                                        "Iceland" => "Izland",	  			   
					"Ireland" => "írország",	   
					"Italy" => "Olaszország",	   
                                        "Japan" => "Japán",	   
                                        "Latvia" => "Lettország",	   
                                        "Lithuania" => "Litvánia",	   
					"Morocco" => "Marokkó",	   
					"Netherlands" => "Hollandia",	   
                                        "Norway" => "Norvégia",	   
					"Poland" => "Lengyelország",	   
					"Portugal" => "Portugália",	   
					"Romania" => "Románia",	   
					"Russia" => "Oroszország",	   
					"SriLanka" => "Sri Lanka",	   
                                        "Sweden" => "Svédország",	   
                                        "Slovakia" => "Szlovákia",	   
                                        "Slovenia" => "Szlovénia",	   
                                        "Switzerland" => "Svájc",	   
                                        "Turkey" => "Törökország",	   
					"UK" => "UK",		   
					"Ukraine" => "Ukrajna",	   
					"USA" => "USA"             
                                        )
                  );

?>
