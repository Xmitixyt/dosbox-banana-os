# DOSBox pour Banana OS

Port **x86_64** de DOSBox SVN/libretro pour Banana OS, avec affichage logiciel,
clavier, souris capturée et audio PCM. La configuration incluse vise Jazz
Jackrabbit et active le recompilateur x86.

## Compiler

Python 3.12+, Git et LLVM (Clang + LLD) sont nécessaires. Sous Windows, LLVM est
recherché dans `C:/Program Files/LLVM/bin` ; définir `LLVM_DIR` pour un autre chemin.
Les scripts téléchargent les dépendances dans le projet, sans installation système.

```powershell
python tools/bootstrap.py
python tools/package.py
```

Le paquet est généré dans `build/dosbox-banana-os-x86_64.bpk`. Les versions du
SDK, de DOSBox et de libretro-common sont figées. Les archives Debian contenant
les en-têtes et bibliothèques C++ sont vérifiées par SHA-256.

## Installer et lancer un jeu

Copier le paquet sur une clé FAT32 et l'installer depuis le terminal de Banana OS 64 bits.

```sh
pkg install /mnt/usb/dosbox-banana-os-x86_64.bpk
dosbox --version
dosbox --selftest
```

Le bureau doit être démarré (`startx` si nécessaire). Copier le dossier complet
de Jazz et placer `/apps/dosbox/jazz.conf` à côté de `JAZZ.EXE`. La commande
`dosbox --help` rappelle où trouver ce profil.

```sh
dosbox /mnt/usb/JAZZ/jazz.conf
```

Les chemins hôtes dans `[autoexec]` sont relatifs au dossier du fichier de
configuration. Le modèle monte ce dossier comme lecteur DOS C: et lance le jeu.
Pour configurer le son dans SETUP, choisir Sound Blaster, adresse 220, IRQ 7
et DMA 1 (DMA haut 5 pour SB16).

Une autre application DOS peut être lancée directement.

```sh
dosbox /mnt/usb/MONJEU/JEU.EXE
```

Un fichier `.conf` permet de régler la vitesse ou de lancer plusieurs commandes.

```ini
[cpu]
core=normal
cycles=10000
[autoexec]
mount c .
c:
programme.exe
```

Le clavier utilise les positions physiques QWERTY, y compris sur AZERTY.
Cliquer dans la fenêtre capture la souris. Échap la libère et est également
transmis au jeu. Fermer la fenêtre quitte DOSBox. Choisir un support inscriptible
pour que le jeu puisse enregistrer ses réglages et sauvegardes dans le dossier monté.

## Limites

- Banana OS 64 bits avec API 20 ou ultérieure. Les essais utilisent l'image build-318.
- `core=dynamic` accélère Jazz. `core=normal` reste disponible pour les programmes qui en ont besoin.
- Affichage logiciel au format 4:3 ; pas d'accélération OpenGL.
- Pas de manette, de réseau IPX, de MIDI externe ni de lecteur CD physique.
- Shift/Ctrl/Alt gauche et droit sont fusionnés par l'API du SDK.
- La gestion des locales C++ est limitée à C/POSIX et reprend l'adaptation
  privée du port LLVM de Banana OS.
- Aucun fichier commercial de jeu n'est inclus.

## Validation et publication

Les résultats des essais QEMU figurent dans [docs/VALIDATION.md](docs/VALIDATION.md).
Sous Windows, l'option QEMU `-accel whpx` a permis d'atteindre la cadence nominale
pendant l'essai sur un hôte où Windows Hypervisor Platform était disponible.

## Sources et licences

- DOSBox SVN/libretro : https://github.com/libretro/dosbox-svn,
  commit `46a3164fc0f1e808ae56143d81698b00fb6b4781`, GPL-2.0-or-later.
- SDK Banana OS : https://github.com/qcom-toolbox/Banana-OS,
  commit `d3a03c1de1590d09dc184ba8793a7658d1371715`, SDK BSD-2-Clause.
- `src/cxx_abi.c` dérive de `ports/llvm/compat.c` du même dépôt, BSD-2-Clause.
- libretro-common : sous-module figé par DOSBox, licences propres à ses fichiers.
- GNU libstdc++/libgcc 12.2 : GPL avec GCC Runtime Library Exception.

Le code propre au port est GPL-2.0-or-later, sauf mentions contraires dans les fichiers.
Une distribution binaire doit être accompagnée des sources correspondantes,
des scripts de construction et des mentions de licence des dépendances.
