"""Cross-build DOSBox SVN for Banana OS x86_64 with LLVM (Windows/Linux)."""
import os, pathlib, re, subprocess, concurrent.futures, sys, hashlib
ROOT = pathlib.Path(__file__).resolve().parents[1]
SDK = ROOT / 'upstream/sdk'
CORE = ROOT / 'dosbox-svn'
COMMON = CORE / 'libretro/deps/common'
OUT = ROOT / 'build/x86_64'
OUT.mkdir(parents=True, exist_ok=True)
LLVM = pathlib.Path(os.environ.get('LLVM_DIR', 'C:/Program Files/LLVM/bin' if os.name=='nt' else '/usr/bin'))
CC = str(LLVM / ('clang.exe' if os.name == 'nt' else 'clang'))
TC = ROOT / 'build/toolchain/usr'
# GNU's precompiled filebuf embeds a 40-byte pthread mutex. Preserve that
# C++ object layout while retaining the SDK mutex fields at the front.
cxx_headers=OUT/'cxx-headers'
cxx_headers.mkdir(exist_ok=True)
pthread_header=(SDK/'include/pthread.h').read_text()
needle='int          count;           /* (recursive) */'
if pthread_header.count(needle)!=1: raise RuntimeError('SDK pthread layout changed')
pthread_header=pthread_header.replace(needle,needle+'\n    unsigned char __dosbox_abi_padding[16]; /* 40-byte GNU x86_64 mutex ABI */')
(cxx_headers/'pthread.h').write_text(pthread_header)
flags = ['--target=x86_64-unknown-linux-gnu', '-O2', '-fPIC', '-mno-red-zone',
         '-ffile-prefix-map='+str(ROOT)+'=.',
         '-ffreestanding', '-fno-stack-protector', '-fno-builtin', '-ffunction-sections',
         '-fdata-sections', '-nostdinc', '-Wno-deprecated-declarations',
         '-D__LIBRETRO__', '-DWITH_FAKE_SDL', '-DC_HAVE_MPROTECT=1',
         '-DGIT_VERSION="46a3164"', '-DSVN_VERSION="r4470"']
includes = [ROOT/'include', CORE/'include', CORE/'src', CORE/'libretro',
            CORE/'libretro/fakesdl/SDL', COMMON/'include', COMMON/'libco']
flags += ['-I'+str(p) for p in includes]
flags += ['-idirafter', str(SDK/'include')]
flags += ['-idirafter', subprocess.check_output([CC,'-print-resource-dir'],text=True).strip()+'/include']
flags += ['-U__linux__'] # Banana has POSIX paths, but no glibc fopencookie/VFS.
flags += ['-include', str(COMMON/'include/retro_endianness.h'), '-include', str(CORE/'libretro/dynarec.h')]
cxx = ['-std=gnu++11', '-nostdinc++', '-I'+str(cxx_headers), '-include', str(ROOT/'include/cxx_compat.h')]
cxx += ['-isystem'+str(p) for p in [TC/'include/c++/12',TC/'include/x86_64-linux-gnu/c++/12',TC/'include/c++/12/backward']]
make = (CORE/'libretro/Makefile.common').read_text()
make = make[:make.index('ifneq ($(WITH_FAKE_SDL), 1)')] + make[make.index('SOURCES_C :='):]
paths = re.findall(r'\$\((CORE_DIR|LIBRETRO_COMM_DIR)\)/([^\s\\]+\.(?:cpp|c))', make)
sources = [(CORE if k == 'CORE_DIR' else COMMON)/p for k,p in paths]
sources += list((ROOT/'src').glob('*.c')) + list((ROOT/'src').glob('*.cpp'))
sdk_names = ['crt0','libc','malloc','stdio','banana','math','posix','setjmp','mman','pthread','socket','signal','dl','cxxrt','unix','utf8','ufd','math99','proc','tty','ufs','regex']
sources += [SDK/'lib'/ (p+'.c') for p in sdk_names if p != 'cxxrt']
# SDK's minimal runtime intentionally has dummy RTTI and abort-only throws.
# DOSBox needs real dynamic_cast and exceptions from libstdc++ instead.
runtime = OUT/'sdk_cxxrt.c'
runtime_text = (SDK/'lib/cxxrt.c').read_text().split('/* the virtual tables')[0]
if not runtime.exists() or runtime.read_text()!=runtime_text: runtime.write_text(runtime_text)
sources.append(runtime)
header_hash=hashlib.sha256()
header_hash.update(pthread_header.encode())
for base in [ROOT/'include',SDK/'include',CORE/'include',CORE/'libretro',CORE/'src',COMMON/'include']:
    for header in sorted(base.rglob('*.h')): header_hash.update(header.read_bytes())
header_hash.update(subprocess.check_output([CC,'--version']))
header_key=header_hash.hexdigest()
def compile_one(src):
    obj = OUT / (str(src.relative_to(ROOT)).replace('\\','_').replace('/','_')+'.o')
    cmd = [CC,*flags,*(cxx if src.suffix=='.cpp' else ['-std=gnu11']),'-c',str(src),'-o',str(obj)]
    stamp = obj.with_suffix('.cmd')
    key = repr(cmd)+header_key
    if obj.exists() and stamp.exists() and stamp.read_text()==key and obj.stat().st_mtime>max(src.stat().st_mtime,(ROOT/'include/cxx_compat.h').stat().st_mtime): return obj
    result=subprocess.run(cmd,capture_output=True,text=True)
    (obj.with_suffix('.log')).write_text(result.stdout+result.stderr)
    if result.returncode: return (src,result.stderr)
    stamp.write_text(key)
    return obj
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
    results=list(pool.map(compile_one,sources))
errors=[x for x in results if isinstance(x,tuple)]
if errors:
    for src,err in errors[:8]: print(src.relative_to(ROOT),err[:3500])
    print(f'{len(errors)}/{len(sources)} compilation failures; full logs in {OUT}')
    sys.exit(1)
print(f'Compiled {len(results)} objects')
lib=TC/'lib/gcc/x86_64-linux-gnu/12'
link=[str(LLVM/('ld.lld.exe' if os.name=='nt' else 'ld.lld')), '-pie','--no-dynamic-linker','--eh-frame-hdr','--gc-sections','-z','notext','-z','max-page-size=4096','-e','_banana_start','-o',str(OUT/'dosbox')]
link += [str(p) for p in results]
link += ['--start-group',str(lib/'libstdc++.a'),str(lib/'libgcc.a'),str(lib/'libgcc_eh.a'),'--end-group']
r=subprocess.run(link,capture_output=True,text=True)
(OUT/'link.log').write_text(r.stdout+r.stderr)
print(r.stdout+r.stderr)
sys.exit(r.returncode)
