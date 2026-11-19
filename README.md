Synchro vidéo autonome, cadencée par lignes (262 × 60 Hz)

- Plate-forme : 4 threads (affichage, synchro vidéo, master, slave).
  La synchro déduit la ligne courante de l'horloge depuis une origine
  fixe, traite chaque frontière dans l'ordre même en retard, et ne
  tourne en attente active qu'autour du début et de la fin du VBlank.
- VDP : rendu ligne par ligne ; échange des buffers avant VBLK = 1.
- Jeu : plus d'appel à la plate-forme hors platform_running() ;
  le slave attend CMD_READY (registre COMM) avant de démarrer.
- Rôle des CPU documenté (broche MD5 : masse = master, Vcc = slave).
- Diagnostics VDP_STATS=1 : VBlank vus par CPU, retards de la synchro,
  images produites/affichées, durées de mfb_update/mfb_wait_sync.
  VDP_QOS=0 désactive la priorité des threads.

Connu : saccades intermittentes de l'affichage sous macOS, côté MiniFB
(blocages par paliers de 100 ms), émulation non affectée. À analyser
avec `sample`.




Je développe d'abord le cadencement par lignes, le rendu ligne par ligne et les diagnostics : c'est le socle que vous pourrez mesurer chez vous. La V_INT viendra ensuite, sur ce socle validé.

La synchro vidéo fonctionne maintenant par lignes, conformément à votre principe : 262 lignes par image à 60 Hz, soit 63,614 µs par ligne, sans jamais étirer le VBlank. Je n'ai pas pu la valider en conditions réelles : sur ma machine de 2 cœurs, le slave a raté 42 à 73 VBlank sur 181, et le master 60 à 70. Le test sur votre Mac est donc indispensable.

**Pourquoi ces chiffres ne permettent pas de conclure.** Ici, trois threads tournent en continu (la synchro, le master et le slave) pour deux cœurs : ils se volent sans cesse le processeur. Pour vérifier que la logique est juste, j'ai fait un essai interne où la synchro dort entre deux lignes, ce qui libère un cœur. On tombe alors à 0 à 3 VBlank ratés sur 180, avec 173 à 174 images correctes sur 174. Le code est donc correct ; ce qui reste dépend des cœurs disponibles. Je n'ai pas livré cette variante, puisque ce n'est pas le mode que vous avez choisi.

**Ce qui change :**
- **Le thread de synchro tourne en continu** et calcule la ligne en cours à partir d'une origine fixe, sans dérive. Il traite chaque ligne franchie dans l'ordre, même en retard. S'il a pris plus d'une image de retard, il reprend au début de l'image en cours.
  - Ligne 0 : fin du VBlank.
  - Lignes 0 à 223 : rendu de la ligne.
  - Ligne 224 : début du VBlank et publication de l'image.
- **Le rendu se fait ligne par ligne** (`vdp_render_line()`), au moment où le balayage atteint chaque ligne. Une palette modifiée en cours d'image s'affichera donc à partir de la bonne ligne.
- **La plate-forme sait quel thread est quel CPU** (`platform_cpu_id()`), ce qui permet au VDP de compter les VBlank vus par chacun.
- **Sous macOS**, les threads de synchro, du master et du slave demandent la classe de QoS « user interactive », pour rester sur les cœurs performance.
- game.c et comm.* ne changent pas.

**Pour me transmettre les résultats**, lancez le programme avec la variable d'environnement :

```
VDP_STATS=1 ./pixel
```

Une ligne s'affiche sur la sortie d'erreur toutes les 5 secondes, et une dernière à la fermeture :

```
[VDP fin 3.0 s] VBlank 181, vus master 181 (rates 0), vus slave 181 (rates 0) | lignes en retard 13362, retard max 7112.8 us, images sautees 0 | images produites 181, affichees 179, ecrasees 2
```

- **rates (master, slave)** : les VBlank qu'un CPU n'a pas vus. C'est le chiffre qui nous intéresse.
- **lignes en retard / retard max** : combien de lignes la synchro a traitées avec plus d'une ligne de retard, et le pire retard. Avec un cœur pour elle seule, le retard maximal devrait rester de l'ordre de quelques dizaines de microsecondes.
- **images sautées** : les retards de plus d'une image complète.
- **écrasées** : les images produites mais jamais affichées, parce que la fenêtre n'a pas suivi. Quelques-unes sont normales, car l'affichage de MiniFB n'est pas calé sur notre horloge.

Ces chiffres, envoyés après une trentaine de secondes, suffiront pour qu'on décide de l'étape suivante, la V_INT.