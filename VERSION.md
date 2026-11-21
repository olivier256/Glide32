Tiles 16x16 alignées sur le cache SH2

- TILE_SHIFT = 4 : une ligne de tile = 16 octets = une ligne du cache
  SH2. Chaque chargement de cache sert en entier à la ligne d'écran
  dessinée, quel que soit le CPU (en 8x8, la moitié servait à l'autre).
- bmp2c aligne le jeu de tiles sur 16 octets (TILE_ALIGN).
- Plans : 90 tiles, 64x64 tiles par plan.
- Simulateur : md_vint() écrit la manette avant l'échange de frame
  buffer (un appui d'une image n'est plus pris en compte en retard).
- VERSION.md : section « Tiles 16 × 16 ».

---

On ajoute les compteurs.
Lancer avec
```
VDP_STATS=1 make amb96
# ou, une fois compilé :
VDP_STATS=1 ./build/amb96
```

---

**Ce qui a changé**
- **Une seule constante.** La taille des tiles est fixée par `TILE_SHIFT` dans `assets_types.h`, que lisent à la fois bmp2c et le moteur. Changer la taille ne demande plus de modifier deux endroits.
- **Numéros de tile sur 16 bits.** Les deux plans n'utilisent que 198 tiles différentes, ce qui tiendrait sur un octet. Mais en 8 × 8, 256 tiles ne représentent que 16 Ko de graphismes, et un vrai décor dépasserait vite cette limite. Chaque plan de 1024 × 1024 devient une grille de 128 × 128 tiles, soit 32 Ko de numéros (au lieu de 1 Ko).
- **bmp2c** recherche les tiles déjà vues avec une table de hachage, au lieu de comparer chaque tile à toutes les autres. Avec des milliers de tiles 8 × 8, la recherche linéaire serait devenue lente.
- **`plane.c`** reste générique : une ligne compte maintenant 40 ou 41 morceaux de 8 pixels au lieu de 10 ou 11 de 32. Ce sera justement le terrain de la réécriture en écritures 16 bits.




---

Fond : écritures 16 bits seulement

**Ce que fait cette version** :
- `plane_draw_line` n'écrit plus dans le frame buffer que des mots de 16 bits, à des adresses paires, quel que soit le défilement. La destination est toujours paire (lignes à adresse paire, écrites à partir de x = 0) ; seule la source hérite de la parité du défilement X, et le choix de la boucle se fait une fois par ligne :
  - défilement pair : un mot lu dans la tile (`MOV.W`), un mot écrit ;
  - défilement impair : deux pixels lus séparément (deux `MOV.B`, depuis le cache), assemblés en un mot écrit. Le pixel de la colonne 15 d'une tile est gardé de côté pour former un mot avec le premier pixel de la tile suivante ;
- fond : 35 840 accès au bus par image dans tous les cas, au lieu de 71 680 pour un défilement impair (17 920 écritures 16 bits par CPU) ;
- `fb_copy` (copie façon memcpy) est supprimée : plus rien ne s'en sert ;
- vérifié : images identiques au pixel près au rendu de référence (défilements pairs, impairs, plans mixtes), et `plane_draw_line` testée seule sur toutes les valeurs de défilement X, négatives et au-delà du plan comprises (20 millions de pixels).

**Ce qui reste pour plus tard** :
- version finale du fond : lectures `MOV.L` alignées et décalages en registres (`XTRCT` pour le cas pair non multiple de 4), à comparer sur SH2 ;
- sprites : ne pas écrire les mots entièrement transparents (les coins coûtent 2 048 accès par image) ;
- mesurer le temps réel sur SH2 (chaîne `sh-elf-gcc`, émulateur 32X, compteur de lignes par H_INT).

---

Routines assembleur SH2 en option

**Ce que fait cette version** :
- `amb96/sh2/plane_draw_line.S` : `plane_draw_line` écrite à la main en assembleur SH2, avec la même interface que la version C (mêmes 5 paramètres, mêmes structures) et le même algorithme. Boucles internes en `DT` + `BF/S`, avec le délai de branchement utilisé ;
- l'assembleur est une option, pas une contrainte : le C n'a pas été modifié pour lui (seulement entouré de `#ifndef AMB96_ASM`). Activation par l'option CMake `AMB96_ASM`, refusée sur le simulateur, qui garde toujours la version C ;
- `amb96/asm_abi.h`, lu par le C et l'assembleur : position des champs de `Tilemap` et `Tileset`, adresse du frame buffer, largeur d'écran. `plane.c` vérifie à la compilation SH2 que ces valeurs correspondent aux structures ; le `.S` refuse de s'assembler si `TILE_SHIFT` change ;
- `assets_types.h` peut être inclus depuis l'assembleur (types C masqués par `__ASSEMBLER__`) ;
- `tools/sh2test/run.sh` : banc d'essai sous `qemu-sh4eb` (Linux). Compile la version C et la version assembleur en SH gros-boutiste dans le même programme et compare leurs lignes : 63 840 lignes, aucune différence. Une erreur volontaire dans le `.S` est bien détectée.

**Ce qui reste pour plus tard** :
- un vrai portage 32X : compilation `sh-elf-gcc`, `vdp_dram_write16()` en écriture directe, recopie des tiles en SDRAM (alignées sur 16) ;
- mesurer sur émulateur 32X la version C contre la version assembleur (compteur de lignes par H_INT) ;
- version finale du fond en `MOV.L` alignés et décalages ;
- sprites : ne pas écrire les mots entièrement transparents.

---

Sprites : les mots transparents ne sont pas écrits

**Ce que fait cette version** :
- `sprite_copy_row` (ex-`copyRow`, désormais visible dans `sprite.h`) n'écrit plus un mot dont les deux pixels sont transparents, ni un pixel seul transparent : la zone overwrite l'aurait ignoré, mais l'accès au bus aurait eu lieu. Les données des sprites ne changent pas ;
- sprites : environ 3 410 accès au bus par image au lieu de 8 704 ;
- `amb96/sh2/sprite_copy_row.S` : version assembleur, en option (`AMB96_ASM`), avec la boucle proposée par Olivier pour une source paire : test du mot avec le `DT` dans le délai du `BT/S`, lecture du mot suivant dans le délai du `BF/S` (le pointeur source est corrigé après la boucle). Source impaire : deux `MOV.B` assemblés, sans `EXTU.B` sur le pixel de gauche (les bits parasites n'apparaissent que si le mot n'est pas transparent) ;
- `tools/sh2test` compare aussi `sprite_copy_row` C et assembleur : 624 384 copies (toutes longueurs, colonnes de départ et parités), aucune différence ; deux erreurs volontaires détectées. Les zones de destination sont pré-remplies, pour voir aussi les écritures en trop ou manquantes ;
- vérifié : images identiques au pixel près au rendu de référence, et `sprite_draw` testée sur 116 250 positions.

**Ce qui reste pour plus tard** :
- un vrai portage 32X (compilation `sh-elf-gcc`, écritures directes, recopie des données en SDRAM) et la mesure sur émulateur, C contre assembleur ;
- version finale du fond en `MOV.L` alignés et décalages.
