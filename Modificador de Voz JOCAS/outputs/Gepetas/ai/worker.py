# SPDX-License-Identifier: GPL-3.0-or-later
"""JOCAS speech relay. No microphone, recordings, key persistence or chat replies.

Protocol: UTF-8 JSON lines on stdin/stdout; PCM is mono signed 16-bit LE.
Models are loaded only from explicit local paths. Downloads are a separate command.
"""
import argparse
import array
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import urllib.error
import urllib.request
import wave

HERE = Path(__file__).resolve().parent
MAX_SECONDS = 13
VOICES = ('alloy', 'ash', 'ballad', 'coral', 'echo', 'fable', 'nova', 'onyx',
          'sage', 'shimmer', 'verse', 'marin', 'cedar')


def emit(**data):
    print(json.dumps(data, ensure_ascii=False), flush=True)


def download_file(item, directory):
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / item['name']
    if path.exists():
        with path.open('rb') as existing:
            if hashlib.file_digest(existing, 'sha256').hexdigest() == item['sha256']:
                return
    temp = path.with_name(path.name + '.part')
    emit(type='status', message='Baixando ' + item['name'] + '…')
    try:
        h = hashlib.sha256()
        with urllib.request.urlopen(item['url'], timeout=90) as response, temp.open('wb') as out:
            while chunk := response.read(1024 * 1024):
                h.update(chunk); out.write(chunk)
        if h.hexdigest() != item['sha256']:
            raise ValueError('O download não passou na conferência de integridade. Tente novamente.')
        temp.replace(path)
    finally:
        temp.unlink(missing_ok=True)


def download_models(root, voice, recognizer):
    catalog = json.loads((HERE / 'catalog.json').read_text())
    if recognizer:
        for item in catalog['recognizer']['base']['files']:
            download_file(item, root / 'models' / 'recognizer' / 'base')
    if voice:
        for item in catalog['voices'][voice]['files']:
            download_file(item, root / 'models' / 'voices' / voice)
    emit(type='ready', message='Download concluído e conferido. Pode usar sem internet.')


def wav_bytes(pcm, rate):
    out = io.BytesIO()
    with wave.open(out, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate); w.writeframes(pcm)
    return out.getvalue()


def request_api(path, payload, content_type):
    key = os.environ.get('JOCAS_OPENAI_API_KEY', '').strip()
    if not key:
        raise ValueError('Digite a chave da API nesta sessão.')
    req = urllib.request.Request('https://api.openai.com/v1/audio/' + path, data=payload,
        headers={'Authorization': 'Bearer ' + key, 'Content-Type': content_type}, method='POST')
    try:
        with urllib.request.urlopen(req, timeout=45) as r:
            data = r.read(12 * 1024 * 1024 + 1)
            if len(data) > 12 * 1024 * 1024:
                raise ValueError('A resposta da API ultrapassou o limite de áudio.')
            return data
    except urllib.error.HTTPError as e:
        # Never echo provider bodies, tokens or request payloads into the UI/logs.
        reason = {401: 'Chave inválida.', 403: 'Acesso negado.', 429: 'Limite ou saldo da API esgotado.',
                  400: 'Confira a voz e as opções da API.', 404: 'Modelo ou voz não disponível.'}.get(e.code, 'Serviço indisponível.')
        raise ValueError(f'API {e.code}: {reason}') from None
    except (urllib.error.URLError, TimeoutError):
        raise ValueError('A API não respondeu. Confira a internet e tente novamente.') from None


class OnlineVoice:
    def __init__(self, config):
        if not config.get('onlineConsent'):
            raise ValueError('Autorize o envio de áudio à API antes de iniciar.')
        voice = config.get('apiVoice', 'marin')
        custom = config.get('customVoiceId', '').strip()
        if custom:
            if not custom.startswith('voice_') or not custom.replace('_', '').isalnum() or len(custom) > 200:
                raise ValueError('Informe um ID de voz personalizado válido (voice_…).')
            self.voice = {'id': custom}
        elif voice in VOICES:
            self.voice = voice
        else:
            raise ValueError('Essa voz não está disponível na API de fala. Rio não é uma opção pública desta API.')
        self.style = config.get('style', '')[:600]
        self.speed = min(1.4, max(.7, float(config.get('speed', 1))))

    def repeat(self, pcm):
        boundary = 'jocas-' + os.urandom(12).hex()
        form = bytearray()
        for name, value in [('model', 'gpt-4o-mini-transcribe'), ('language', 'pt'), ('response_format', 'json')]:
            form.extend(f'--{boundary}\r\nContent-Disposition: form-data; name="{name}"\r\n\r\n{value}\r\n'.encode())
        form.extend(f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="speech.wav"\r\nContent-Type: audio/wav\r\n\r\n'.encode())
        form.extend(wav_bytes(pcm, 16000)); form.extend(f'\r\n--{boundary}--\r\n'.encode())
        result = json.loads(request_api('transcriptions', bytes(form), 'multipart/form-data; boundary=' + boundary))
        text = str(result.get('text', '')).strip()[:3000]
        if not text:
            return b'', 24000
        payload = {'model': 'gpt-4o-mini-tts', 'voice': self.voice, 'input': text,
                   'response_format': 'pcm', 'speed': self.speed}
        if self.style:
            payload['instructions'] = self.style
        # This is transcription -> speech, never a conversational model response.
        audio = request_api('speech', json.dumps(payload).encode(), 'application/json')
        return audio, 24000


class LocalVoice:
    def __init__(self, config):
        # Offline means no model discovery or automatic download during a session.
        os.environ['HF_HUB_OFFLINE'] = '1'
        os.environ['HF_HUB_DISABLE_TELEMETRY'] = '1'
        import onnxruntime
        onnxruntime.disable_telemetry_events()
        from faster_whisper import WhisperModel
        from piper import PiperVoice, SynthesisConfig
        stt = Path(config['recognizer'])
        for name in ('model.bin', 'config.json', 'tokenizer.json', 'vocabulary.txt'):
            if not (stt / name).is_file():
                raise ValueError('Baixe o reconhecimento ou selecione a pasta de um modelo Faster Whisper multilíngue.')
        voice = Path(config['voicePath'])
        if voice.suffix != '.onnx' or not voice.is_file() or not Path(str(voice) + '.json').is_file():
            raise ValueError('Adicione uma voz Piper .onnx acompanhada do arquivo .onnx.json.')
        cfg = json.loads(Path(str(voice) + '.json').read_text())
        if cfg.get('phoneme_type', 'espeak') not in ('espeak', 'text'):
            raise ValueError('Use uma voz Piper com fonemas eSpeak ou text, que funcione totalmente offline.')
        self.stt = WhisperModel(str(stt), device='cpu', compute_type='int8', cpu_threads=min(4, os.cpu_count() or 2),
                               num_workers=1, local_files_only=True)
        from piper.config import PiperConfig
        options = onnxruntime.SessionOptions()
        options.intra_op_num_threads = min(2, os.cpu_count() or 1)
        options.inter_op_num_threads = 1
        self.tts = PiperVoice(session=onnxruntime.InferenceSession(str(voice), sess_options=options,
                              providers=['CPUExecutionProvider']), config=PiperConfig.from_dict(cfg))
        self.tts.use_tashkeel = False  # No implicit auxiliary model downloads.
        speaker = int(config.get('speaker', 0))
        if speaker < 0 or speaker >= int(cfg.get('num_speakers', 1)):
            raise ValueError('Essa voz não contém o número de falante escolhido. Para voz simples, use 0.')
        self.syn = SynthesisConfig(speaker_id=speaker,
            length_scale=1 / min(1.4, max(.7, float(config.get('speed', 1)))),
            noise_scale=min(1, max(0, float(config.get('variation', .667)))))

    def synthesize(self, text):
        out = io.BytesIO()
        with wave.open(out, 'wb') as wav:
            self.tts.synthesize_wav(text, wav, syn_config=self.syn)
        with wave.open(io.BytesIO(out.getvalue()), 'rb') as wav:
            if wav.getnchannels() != 1 or wav.getsampwidth() != 2:
                raise ValueError('Formato de voz local inválido.')
            return wav.readframes(wav.getnframes()), wav.getframerate()

    def repeat(self, pcm):
        import numpy as np
        audio = np.frombuffer(pcm, dtype='<i2').astype(np.float32) / 32768
        segments, _ = self.stt.transcribe(audio, language='pt', beam_size=3, vad_filter=True,
            condition_on_previous_text=False, temperature=0, no_speech_threshold=.6)
        text = ' '.join(s.text.strip() for s in segments if s.no_speech_prob < .6).strip()[:3000]
        return self.synthesize(text) if text else (b'', 22050)


def serve():
    config_line = sys.stdin.buffer.readline(32768)
    config = json.loads(config_line)
    engine = OnlineVoice(config) if config.get('mode') == 'api' else LocalVoice(config)
    emit(type='ready', message='IA pronta. Fale e faça uma pausa para a voz repetir.')
    while line := sys.stdin.buffer.readline(600000):
        request = json.loads(line)
        epoch = request.get('epoch', 0)
        try:
            pcm = base64.b64decode(request['pcm'], validate=True)
            if len(pcm) % 2 or len(pcm) > 16000 * 2 * MAX_SECONDS:
                raise ValueError('Frase maior que o limite de áudio.')
            audio, rate = engine.repeat(pcm)
            if len(audio) % 2 or len(audio) > rate * 2 * 30:
                raise ValueError('Resposta de voz maior que o limite de 30 segundos.')
            emit(type='audio', epoch=epoch, rate=rate, pcm=base64.b64encode(audio).decode('ascii'))
        except Exception as e:
            emit(type='error', epoch=epoch, message=safe_error(e))


def safe_error(error):
    if isinstance(error, (ValueError, FileNotFoundError, ImportError)):
        return str(error)[:400]
    # Avoid leaking phrase text, credentials or paths from third-party exceptions.
    return 'Não foi possível processar a voz. Confira os modelos e tente novamente (' + type(error).__name__ + ').'


def main():
    p = argparse.ArgumentParser()
    p.add_argument('command', choices=['serve', 'download'])
    p.add_argument('--root', type=Path)
    p.add_argument('--voice', choices=['cadu', 'jeff'])
    p.add_argument('--recognizer', action='store_true')
    args = p.parse_args()
    try:
        if args.command == 'download':
            if not args.root:
                raise ValueError('Pasta de modelos não informada.')
            download_models(args.root, args.voice, args.recognizer)
        else:
            serve()
    except Exception as e:
        emit(type='fatal', message=safe_error(e)); return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
