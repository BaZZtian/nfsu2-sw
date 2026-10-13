"""Materialize the opt-in PAL/DE source profile without changing the US tree."""
from pathlib import Path
import argparse,hashlib,io,json,subprocess,tempfile,zipfile

ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def materialize(output,profile):
    archive=subprocess.check_output(['git','-C',str(ROOT),'archive','--format=zip',profile['base_commit']])
    with zipfile.ZipFile(io.BytesIO(archive)) as z:
        for entry in z.infolist():
            target=(output/entry.filename).resolve()
            if not target.is_relative_to(output.resolve()):raise ValueError('Invalid archive path')
            if entry.is_dir():target.mkdir(parents=True,exist_ok=True)
            else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(z.read(entry))
    for name,value in profile['files'].items():
        path=output/name
        data=path.read_bytes().replace(b'\r\n',b'\n') if path.exists() else None
        if (sha(data) if data is not None else None)!=value['base_sha256']:
            raise ValueError('Unexpected base source: '+name)
    subprocess.run(['git','-C',str(output),'apply','--check',str(ROOT/'pal/source.patch')],check=True)
    subprocess.run(['git','-C',str(output),'apply',str(ROOT/'pal/source.patch')],check=True)
    for name,value in profile['files'].items():
        if sha((output/name).read_bytes().replace(b'\r\n',b'\n'))!=value['pal_sha256']:
            raise ValueError('PAL source mismatch: '+name)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--xbe',type=Path,required=True,help='Owner-provided EU default.xbe')
    ap.add_argument('--gen-dir',type=Path,help='Existing verified PAL translation; never copied or modified')
    ap.add_argument('--output',type=Path,default=ROOT/'build-eu/source')
    ap.add_argument('--check',action='store_true',help='Verify in a temporary local source directory')
    args=ap.parse_args();profile=json.loads((ROOT/'pal/profile.json').read_text(encoding='utf-8'))
    data=args.xbe.read_bytes()
    if len(data)!=profile['xbe']['size'] or sha(data)!=profile['xbe']['sha256']:
        raise ValueError('Unsupported executable: this profile requires the verified EU XBE')
    if sha((ROOT/'pal/source.patch').read_bytes())!=profile['patch_sha256']:
        raise ValueError('PAL patch checksum mismatch')
    if args.gen_dir:
        for name,digest in profile['tested_translation_sha256'].items():
            path=args.gen_dir/name
            if not path.is_file() or sha(path.read_bytes())!=digest:
                raise ValueError('Translation differs from the tested PAL profile: '+name)
    if args.check:
        temporary_root=(ROOT/'build-eu').resolve();temporary_root.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='check-',dir=temporary_root) as name:
            output=Path(name).resolve()
            assert output.is_relative_to(temporary_root)
            materialize(output,profile)
            # Temporary cleanup is confined to the explicitly checked local directory.
            assert output.is_relative_to(temporary_root)
    else:
        output=args.output.resolve()
        if output==ROOT.resolve() or ROOT.resolve().is_relative_to(output):
            raise ValueError('Output must not contain the working repository')
        if output.exists() and any(output.iterdir()):raise ValueError('Output directory must be empty')
        output.mkdir(parents=True,exist_ok=True);materialize(output,profile)
        print('PAL source:',output)
    print('PASS: EU fingerprint and',len(profile['files']),'PAL source deltas verified')
    if args.gen_dir:print('PASS: existing PAL translation verified; generated files were not changed')
    else:print('EU translation integration is still draft; no generated game code is included')

if __name__=='__main__':main()
