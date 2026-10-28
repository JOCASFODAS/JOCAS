"""API contract and privacy tests; no key, account, microphone or live API used."""
import base64
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import urllib.error
import wave

spec = importlib.util.spec_from_file_location('worker', Path(__file__).resolve().parents[1]/'ai/worker.py')
w = importlib.util.module_from_spec(spec); spec.loader.exec_module(w)

class Contract(unittest.TestCase):
    def test_exact_transcript_to_tts_no_chat_and_custom_voice(self):
        calls = []
        def request(path, data, content_type):
            calls.append((path, data, content_type))
            if path == 'transcriptions':
                self.assertIn(b'name="file"', data); self.assertIn(b'RIFF', data)
                self.assertIn(b'gpt-4o-mini-transcribe', data)
                return json.dumps({'text':'Olá, vamos jogar juntos?'}).encode()
            payload = json.loads(data)
            self.assertEqual(payload['input'], 'Olá, vamos jogar juntos?')
            self.assertEqual(payload['voice'], {'id':'voice_meuteste123'})
            self.assertEqual(payload['model'], 'gpt-4o-mini-tts')
            self.assertEqual(payload['response_format'], 'pcm')
            return b'\x01\x02'*2400
        with patch.object(w, 'request_api', request):
            engine = w.OnlineVoice({'onlineConsent':True,'customVoiceId':'voice_meuteste123','style':'Voz grave, clara.'})
            pcm, rate = engine.repeat(b'\x00\x00'*16000)
        self.assertEqual([x[0] for x in calls], ['transcriptions', 'speech'])
        self.assertEqual(rate,24000); self.assertEqual(len(pcm),4800)

    def test_consent_and_unavailable_voice(self):
        with self.assertRaises(ValueError): w.OnlineVoice({})
        with self.assertRaises(ValueError): w.OnlineVoice({'onlineConsent':True,'apiVoice':'Rio'})
        for voice in w.VOICES:
            self.assertEqual(w.OnlineVoice({'onlineConsent':True,'apiVoice':voice}).voice,voice)

    def test_empty_transcript_produces_silence(self):
        with patch.object(w,'request_api',return_value=b'{"text":""}') as request:
            self.assertEqual(w.OnlineVoice({'onlineConsent':True}).repeat(b'\x00\x00'*4000),(b'',24000))
            self.assertEqual(request.call_count,1)

    def test_api_errors_never_echo_key_or_provider_body(self):
        for code in (400,401,403,404,429,500):
            error=urllib.error.HTTPError('https://api.openai.com',code,'SECRET_KEY',{},io.BytesIO(b'SECRET_PHRASE'))
            with patch.dict(w.os.environ,{'JOCAS_OPENAI_API_KEY':'SECRET_KEY'}), patch.object(w.urllib.request,'urlopen',side_effect=error):
                with self.assertRaises(ValueError) as e: w.request_api('speech',b'{}','application/json')
                self.assertNotIn('SECRET',str(e.exception))
            error.close()

    def test_download_integrity_and_existing_model_preserved(self):
        import hashlib
        item={'name':'voice.onnx','url':'https://example.invalid/model','sha256':hashlib.sha256(b'good').hexdigest()}
        with tempfile.TemporaryDirectory() as d, patch.object(w,'emit'):
            p=Path(d)/'voice.onnx';p.write_bytes(b'previous')
            with patch.object(w.urllib.request,'urlopen',return_value=io.BytesIO(b'corrupt')):
                with self.assertRaises(ValueError):w.download_file(item,Path(d))
            self.assertEqual(p.read_bytes(),b'previous');self.assertFalse(Path(str(p)+'.part').exists())
            with patch.object(w.urllib.request,'urlopen',return_value=io.BytesIO(b'good')):w.download_file(item,Path(d))
            with patch.object(w.urllib.request,'urlopen',side_effect=AssertionError('Unexpected network')):w.download_file(item,Path(d))

    def test_epoch_survives_child_protocol(self):
        class FakeVoice:
            def __init__(self,c):pass
            def repeat(self,pcm):return b'\x01\x02'*1000,24000
        data=json.dumps({'mode':'api'})+'\n'+json.dumps({'epoch':27,'pcm':base64.b64encode(b'\x00\x00'*4000).decode()})+'\n'
        class Input:
            buffer=io.BytesIO(data.encode())
        out=io.StringIO()
        with patch.object(w,'OnlineVoice',FakeVoice),patch.object(w.sys,'stdin',Input()),patch.object(w.sys,'stdout',out):w.serve()
        lines=[json.loads(s) for s in out.getvalue().splitlines()]
        self.assertEqual(lines[0]['type'],'ready');self.assertEqual(lines[1]['epoch'],27)
        self.assertEqual(lines[1]['type'],'audio')

if __name__=='__main__':unittest.main()
