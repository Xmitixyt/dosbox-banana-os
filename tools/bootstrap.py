"""Fetch the exact SDK and DOSBox revisions used by this port."""
import pathlib,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[1]
for directory,url,commit in [
    ('upstream','https://github.com/qcom-toolbox/Banana-OS.git','d3a03c1de1590d09dc184ba8793a7658d1371715'),
    ('dosbox-svn','https://github.com/libretro/dosbox-svn.git','46a3164fc0f1e808ae56143d81698b00fb6b4781')]:
    dest=root/directory
    if not dest.exists():
        subprocess.run(['git','init',str(dest)],check=True)
        subprocess.run(['git','-C',str(dest),'remote','add','origin',url],check=True)
        subprocess.run(['git','-C',str(dest),'fetch','--depth','1','origin',commit],check=True)
        subprocess.run(['git','-C',str(dest),'checkout','--detach','FETCH_HEAD'],check=True)
    actual=subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()
    if actual!=commit:sys.exit(f'{dest}: expected {commit}, got {actual}; refusing to alter existing checkout')
subprocess.run(['git','-C',str(root/'dosbox-svn'),'submodule','update','--init','--depth','1','libretro/deps/common'],check=True)
subprocess.run([sys.executable,str(root/'tools/fetch_toolchain.py')],cwd=root,check=True)
