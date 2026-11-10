# Validation de DOSBox 0.1.0

Essais du 10 novembre 2026 sur Banana OS 0.6 x86_64, image officielle `build-318`.
QEMU a démarré depuis l'ISO avec 4 vCPU, 1 Gio de RAM, VGA standard et AC97.
Les fichiers de Jazz Jackrabbit CD ont été copiés sur un disque FAT32 de test.

- ISO `Banana_OS.iso`, SHA-256 `8ffe2f1f113a8289551a0739b1a9f326d27a482a4071eff519f9537bcd04088b`.
- Paquet `dosbox-banana-os-x86_64.bpk`, SHA-256 `eae2be7229761b7f6db74532bf5d023b7c6b7a4c558c811e03d330c1769f75e9`.
- `bpkg check` a accepté le paquet. Le binaire ELF PIE utilise seulement des relocations relatives.
- `pkg install` et les sept vérifications de `dosbox --selftest` ont réussi dans Banana OS.
- Un programme DOS `.COM` a créé un fichier sur C: et écrit le marqueur `DOSBOX_DOS_IO_OK`.
- Jazz Jackrabbit CD a lancé le niveau Diamondus. Les commandes de déplacement,
  saut et tir ont été envoyées ensemble au jeu. Les captures montrent le déplacement
  du personnage et le défilement du niveau. La sortie PCM a été enregistrée via AC97.
- La session de jeu s'est terminée proprement après 6 000 passages dans la boucle
  d'émulation. Elle a généré 4 464 558 échantillons stéréo à 48 kHz, soit 93 secondes
  de son sur 114 secondes mesurées dans le système invité avec TCG.
- Avec `-accel whpx`, un essai distinct a terminé 2 100 passages en 29,911 s.
  DOSBox a tenu la cadence nominale de 70,09 Hz sans retard signalé par la boucle.

Dans les captures AC97, entre le premier et le dernier échantillon non nul,
la part des tranches de 10 ms entièrement silencieuses est passée de 47 % avec
le prototype à 11 % avec le paquet final. Les essais comportent des scènes et
des entrées différentes. Cette mesure inclut les silences du jeu.

TCG peut encore ralentir certaines scènes. Les essais couvrent l'intro, les
menus, les démos et le début de Diamondus. Les autres jeux, les sauvegardes
propres à Jazz et les machines physiques restent à vérifier.
