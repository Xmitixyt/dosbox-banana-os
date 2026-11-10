"""Build and validate an experimental package; never publish automatically."""
import pathlib,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,str(root/'tools/build.py')],cwd=root,check=True)
bpkg=root/'upstream/sdk/tools/bpkg'
out=root/'build/dosbox-banana-os-x86_64.bpk'
binary=root/'build/x86_64/dosbox'
subprocess.run([sys.executable,str(bpkg),'pack','-o',str(out),str(root/'manifest'),
    'app-x86_64='+str(binary),str(root/'jazz.conf'),
    'COPYING='+str(root/'dosbox-svn/COPYING'),
    'COPYING.Banana='+str(root/'upstream/sdk/LICENSE')],check=True)
subprocess.run([sys.executable,str(bpkg),'check',str(out)],check=True)
