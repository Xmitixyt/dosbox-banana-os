# DOSBox 0.1.1 pour Banana OS

## Changements depuis la 0.1.0

- `dosbox --version` indique la version du port installée.
- `dosbox --help` affiche les formats pris en charge et le chemin du profil Jazz livré dans le paquet.

## Corrections apportées depuis le prototype

Ces corrections sont présentes depuis la 0.1.0 et restent incluses dans la 0.1.1.

- **Coupures audio**. DOSBox ralentit la production audio quand la file se remplit et conserve les échantillons en attente. Le profil Jazz utilise une sortie à 48 kHz.
- **Ralentissements dans Jazz Jackrabbit**. Le profil active le recompilateur x86 (`core=dynamic`) et règle l'émulation sur 9 000 cycles. L'affichage calcule la correspondance des pixels à l'avance et réutilise les lignes identiques.
- **Écriture des fichiers DOS**. Les écritures se font dans le dossier monté. Un programme `.COM` a créé et écrit un fichier de test sur un disque FAT32.

Sur Banana OS **64 bits** dans QEMU avec WHPX, Jazz a tenu sa cadence nominale d'environ 70 Hz pendant un essai de 30 secondes. Certaines scènes ralentissaient encore avec TCG. [Rapport de validation](https://github.com/Xmitixyt/dosbox-banana-os/blob/v0.1.0/docs/VALIDATION.md)

## Installation

Dans l'App Store, actualiser la liste puis rechercher DOSBox. Si le dépôt est déjà configuré, la mise à jour peut aussi se faire depuis le terminal.

```sh
apt update
apt install dosbox
dosbox --version
```

Pour une installation manuelle, copier `dosbox-banana-os-x86_64.bpk` sur une clé FAT32 et installer le paquet. Démarrer le bureau avec `startx` si nécessaire.

```sh
pkg install /mnt/usb/dosbox-banana-os-x86_64.bpk
dosbox --selftest
```

Dans une VM, remplacer `/mnt/usb` par le point de montage du disque partagé,
par exemple `/mnt/virtio`.

## Jazz Jackrabbit

1. Copier le dossier du jeu avec `JAZZ.EXE` et ses données sur un disque accessible à Banana OS.
2. Copier `/apps/dosbox/jazz.conf` à côté de `JAZZ.EXE`. Le profil est aussi disponible dans les fichiers de la release.
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

Sur Windows, ajouter `-accel whpx` à la commande QEMU si Windows Hypervisor Platform est disponible.
