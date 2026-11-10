from pathlib import Path
from pyfatfs.PyFat import PyFat
from pyfatfs.PyFatFS import PyFatFS
p=Path('build/test-disk.img');p.touch()
f=PyFat();f.mkfs(str(p),fat_type=32,size=128*1024*1024,label='DOSBOXTEST');f.close()
with PyFatFS(str(p)) as disk:
 for file in Path('build/share').rglob('*'):
  if file.is_file():
   rel=file.relative_to('build/share').as_posix()
   parent=str(Path(rel).parent).replace('\\','/')
   if parent!='.':disk.makedirs(parent,recreate=True)
   disk.writebytes(rel,file.read_bytes())
print('FAT32 test image created')
