Je fais l'étape 1 : séparer les couches sans rien changer à ce qui s'affiche. Je pars de votre dernier main.c, en y intégrant les deux modifications dont nous avons convenu (le garde-fou juste avant le flip, et le `waitForVBlank()` au démarrage du slave).

L'étape 1 est faite : l'affichage n'a pas changé. Dans mon simulateur, les deux points sont présents comme avant, le programme se ferme proprement, et la compilation en `-std=c99 -Wall -Wextra` ne donne aucun avertissement.

**Ce que fait chaque couche maintenant :**
- **Plate-forme** (platform_minifb.c) : elle possède le `main()`, qui joue le rôle de la BootROM : il initialise le VDP, ouvre la fenêtre, lance le slave puis appelle `master_main()`. Elle ne lit plus aucun registre du VDP ; elle se contente d'appeler `vdp_begin_vblank()`, `vdp_render()` et `vdp_end_vblank()`.
- **VDP** (vdp.c) : le décodage Packed Pixel et la conversion de palette y ont déménagé, sous la forme de `vdp_render(rgb)`. `displayFrameBuffer`, `vdp_dram0` et `vdp_dram1` sont désormais privés au module.
- **COMM** (comm.c) : `comm_read()` et `comm_write()` quittent le code du jeu. C'est ce module qui appelle `platform_sync()`, ce qui est logique puisque cette barrière émule une garantie du matériel. Les registres eux-mêmes ne sont plus accessibles directement.
- **Jeu** (game.c, l'ancien main.c) : il contient `master_main()` et `slave_main()`, avec vos fonctions de dessin et d'attente. Il connaît la hauteur d'écran par `vdp_screen_height()`.

**Les écarts, tous volontaires :**
- **Le jeu appelle encore `platform_wait_vsync()`.** C'est la seule entorse aux sens d'appel, et elle est signalée dans le code. Elle disparaîtra à l'étape 2.
- **Le master tourne toujours sur le thread principal.** C'est lui qui provoque l'appel à MiniFB, et Cocoa impose le thread principal. Il aura son propre thread à l'étape 2.
- **Le mode Blank affiche du noir.** Avant, le buffer de la fenêtre restait non initialisé.
- **`COMM_COUNT` passe à 8**, comme dans le manuel (p. 25).
- **`platform_vblank_elapsed_ms()` mesure enfin quelque chose** : `_vblank_start` n'était jamais renseigné dans le fichier d'origine.
- **J'ai retiré le code mort** : `elapsed_ms()` et `wait_vblank()`.

Dans votre CMakeLists.txt, remplacez `main.c` par `game.c` et ajoutez `comm.c` à la liste des sources.