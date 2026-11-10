"""Extract pinned Debian C++ headers/static libraries locally, without installation."""
import urllib.request,io,tarfile,pathlib,hashlib
out=pathlib.Path(__file__).resolve().parents[1]/'build/toolchain'; out.mkdir(parents=True,exist_ok=True)
hashes={
    'libstdc++-12-dev':'d28def6c23630432b57cb38a4c2fd67a79d4e0484027386ca6e8d6005c3d7a73',
    'libgcc-12-dev':'d720259380a84f2ffc6fe516eff5cbe9c8a005138e8d6a5748747ba4b3404a82',
}
for name in ('libstdc++-12-dev','libgcc-12-dev'):
    filename=name+'_12.2.0-14+deb12u1_amd64.deb'
    archive=out/filename
    data=archive.read_bytes() if archive.exists() else urllib.request.urlopen('https://deb.debian.org/debian/pool/main/g/gcc-12/'+filename).read()
    if hashlib.sha256(data).hexdigest()!=hashes[name]: raise RuntimeError('SHA256 mismatch: '+filename)
    archive.write_bytes(data)
    print(filename,hashlib.sha256(data).hexdigest())
    pos=8
    while pos<len(data):
        head=data[pos:pos+60]; size=int(head[48:58]); member=head[:16].decode().strip().rstrip('/'); pos+=60
        if member.startswith('data.tar'):
            with tarfile.open(fileobj=io.BytesIO(data[pos:pos+size])) as tf:
                for item in tf:
                    if item.isfile() or item.isdir(): tf.extract(item,out,filter='data')
        pos+=size+(size%2)
