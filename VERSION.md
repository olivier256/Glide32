Endianness : macros BE16/BE32 (bigendian.h)

- BE16(v) / BE32(v) renvoient la valeur à écrire en mémoire (ou relue)
  pour respecter l'ordre des octets du SH2. Elles échangent les octets
  sur un hôte petit-boutiste (__builtin_bswap16/32, argument évalué une
  seule fois) et ne font rien sur 32X.
- setTwoPixels utilise BE16 (équivalent MOV.W) ; plus de #if dans le jeu.
- Règle : seuls les pixels du mode Packed Pixel écrits par mot ou mot
  long en ont besoin. La table de lignes, les accès par octet, la
  palette, la SDRAM et les registres COMM (accédés avec une même
  taille) s'en passent.