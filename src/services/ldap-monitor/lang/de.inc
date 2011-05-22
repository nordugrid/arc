<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- German [de]
// -- Translation: Steffen MÃ ller (moeller@pzr.uni-rostock.de)
// --              Christian Häberli (christian.haeberli@cern.ch)
// -- Author: oxana.smirnova@hep.lu.se

// Some common strings:

$clickable = "ANKLICKBAR";

$str_att = ", direkt lesbar bis auf einige MDS-spezifische Attribute. Klicken Sie auf den Attributnamen, um die Liste aller seiner Werte im ARC zu erhalten.";

$str_val = "Attributwert wie im Informationssystem eingegeben.";

$str_que = "Verschiedene Queues entsprechen verschiedenen Begrenzungen der Rechenzeit oder der Nutzergruppe. Klicken Sie auf den Namen einer Queue, um detailierte Information zu erhalten, einschliesslich der laufenden, wartenden und bereits beendeten Jobs.";

$str_job = "Durch Anwender festgelegter Name des Jobs.
          <br> <font color=\"red\">N/A</font> bezeichnet einen Job ohne Namen.
          <br> <font color=\"red\"><b>X</b></font> bezeichnet einen Job, der durch den Nutzer abgebrochen wurde.
          <br> <font color=\"red\"><b>!</b></font> bezeichnet einen Job, der nicht erfolgreich ausgef&uuml;hrt wurde.
          <br> Klicken Sie auf den jeweiligen Jobnamen f&uuml;r eine detaillierte Beschreibung.";

$str_nam = "Name des Nutzers wie im <i>user certificate</i> spezifiziert. Klicken Sie auf den Namen, um eine Liste aller Ressourcen zu erhalten, die f&uuml;r diesen Nutzer zur Verf&uuml;gung stehen, sowie eine Liste aller Jobs dieses Users im System.";

$str_sta = "Jobstatus wie angegeben durch the Grid Manager (GM) und LRMS.;
   Jobs durchlaufen die Zust&auml;nde in der folgenden Reihenfolge:
   <dl>
     <dt>ACCEPTED &ndash; job wurde submitted aber er wird noch nicht ausgef&uuml;hrt.</dt>
     <dt>PREPARING &ndash; Eingabedateien werden &uuml;bertragen</dt>
     <dt>SUBMITTING &ndash; Interaktion mit dem LRMS</dt>
     <dt>INLRMS &ndash; der Job ist unter der Kontrolle des LRMS; 
       sein interner Zustand wird durch das Infosystem bestimmt.
       M&ouml;gliche solche Zust&auml;nde sind:</dt>
       <dd>: Q &ndash; Job ist in der Queue (queued)
       <br>: U &ndash; Job wurde unterbrochen (suspended) wegen eines anderen Prozesses (PBSPro)
       <br>: S &ndash; Job wurde unterbrochen (suspended) (Condor)
       <br>: R, run &ndash; Job wird ausgef&uuml;hrt
       <br>: E &ndash; Job wird beendet (PBS)
     </dd>
     <dt>FINISHING &ndash; Ausgabedateien werden durch den GM transferiert</dt>
     <dt>FINISHED &ndash; Job wurde beendet, eine Zeitmarke (time stamp) wird durch das Infosystem hinzugef&uuml;gt</dt>
     <dt>CANCELING &ndash; Job wurde abgebrochen</dt>
     <dt>DELETED &ndash; Job wurde nicht nach dem Download des Anwenders, sondern durch
       den GM wegen &Uuml;berschreitung der Ablauffrist (expiration date) gel&ouml;scht.
     </dt>
   </dl>
   Jeder der Zust&aauml;nde kann durch den Prefix PENDING
   gekennzeichnet sein. Der GM versucht dann, diesen Zustand f&uuml;r
   diesen Job zu erreichen.";

$str_tim = "CPU-Zeit des Jobs, gemessen in Minuten.";

$str_mem = "Speicherbedarf des Jobs, gemessen in KB.";

$str_cpu = "Die Anzahl genutzter Prozessoren des Jobs.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    Diese &Uuml;bersicht zeigt alle Teilnehmer, die an der Spitze des ARC registiert sind. Sie sind prim&auml;r nach ihrem Land sortiert und dann anhand deren Namen. Ausgew&auml;hlte Parameter werden &uuml;berwacht: Cluster alias, die Anzahl aller CPUs und solcher reserviert f&uuml;r lokale Jobs, die Anzahl laufender und wartender Auftr&auml;ge. Nutzen Sie die &quot;Search&quot; Funktion, um andere Charakteristica von Clustern, Queues, Auftr&auml;gen, etc. zu vergleichen
						  </dt>
                                                  <dt><b>Land</b></dt>
                                                  <dd>".$clickable.". Landesflagge und -name wie abgeleitet von der <i>resource</i>-Beschreibung. Anklicken, um Informationen zur Gridnutzung dieses Landes zu sehen.</dd>
                                                  <dt><b>Cluster</b></dt>
                                                  <dd>".$clickable.". Alternativer Name des Cluster wie durch dessen <i>owner</i> festgelegt. Es werden maximal 22 Zeichen dargestellt. Durch Anlicken werden detaillierte Informationen zum Cluster dargestellt.</dd>
                                                  <dt><b>CPUs</b></dt>
                                                  <dd>Gesamtanzahl der CPUs im Cluster. <b>NB!</b> Nur ein Teil dieser mag tats&auml;chlich auch f&uuml;r Grid Nutzer verf&uuml;gbar sein.</dd>
                                                  <dt><b>Last (Prozesse:Grid&#43lokal)</b></dt>
                                                  <dd>".$clickable.". Relative Auslastung des Clusters, abgeleitet von der Anzahl belegter CPUs. Graue Balken stellen die mit lokalen Jobs belegten CPUs dar, rote Balken solche, die von &uuml;ber das Grid submitteten Jobs beansprucht sind. Klicke auf die Balken, um eine detaillierte Liste aller Jobs zu erhalten, inklusive der Anzahl genutzter Prozessoren je Job.</dd>
                                                  <dt><b>Wartend</b></dt>
                                                  <dd>".$clickable.".
						      Anzahl aller wartenden Jobs auf dem Cluster, angezeigt als die Anzahl solcher durch das Grid submitteter Jobs plus die Anzahl derjenigen, die lokal submitted wurden. Klicke auf die erste Nummer, um die Liste der wartenden Grid-Jobs zu erhalten.</dd>
                                                </dl>
                                                </div>",
                                      "Land"                        => 30,
                                      "Site"                        => 160,
                                      "CPUs"                        => 10,
                                      "Last (Prozesse: Grid+lokal)" => 210,
                                      "In einer Queue"              => 10
                                      ),
                  "clusdes"  => array("0" => "Details einer Resource zu",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attribut</b></dt>
                                                  <dd>".$clickable.". Cluster Attributename".$str_att."</dd>
                                                  <dt><b>Wert</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Queue</b></dt>
                                                  <dd>".$clickable.". Namen von <i>batch queues</i>
						    verf&uuml;gbar f&uuml; ARC Nutzer,
						    wie festgelegt durch die <i>owner</i> des Clusters."
						    .$str_que."</dd>
                                                  <dt><b>Status</b></dt>
                                                  <dd>Queue status. Eine operationelle Queue hat typischerweise
						    den Status <i>active</i>.</dd>
                                                  <dt><b>CPU (min)</b></dt>
                                                  <dd>Zeitbegrenzung f&uuml;r einen Job. Der erste Wert ist 
						   untere Grenze, der zweite die obere.
                                                    Wenn keine Begrenzungen gesetzt sind, es wird dann
						    jede Laufl&auml;nge akzeptiert, wird <b>N/A</b> angezeigt.
						  </dd>
                                                  <dt><b>Running</b></dt>

                                                  <dd>Die Anzahl von Jobs, die in der Queue aktiv sind.
						    Die Gesamtanzahl der Jobs wird angezeigt, mit der Anzahl
						    belegter Prozessoren in Klammern. 
                                                    NB! F&uuml;r Jobs mit Parallelverarbeitung kann diese Anzahl deutlich h&ouml;her sein als die Anzahl der Jobs.
						  </dd>
                                                  <dt><b>Queing</b></dt>
                                                  <dd>
						    Anzahl von Jobs, die auf deren Ausf&uuml;hrung warten.
						    Die Gesamtanzahl wird angezeigt mit der Anzahl durch das Grid submitteter Jobs in Klammern.</dd>
                                                </dl>
                                                </div>
",
                                      "Queue"                        => 0,
                                      "Status"                       => 0,
                                      "Limiten (min)"                => 0,
                                      "CPUs"                         => 0,
                                      "Running"                      => 0,
                                      "Queueing"                     => 0
                                      ),
                  "jobstat"  => array("0" => "Jobs at:Job ID",
                                      "help" => "<div align=\"left\">
                                                <b>JOB LIST:</b>
                                                <dl>
                                                <dt><b>Job name</b></dt>
                                                <dd>".$clickable.". Name des Jobs wie durch den <i>owner</i> festgelegt. Wenn kein Name zugewiesen wurde, so wird &quot;<span style={color:red;}>N/A</span>&quot; angezeigt. Bei Klick auf dem Namen wird eine detaillierte Beschreibung des Jobs angezeigt.
						</dd>
                                                <dt><b>Owner</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Queue</b></dt>
                                                <dd>".$clickable.". Name der Queue in der der Job ausgef&uuml;hrt wird. ".$str_que."</dd>
                                                <dt><b>CPUs</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>JOB DETAILS:</b>
                                                <dl>
                                                <dt><b>Attribut</b></dt>
                                                <dd>".$clickable.". Job Attributname".$str_att."</dd>
                                                <dt><b>Wert</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Jobname"                      => 0,
                                      "Eigner"                       => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Queue"                        => 0,
                                      "CPUs"                         => 0
                                      ),
                  "volist"   => array("0" => "Virtuelle Organisationen",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtuelle Organisation</b></dt>
                                                   <dd>".$clickable.". Gruppe von Anwendern,
						     &uuml;blicherweise gemeinsame Aktivit&aul;ten und Ressourcen teilend. Wenigstens ein Cluster des ARC akzeptiert diese. Klicken Sie auf den Namen, um eine Liste der Mitglieder zu erhalten.</dd>
                                                   <dt><b>Mitglieder</b></dt>
                                                   <dd>Anzahl der Mitglieder.</dd>
                                                   <dt><b>Verwaltet durch</b></dt>
                                                   <dd>LDAP Server der die Mitglieder-Datenbank h&auml;lt.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtuelle Organisation"         => 0,
                                      "Mitglieder"                      => 0,
                                      "Verwaltet durch"                    => 0
                                      ),
                  "vousers"  => array("0" => "Grid User Base",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Name</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Affiliation</b></dt>
                                                   <dd>Des Nutzers Arbeitgeber wie durch den VO manager angegeben.
                                                     Kann freigelassen werden.</dd>
                                                   <dt><b>E-mail</b></dt>
                                                   <dd>".$clickable.". Des Nutzers eMail-Adresse wie angegeben durch den VO Manager. Darf freigelassen werden. Durch Anlicken der Adresse kann eine eMail an den Nutzer gesendet werden.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Name"                         => 0,
                                      "Zugeh&ouml;rigkeit"           => 0,
                                      "E-mail"                       => 0
                                      ),
                  "userlist" => array("0" => "Information f&uuml;r",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Cluster:queue</b></dt>
                                                <dd>".$clickable.".
						  Namen der Cluster und deren Queues (getrennt durch einen Doppelpunkt
                                                  &quot;:&quot;) auf welche ein Nutzer Zugriff hat.
						  Ist ein Nutzer nicht autorisiert,
                                                  wird die Nachricht &quot;<b><font color=\"red\">Not authorised at host
                                                      ...</font></b>&quot; angezeigt.
						  Bei Anlicken der Cluster Namens wird die Beschreibung des Clusters gegeben, genauso wie bei einer Auswahl der Queue.
                                                  </dd>
                                                <dt><b>Freie CPUs</b></dt>
                                                <dd>
						  Die Anzahl von freien CPUs, die f&uuml;r eine bestimmte
						  Queue f&uuml;r einen bestimmten Nutzer zu einem bestimmten
						  Moment, ggf. eingeschr&auml;nkt durch die Angabe der maximalen Laufzeit (Angabe in Minuten), verf&uuml;gbar sind.
                                                  Zum Beispiel bedeutet
                                                  &quot;3&quot;, da&szlig; 3 CPUs f&uuml;r einen Job unbeschr&auml;nkter Laufzeit verf&uuml;gbar sind.
                                                  &quot;4:360&quot; beschreibt die Verf&uuml;gbarkeit von vier Jobs f&uuml;r nicht l&auml;nger al 6 Stunden. 
                                                  &quot;10:180 30&quot; bedeutet, da&szlig; 10 CPUs verf&uuml;gbar sind f&uuml;r Jobs, die nicht l&auml;nger rechnen als 3 Stunden, sowie weitere 30 f&uuml;r Jobs mit unbeschr&auml;nkter Laufzeit. 
                                                  &quot;0&quot; bedeutet, da&szlig; keine CPUs verf&uuml;gbar sind
						  und neue Jobs entsprechend warten m&uuml;ssen.
                                                  </dd>
                                                <dt><b>Wartenden Jobs</b></dt>
                                                <dd>Anzahl von Jobs des Anwenders, die in der Wartschlange vor einem Neuen Job sind.
                                                  Die Zahl &quot;0&quot; bedeutet, dass der Job sofort ausgef&uuml;hrt wird.
                                                  <b>NB!</b> Dies ist nur eine Absch&auml;tzung, durch den Einfluss lokaler Administratoren ist eine sichere Angabe nicht m&ouml;glich. 
                                                  </dd>
                                                <dt><b>Freier Diskplatz (MB)</b></dt>
                                                <dd>F&uuml;r einen Nutzer verf&uuml;gbarer Diskplatz (in
                                                  Megabytes). <b>NB!</b> Dies ist nur eine Absch&auml;tzung,
						  die meisten Cluster haben keine solchen Quotas festgelegt.</dd>
                                                <dt><b>Jobname</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Cluster</b></dt>
                                                <dd>".$clickable.". Name des Clusters bei dem der Job ausgef&uuml;hrt wird. Bei Klick auf den Namen werden detaillierte Informationen zu dem Cluster pr&auml;sentiert.
                                                </dd>
                                                <dt><b>Queue</b></dt>
                                                <dd>".$clickable.". Name der Queue, in which der Job ausgef&uuml;hrt wird oder wurde. ".$str_que."</dd>
                                                <dt><b>CPUs</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Jobname"                     => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Cluster"                      => 0,
                                      "Queue"                        => 0,
                                      "CPUs"                         => 0
                                      ),
                  "attlist"  => array("0" => "Attributwerte",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objekt</b></dt>
                                                   <dd>".$clickable.". Names des Objektes dessen Attribute angezeigt werden. Es kann ein Cluster sein, dessen Queue, ein Job, ein Anwender etc. Nach einer Auswahl durch Anklicken der Zeichenkette werden detaillierte Information angezeigt.</dd>
                                                   <dt><b>Attribute</b></dt>
                                                   <dd>F&uuml; jedes Objekt wird eines oder mehrere Attribute angezeigt. Der Spaltentitel ist der Klarname des Attributes, von einigen MDS-spezifischen Attributen abgesehen, der Inhalt entspricht den Werten wie sie im Informationssystem abgelegt sind.
                                                   </dd>
                                                 </dl>
                                                 </div>",
                                      "Object"                       => 0,
                                      "Attribute"                    => 0
                                      ),
                  "quelist"  => array("0" => "Queue",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>".$clickable.". Name des Queue Attributs".$str_att."</dd>
                                                   <dt><b>Wert</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Jobname</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Eigner</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Status</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Speicher (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPUs</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Jobname"                      => 0,
                                      "Eigner"                       => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Speicher (KB)"                => 0,
                                      "CPUs"                         => 0
                                      ),
                  "sestat"   => array("0" => "Storage Elements",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Storage Element Zweitname wie festgelegt im Informationssystem.
						    Maximal 15 Zeichen werden angezeigt.
						  </dd>
                                                  <dt><b>Tot. space</b></dt>
                                                  <dd>Totaler Plattenplatz (GB).</dd>
                                                  <dt><dt><b>Freier Plattenplatz</b></dt>
                                                  <dd>Verf&uuml;gbarer Plattenplatz in GB.</dd>
                                                  <dt><b>Name</b></dt>
                                                  <dd>Name des Storage Elements, bestehend aus einem logischen Namen und dem Namen des Hosts, getrennt durch einen Doppelpunk &quot;:&quot;). Der logische Name wird nur f&uuml;r die interne Verwaltung genutzt, um verschiedene Einheiten auf demselben System zu unterscheiden.
                                                  </dd>
                                                  <dt><b>Basis URL</b></dt>
                                                  <dd>URL des Storage Elements, &uuml;blich ist die
						    Verwendung des gsiftp://
                                                    Protokols. Dieser URL dient als Basis f&uuml;r den Zugriff auf Dateien.</dd>
                                                  <dt><b>Typ</b></dt>
                                                  <dd>Storage Element typ. &quot;gridftp-based&quot; beschreibt
                                                    Plattenplatz der &uuml;ber die GridFTP Schnittstelle verf&uuml;gbar ist.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
				      //                                      "Tot.   Platz"                 => 0,
                                      "Freier/Tot. Platz, GB"        => 0,
                                      "Name"                         => 0,
                                      "Basis URL"                    => 0,
                                      "Typ"                          => 0
                                      ),
                  "allusers" => array("0" => "Authorised Grid Users:Active Grid Users",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Name</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Zugeh&ouml;rigkeit:</b></dt>
                                                   <dd>Das Institut oder die Firma, die den Anwender besch&auml;ftigt. Der Eintrag ist abgeleitet vom <i>personal certificate</i></dd>
                                                   <dt><b>Jobs</b></dt>
                                                   <dd>Z&auml;hlt alle Jobs des Anwenders im System (running, pending, finished oder deleted)</dd>
                                                   <dt><b>Sites</b></dt>
                                                   <dd>Gibt an, wieviele teilnehmende Cluster Auftr&auml;ge dieses Nutzers annehmen.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Name"                         => 0,
                                      "Zugeh&ouml;rigkeit"           => 0,
                                      "Jobs"                         => 0,
                                      "Sites"                        => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Cluster:queue"                => 0,
                                      "Freie CPUs"                   => 0,
                                      "Wartenden Jobs"                  => 0,
                                      "Freier Plattenplatz (MB)"     => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attribut"                     => 0,
                                      "Wert"                         => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info g&uuml;ltig von (GMT)",
                                      "Mds-validto"   => "Info g&uuml;ltig bis (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Front-end domain Name",
                                      "nordugrid-cluster-aliasname"          => "Cluster Alias",
                                      "nordugrid-cluster-contactstring"      => "Kontakt",
                                      "nordugrid-cluster-interactive-contactstring" => "Interactiver Kontakt",
                                      "nordugrid-cluster-comment"            => "Kommentar",
                                      "nordugrid-cluster-support"            => "E-mail Kontakt",
				      "nordugrid-cluster-acl"                => "Autorisierte VOs",
                                      "nordugrid-cluster-lrms-type"          => "LRMS Typ",
                                      "nordugrid-cluster-lrms-version"       => "LRMS Version",
                                      "nordugrid-cluster-lrms-config"        => "LRMS Details",
                                      "nordugrid-cluster-architecture"       => "Architektur",
                                      "nordugrid-cluster-opsys"              => "Operating System",
                                      "nordugrid-cluster-homogeneity"        => "Homogener Cluster",
                                      "nordugrid-cluster-nodecpu"            => "CPU Typ (langsamster)",
                                      "nordugrid-cluster-nodememory"         => "Memory (MB, kleinster)",
                                      "nordugrid-cluster-totalcpus"          => "CPUs, gesamt",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:Hosts",
                                      "nordugrid-cluster-benchmark"          => "Benchmark",
                                      "nordugrid-cluster-sessiondir-free"    => "Plattenplatz, verf&uuml;gbar (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Plattenplatz, gesamt (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Lebensdauer der Grid Session  (min)",
                                      "nordugrid-cluster-cache-free"         => "Cache size, verf&uuml;gbar (MB)",
                                      "nordugrid-cluster-cache-total"        => "Cache size, gesamt (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Runtime environment",
                                      "nordugrid-cluster-localse"            => "Storage Element, lokal",
                                      "nordugrid-cluster-middleware"         => "Grid middleware",
                                      "nordugrid-cluster-totaljobs"          => "Jobs, totale Anzahl",
                                      "nordugrid-cluster-usedcpus"           => "CPUs, belegt",
                                      "nordugrid-cluster-queuedjobs"         => "Jobs, in Queue wartend",
				      "nordugrid-cluster-prelrmsqueued"      => "Grid jobs, noch nicht submitted",
                                      "nordugrid-cluster-location"           => "Postleitzahl",
                                      "nordugrid-cluster-owner"              => "Eigner",
                                      "nordugrid-cluster-issuerca"           => "Zertifikat-Aussteller",
                                      "nordugrid-cluster-issuerca-hash"      => "Zertifikat-Aussteller's kash",
                                      "nordugrid-cluster-trustedca"          => "Akzeptierte Zertificat-Aussteller",
                                      "nordugrid-cluster-nodeaccess"         => "IP Konnektivit&auml;t der Hosts",
                                      "nordugrid-cluster-gridarea"           => "Session area (OBSOLETE)",
                                      "nordugrid-cluster-gridspace"          => "Grid Plattenplatz (OBSOLETE)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS Distribution (OBSOLETE)",
                                      "nordugrid-cluster-runningjobs"        => "Jobs, running (OBSOLETE)",
				      "nordugrid-cluster-credentialexpirationtime" => "Credential expiration time",
                                      "nordugrid-queue-name"                 => "Queue Name",
                                      "nordugrid-queue-comment"              => "Kommentar",
                                      "nordugrid-queue-status"               => "Queue Status",
                                      "nordugrid-queue-running"              => "CPUs, belegt",
				      "nordugrid-queue-localqueued"          => "Local jobs, queued",
				      "nordugrid-queue-prelrmsqueued"        => "Grid jobs, noch nicht submitted",
                                      "nordugrid-queue-queued"               => "Jobs, queued (OBSOLETE)",
                                      "nordugrid-queue-maxrunning"           => "Jobs, running (max)",
                                      "nordugrid-queue-maxqueuable"          => "Jobs, queueable (max)",
                                      "nordugrid-queue-maxuserrun"           => "Jobs pro Unix User (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU Zeit, max. (min)",
                                      "nordugrid-queue-mincputime"           => "CPU Zeit, min. (min)",
                                      "nordugrid-queue-defaultcputime"       => "CPU Zeit, default (min)",
                                      "nordugrid-queue-maxwalltime"          => "Zeit auf K&uuml;chenuhr, max. (min)",
                                      "nordugrid-queue-minwalltime"          => "Zeit auf K&uuml;chenuhr, min. (min)",
                                      "nordugrid-queue-defaultwalltime"      => "Zeit auf K&uuml;chenuhr, default (min)",
                                      "nordugrid-queue-schedulingpolicy"     => "Scheduling policy",
                                      "nordugrid-queue-totalcpus"            => "CPUs, gesamt",
                                      "nordugrid-queue-nodecpu"              => "CPU Typ",
                                      "nordugrid-queue-nodememory"           => "Speicher (MB)",
                                      "nordugrid-queue-architecture"         => "Architektur",
                                      "nordugrid-queue-opsys"                => "Betriebssystem",
                                      "nordugrid-queue-homogeneity"          => "Homogene Queue",
                                      "nordugrid-queue-gridrunning"          => "CPUs occupied by Grid jobs",
                                      "nordugrid-queue-gridqueued"           => "Grid jobs, queued",
                                      "nordugrid-queue-benchmark"            => "Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "CPUs je Queue (OBSOLETE)",
                                      "nordugrid-queue-assignedcputype"      => "CPU typ (OBSOLETE)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Eigner",
                                      "nordugrid-job-execcluster"            => "Execution Cluster",
                                      "nordugrid-job-execqueue"              => "Execution Queue",
                                      "nordugrid-job-stdout"                 => "Standard output file",
                                      "nordugrid-job-stderr"                 => "Standard error file",
                                      "nordugrid-job-stdin"                  => "Standard input file",
                                      "nordugrid-job-reqcputime"             => "Verlangte CPU Zeit",
                                      "nordugrid-job-reqwalltime"            => "Verlangte Zeit auf K&uuml;chenuhr",
                                      "nordugrid-job-status"                 => "Status",
                                      "nordugrid-job-queuerank"              => "Position in der Queue",
                                      "nordugrid-job-comment"                => "LRMS Kommentar",
                                      "nordugrid-job-submissionui"           => "Submitted von",
                                      "nordugrid-job-submissiontime"         => "Submitted um (GMT)",
                                      "nordugrid-job-usedcputime"            => "Ben&ouml;tigte CPU Zeit",
                                      "nordugrid-job-usedwalltime"           => "Ben&ouml;tigte Zeit auf K&uuml;chenuhr",
                                      "nordugrid-job-completiontime"         => "Job beendet um (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Job gel&ouml;scht um (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxy Verfallzeit (GMT)",
                                      "nordugrid-job-usedmem"                => "Verwendeter Speicher (KB)",
                                      "nordugrid-job-errors"                 => "Errors",
                                      "nordugrid-job-exitcode"               => "Exit Code",
                                      "nordugrid-job-jobname"                => "Name",
                                      "nordugrid-job-runtimeenvironment"     => "Runtime environment",
                                      "nordugrid-job-cpucount"               => "Angeforderte CPUs",
                                      "nordugrid-job-executionnodes"         => "Aufr&uuml;hrende Rechner",
                                      "nordugrid-job-gmlog"                  => "GM Logdatei",
                                      "nordugrid-job-clientsoftware"         => "Version des Clients",
                                      "nordugrid-job-rerunable"              => "Rerunnable",
                                      "nordugrid-job-reqcput"                => "Requested time (OBSOLETE)",
                                      "nordugrid-job-gridlog"                => "Gridlog file (OBSOLETE)",
                                      "nordugrid-job-lrmscomment"            => "LRMS comment (OBSOLETE)",
                                      "nordugrid-authuser-name"              => "Name",
                                      "nordugrid-authuser-sn"                => "Subject Name",
                                      "nordugrid-authuser-freecpus"          => "Freie CPUs",
                                      "nordugrid-authuser-diskspace"         => "Freier Plattenplatz (MB)",
                                      "nordugrid-authuser-queuelength"       => "Wartende Jobs des Users",
                                      "nordugrid-se-name"                    => "Name",
                                      "nordugrid-se-aliasname"               => "Storage Element Alias",
                                      "nordugrid-se-type"                    => "Storage Element Typ",
				      "nordugrid-se-acl"                     => "Autorisierte VOs",
                                      "nordugrid-se-freespace"               => "Freier Plattenplatz (MB)",
                                      "nordugrid-se-totalspace"              => "Gesamter Plattenplatz (MB)",
                                      "nordugrid-se-url"                     => "Kontakt URL",
                                      "nordugrid-se-baseurl"                 => "Kontakt URL (OBSOLETE)",
                                      "nordugrid-se-accesscontrol"           => "Zugangskontrolle",
                                      "nordugrid-se-authuser"                => "Zugelassene User (DN)",
                                      "nordugrid-se-location"                => "Postleitzahl",
                                      "nordugrid-se-owner"                   => "Eigner",
                                      "nordugrid-se-middleware"              => "Middleware",
                                      "nordugrid-se-issuerca"                => "Zertifikat-Aussteller",
                                      "nordugrid-se-issuerca-hash"           => "Zertifikat-Aussteller ID",
                                      "nordugrid-se-trustedca"               => "Vertrauten Zertifikat-Ausstellern",
                                      "nordugrid-se-comment"                 => "Kommentar",
                                      "nordugrid-rc-name"                    => "Domainname",
                                      "nordugrid-rc-aliasname"               => "Replica Catalog Alias",
                                      "nordugrid-rc-baseurl"                 => "Kontakt URL",
                                      "nordugrid-rc-authuser"                => "Zugelassene User (DN)",
                                      "nordugrid-rc-location"                => "Postleitzahl",
                                      "nordugrid-rc-owner"                   => "Eigner",
                                      "nordugrid-rc-issuerca"                => "Zertifikat-Aussteller"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Die top-level resource Indizes konnten nicht gelesen werden",
                                      "2"   => "Keiner der lokalen Indizes konnte erreicht werden", // ?
                                      "3"   => " schlechte Konfiguration oder Zeit&uuml;berschreitung bei der Anfrage",
                                      "4"   => "Keine Grid Jobs gefunden",
                                      "5"   => "Keine Information gefunden",
                                      "6"   => "Server nicht verf&uuml;gbar",
                                      "7"   => " - sp&auml;ter neu laden",
                                      "8"   => "Keine Informationen zur Queue gefunden",
                                      "9"   => "Keine Ei ntr&auml;ge gefunden.",
                                      "10"  => "Keine Nutzer gefunden.",
                                      "11"  => "Bei diesem Host nicht autorisiert",
                                      "12"  => "antwortet nicht",
                                      "13"  => "Keine j&uuml;ngst submitteten Jobs gefunden f&uuml;r ",
                                      // debug messages
                                      "101" => " Monitor timeouts f&uuml;r GRIS: ",
                                      "102" => " sek f&uuml;r Verbindung und ",
                                      "103" => " sek beim Suchen",
                                      "104" => " sek verbracht beim Suchen",
                                      "105" => "Zeige Ressourcen nur in ",
                                      "106" => "Polled top-level Indizes: ",
                                      "107" => "Erhielt geographische Ortsangaben, gescante Sites: ", // ?
                                      "108" => " <i>sites</i> geographisch geordnet",
                                      "109" => "Suche nach Cluster Attributen",
                                      "110" => "Suche for Queue Attributen",
                                      "111" => "Keine Daten von ",
                                      "112" => " funktioniert in ", // ?
                                      "113" => " hat keine Resourcen anzubieten",
                                      "114" => " Monitor timeouts f&uuml;r GIIS: ",
                                      "115" => "&Uuml;berspringe GRIS: ",
                                      "116" => "nicht ein ",
                                      "117" => "Teste Verbindung: ",
                                      "118" => "OK",
                                      "119" => "Entdeckte bislang Ressourcen der folgenden Art ",
                                      "120" => "LDAP Fehler beim Suchen ",
                                      "121" => " Status bei ",
                                      "122" => "Blacklisted: ",
                                      "123" => "Registrant gefunden f&uuml;r ",
                                      "124" => "Suche nach SE Attributen",
                                      "125" => "Suche nach Nutzern",
                                      "126" => "Suche nach jobs",
                                      "127" => " hat Job ",
                                      "128" => " obwohl nicht autorisiert",
                                      "129" => "Kann die Objektdaten nicht erhalten: Fehler ",
                                      // icon titles
                                      "301" => "Update",
                                      "302" => "Drucken",
                                      "303" => "Hilfe",
                                      "304" => "Schlie&szlig;en",
                                      "305" => "Rot",
                                      "306" => "Gr&uuml;n",
                                      "307" => "Alle Nutzer",
                                      "308" => "Aktive Nutzer",
                                      "309" => "Suchen",
                                      "310" => "Storage",
                                      "311" => "VOs",
                                      "312" => "Flagge von ",
                                      "313" => " Grid Prozesse und ",
                                      "314" => " lokale Prozesse",
                                      // auxilliary strings
                                      "401" => "Prozesse",
                                      "402" => "Grid",
                                      "403" => "Lokal",
                                      "404" => "Globus",
                                      "405" => "TOTAL",
                                      "406" => " sites",
                                      "407" => "eine Menge",
                                      "408" => " GB",
                                      "409" => " ALLE",
                                      "410" => "Cluster",
                                      "411" => "Queue",
                                      "412" => "Job",
                                      "413" => "Nutzer",
                                      "414" => "Storage",
                                      "415" => "Replica Cat.",
                                      "416" => "Definiere Attribute, die f&uuml;r das Objekt anzuzeigen sind: ",
                                      "417" => "AND von allen Ausdr&uuml;cken wird gesucht", // ?
                                      "418" => "Feld ganz rechts freilassen, um alles anzuzeigen",
                                      "419" => "Personalisierte Anzeige von Ressourcen",
                                      "420" => "Eindeutiger Name",
                                      "421" => "Kann insgesamt nutzen ",
                                      "422" => " sites",
                                      "423" => "Resource / Object:",
                                      "424" => "Nr. von Attributen (def. 6):",
                                      "425" => "Objekt",
                                      "426" => "N&auml;chstes",
                                      "427" => "Auswahl",
                                      "428" => "Reset",
                                      "429" => "ANZEIGEN"
                                      ),
                  // Post code conversion
                  "tlconvert" => array (
                                        "Australia" => "Australien",	   
					"Austria" => "&Ouml;sterreich",	   
					"Armenia" => "Armenien",	   
					"Algeria" => "Algerien",	   
					"Belgium" => "Belgien",	   
					"Bulgaria" => "Bulgarien",	   
                                        "Canada" => "Canada",	   
                                        "China" => "China",	   
					"Czechia" => "Tschechien",	   
                                        "Denmark" => "D&auml;nemark",	   
                                        "Estonia" => "Estland",	   
                                        "Finland" => "Finnland",	   
					"France" => "Frankreich",	   
					"Georgia" => "Georgien",	   
                                        "Germany" => "Deutschland",	   
					"Greece" => "Griechenland",	   
					"Hungary" => "Ungarn",	   
                                        "Iceland" => "Island",	   
					"Ireland" => "Irland",	   
					"Italy" => "Italien",  
                                        "Japan" => "Japan",	   
                                        "Latvia" => "Lettland",	   
                                        "Lithuania" => "Litauen",	   
					"Morocco" => "Marokko",	   
					"Netherlands" => "Niederlande",	   
                                        "Norway" => "Norwegen",	   
					"Poland" => "Polen",	   
					"Portugal" => "Portugal",	   
					"Romania" => "Rum&auml;nien",	   
					"Russia" => "Russland",	   
					"SriLanka" => "Sri&nbsp;Lanka",	   
                                        "Sweden" => "Schweden",	   
                                        "Slovakia" => "Slowakei",	   
                                        "Slovenia" => "Slowenien",	   
                                        "Switzerland" => "Schweiz",	   
                                        "Turkey" => "T&uuml;rkei", 
					"UK" => "UK",		   
					"Ukraine" => "Ukraine",	   
					"USA" => "USA"             
                                        )
                  );

?>
