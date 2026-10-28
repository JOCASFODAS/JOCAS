# SPDX-License-Identifier: GPL-3.0-or-later
"""For source builds only: fetch pinned, verified upstream uv executables."""
import argparse,hashlib,io,tarfile,urllib.request,zipfile
from pathlib import Path
ASSETS={
 'linux':('uv-x86_64-unknown-linux-gnu.tar.gz','f68d291c75783122dad58ab77def0e13f3233f358731db5d6d7e8715603b928f','uv-x86_64-unknown-linux-gnu/uv','uv'),
 'windows':('uv-x86_64-pc-windows-msvc.zip','a38b8d1902b7cfbc76b423ba1bb456f623f42064cdc72628b6075b89afd1019a','uv.exe','uv.exe')}
VERSION='0.13.5'
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('platform',choices=['linux','windows','both']);a=p.parse_args()
 dest=Path(__file__).resolve().parent/'tools';dest.mkdir(exist_ok=True)
 for platform in ASSETS if a.platform=='both' else [a.platform]:
  name,sha,member,out=ASSETS[platform]
  raw=urllib.request.urlopen(f'https://github.com/astral-sh/uv/releases/download/{VERSION}/{name}',timeout=90).read()
  if hashlib.sha256(raw).hexdigest()!=sha:raise RuntimeError('Invalid uv checksum')
  if name.endswith('.gz'):
   with tarfile.open(fileobj=io.BytesIO(raw)) as archive:data=archive.extractfile(member).read()
  else:
   with zipfile.ZipFile(io.BytesIO(raw)) as archive:data=archive.read(member)
  path=dest/out;path.write_bytes(data);path.chmod(0o755);print('Verified',platform,path)
