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
