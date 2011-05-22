<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- Danish [da]
// -- Author: Niels Elgaard Larsen  elgaard@diku.dk

// Some common strings:

$clickable = "KLIKBAR";

$str_att = ", menneskel&aelig;sbart uddrag af nogle af MDS-attributterne. Tryk p&aring; attributnavnet for at f&aring; en liste af attributvardier for hele ARC-universet.";

$str_val = "Attributv&aelig;rdi som indtastet i informationssystemet.";

$str_que = "Som regel svarer forskellige k&oslash;er til forskellige tilladte varigheder for en opgave eller forskellige grupper af brugere. Tryk p&aring; et k&oslash;navn for at f&aring; detaljerede oplysninger om k&oslash;en, inklusiv k&oslash;rende, ventende og f&aelig;rdige opgaver.";

$str_job = "Navn p&aring; job som tildelt af jobejeren.
          <br> <font color=\"red\">N/A</font> viser at brugeren ikke tildelte et navn.
          <br> <font color=\"red\"><b>X</b></font> viser at jobbet er sl&aring;et ihjel af ejeren.
          <br> <font color=\"red\"><b>!</b></font> viser at jobbet fejlede i systemet.
          <br> Tryk p&aring; et navn for at f&aring; en detaljeret beskrivelse af jobbet.";

$str_nam = "Navn p&aring; brugeren som angivet i det personlige certifikat. Tryk p&aring; navnet for at f&aring; en liste af alle ressourcer tilg&aelig;ngelige for denne bruger og alle brugerens job i systemet.";

$str_sta = "Jobstatus som returneret af Gridmanageren (GM) og
   LRMS. Tilstandene er i sekventiel r&aelig;kkef&oslash;lge:
   <dl>
     <dt>ACCEPTED &ndash; jobbet er overf&oslash;rt til systemet men endnu ikke behandlet</dt>
     <dt>PREPARING &ndash; inputfilerne hentes</dt>
     <dt>SUBMITTING &ndash; der udveksles data med LRMS</dt>
     <dt>INLRMS &ndash; jobbet overf&oslash;res til LRMS; intern
       status tilf&oslash;jes af informationssystemet. Mulige tilstande er:</dt>
       <dd>: Q &ndash; jobbet venter i k&oslash;
       <br>: U &ndash; jobbets udf&oslash;relse er udskudt p&aring; en travl knude (PBSPro)
       <br>: S &ndash; jobbets udf&oslash;relse er udskudt (Condor)
       <br>: R, run &ndash; jobbet k&oslash;rer
       <br>: E &ndash; jobbet er f&aelig;rdigt (PBS)
     </dd>
     <dt>FINISHING &ndash; uddatafilerne overf&oslash;res af GM</dt>
     <dt>FINISHED &ndash; jobbet er f&aelig;rdigt; tidsstemplet tilf&oslash;jes af informationssystemet</dt>
     <dt>CANCELING &ndash; jobbet afbrydes</dt>
     <dt>DELETED &ndash; jobbet er ikke afryddet p&aring; anmodning af brugeren men fjernet af GM fordi udl&oslash;bstiden er passeret</dt>
   </dl>
   Hver tilstand kan rapporteres med et PENDING pr&aelig;fiks som betyder at GM fors&oslash;ger at rykke jobbet op i n&aelig;ste tilstand";

$str_tim = "CPU-tid brugt at jobbet, minutter.";

$str_mem = "Lager (RAM) brugt af jobbet, KB";

$str_cpu = "Antal processorer brugt af jobbet.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitor",
                                      "help" => "<div align=\"left\">
                                              <dl>
                                               <dt>
                                                  Denne sk&aelig;rmside viser alle steder, der registrerer sig hos den 
																								  &oslash;verste ARC indekseringstjeneste sorteret f&oslash;rst efter land s&aring; efter
					 																				maskinnavn. Udvalgte lokale parametre overv&aring;ges: klyngealias, k&oslash;, job, o.s.v.
																									Brug quot;Search&quot; tjenesten hvis du vil sammenligne klynger, k&oslash;er, job, osv.                                     
                                            </dt>
                                             <dt><b>Land</b></dt>
                                                  <dd>".$clickable.". Landets flag og navn fra tilg&aelig;ngelig resource.
                                                     Tryk for vise landeinformation. </dd>
                                                  <dt><b>Klynge</b></dt>
                                                  <dd>".$clickable.". Klyngealias som tildelt af ejeren. H&oslash;jst 22 tegn
						 																				vises. Tryk p&aring; aliases for deltaljerede klyngebeskrivelse.</dd>
                                                  <dt><b>CPU-er</b></dt>
                                                  <dd>Totalt antal CPU-er i en klynge. <b>NB!</b> Kun en del af disse kan bruges af Grid-brugere.</dd>
                                                  <dt><b>Belastning (processer:Grid + lokale)</b></dt>
                                                  <dd>".$clickable.". Relativ klyngebelastning, svarende til antallet af optagede CPU-er
                                                      . Gr&aring; felter viser processorer optaget af lokale
						 																		job, r&oslash;de felter viser CPU-er optaget af Grid-job. Tryk
                                                 p&aring; feltet for at f&aring; en detaljeret liste med alle k&oslash;rende Grid-job p&aring; klyngen
                                                , inklusiv antallet af processorer per job.</dd>
                                                  <dt><b>job I K&oslash;</b></dt>
                                                  <dd>".$clickable.". Antal job i k&oslash;en p&aring; klyngen, vist som antallet Grid-job plus antal lokale job i
																											 k&oslash;en. Tryk p&aring; det f&oslash;rste antal for at f&aring; en liste af Grid-job i k&oslash;en p&aring; klyngen</dd>
                                                </dl>
                                                </div>",
                                      "Land"                      => 30,
                                      "Sted"                         => 160,
                                      "CPU-er"                         => 10,
                                      "Belastning (processer: Grid+lokale)" => 210,
                                      "I k&oslash;"                     => 10
                                      ),
                  "clusdes"  => array("0" => "Ressourcedetaljer for",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attribut</b></dt>
                                                  <dd>".$clickable.". Klyngeattributnavn".$str_att."</dd>
                                                  <dt><b>V&aelig;rdi</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>K&oslash;</b></dt>
                                                  <dd>".$clickable.". Navn p&aring; batchk&oslash;er til r&aring;dighed for ARC brugere, 
																										som angivet af klyngeejere. ".$str_que."</dd>
                                                  <dt><b>Status</b></dt>
                                                  <dd>K&oslash;status. Fungerende k&oslash;er viser som regel
                                                    <i>active</i> status.</dd>
                                                  <dt><b>CPU (min)</b></dt>
                                                  <dd>Tidsgr&aelig;nse for varigheden af job per k&oslash;, hvis sat, i
                                                    CPU-minutter. Den f&oslash;rste v&aelig;rdi er den nedre gr&aelig;nse, den anden er den &oslash;vre gr&aelig;nse.
																										 Hvis der ikke er en gr&aelig;nse (job med envher varighed accepteres), vises <b>N/A</b>.</dd>
                                                  <dt><b>K&oslash;rende</b></dt>
                                                  <dd>Antal job der udf&oslash;res i k&oslash;en. Det totale antal job
                                                    is vises, med antallet af <b>processorer</b> optaget af
                                                    Grid-job vist i parentes, fx (Grid:
                                                    12). NB! For parallelle multiprocessorjob kan tallet i
                                                    parentes v&aelig;re st&oslash;rre end antallet af job.</dd>
                                                  <dt><b>I k&oslash;</b></dt>
                                                  <dd>Antallet af job, der venter i k&oslash;en p&aring; at komme til at k&oslash;re. Det totale antal vises, 
																									med Grid-job i parentes, fx (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "K&oslash;"                        => 0,
                                      "Status"                       => 0,
                                      "Gr&aelig;nse (min)"                 => 0,
                                      "CPU-er"                         => 0,
                                      "k&oslash;rende"                      => 0,
                                      "I K&oslash;"                     => 0
                                      ),
                  "jobstat"  => array("0" => "Job p&aring;:Job ID",
                                      "help" => "<div align=\"left\">
                                                <b>JOBLISTE:</b>
                                                <dl>
                                                <dt><b>Job navn</b></dt>
                                                <dd>".$clickable.". Navn p&aring; et job som tildelt af ejeren. Hvis der ikke er tildelt
                                                  et navn, vises &quot;<span style={color:red;}>N/A</span>&quot;. Tryk
                                                  p&aring; et navn for at f&aring; en detaljeret beskrivelse af jobbet.</dd>
                                                <dt><b>Ejer</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>K&oslash;</b></dt>
                                                <dd>".$clickable.". Navn p&aring; batchk&oslash;en, hvor jobbet udf&oslash;res. ".$str_que."</dd>
                                                <dt><b>CPU'er</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>JOBDETALJER:</b>
                                                <dl>
                                                <dt><b>Attribut</b></dt>
                                                <dd>".$clickable.". Jobattributens navn".$str_att."</dd>
                                                <dt><b>V&aelig;rdi</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Job navn"                     => 0,
                                      "Ejer"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "K&oslash;"                        => 0,
                                      "CPU'er"                         => 0
                                      ),
                  "volist"   => array("0" => "Virtuel Organisations",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuel Organisation</b></dt>
                                                   <dd>".$clickable.". Group of users, typically sharing common
                                                     activities and resources, authorised at at least one
                                                     ARC-enabled site. Click on the navn to get the list of
                                                     group members.</dd>
                                                   <dt><b>Members</b></dt>
                                                   <dd>Number of group members.</dd>
                                                   <dt><b>Served by</b></dt>
                                                   <dd>LDAP server that supports the group membership
                                                     database.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtuel Organisation"         => 0,
                                      "Medlemmer"                      => 0,
                                      "Server"                    => 0
                                      ),
                  "vousers"  => array("0" => "Gridbrugerbase",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Navn</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Tilknytning</b></dt>
                                                   <dd>Brugerens hjemmeinstitut som anmeldt af VO bestyreren. Kan v&aelig;re tom.</dd>
                                                   <dt><b>E-mail</b></dt>
                                                   <dd>".$clickable.". Brugerens e-mail som anmeldt af VO bestyreren. Kan v&aelig;re tom.
																										Tryk p&aring; adressen for sende en email til brugeren.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Navn"                         => 0,
                                      "Tilknytning"                  => 0,
                                      "E-mail"                       => 0
                                      ),
                  "userlist" => array("0" => "Information om",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Klynge:k&oslash;</b></dt>
                                                <dd>".$clickable.". Navne p&aring; klynger of respektive k&oslash;er
                                                  (adskilt af kolon, &quot;:&quot;) hvor en bruger er
                                                  autoriseret til at indlevere job. Hvis brugeren ikke er autoriseret,
                                                  vises beskeden: &quot;<b><font color=\"red\">Ikke authoriset p&aring; v&aelig;rt
                                                      ...</font></b>&quot;. Tryk p&aring; et klyngenavn
                                                  for at f&aring; en detaljeret beskrivelse af klyngen. 
																									Tryk p&aring; et k&oslash;navn for at f&aring; en detaljeret beskrivelse af k&oslash;en</dd>
                                                <dt><b>Ledige CPU'er</b></dt>
                                                <dd>Det aktuelle antal ledige CPU'er i en given k&oslash; til r&aring;dighed for brugeren 
 																									, evt tilf&oslash;jet den &oslash;vre gr&aelig;nse for varigheden af jobs (i minutter). Fx betyder
                                                  &quot;3&quot; at der er 3 CPU'er til r&aring;dighed for et job med ubegr&aelig;nset
                                                  k&oslash;retid; &quot;4:360&quot; angiver at der er  4 CPU'er
                                                  til r&aring;dighed for job, der k&oslash;rer mindre end 6 timer; &quot;10:180
                                                  30&quot; betyder at der er 10 CPU'er til r&aring;dighed for job, der k&oslash;rer mindre end 3 timer, 
																								  samt 30 CPU'er til r&aring;dighed for jobs, der kan k&oslash;re en vilk&aring;rlig tid; 
																									&quot;0&quot; betyder at der ikke er nogen  CPU'er til r&aring;dighed for tiden
                                                  the moment, og nye job vil havne i en ventek&oslash;.</dd>
                                                <dt><b>Job i ventek&oslash;</b></dt>
                                                <dd>Antal brugerjob, der forventes at v&aelig;re foran en brugers nye job i en ventek&oslash; (for this user).
																									Antallet &quot;0&quot; betyder at jobbet forventes udf&oslash;rt med det samme. 
																									<b>NB!</b> Det er kun et estimat, som kan tilsides&aelig;ttes af lokale regler.</dd>
                                                <dt><b>Ledig diskplads (MB)</b></dt>
                                                <dd>Diskplads til r&aring;dighed for brugeren i en given bestemt k&oslash; (i
                                                  MegaBytes). <b>NB!</b> Det er kun et estimat, da de f&aelig;rreste klynges kan tilbyde faste diskpladskvoter.</dd>
                                                <dt><b>Jobnavn</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Klynge</b></dt>
                                                <dd>".$clickable.". Navn p&aring; klyngen hvor jobbet udf&oslash;res. Tryk p&aring; et klyngenavn for at f&aring; detaljerede
                                                  oplysninger om klyngen.</dd>
                                                <dt><b>K&oslash;</b></dt>
                                                <dd>".$clickable.". Navn p&aring; batchk&oslash;en hvor jobbet udf&oslash;res eller blev udf&oslash;rt. ".$str_que."</dd>
                                                <dt><b>CPUs</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Jobnavn"                     => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Klynge"                      => 0,
                                      "K&oslash;"                        => 0,
                                      "CPU'er"                         => 0
                                      ),
                  "attlist"  => array("0" => "Attributv&aelig;rdier",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objekt</b></dt>
                                                   <dd>".$clickable.". Navn objektet hvis attributter vises.
                                                     Det kan v&aelig;re et klyngenavn, et klyngek&oslash;navn,
                                                     et jobnavn, et brugernavn osv. Tryk p&aring; navneteksten for at f&aring; en detaljeret beskrivelse af objektet.</dd>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>For hvert objekt kan en eller flere v&aelig;rdier vises.
                                                     Kolonnetitel er det menneskel&aelig;selige navn
                                                     (bortset fra visse MDS-specifikke attributter) og indholdet i kolonnen er
                                                     attributv&aelig;rdier for objektet som det blev til indtastet i informationssystemet.</dd>
                                                 </dl>
                                                 </div>",
                                      "Objelt"                       => 0,
                                      "Attribut"                    => 0
                                      ),
                  "quelist"  => array("0" => "K&oslash;",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>".$clickable.". Navn p&aring; en k&oslash;attribut".$str_att."</dd>
                                                   <dt><b>V&aelig;rdi</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Jobnavn</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Ejer</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Status</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Lager (RAM) (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPUs</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Jobnavn"                     => 0,
                                      "Ejer"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Lager (RAM) (KB)"                  => 0,
                                      "CPU'er"                         => 0
                                      ),
                  "sestat"   => array("0" => "Lagerenhed",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Lagerenheder alias som angivet i informationssystemet.
																									 H&oslash;jst 15 tegn vises.</dd>
                                                  <dt><b>Total lagerplads</b></dt>
                                                  <dd>Total lagetplads p&aring; harddisken, GigaByte.</dd>
                                                  <dt><dt><b>Ledig Plads</b></dt>
                                                  <dd>Aktuel Ledig diskplads, GigaByte.</dd>
                                                  <dt><b>Navn</b></dt>
                                                  <dd>Lagerenheds, best&aring;ende af et logsik navn og et v&aelig;rtsnavn
                                                    (adskilt af kolon, &quot;:&quot;). Det logiske navn
                                                    bruges kun af hensyn til informationssystemet, for at skelne mellem 
                                                    forskellige lagringsenheder p&aring; den samme maskine.</dd>
                                                  <dt><b>Grund-URL</b></dt>
                                                  <dd>Lagringsenhedens URL, som regel en gsiftp://
                                                    protokol. Brug denne URL som udgangspunkt for at tilg&aring; filer.</dd>
                                                  <dt><b>Type</b></dt>
                                                  <dd>Lagringselement type. &quot;gridftp-baseret&quot; angiver
                                                    en harddisk med GridFTP-gr&aelig;nseflade.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
				      //                                      "Total lagerplads"                   => 0,
                                      "Ledig/Total Diskplads, GB"    => 0,
                                      "Navn"                         => 0,
                                      "Grund-URL"                    => 0,
                                      "Type"                         => 0
                                      ),
                  "allusers" => array("0" => "Authoriserede Gridbrugere:Aktive Gridbrugere",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Navn</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Tilknytning</b></dt>
                                                   <dd>Brugertilknytning, uddraget af det personlige  certifikat</dd>
                                                   <dt><b>Job</b></dt>
                                                   <dd>Antal brugerjob i systemet (k&oslash;rende, ventende,
                                                     afsluttede eller slettede)</dd>
                                                   <dt><b>Steder</b></dt>
                                                   <dd>Viser hvor mange steder brugeren er autoriseret</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Navn"                         => 0,
                                      "Tilknytning"                   => 0,
                                      "Job"                         => 0,
                                      "Steder"                        => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Klynge:k&oslash;"                => 0,
                                      "Ledige CPU'er"                    => 0,
                                      "Job, i ventek&oslash;"            => 0,
                                      "Ledig diskplads (MegaByte)"               => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attribut"                    => 0,
                                      "V&aelig;rdi"                        => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objektklasse",
                                      "Mds-validfrom" => "Info gyldig fra (GMT)",
                                      "Mds-validto"   => "Info gyldig til (GMT)"
                                      ),
               
                "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Frontend dom&aelig;nenavn",
                                      "nordugrid-cluster-aliasname"          => "Klyngealias",
                                      "nordugrid-cluster-contactstring"      => "Kontakttekst",
                                      "nordugrid-cluster-interactive-contactstring" => "Interaktiv kontakt",
                                      "nordugrid-cluster-comment"            => "Kommentar",
                                      "nordugrid-cluster-support"            => "E-mail kontact",
				      "nordugrid-cluster-acl"                => "Authoriserede VO'er",
                                      "nordugrid-cluster-lrms-type"          => "LRMS type",
                                      "nordugrid-cluster-lrms-version"       => "LRMS version",
                                      "nordugrid-cluster-lrms-config"        => "LRMS detaljer",
                                      "nordugrid-cluster-architecture"       => "Arkitektur",
                                      "nordugrid-cluster-opsys"              => "Styresystem",
                                      "nordugrid-cluster-homogeneity"        => "Homogen klynge",
                                      "nordugrid-cluster-nodecpu"            => "CPU type (langsomste)",
                                      "nordugrid-cluster-nodememory"         => "Lager (MB, mindst)",
                                      "nordugrid-cluster-totalcpus"          => "CPU's, i alt",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:maskiner",
                                      "nordugrid-cluster-benchmark"          => "Benchmark",
                                      "nordugrid-cluster-sessiondir-free"    => "Harddiskplads, til r&aring;dighed (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Harddiskplads, i alt (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Gridsession levetid (min)",
                                      "nordugrid-cluster-cache-free"         => "Cachest&oslash;rrelse, til r&aring;dighed (MB)",
                                      "nordugrid-cluster-cache-total"        => "Cachest&oslash;rrelse, i alt (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "K&oslash;retidsomgivelser",
                                      "nordugrid-cluster-localse"            => "Lagringsenhed, lokal",
                                      "nordugrid-cluster-middleware"         => "Gridmiddleware",
                                      "nordugrid-cluster-totaljobs"          => "Jobs, samlet antal",
                                      "nordugrid-cluster-usedcpus"           => "CPU'er, optagede",
                                      "nordugrid-cluster-queuedjobs"         => "Jobs, i k&oslash;",
				      "nordugrid-cluster-prelrmsqueued"      => "Grid jobs, awaiting submission",
                                      "nordugrid-cluster-location"           => "Postnummer",
                                      "nordugrid-cluster-owner"              => "Ejer",
                                      "nordugrid-cluster-issuerca"           => "Certifikatudsteder",
                                      "nordugrid-cluster-issuerca-hash"      => "Certificate issuer's hash",
                                      "nordugrid-cluster-trustedca"          => "Trusted certificate issuers",
                                      "nordugrid-cluster-nodeaccess"         => "Node IP sammenh&aelig;ng",
                                      "nordugrid-cluster-gridarea"           => "Session area (FORфLDET)",
                                      "nordugrid-cluster-gridspace"          => "Griddiskplads (FORфLDET)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS fordeling (FORфLDET)",
                                      "nordugrid-cluster-runningjobs"        => "Job, k&oslash;rende (FORфLDET)",
				      "nordugrid-cluster-credentialexpirationtime" => "Credential expiration time",
                                      "nordugrid-queue-name"                 => "K&oslash;navn",
                                      "nordugrid-queue-comment"              => "Kommentar",
                                      "nordugrid-queue-status"               => "K&oslash;status",
                                      "nordugrid-queue-running"              => "CPU'er, optagede",
				      "nordugrid-queue-localqueued"          => "Local jobs, queued",
				      "nordugrid-queue-prelrmsqueued"        => "Grid jobs, awaiting submission",
                                      "nordugrid-queue-queued"               => "Job, i ventek&oslash; (FORфLDET)",
                                      "nordugrid-queue-maxrunning"           => "Jobs, k&oslash;rende (max)",
                                      "nordugrid-queue-maxqueuable"          => "Jobs, kan udskydes (max)",
                                      "nordugrid-queue-maxuserrun"           => "Jobs per Unixbruger (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU-tid, max. (minutter)",
                                      "nordugrid-queue-mincputime"           => "CPU-tid, min. (minutter)",
                                      "nordugrid-queue-defaultcputime"       => "CPU-tid, default (minutter)",
                                      "nordugrid-queue-maxwalltime"          => "V&aelig;gurstid, max. (minutter)",
                                      "nordugrid-queue-minwalltime"          => "V&aelig;gurstid, min. (minutter)",
                                      "nordugrid-queue-defaultwalltime"      => "V&aelig;gurstid, default (minutter)",
                                      "nordugrid-queue-schedulingpolicy"     => "Skeduleringspolitik",
                                      "nordugrid-queue-totalcpus"            => "CPU'er, i alt",
                                      "nordugrid-queue-nodecpu"              => "CPUtype",
                                      "nordugrid-queue-nodememory"           => "Lager (RAM) (MB)",
                                      "nordugrid-queue-architecture"         => "Arkitektur",
                                      "nordugrid-queue-opsys"                => "Styresystem",
                                      "nordugrid-queue-homogeneity"          => "Homogen k&oslash;",
                                      "nordugrid-queue-gridrunning"          => "CPU'er, optagede af Gridjobs",
                                      "nordugrid-queue-gridqueued"           => "Gridjobs, i ventek&oslash;",
                                      "nordugrid-queue-benchmark"            => "Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "CPU'er per k&oslash; (FORфLDET)",
                                      "nordugrid-queue-assignedcputype"      => "CPUtype (FORфLDET)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Ejer",
                                      "nordugrid-job-execcluster"            => "Execution cluster",
                                      "nordugrid-job-execqueue"              => "Aktivk&oslash;",
                                      "nordugrid-job-stdout"                 => "Standard uddatafil",
                                      "nordugrid-job-stderr"                 => "Standard fejlfil",
                                      "nordugrid-job-stdin"                  => "Standard inddatafile",
                                      "nordugrid-job-reqcputime"             => "Anmodet CPU-tid",
                                      "nordugrid-job-reqwalltime"            => "Anmodet v&aelig;gurstid",
                                      "nordugrid-job-status"                 => "Status",
                                      "nordugrid-job-queuerank"              => "Position i k&oslash;en",
                                      "nordugrid-job-comment"                => "LRMS kommentar",
                                      "nordugrid-job-submissionui"           => "Indleveringsmaskine",
                                      "nordugrid-job-submissiontime"         => "Indleveringstid (GMT)",
                                      "nordugrid-job-usedcputime"            => "Forbrugt CPU-tid",
                                      "nordugrid-job-usedwalltime"           => "Forbrugt v&aelig;gurstid",
                                      "nordugrid-job-completiontime"         => "Job afslutningstidspunkt (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Sletningstidspunkt (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxy udl&oslash;bstidspunkt (GMT)",
                                      "nordugrid-job-usedmem"                => "Benyttet lager (RAM) (KB)",
                                      "nordugrid-job-errors"                 => "Fejl",
                                      "nordugrid-job-exitcode"               => "Afslutningskode",
                                      "nordugrid-job-jobname"                => "Navn",
                                      "nordugrid-job-runtimeenvironment"     => "K&oslash;retidsomgivelser",
                                      "nordugrid-job-cpucount"               => "Anmodede CPU'er",
                                      "nordugrid-job-executionnodes"         => "Udf&oslash;relsesknuder",
                                      "nordugrid-job-gmlog"                  => "GM logfil",
                                      "nordugrid-job-clientsoftware"         => "klientversion",
                                      "nordugrid-job-rerunable"              => "Genk&oslash;rbare",
                                      "nordugrid-job-reqcput"                => "Anmodet tid (For&aelig;ldet)",
                                      "nordugrid-job-gridlog"                => "Gridlogfil (For&aelig;ldet)",
                                      "nordugrid-job-lrmscomment"            => "LRMS kommentar (For&aelig;ldet)",
                                      "nordugrid-authuser-name"              => "Navn",
                                      "nordugrid-authuser-sn"                => "Subjektnavn",
                                      "nordugrid-authuser-freecpus"          => "Ledige CPU'er",
                                      "nordugrid-authuser-diskspace"         => "Ledig harddiskplads (MB)",
                                      "nordugrid-authuser-queuelength"       => "User's queued jobs",
                                      "nordugrid-se-name"                    => "Nabn",
                                      "nordugrid-se-aliasname"               => "Lagerelement alias",
                                      "nordugrid-se-type"                    => "Lagerelement type",
				      "nordugrid-se-acl"                     => "Authoriserede VO'er",
                                      "nordugrid-se-freespace"               => "Legid plads (MB)",
                                      "nordugrid-se-totalspace"              => "Total plads (MB)",
                                      "nordugrid-se-url"                     => "Kontakt URL",
                                      "nordugrid-se-baseurl"                 => "Kontact URL (For&aelig;ldet)",
                                      "nordugrid-se-accesscontrol"           => "Adgangskontrol",
                                      "nordugrid-se-authuser"                => "Autoriseret bruger (DN)",
                                      "nordugrid-se-location"                => "Postnummer",
                                      "nordugrid-se-owner"                   => "Ejer",
                                      "nordugrid-se-middleware"              => "Middleware",
                                      "nordugrid-se-issuerca"                => "Certifikatudsteder",
                                      "nordugrid-se-issuerca-hash"           => "Certificate issuer's hash",
                                      "nordugrid-se-trustedca"               => "Trusted certificate issuers",
                                      "nordugrid-se-comment"                 => "Kommentar",
                                      "nordugrid-rc-name"                    => "Dom&aelig;nenavn",
                                      "nordugrid-rc-aliasname"               => "Replikeringskatalogalias",
                                      "nordugrid-rc-baseurl"                 => "Kontact URL",
                                      "nordugrid-rc-authuser"                => "Autoriseret bruger (DN)",
                                      "nordugrid-rc-location"                => "Postnummer",
                                      "nordugrid-rc-owner"                   => "Ejer",
                                      "nordugrid-rc-issuerca"                => "Certifikatudsteder"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Kan ikke topniveau ressourceindekser",
                                      "2"   => "ingen af de lokale indekser returnerede en forbindelse",
                                      "3"   => " d&aring;rlig konfigurations eller anmodning udl&oslash;b",
                                      "4"   => "Ingen Gridjobs fundet",
                                      "5"   => "ingen information fundet",
                                      "6"   => "Server ikke tilg&aelig;ngelig",
                                      "7"   => " - genl&aelig;s senere",
                                      "8"   => "ingen k&oslash;informationer fundet",
                                      "9"   => "Ingen indgange fundet",
                                      "10"  => "Ingen brugere fundet",
                                      "11"  => "Ingen autoriserede p&aring; v&aelig;rten",
                                      "12"  => "svarer ikke",
                                      "13"  => "Fandt ingen nylige jobs for ",
                                      // debug messages
                                      "101" => " Monitor timeout for GRIS: ",
                                      "102" => " sek on forbindelse og ",
                                      "103" => " sek on s&oslash;gning",
                                      "104" => " sek brugt p&aring; at s&oslash;ge",
                                      "105" => "Viser kun ressourcer i  ",
                                      "106" => "Spurgte topniveau indeksservere: ",
                                      "107" => "Fik geokrafiske  placeringer, skanned steder: ",
                                      "108" => " steder sorteret efter geografisk placering",
                                      "109" => "Leder efter klyngeattributter",
                                      "110" => "Leder efter k&oslash;attributter",
                                      "111" => "Ingen daya fra ",
                                      "112" => " er oppe i ",
                                      "113" => " tilbyder ingen ressourcer",
                                      "114" => " Monitor timeouts for GIIS: ",
                                      "115" => "springer over GRIS: ",
                                      "116" => "ikke en ",
                                      "117" => "Checker forbindelse: ",
                                      "118" => "OK",
                                      "119" => "S&aring; vidt, opdagede ressource at typen ",
                                      "120" => "LDAP fejl ved s&oslash;gning ",
                                      "121" => " status ved ",
                                      "122" => "Sortlistet: ",
                                      "123" => "Registrant fundet for ",
                                      "124" => "Led efter SE attributter",
                                      "125" => "Led efter brugere",
                                      "126" => "Led efter jobs",
                                      "127" => " har job ",
                                      "128" => " men ikke autoriseret",
                                      "129" => "Kan ikke f&aring; objektdata: fejl ",
                                      // icon titles
                                      "301" => "Genl&aelig;s",
                                      "302" => "Udskriv",
                                      "303" => "Hj&aelig;lp",
                                      "304" => "Luk",
                                      "305" => "R&oslash;d",
                                      "306" => "Gr&aring;",
                                      "307" => "Alle brugere",
                                      "308" => "Aktive brugere",
                                      "309" => "S&oslash;g",
                                      "310" => "Lager",
                                      "311" => "VO-er",
                                      "312" => "Flaget for ",
                                      "313" => " Grid processer og ",
                                      "314" => " locale processer",
                                      // auxilliary strings
                                      "401" => "Processer",
                                      "402" => "Grid",
                                      "403" => "Lokal",
                                      "404" => "Verden",
                                      "405" => "TOTAL",
                                      "406" => " steder",
                                      "407" => "en masse",
                                      "408" => " GB",
                                      "409" => " ALLE",
                                      "410" => "Klynge",
                                      "411" => "K&oslash;",
                                      "412" => "Job",
                                      "413" => "Bruger",
                                      "414" => "Lager",
                                      "415" => "Replikerings Kat.",
                                      "416" => "Definer attributter for at vise objektet: ",
                                      "417" => "logisk OG af alle udtrykkene findes",
                                      "418" => "Efterlad feltet l&aelig;ngst til h&oslash;jre tomt for vise alt",
                                      "419" => "Vis de ressourcer eller objekter, du vil",
                                      "420" => "Distinguished name",
                                      "421" => "Kan bruge i alt ",
                                      "422" => " steder",
                                      "423" => "Ressource / objekt:",
                                      "424" => "Ant.  attributter (def. 6):",
                                      "425" => "Objekt",
                                      "426" => "N&aelig;ste",
                                      "427" => "V&aelig;lg een",
                                      "428" => "Nulstil",
                                      "429" => "VIS"
                                      ),
                  // Post code conversion: only for [en]!
                  "tlconvert" => array (
                                        "Australia"    => "Australien",
					"Austria"      => "ьstrig",
					"Armenia"      => "Armenien",
					"Algeria"      => "Algeriet",
					"Belgium"      => "Belgien",
					"Bulgaria"     => "Bulgarien",
                                        "Canada"       => "Canada",
                                        "China"        => "Kina",
					"Czechia"      => "Tjekkiet",
                                        "Denmark"      => "Danmark", 
                                        "Estonia"      => "Estland", 
                                        "Finland"      => "Finland", 
					"France"       => "Frankrig",
					"Georgia"      => "Georgien",
                                        "Germany"      => "Tyskland", 
					"Greece"       => "Gr&aelig;kenland",
					"Hungary"      => "Ungarn",
                                        "Iceland"      => "Island",
					"Ireland"      => "Irland",
					"Italy"        => "Italien",
                                        "Japan"        => "Japan", 
                                        "Latvia"       => "Letland", 
                                        "Lithuania"    => "Lithauen", 
					"Morocco"      => "Marocco",
					"Netherlands"  => "Nederlandene",
                                        "Norway"       => "Norge", 
					"Poland"       => "Polen",
					"Portugal"     => "Portugal",
					"Romania"      => "Rum&aelig;nien",
					"Russia"       => "Rusland", 
					"SriLanka"     => "Sri&nbsp;Lanka",
                                        "Sweden"       => "Sverige", 
                                        "Slovakia"     => "Slovakiet", 
                                        "Slovenia"     => "Slovenien", 
                                        "Switzerland"  => "Schweiz", 
                                        "Turkey"       => "Tyrkiet", 
					"UK"           => "UK",
					"Ukraine"      => "Ukraine",
					"USA"          => "USA", 
					"World"        => "Verden"
                                        )
                  );

?>
