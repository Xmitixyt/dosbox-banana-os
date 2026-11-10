"""Bundle the exact sources needed to reproduce a published DOSBox package."""
import io
from pathlib import Path
import subprocess
from zipfile import ZIP_DEFLATED, ZipFile

root = Path(__file__).resolve().parents[1]
output = root / 'build/dosbox-banana-os-source.zip'
source_trees = (
    (root, 'HEAD', '', ()),
    (root / 'upstream', 'd3a03c1de1590d09dc184ba8793a7658d1371715', 'upstream/', ('sdk',)),
    (root / 'dosbox-svn', '46a3164fc0f1e808ae56143d81698b00fb6b4781', 'dosbox-svn/', ()),
    (root / 'dosbox-svn/libretro/deps/common', 'fa8a1b572a444fcb552007019f8caf68281108aa',
     'dosbox-svn/libretro/deps/common/', ()),
)

with ZipFile(output, 'w', compression=ZIP_DEFLATED) as archive:
    for directory, revision, prefix, paths in source_trees:
        data = subprocess.check_output([
            'git', '-C', str(directory), 'archive', '--format=zip', revision, *paths,
        ])
        with ZipFile(io.BytesIO(data)) as tree:
            for member in tree.infolist():
                if not member.is_dir():
                    archive.writestr(prefix + member.filename, tree.read(member))
    archive.writestr('BUILD-FROM-ARCHIVE.txt', '''The DOSBox, libretro-common and Banana OS SDK sources are included.
Extract this archive to an empty directory. With Python 3.12+ and Clang/LLD:

    python tools/fetch_toolchain.py
    python tools/package.py

The first command retrieves the pinned Debian GCC runtime headers and archives.
See README.md for LLVM_DIR and source license information.
''')
print(output)
