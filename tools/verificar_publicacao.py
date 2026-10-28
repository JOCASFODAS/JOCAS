"""Validate the tracked central sources without reading local user settings."""
import ast,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
files=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
for name in filter(None,files):
 p=ROOT/name
 if not p.is_file():continue  # Git submodule is pinned separately.
 assert not any(part in ('.config','__pycache__','node_modules','work') for part in p.relative_to(ROOT).parts),name
 assert p.stat().st_size<15*1024*1024,(name,'large payload belongs in a release')
 if p.suffix.lower() in ('.png','.ico','.gz'):continue
 raw=p.read_bytes()
 try:text=raw.decode('utf8')
 except UnicodeError:raise AssertionError((name,'unexpected binary'))
 assert not re.search(r'(?:gh[pousr]_[A-Za-z0-9]{25,}|github_pat_[A-Za-z0-9_]{30,}|sk-(?:proj-)?[A-Za-z0-9_-]{30,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----)',text),(name,'possible credential')
 assert not re.search(r'/home/(?!SEU_USUARIO/|username/|usuario/|user/|test/|teste/|someone/)[a-zA-Z0-9_.-]+/',text),(name,'local home path')
 if p.suffix=='.py':ast.parse(text,filename=name)
 if p.suffix=='.json':json.loads(text)
projects=json.loads((ROOT/'PROJETOS.json').read_text())['produtos']
assert len(projects)==3
for product in projects:
 assert (ROOT/product['pasta']).is_dir(),product['nome']
 assert product['versao'] and product['tag']
print('PASS: publication files, syntax, project metadata and common credential patterns')
