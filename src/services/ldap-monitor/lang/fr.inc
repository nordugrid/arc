<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- French [fr]
// -- Translation: Philippe Gros


// Some common strings:

$clickable = "CLIQUABLE";

$str_att = ", human-readable sauf pour certains attributs MDS-specifiques. Cliquer sur le nom de l'attribut pour obtenir la 
liste des valeur pour l'attribut dans l'univers ARC.";

$str_val = "Valeur de l'attribut telle qu'entrée dans le Système d'Information.";

$str_que = "Typiquement, differentes files correspondent aux différentes durées de tâche permises, ou bien aux differents 
groupes d'utilisateurs. Cliquer sur le nom d'une file pour obtenir des informations détaillées sur la file, y compris les 
tâches courantes, en attente et terminées.";

$str_job = "Nom d'un job tel qu'assigné par le propriétaire du job.
          <br> <font color=\"red\">N/A</font> indique que l'utilisateur n'a pas donné de nom.
          <br> <font color=\"red\"><b>X</b></font> indique que le job a été tué par le propriétaire.
          <br> <font color=\"red\"><b>!</b></font> indique que le job a échoué dans le systÈme
          <br> Cliquer sur un nom pour voir une description détaillée du job.";

$str_nam = "Nom de l'utilisateur, tel que spécifié dans le certificat personnel. Cliquer sur un nom pour voir la list de
toutes les ressource disponibles pour cet utilisateur et tous les jobs soumis par cet utilisateur qui sont actuellement 
dans le système.";

$str_sta = "Statut du job tel que fourni par le Grid Manager (GM) et LRMS. Dans l'ordre, les états sont :
   <dl>
     <dt>ACCEPTED &ndash; job soumis mais non encore pris en charge</dt>
     <dt>PREPARING &ndash; les fichiers d'entrée sont en train d'être récupérés</dt>
     <dt>SUBMITTING &ndash; interaction avec LRMS en cours</dt>
     <dt>INLRMS &ndash; le job est transféré au LRMS; un statut interne est ajouté par l'infosystem. Les états possible sont :</dt>
       <dd>: Q &ndash; le job est en attente
       <br>: U &ndash; le job est suspendu dans un node occupé (PBSPro)
       <br>: S &ndash; le job est suspendu (Condor)
       <br>: R, run &ndash; le job est en cours
       <br>: E &ndash; le job se termine (PBS)
     </dd>
     <dt>FINISHING &ndash; les fichiers de sortie sont en train d'être transférés par le GM</dt>
     <dt>FINISHED &ndash; le job est terminé; un indicateur temporel est ajouté par l'infosystem</dt>
     <dt>CANCELING &ndash; le job est en train d'être annulé</dt>
     <dt>DELETED &ndash; le job n'a pas été supprimé par l'utilisateur mais par le GM à cause de la date d'expiration</dt>
   </dl>
   Chaque état peut être donné avec le préfixe PENDING:, ce qui signifie que le GM essaie de déplacer le job vers l'état suivant";

$str_tim = "Temps CPU utilisé par le job, en minutes.";

$str_mem = "Mémoire consomée par le job, en ko";

$str_cpu = "Nombre de processeurs utilisés par le job.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    Cet écran montre tous les sites enrégistrés dans l'indexing service d'ARC,
                                                    triés par pays puis par nom de site. 
						    Une selection de paramètres de site sont affichés : alias du cluster, 
                                                    capacité CPU totale et nombre de jobs courants et en attente, aussi bien 
                                                    du Grid que locaux. 
                                                    Utiliser &quot;Search&quot; pour comparer d'autres caractéristiques de 
                                                    cluster, file, job etc...
                                                  </dt>
                                                  <dt><b>Pays</b></dt>
                                                  <dd>".$clickable.". Drapeau et nom du pays, extrait des descriptions de 
                                                    ressource disponibles. Cliquer pour montrer les information concernant 
                                                    ce pays uniquement. </dd>
                                                  <dt><b>Cluster</b></dt>
                                                  <dd>".$clickable.". Alias du cluster assigné par le propriétaire. 
                                                    La longueur maximale affichée est 22 caractÈres. Cliquer sur l'alias pour 
                                                    voir une description détaillée du cluster.</dd>
                                                  <dt><b>CPU</b></dt>
                                                  <dd>nombre total de CPU dans un cluster. <b>NB!</b> Seule une fraction de 
                                                    ceux-ci est effectivement accessible aux utilisateurs du Grid.</dd>
                                                  <dt><b>Charge (processus:Grid&#43local)</b></dt>
                                                  <dd>".$clickable.". Charge relative du cluster, correspondant au nombre de
                                                    CPU occupés. Les barres grises indiquent les processeurs occupés par les
                                                    jobs soumis localement, les barres rouges montrent les CPU occupés par
                                                    des jobs soumis à travers le Grid. Cliquer sur la barre pour voir la 
                                                    liste détaillée de tous les jobs d'origine Grid dans le cluster, 
                                                    y compris le nombre de processus par job.</dd>
                                                  <dt><b>Files d'attente</b></dt>
                                                  <dd>".$clickable.". Nombre total de jobs en attente dans le cluster, 
                                                    montré comme le nombre de jobs du grid en attente plus le nombre de jobs 
                                                    en attente soumis localement. Cliquer sur le premier nombre pour voir la 
                                                    liste des jobs du Grid en attente dans le cluster.</dd>
                                                </dl>
                                                </div>",
                                      "Pays"                           => 30,
                                      "Cluster"                        => 160,
                                      "CPU"                            => 10,
                                      "Charge (processus: Grid+local)" => 210,
                                      "File d'attente"                 => 10
                                      ),
                  "clusdes"  => array("0" => "Details des ressources pour",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attribut</b></dt>
                                                  <dd>".$clickable.". Nom de l'attribut de cluster".$str_att."</dd>
                                                  <dt><b>Valeur</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>File d'attente</b></dt>
                                                  <dd>".$clickable.". Noms des files batch disponibles pour les utilisateurs 
                                                    d'ARC, donné par le propriétaire du cluster. ".$str_que."</dd>
                                                  <dt><b>Statut</b></dt>
                                                  <dd>Statut des files. Une file active indiquera typiquement le statut :
                                                    <i>active</i>.</dd>
                                                  <dt><b>CPU (min)</b></dt>
                                                  <dd>Limite de durée pour un job dans une file, si elle existe, en minutes. 
                                                    La première valeur affichée est la limite basse, la seconde la limite haute.
                                                    Si les limites ne sont pas données (jobs de durée quelconque acceptés),
                                                    le symbole <b>N/A</b> est affiché.</dd>
                                                  <dt><b>En cours</b></dt>
                                                  <dd>Nombre de jobs en cours dans la file. Le nombre total de jobs est indiqué,
                                                    avec le nombre de <b>processeurs</b> occupś par des jobs du Grid entre
                                                    parenthèses, par ex. (Grid:12). <b>NB!</b> Pour les jobs multiprocesseurs en
                                                    parallèle, le nombre entre parenthèse peut être plus élevé que le nombre de
                                                    jobs.</dd>
                                                  <dt><b>En attente</b></dt>
                                                  <dd>Nombre de jobs en attente d'execution dans la file. Le nombre total de jobs
                                                    est affiché, avec les jobs du Grid entre parenthèse, par ex. (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "File d'attente"               => 0,
                                      "Statut"                       => 0,
                                      "Limites (min)"                => 0,
                                      "CPU"                          => 0,
                                      "En cours"                     => 0,
                                      "En attente"                   => 0
                                      ),
                  "jobstat"  => array("0" => "Jobs à:Job ID",
                                      "help" => "<div align=\"left\">
                                                <b>LIST DES JOBS:</b>
                                                <dl>
                                                <dt><b>Nom du job</b></dt>
                                                <dd>".$clickable.". Nom d'un job, assigné par le propriétaire. Si aucun nom n'a été
                                                  assigné, &quot;<span style={color:red;}>N/A</span>&quot; est affiché. Cliquer
                                                  sur un nom pour voir une description détaillée du job.</dd>
                                                <dt><b>Propriétaire</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Statut</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>File d'attente</b></dt>
                                                <dd>".$clickable.". Nom de la file batch dans laquelle le job est exécuté. 
                                                  ".$str_que."</dd>
                                                <dt><b>CPU</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>DETAILS DU JOB:</b>
                                                <dl>
                                                <dt><b>Attribut</b></dt>
                                                <dd>".$clickable.". Nom de l'attribut du job".$str_att."</dd>
                                                <dt><b>Valeur</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Nom du job"                   => 0,
                                      "propriétaire"                 => 0,
                                      "Statut"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Filer d'attente"              => 0,
                                      "CPU"                          => 0
                                      ),
                  "volist"   => array("0" => "Virtual Organisations",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtual Organisation</b></dt>
                                                   <dd>".$clickable.". Groupe d'utilisateurs, généralement partageant la même
                                                     activités et les mêmes ressources, autorisé à au moins un site ARC.
                                                     Cliquer sur le nom pour voir la liste des membres du groupe.</dd>
                                                   <dt><b>Membres</b></dt>
                                                   <dd>Nombre de membres du groupe.</dd>
                                                   <dt><b>Desservi par</b></dt>
                                                   <dd>serveur LDAP qui supporte la base de données des membres du groupe.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtual Organisation"         => 0,
                                      "Membres"                      => 0,
                                      "Desservi par"                    => 0
                                      ),
                  "vousers"  => array("0" => "Utilisateurs du Grid",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Nom</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Affiliation</b></dt>
                                                   <dd>Institut d'origine de l'utilisateur, entré par le VO manager. Peut être
                                                     vide.</dd>
                                                   <dt><b>Adresse électronique</b></dt>
                                                   <dd>".$clickable.". Adresse électronique de l'utilisateur, entré par le VO manager.
                                                     Peut être vide. Cliquer sur l'adresse pour envoyer un courriel à l'utilisateur.
                                                   </dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Nom"                          => 0,
                                      "Affiliation"                  => 0,
                                      "Adresse èlectronique"         => 0
                                      ),
                  "userlist" => array("0" => "Information pour",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Cluster:file</b></dt>
                                                <dd>".$clickable.". Noms des clusters et file respective
                                                  (separés par deux points, &quot;:&quot;) où un utilisateur
                                                  est autorisé à soumettre des jobs. Si un utilisateur n'est pas
                                                  autorisé, le message &quot;<b><font color=\"red\">Not authorised at host
                                                      ...</font></b>&quot; est affiché. Cliquer sur un nom de cluster
                                                  pour voir une description détaillée du cluster. Cliquer sur un nom de file
                                                  pour voir une description détaillée de la file. </dd>
                                                <dt><b>CPU libres</b></dt>
                                                <dd>Nombre de CPU libres disponibles dans une file données pour l'utilisateur
                                                  à cet instant, eventuellement associé avec la durée maximum (en minutes)
                                                  Par exemple, &quot;3&quot; signifie 3 CPU disponible pour un job de durée
                                                  illimitée; &quot;4:360&quot; indique qu'il y a 4 CPU disponibles pour des
                                                  jobs de moins de 6 heures; &quot;10:180 30&quot; signifie qu'il y a 10 CPU
                                                  disponibles pour des jobs n'excédant pas 3 heures, plus 30 CPU disponibles
                                                  pour des jobs de n'omporte quelle durée; &quot;0&quot; signifie qu'il n'y a
                                                  pas de CPU disponible à cet instant, et les jobs seront placés dans une file
                                                  d'attente.</dd>
                                                <dt><b>Jobs en attente</b></dt>
                                                <dd>Nombre de jobs d'utilisateur qui seront avant un nouveau job (pour cet
                                                  utilisateur) dans une file d'attente. Un nombre de &quot;0&quot; signifie que
                                                  le job devrait être exécuté immédiatement. <b>NB!</b> Ceci n'est qu'une
                                                  estimation, qui peut être outrepassée par des politiques locales.</dd>
                                                <dt><b>Disque libre (Mo)</b></dt>
                                                <dd>Espace disque disponible pour l'utilisateur dans une file donnée (en Mégaoctets).
                                                  <b>NB!</b> Ceci n'est qu'une estimation, étant donné que la plupart
                                                  des clusters ne fournissent pas de quotas fixes.</dd>
                                                <dt><b>Nom du job</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Statut</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Cluster</b></dt>
                                                <dd>".$clickable.". Nom du cluster où le job est exécuté. Cliquer sur un nom de
                                                  cluster pour voir des information détaillées sur le cluster.</dd>
                                                <dt><b>Queue</b></dt>
                                                <dd>".$clickable.". Nom de la file batch dans laquelle le job est/était exécuté.
                                                  ".$str_que."</dd>
                                                <dt><b>CPU</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Nom du job"                   => 0,
                                      "Statut"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Cluster"                      => 0,
                                      "File"                         => 0,
                                      "CPU"                          => 0
                                      ),
                  "attlist"  => array("0" => "Valeur des attributs",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Objet</b></dt>
                                                   <dd>".$clickable.". Nom de l'objet dont les attributs sont affichés
                                                     Ce peut être le nom d'un cluster, d'une file de clusters, d'un job,
                                                     d'un utilisateur etc... Cliquer sur le texte pour voir une description 
                                                     détaillée de l'objet.</dd>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>Pour chaque objet, un ou plusieurs attributs peuvent Être listés
                                                     Le titre de la colonne est une version &quot;human-readable&quot; du nom
                                                     de l'attribut (sauf pour certain attributs MDS-specifiques), et le contenu
                                                     de la colonne est la valeur de l'attribut par objet, telle qu'elle est 
                                                     entrée dans l'Information System.</dd>
                                                 </dl>
                                                 </div>",
                                      "Objet"                        => 0,
                                      "Attribut"                     => 0
                                      ),
                  "quelist"  => array("0" => "File",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attribut</b></dt>
                                                   <dd>".$clickable.". Nom d'un attribut de file".$str_att."</dd>
                                                   <dt><b>Valeur</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Nom du job</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Propriétaire</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Statut</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Memoire (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPU</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Nom du job"                   => 0,
                                      "Propriétaire"                 => 0,
                                      "Statut"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Memoire (ko)"                 => 0,
                                      "CPU"                          => 0
                                      ),
                  "sestat"   => array("0" => "Elements de Stockage",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Alias de l'Element de Stockage specifié dans l'Information
                                                    System. Longueur maximal affichée de 15 caractères.</dd>
                                                  <dt><b>Espace total</b></dt>
                                                  <dd>Espace disque total, Go.</dd>
                                                  <dt><dt><b>Espace libre</b></dt>
                                                  <dd>Espace disque disponible sur le moment, Go.</dd>
                                                  <dt><b>Nom</b></dt>
                                                  <dd>Nom de l'Element de Stockage, composé d'un nom logique et d'un nom d'hôte
                                                    (séparés par deux points, &quot;:&quot;). Le nom logique est utilisé
                                                    uniquement pour le système d'information, pour distinguer différents
                                                    élément de stockages accueillis par la même machine-hôte.</dd>
                                                  <dt><b>URL de base</b></dt>
                                                  <dd>URL pour l'élément de stockage, généralement un protocole gsiftp://
                                                    Utiliser cet URL comme base pour accéder aux fichiers.</dd>
                                                  <dt><b>Type</b></dt>
                                                  <dd>Type d'élément de stockage. &quot;gridftp-based&quot; indique
                                                    un stockage disque avec une interface GridFTP.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
                                      "Espace libre/total, GB"       => 0,
                                      "Nom"                          => 0,
                                      "URL de base"                  => 0,
                                      "Type"                         => 0
                                      ),
                  "allusers" => array("0" => "Utilisateurs Grid autorisés:Utilisateurs Grid actifs",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Nom</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Affiliation</b></dt>
                                                   <dd>Affiliation de l'utilisateur, dérivé du certificat personnel</dd>
                                                   <dt><b>Jobs</b></dt>
                                                   <dd>Compteur de tous les jobs d'utilisateur dans le système (en cours, 
                                                     en attente, terminés ou supprimés)</dd>
                                                   <dt><b>Sites</b></dt>
                                                   <dd>Affiche le nombre de sites qui admettent cet utilisateur</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Nom"                          => 0,
                                      "Affiliaton"                   => 0,
                                      "Jobs"                         => 0,
                                      "Sites"                        => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Cluster:file"                 => 0,
                                      "CPU libres"                   => 0,
                                      "Jobs en attente"              => 0,
                                      "Espace disque libre (Mo)"     => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attribut"                     => 0,
                                      "Valeur"                       => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info valide depuis (GMT)",
                                      "Mds-validto"   => "Info valide jusqu'à (GMT)"
                                      ),
                  "isattr"   => array(
                                      "nordugrid-cluster-name"               => "Nom du domaine Front-end",
                                      "nordugrid-cluster-aliasname"          => "Alias du cluster",
                                      "nordugrid-cluster-contactstring"      => "Fil de contact",
                                      "nordugrid-cluster-interactive-contactstring" => "Contact interactif",
                                      "nordugrid-cluster-comment"            => "Commentaire",
                                      "nordugrid-cluster-support"            => "Contact courriel",
				      "nordugrid-cluster-acl"                => "VO authorisés",
                                      "nordugrid-cluster-lrms-type"          => "type de LRMS",
                                      "nordugrid-cluster-lrms-version"       => "Version de LRMS",
                                      "nordugrid-cluster-lrms-config"        => "détails de LRMS",
                                      "nordugrid-cluster-architecture"       => "Architecture",
                                      "nordugrid-cluster-opsys"              => "Système d'exploitation",
                                      "nordugrid-cluster-homogeneity"        => "Cluster homogène",
                                      "nordugrid-cluster-nodecpu"            => "type de CPU (le plus lent)",
                                      "nordugrid-cluster-nodememory"         => "Memoire (Mo, la plus petite)",
                                      "nordugrid-cluster-totalcpus"          => "CPU, total",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:machines",
                                      "nordugrid-cluster-benchmark"          => "Benchmark",
                                      "nordugrid-cluster-sessiondir-free"    => "Espace disque, disponible (Mo)",
                                      "nordugrid-cluster-sessiondir-total"   => "Espace disque, total (Mo)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Durée de vie d'une session Grid (min)",
                                      "nordugrid-cluster-cache-free"         => "Taille du cache, disponible (Mo)",
                                      "nordugrid-cluster-cache-total"        => "Taille du cache, total (Mo)",
                                      "nordugrid-cluster-runtimeenvironment" => "Moteur d'exécution (runtime environment)",
                                      "nordugrid-cluster-localse"            => "Element de stockage, local",
                                      "nordugrid-cluster-middleware"         => "Intergiciel (middleware) du Grid",
                                      "nordugrid-cluster-totaljobs"          => "Jobs, quantité totale",
                                      "nordugrid-cluster-usedcpus"           => "CPU, occupés",
                                      "nordugrid-cluster-queuedjobs"         => "Jobs, en attente",
				      "nordugrid-cluster-prelrmsqueued"      => "Jobs Grid, en attente d'être soumis",
                                      "nordugrid-cluster-location"           => "Code postal",
                                      "nordugrid-cluster-owner"              => "Propriétaire",
                                      "nordugrid-cluster-issuerca"           => "Fournisseur du certificat",
                                      "nordugrid-cluster-issuerca-hash"      => "Hachage du fournisseur du certificat",
                                      "nordugrid-cluster-trustedca"          => "Fournisseurs de certificat fiables",
                                      "nordugrid-cluster-nodeaccess"         => "IP-connectivité du node",
                                      "nordugrid-cluster-gridarea"           => "zone de la session (OBSOLETE)",
                                      "nordugrid-cluster-gridspace"          => "Espace disque Grid (OBSOLETE)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS distribution (OBSOLETE)",
                                      "nordugrid-cluster-runningjobs"        => "Jobs, en cours (OBSOLETE)",
				      "nordugrid-cluster-credentialexpirationtime" => "Credential expiration time",
                                      "nordugrid-queue-name"                 => "Nom de file",
                                      "nordugrid-queue-comment"              => "Commentaire",
                                      "nordugrid-queue-status"               => "Statut de file",
                                      "nordugrid-queue-running"              => "CPU, occupés",
				      "nordugrid-queue-localqueued"          => "Jobs locaux, en attente",
				      "nordugrid-queue-prelrmsqueued"        => "Jobs Grid, en attente d'être soumis",
                                      "nordugrid-queue-queued"               => "Jobs, en attente (OBSOLETE)",
                                      "nordugrid-queue-maxrunning"           => "Jobs, en cours (max)",
                                      "nordugrid-queue-maxqueuable"          => "Jobs, pouvant être mis en attente (max)",
                                      "nordugrid-queue-maxuserrun"           => "Jobs par utilisateur Unix (max)",
                                      "nordugrid-queue-maxcputime"           => "Temps CPU, max. (minutes)",
                                      "nordugrid-queue-mincputime"           => "Temps CPU, min. (minutes)",
                                      "nordugrid-queue-defaultcputime"       => "Temps CPU, default (minutes)",
                                      "nordugrid-queue-maxwalltime"          => "Temps d'horloge, max. (minutes)",
                                      "nordugrid-queue-minwalltime"          => "Temps d'horloge, min. (minutes)",
                                      "nordugrid-queue-defaultwalltime"      => "Temps d'horloge, defaut (minutes)",
                                      "nordugrid-queue-schedulingpolicy"     => "Scheduling policy",
                                      "nordugrid-queue-totalcpus"            => "CPU, total",
                                      "nordugrid-queue-nodecpu"              => "type de CPU",
                                      "nordugrid-queue-nodememory"           => "Memoire (Mo)",
                                      "nordugrid-queue-architecture"         => "Architecture",
                                      "nordugrid-queue-opsys"                => "System d'exploitation",
                                      "nordugrid-queue-homogeneity"          => "File homogène",
                                      "nordugrid-queue-gridrunning"          => "CPU, occupés par jobs Grid",
                                      "nordugrid-queue-gridqueued"           => "Jobs Grid, en attente",
                                      "nordugrid-queue-benchmark"            => "Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "CPU par file (OBSOLETE)",
                                      "nordugrid-queue-assignedcputype"      => "Type de CPU (OBSOLETE)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Propriétaire",
                                      "nordugrid-job-execcluster"            => "Cluster d'execution",
                                      "nordugrid-job-execqueue"              => "File d'execution",
                                      "nordugrid-job-stdout"                 => "Fichier de sortie standard",
                                      "nordugrid-job-stderr"                 => "Fichier d'erreur standard",
                                      "nordugrid-job-stdin"                  => "Fichier d'entrée standard",
                                      "nordugrid-job-reqcputime"             => "Temps CPU requis",
                                      "nordugrid-job-reqwalltime"            => "Temps d'horloge requis",
                                      "nordugrid-job-status"                 => "Statut",
                                      "nordugrid-job-queuerank"              => "Position dans la file",
                                      "nordugrid-job-comment"                => "Commentaire LRMS",
                                      "nordugrid-job-submissionui"           => "Machine de soumission",
                                      "nordugrid-job-submissiontime"         => "Date de soumission (GMT)",
                                      "nordugrid-job-usedcputime"            => "Temps CPU utilisé",
                                      "nordugrid-job-usedwalltime"           => "Temps d'horloge utilisé",
                                      "nordugrid-job-completiontime"         => "Date de termination (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Date de suppression (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Date d'expiration du proxy (GMT)",
                                      "nordugrid-job-usedmem"                => "Memoire utilisée (Ko)",
                                      "nordugrid-job-errors"                 => "Erreurs",
                                      "nordugrid-job-exitcode"               => "Code de sortie",
                                      "nordugrid-job-jobname"                => "Nom",
                                      "nordugrid-job-runtimeenvironment"     => "Moteur d'exécution (runtime environment)",
                                      "nordugrid-job-cpucount"               => "CPU requis",
                                      "nordugrid-job-executionnodes"         => "Nodes d'exécution",
                                      "nordugrid-job-gmlog"                  => "Fichier de journal du GM",
                                      "nordugrid-job-clientsoftware"         => "Version du client",
                                      "nordugrid-job-rerunable"              => "Réexecutable",
                                      "nordugrid-job-reqcput"                => "Temps requis  (OBSOLETE)",
                                      "nordugrid-job-gridlog"                => "Fichier Gridlog (OBSOLETE)",
                                      "nordugrid-job-lrmscomment"            => "commentaire LRMS (OBSOLETE)",
                                      "nordugrid-authuser-name"              => "Nom",
                                      "nordugrid-authuser-sn"                => "Nom du sujet",
                                      "nordugrid-authuser-freecpus"          => "CPU libres",
                                      "nordugrid-authuser-diskspace"         => "Espace disque libre (Mo)",
                                      "nordugrid-authuser-queuelength"       => "Jobs en attente de l'utilisateur",
                                      "nordugrid-se-name"                    => "Nom",
                                      "nordugrid-se-aliasname"               => "Alias de l'élément de stockage",
                                      "nordugrid-se-type"                    => "Type d'élément de stockage",
				      "nordugrid-se-acl"                     => "VO autorisés",
                                      "nordugrid-se-freespace"               => "Espace libre (Mo)",
                                      "nordugrid-se-totalspace"              => "Espace total (Mo)",
                                      "nordugrid-se-url"                     => "URL de contact",
                                      "nordugrid-se-baseurl"                 => "URL de contact (OBSOLETE)",
                                      "nordugrid-se-accesscontrol"           => "Contrôle d'accès",
                                      "nordugrid-se-authuser"                => "Utilisateur autorisé (DN)",
                                      "nordugrid-se-location"                => "Code postal",
                                      "nordugrid-se-owner"                   => "Propriétaire",
                                      "nordugrid-se-middleware"              => "Intergiciel (middleware)",
                                      "nordugrid-se-issuerca"                => "Fournisseur de certificate",
                                      "nordugrid-se-issuerca-hash"           => "Hachage du fournisseur de certificat",
                                      "nordugrid-se-trustedca"               => "Fournisseurs de certificat fiables",
                                      "nordugrid-se-comment"                 => "Commentaire",
                                      "nordugrid-rc-name"                    => "Nom de domaine",
                                      "nordugrid-rc-aliasname"               => "Alias du duplicata du Catalogue",
                                      "nordugrid-rc-baseurl"                 => "URL de contact",
                                      "nordugrid-rc-authuser"                => "Utilisateur autorisé (DN)",
                                      "nordugrid-rc-location"                => "Code postal",
                                      "nordugrid-rc-owner"                   => "Propriétaire",
                                      "nordugrid-rc-issuerca"                => "Fournisseur de certificat"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Impossible de lire les index de ressource",
                                      "2"   => "Aucun des index locaux ne retourne de connexion",
                                      "3"   => " mauvaise configuration ou la requête a expiré",
                                      "4"   => "Aucun job Grid trouvé",
                                      "5"   => "Aucune information trouvée",
                                      "6"   => "Serveur indisponible",
                                      "7"   => " - rafraîchir plus tard",
                                      "8"   => "Aucune information de liste trouvée",
                                      "9"   => "Aucune entrée trouvée",
                                      "10"  => "Aucun utilisateur trouvé",
                                      "11"  => "Non autorisé chez l'hôte",
                                      "12"  => "Ne répond pas",
                                      "13"  => "Aucun job récent trouvé pour ",
                                      // debug messages
                                      "101" => " Monitor timeouts pour GRIS: ",
                                      "102" => " sec pendant la connection et ",
                                      "103" => " sec pendant la recherche",
                                      "104" => " sec en recherche",
                                      "105" => "N'affiche les ressources qu'en ",
                                      "106" => "Polled top-level indices: ",
                                      "107" => "Situations géographique obtenues, sites scannés: ",
                                      "108" => " sites rangés par situation géographique",
                                      "109" => "Recherche d'attributs du cluster",
                                      "110" => "Recherche d'attributs de la file",
                                      "111" => "Aucune donnée de ",
                                      "112" => " is up in ",
                                      "113" => " n'a aucune ressource à proposer",
                                      "114" => " Monitor timeouts for GIIS: ",
                                      "115" => "Saute GRIS: ",
                                      "116" => "pas un ",
                                      "117" => "Vérifie la connexion: ",
                                      "118" => "OK",
                                      "119" => "Jusqu'ici, a détecté des ressource de genre ",
                                      "120" => "Erreur LDAP en cherchant ",
                                      "121" => " statut à ",
                                      "122" => "Sur liste noire: ",
                                      "123" => "Registrant found for ",
                                      "124" => "Recherche d'attributs de SE",
                                      "125" => "Recherche d'utilisateurs",
                                      "126" => "Recherche de jobs",
                                      "127" => " a un job ",
                                      "128" => " alors que ce n'est pas autorisé",
                                      "129" => "Impossible d'obtenir les données d'object: erreur ",
                                      // icon titles
                                      "301" => "Rafraîchir",
                                      "302" => "Imprimer",
                                      "303" => "Aide",
                                      "304" => "Fermer",
                                      "305" => "Rouge",
                                      "306" => "Gris",
                                      "307" => "Tous utilisateurs",
                                      "308" => "Utilisateurs actifs",
                                      "309" => "Rechercher",
                                      "310" => "Stockage",
                                      "311" => "VO",
                                      "312" => "Drapeau de ",
                                      "313" => " processus Grid et ",
                                      "314" => " processus locaux",
                                      // auxilliary strings
                                      "401" => "Processus",
                                      "402" => "Grid",
                                      "403" => "Local",
                                      "404" => "Monde",
                                      "405" => "TOTAL",
                                      "406" => " sites",
                                      "407" => "beaucoup de",
                                      "408" => " Go",
                                      "409" => " ALL",
                                      "410" => "Cluster",
                                      "411" => "File",
                                      "412" => "Job",
                                      "413" => "Utilisateur",
                                      "414" => "Stockage",
                                      "415" => "Duplicata Cat.",
                                      "416" => "Définir les attributs à afficher pour l'objet: ",
                                      "417" => "Le produit logique (ET) de toutes les expressions va être testé",
                                      "418" => "Laisser le champ de droite vide pour tout afficher",
                                      "419" => "Afficher les ressources ou objets de votre choix",
                                      "420" => "Nom Distinct",
                                      "421" => "Peut utiliser un total de ",
                                      "422" => " sites",
                                      "423" => "Ressource / objet:",
                                      "424" => "Nr. des attributs (def. 6):",
                                      "425" => "Objet",
                                      "426" => "Suivant",
                                      "427" => "Choisir un",
                                      "428" => "Reinitialiser",
                                      "429" => "AFFICHER"
                                      ),
                  // Post code conversion
                  "tlconvert" => array (
                                        "Australia" => "Australie",	   
					"Austria" => "Autriche",	   
					"Armenia" => "Armenie",	   
					"Algeria" => "Algerie",	   
					"Belgium" => "Belgique",	   
					"Bulgaria" => "Bulgarie",	   
                                        "Canada" => "Canada",	   
                                        "China" => "Chine",	   
					"Czechia" => "République Tchèque",	   
                                        "Denmark" => "Danemark",	   
                                        "Estonia" => "Estonie",	   
                                        "Finland" => "Finlande",	   
					"France" => "France",	   
					"Georgia" => "Georgie",	   
                                        "Germany" => "Allemagne",	   
					"Greece" => "Grèce",	   
					"Hungary" => "Hongrie",	   
                                        "Iceland" => "Islande",	   
					"Ireland" => "Irlande",	   
					"Italy" => "Italie",	   
                                        "Japan" => "Japon",	   
                                        "Latvia" => "Lettonie",	   
                                        "Lithuania" => "Lithuanie",	   
					"Morocco" => "Maroc",	   
					"Netherlands" => "Pays-Bas",	   
                                        "Norway" => "Norvège",	   
					"Poland" => "Pologne",	   
					"Portugal" => "Portugal",	   
					"Romania" => "Roumanie",	   
					"Russia" => "Russie",	   
					"SriLanka" => "Sri&nbsp;Lanka",	   
                                        "Sweden" => "Suède",	   
                                        "Slovakia" => "Slovaquie",	   
                                        "Slovenia" => "Slovenie",	   
                                        "Switzerland" => "Suisse",	   
                                        "Turkey" => "Turquie",	   
					"UK" => "Grande-Bretagne",		   
					"Ukraine" => "Ukraine",	   
					"USA" => "USA"             
                                        )
                  );

?>
