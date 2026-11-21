# AMB96 en C

Portage en C d'AMB96 (2021, assembleur SH2 + 68000) sur le simulateur 32X
`pixel`. Deux plans de tiles défilent indépendamment, l'un au-dessus de
l'autre, séparés par une ligne mobile. Un grand sprite (Sonic) et quatre
coins arrondis sont dessinés par-dessus. Les deux SH2 se partagent le
travail.

## Compiler et lancer

```sh
make amb96                # compile tout et lance la fenêtre
./build/amb96_headless --frames 120 --shot 60:image.ppm   # sans fenêtre
```

Sous Linux, MiniFB a besoin des en-têtes OpenGL. On peut s'en passer :
`cmake -S . -B build -DMINIFB_USE_OPENGL_API=OFF`.

## Commandes

| Manette          | Clavier      | Effet                                 |
|------------------|--------------|---------------------------------------|
| Haut / Bas       | flèches      | déplace la ligne de séparation        |
| A + flèches      | J + flèches  | fait défiler le plan A (haut)         |
| B + flèches      | K + flèches  | fait défiler le plan B (bas)          |
| Start + flèches  | Entrée + flèches | déplace le sprite                 |
| C maintenu       | L            | 4 pixels par image au lieu de 1       |

Le contenu suit la flèche. J, K et L sont repérés par leur position :
ils sont au même endroit en AZERTY et en QWERTY.

## Les images

Toutes les images sont des **BMP 8 bits indexés** (256 couleurs) qui
partagent la même palette. `tools/bmp2c` les convertit en C à chaque
compilation, d'après le manifeste `assets/assets.txt` :

```
plane     plane_a     plane_a.bmp
sprite    sonic       sonic.bmp
```

- **Plan** : l'image entière du niveau. bmp2c la découpe en tiles de
  16 × 16, ne garde qu'un exemplaire des tiles identiques, et fait un seul
  jeu de tiles pour tous les plans (65 535 tiles au plus, numéros sur
  16 bits). Le nombre de tiles en largeur et en hauteur doit être une
  puissance de 2 : les plans actuels font 1024 × 1024 pixels, soit
  64 × 64 tiles, et partagent 90 tiles. La taille des tiles se règle
  par `TILE_SHIFT` dans `assets_types.h`, lu par bmp2c et par le moteur.
  Pourquoi 16 × 16 : une ligne de tile fait alors 16 octets, une ligne du
  cache SH2 ; chaque chargement de cache sert en entier à la ligne d'écran
  dessinée, alors qu'en 8 × 8 la moitié servirait à l'autre CPU.
- **Sprite** : l'image telle quelle. L'index 0 est transparent.
- **Palette** : celle de la première image du manifeste. bmp2c refuse une
  image qui donne une autre couleur à un index qu'elle utilise.

GIMP (Image > Mode > Couleurs indexées, puis Exporter en BMP), Aseprite
ou Pillow (mode `P`) produisent ce format. Pour ajouter une image : la
déposer dans `assets/`, l'ajouter au manifeste, puis l'utiliser sous le
nom `asset_<nom>` (déclaré dans `amb96_assets.h`, généré).

## Mesurer : compteurs d'écritures

Le simulateur ne mesure pas le temps SH2, mais il compte les accès au
frame buffer, ce qui coûte le plus sur 32X (bus 16 bits du VDP). Toutes
les écritures du moteur passent par `vdp_dram_write8/16/32()` (zone
normale) ou `vdp_overwrite_write8/16/32()` (zone overwrite), qui comptent
par CPU, par taille et par zone, image par image. Une écriture 8 ou
16 bits = un accès au bus ; une écriture 32 bits = deux.

- Fenêtre : `VDP_STATS=1 ./build/amb96` (toutes les 5 secondes).
- Sans fenêtre : à chaque `--shot` et à la fin.

Repère : redessiner l'écran entier en écritures 16 bits demande
35 840 accès (320 × 224 / 2). C'est ce que fait le fond, quel que soit
le défilement :

| Défilement X des plans | Fond, accès bus      | Sprites, accès bus |
|------------------------|----------------------|--------------------|
| pair                   | 35 840 (16 bits)     | ≈ 3 410            |
| impair                 | 35 840 (16 bits)     | ≈ 3 410            |

Avec la copie précédente (comme un memcpy ordinaire), un défilement
impair coûtait 71 680 accès : tout partait octet par octet.
`plane.c` explique les deux boucles (paire : un mot lu, un mot écrit ;
impaire : deux octets lus, assemblés en un mot écrit).

Les sprites sont parcourus sur tout leur rectangle, mais un mot dont les
deux pixels sont transparents n'est pas écrit : environ 3 410 accès au
lieu de 8 704. Le chiffre varie un peu avec la position (un pixel seul en
début ou en fin de ligne à une colonne impaire demande une écriture 8 bits).

## Routines assembleur SH2 (option)

Certaines fonctions peuvent être remplacées par une version écrite à la
main en assembleur SH2, dans `amb96/sh2/`. C'est une option, jamais une
obligation : le C reste la référence et n'est pas écrit en fonction de
l'assembleur. Une routine `.S` reprend exactement l'interface C (même
nom, mêmes paramètres, mêmes structures).

Aujourd'hui :
- `plane_draw_line` (`amb96/sh2/plane_draw_line.S`), avec `DT` et le
  délai de branchement dans les boucles internes ;
- `sprite_copy_row` (`amb96/sh2/sprite_copy_row.S`) : test du mot
  transparent avec le `DT` dans le délai du `BT/S`, et lecture du mot
  suivant dans le délai du `BF/S` de la boucle ; écriture par adressage
  indexé `@(R0,R5)`, R0 gardant l'écart destination - source.

- **Activation** : option CMake `AMB96_ASM` (ou `-DAMB96_ASM` à la
  compilation). Elle ne sert que pour une compilation SH2 ; sur le Mac, le
  simulateur garde toujours la version C, et CMake refuse l'option.
- **Accord C / assembleur** : `asm_abi.h` donne à l'assembleur la
  position des champs des structures et la largeur d'écran ; `plane.c`
  vérifie à la compilation SH2 qu'elles sont justes. Le `.S` refuse de
  s'assembler si `TILE_SHIFT` n'est plus celui qu'il suppose.
- **Vérification** : `tools/sh2test/run.sh` compile la version C et la
  version assembleur pour SH gros-boutiste dans le même programme, les
  lance sous `qemu-sh4eb` et compare leurs résultats : toutes les valeurs
  de défilement X pour le fond ; toutes les longueurs, colonnes de départ
  et parités pour les sprites. L'assembleur est contrôlé en SH2 strict
  (`--isa=sh2`). Il faut Linux (qemu en mode utilisateur n'existe pas sous
  macOS : VM ou Docker) et `apt install gcc-sh4-linux-gnu qemu-user`.

## Organisation du code

| Fichier           | Rôle                                                       |
|-------------------|------------------------------------------------------------|
| `amb96.c`         | boucles du Master et du Slave, logique, dessin d'une image |
| `md68k.c`         | programme 68000 : manette → registre COMM                  |
| `shared.h`        | registres COMM utilisés, scène partagée, accès sans cache  |
| `plane.c`         | dessin d'une ligne de plan de tiles                        |
| `sprite.c`        | dessin d'un sprite (découpage aux bords, zone overwrite)   |
| `framebuffer.c`   | table de lignes et accès au frame buffer de dessin         |
| `assets_types.h`  | types `Tileset`, `Tilemap`, `Sprite`                       |
| `../tools/bmp2c.c`| convertisseur BMP → C                                      |

### Une image

1. Le 68000 écrit la manette dans `COMM_PAD` à chaque VBlank.
2. Le Master lit la manette, calcule la scène (défilements, séparation,
   position du sprite), la publie, puis écrit le numéro d'image dans
   `COMM_FRAME`.
3. Chaque CPU dessine **une ligne sur deux** : le Master les paires, le
   Slave les impaires, fond puis sprites. Un CPU possède entièrement ses
   lignes, donc aucun des deux n'attend l'autre au milieu de l'image
   (l'original séparait l'écran en deux à la ligne 128, et devait se
   synchroniser entre les tiles et les sprites).
4. Le Slave écrit `COMM_SLAVE_DONE`. Le Master attend la fin du VBlank en
   cours, demande l'échange de frame buffer, puis attend qu'il ait lieu.

### Différences avec AMB96.asm

- Le 68000 ne calcule plus rien : il transmet la manette, et la logique
  est sur le Master. La ligne de séparation ne passe plus par les
  registres DREQ.
- Les sprites sont placés en coordonnées d'écran (plus de décalage de
  128), et le sprite se déplace avec Start au lieu de suivre les
  défilements verticaux.
- Le fondu de palette (`palFader`) et le benchmark par H_INT ne sont pas
  encore portés.
- Tiles de 16 × 16 au lieu de 32 × 32, numérotées sur 16 bits à partir
  de 0 (l'original commençait à $80, qui correspondait à l'adresse
  $06020000).
- La couleur 255 n'a plus son bit de priorité ($FC00 → $7C00) : un BMP
  ne peut pas le contenir.

## Vers une vraie 32X

Le code du jeu n'utilise que l'API du simulateur, qui imite les registres
de la 32X (`vdp_dram`, `vdp_palette`, FBCR, `comm[]`). À reprendre pour la
console :

- `SHARED()` dans `shared.h` donne déjà l'adresse cache-through sur SH2
  (non testé sur matériel).
- Les sprites passent par la zone overwrite du frame buffer
  (2402 0000h), où un octet nul n'est pas écrit : la transparence est
  faite par le matériel. Dans le simulateur, `vdp_overwrite_write8/16/32()`
  tiennent lieu de ces écritures ; sur console, ce seront de simples
  écritures à l'adresse 2402 0000h + offset. Comme le SH2, le simulateur
  s'arrête sur un accès 16 ou 32 bits mal aligné.
- `memcpy` dans `plane.c` et `copyRow()` dans `sprite.c` sont les premiers
  candidats à l'optimisation (écritures 32 bits, ou les routines
  assembleur d'origine).
- Le simulateur ne mesure pas le temps SH2 : le budget d'une image
  (environ 23 MHz par CPU) reste à vérifier.
