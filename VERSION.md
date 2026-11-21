**Ce qui a changé**
- **Une seule constante.** La taille des tiles est fixée par `TILE_SHIFT` dans `assets_types.h`, que lisent à la fois bmp2c et le moteur. Changer la taille ne demande plus de modifier deux endroits.
- **Numéros de tile sur 16 bits.** Les deux plans n'utilisent que 198 tiles différentes, ce qui tiendrait sur un octet. Mais en 8 × 8, 256 tiles ne représentent que 16 Ko de graphismes, et un vrai décor dépasserait vite cette limite. Chaque plan de 1024 × 1024 devient une grille de 128 × 128 tiles, soit 32 Ko de numéros (au lieu de 1 Ko).
- **bmp2c** recherche les tiles déjà vues avec une table de hachage, au lieu de comparer chaque tile à toutes les autres. Avec des milliers de tiles 8 × 8, la recherche linéaire serait devenue lente.
- **`plane.c`** reste générique : une ligne compte maintenant 40 ou 41 morceaux de 8 pixels au lieu de 10 ou 11 de 32. Ce sera justement le terrain de la réécriture en écritures 16 bits.
