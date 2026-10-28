from pathlib import Path
import tempfile
import unittest
import zipfile
from catalogo import publish

class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.downloads=self.root/'downloads'

    def packages(self,version,prefix='JOCAS-Lista-de-Mods'):
        files=[]
        for kind in ('Instalacao','Atualizacao','Fontes'):
            path=self.root/f'{prefix}-{version}-{kind}.zip'
            with zipfile.ZipFile(path,'w') as z:z.writestr('teste.txt',version)
            files.append(path)
        return files

    def test_incomplete_release_keeps_old_files(self):
        publish(self.downloads,'JOCAS-Lista-de-Mods','1.0.0',self.packages('1.0.0'))
        old={p.name:p.read_bytes() for p in self.downloads.glob('*.zip')}
        with self.assertRaises(ValueError):
            publish(self.downloads,'JOCAS-Lista-de-Mods','1.1.0',self.packages('1.1.0')[:1])
        self.assertEqual(old,{p.name:p.read_bytes() for p in self.downloads.glob('*.zip')})

    def test_corrupt_archive_keeps_old_files(self):
        publish(self.downloads,'JOCAS-Lista-de-Mods','1.0.0',self.packages('1.0.0'))
        files=self.packages('1.1.0');files[1].write_bytes(b'corrupt')
        with self.assertRaises(zipfile.BadZipFile):publish(self.downloads,'JOCAS-Lista-de-Mods','1.1.0',files)
        self.assertEqual(len(list(self.downloads.glob('*-1.0.0-*.zip'))),3)

    def test_new_version_removes_only_its_own_old_release(self):
        publish(self.downloads,'JOCAS-Wallpapers','1.0.0',self.packages('1.0.0','JOCAS-Wallpapers'))
        publish(self.downloads,'JOCAS-Lista-de-Mods','1.0.0',self.packages('1.0.0'))
        publish(self.downloads,'JOCAS-Lista-de-Mods','1.1.0',self.packages('1.1.0'))
        self.assertEqual(len(list(self.downloads.glob('JOCAS-Lista-de-Mods-1.0.0-*.zip'))),0)
        self.assertEqual(len(list(self.downloads.glob('JOCAS-Lista-de-Mods-1.1.0-*.zip'))),3)
        self.assertEqual(len(list(self.downloads.glob('JOCAS-Wallpapers-1.0.0-*.zip'))),3)

    def test_downgrade_does_not_remove_newest_release(self):
        publish(self.downloads,'JOCAS-Lista-de-Mods','1.1.0',self.packages('1.1.0'))
        with self.assertRaises(ValueError):publish(self.downloads,'JOCAS-Lista-de-Mods','1.0.0',self.packages('1.0.0'))
        self.assertEqual(len(list(self.downloads.glob('JOCAS-Lista-de-Mods-1.1.0-*.zip'))),3)

if __name__=='__main__':unittest.main()
