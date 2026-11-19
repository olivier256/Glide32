V_INT

**Ce qui reste pour plus tard** :
- le masque du SR (sections critiques entre une routine d'interruption et la boucle principale) ;
- H_INT, avec la synchro qui repasse en attente active ligne par ligne quand un CPU la démasque ;
- les interruptions CMD (venant du 68000) et PWM ;
- le compteur d'images (horloge du jeu) dans `master_vint()`, le jour où vous en aurez besoin ;
- les blocages intermittents de MiniFB, à analyser avec `sample` à la prochaine occurrence ;
- `setTwoPixels()`, qui n'est pas appelée pour l'instant.
