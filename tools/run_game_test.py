"""Run the port in a temporary QEMU session with serial and audio capture."""
import argparse
import pathlib
import socket
import subprocess
import sys

from qmp import command

root = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('name', help='name for the serial and WAV files')
parser.add_argument('--iso', type=pathlib.Path, default=pathlib.Path.home() / 'Downloads/Banana_OS.iso')
parser.add_argument('--disk', type=pathlib.Path, default=root / 'build/test-disk.img')
parser.add_argument('--qemu', default='qemu-system-x86_64')
parser.add_argument('--package', default='/mnt/virtio/dosbox.bpk')
parser.add_argument('--config', default='/mnt/virtio/JAZZUSER/jazz.conf')
parser.add_argument('--frames', type=int, default=1800)
parser.add_argument('--selftest', action='store_true')
parser.add_argument('--smoke', action='store_true', help='run the mounted DOS file I/O fixture first')
parser.add_argument('--accel', choices=['tcg', 'whpx'], default='tcg')
args = parser.parse_args()

log = root / 'build' / f'{args.name}.log'
audio = root / 'build' / f'{args.name}.wav'
commands = ['--cmd', f'pkg install {args.package} ## pkg: installed']
if args.selftest:
    commands += ['--cmd', 'dosbox --selftest ## DOSBOX_RUNTIME_SELFTEST: 0 failed']
commands += ['--cmd', 'startx ## GUI enabled']
if args.smoke:
    commands += ['--cmd', 'rm -f /mnt/virtio/SMOKE/RESULT.TXT',
                 '--cmd', 'dosbox --frames 300 /mnt/virtio/SMOKE/smoke.conf ## status=closed',
                 '--cmd', 'cat /mnt/virtio/SMOKE/RESULT.TXT ## DOSBOX_DOS_IO_OK']
commands += ['--cmd', f'dosbox --frames {args.frames} {args.config} ## status=closed']
try:
    result = subprocess.run([
        sys.executable, str(root / 'upstream/tools/serialtest.py'),
        '--qemu', args.qemu, '--log', str(log), '--timeout', '180', *commands, '--',
        '-accel', args.accel, '-vga', 'std', '-m', '1G', '-smp', '4', '-nic', 'none',
        '-cdrom', str(args.iso), '-boot', 'd',
        '-drive', f'file={args.disk},format=raw,if=virtio,snapshot=on',
        '-audiodev', f'wav,id=snd0,path={audio}', '-device', 'AC97,audiodev=snd0',
        '-qmp', 'tcp:127.0.0.1:55551,server=on,wait=off',
    ], cwd=root)
finally:
    try:
        command({'execute': 'quit'})
    except (OSError, EOFError):
        pass
sys.exit(result.returncode)
