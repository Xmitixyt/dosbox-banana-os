# DOSBox 0.1.0 pour Banana OS

Cette première version pour Banana OS **64 bits** réduit les ralentissements de
Jazz Jackrabbit. Le profil fourni utilise le recompilateur x86 de DOSBox, un
affichage logiciel plus léger et une sortie audio qui conserve les échantillons
déjà en attente. Le jeu est fourni séparément par l'utilisateur.

## Installation

Copier `dosbox-banana-os-x86_64.bpk` sur une clé FAT32. Dans le terminal de Banana OS,
installer le paquet et vérifier l'autotest. Démarrer le bureau avec `startx` si nécessaire.

```sh
pkg install /mnt/usb/dosbox-banana-os-x86_64.bpk
dosbox --selftest
```

Dans une VM, remplacer `/mnt/usb` par le point de montage du disque partagé,
par exemple `/mnt/virtio`.

## Jazz Jackrabbit

1. Copier le dossier du jeu avec `JAZZ.EXE` et ses données sur un disque accessible à Banana OS.
2. Placer le **nouveau** `jazz.conf` à côté de `JAZZ.EXE`. Remplacer l'ancien profil si tu en utilisais un.
3. Lancer le fichier `.conf` depuis Banana OS.

```sh
dosbox /mnt/usb/JAZZ/jazz.conf
```

Si le jeu demande une configuration audio, lancer son `SETUP.EXE` et choisir
Sound Blaster avec le port 220, l'IRQ 7 et le DMA 1. Pour conserver les réglages
et les sauvegardes, utiliser un dossier inscriptible.

## Autres jeux et logiciels DOS

Copier tous les fichiers du programme dans le même dossier et lancer son
exécutable DOS. Les fichiers `.EXE`, `.COM` et `.BAT` sont acceptés.

```sh
dosbox /mnt/usb/MONJEU/JEU.EXE
```

Pour personnaliser la vitesse ou choisir un répertoire à monter, créer un
fichier `.conf` dans le dossier. Par exemple, `lancer.conf` peut contenir les
lignes suivantes.

```ini
[cpu]
core=normal
cycles=6000
[autoexec]
mount c .
c:
programme.exe
```

Lancer ensuite `dosbox /mnt/usb/MONJEU/lancer.conf`. Le clavier utilise les
positions physiques QWERTY. Cliquer dans la fenêtre pour capturer la souris,
appuyer sur Échap pour la libérer et fermer la fenêtre pour quitter.

Les essais ont été faits avec Banana OS build-318 dans QEMU. La fluidité dépend
de la vitesse de l'hôte et de l'accélérateur QEMU. Sur Windows, ajouter
`-accel whpx` à la commande QEMU si Windows Hypervisor Platform est disponible.
Cette option a tenu la cadence nominale de Jazz pendant l'essai. Voir les
résultats et limites dans [le rapport de validation](https://github.com/Xmitixyt/dosbox-banana-os/blob/v0.1.0/docs/VALIDATION.md).
