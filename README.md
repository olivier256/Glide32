Mémoire 32X rangée comme sur le SH2 (big-endian)

- sh2mem.h : read16/write16/read32/write32. Les accès 8 bits restent
  directs ; les accès 16/32 bits passent par ces fonctions, qui
  échangent les octets sur un hôte petit-boutiste et se réduisent à
  MOV.W/MOV.L sur 32X.
- VDP : en mode Packed Pixel, les pixels sont lus octet par octet dans
  l'ordre des adresses (pixel x = octet adresse_ligne * 2 + x) ; la
  table de lignes est lue avec read16.
- Jeu : setPixel écrit un octet (équivalent MOV.B) ; nouveau
  setTwoPixels (équivalent MOV.W) ; table de lignes, effacement et
  registres COMM accédés via write16/read16.
- comm.c/.h renommés en interface.c/.h : IC4 (315-5818, MEGA Drive I/F),
  qui accueillera les autres registres système. Les registres de
  communication y sont un tableau partagé en accès direct.