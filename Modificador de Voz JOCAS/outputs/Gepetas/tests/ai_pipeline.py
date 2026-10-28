"""Actual local speech -> native virtual mic + monitoring. Synthetic speech only."""
import json,os,shlex,subprocess as sp,sys,tempfile,time,uuid
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'ai'))
from worker import LocalVoice,wav_bytes
from unittest.mock import patch
RUNTIME=Path(os.environ.get('JOCAS_AI_RUNTIME', str(Path.home()/'.config/Gepetas/ia')))

def pactl(*a):return sp.check_output(['pactl',*a],text=True,timeout=8).strip()
def rms(x):return float(np.sqrt(np.mean(x*x))) if len(x) else 0.
modules=[];processes=[];files=[]
source=pactl('get-default-source');sink=pactl('get-default-sink');prefix='jocas_ai_'+uuid.uuid4().hex[:8]
with tempfile.TemporaryDirectory(prefix='jocas-ai-test-') as td:
 d=Path(td)
 try:
  flags=shlex.split(sp.check_output(['pkg-config','--cflags','--libs','Qt6Core','libpulse','rubberband'],text=True))
  sp.run(['g++','-std=c++17','-O2','-fPIC',str(ROOT/'tests/ai_pipeline_test.cpp'),'-o',str(d/'pipeline'),*flags,'-pthread'],check=True)
  config={'recognizer':str(RUNTIME/'models/recognizer/base'),'voicePath':str(RUNTIME/'models/voices/cadu/pt_BR-cadu-medium.onnx')}
  engine=LocalVoice(config)
  speech,rate=engine.synthesize('Olá pessoal, esta é a minha nova voz. Vamos jogar juntos hoje.')
  pcm=np.frombuffer(speech,dtype='<i2');samples=np.interp(np.arange(int(len(pcm)*48000/rate))*rate/48000,np.arange(len(pcm)),pcm).astype(np.float32)/32768
  # Test offline inference while blocking HTTP in this process.
  small=np.interp(np.arange(int(len(pcm)*16000/rate))*rate/16000,np.arange(len(pcm)),pcm).astype('<i2')
  with patch('urllib.request.urlopen',side_effect=AssertionError('Network forbidden in offline test')):
   begin=time.monotonic();generated,generated_rate=engine.repeat(small.tobytes());elapsed=time.monotonic()-begin
  assert len(generated)>generated_rate,'Local engine did not repeat speech'
  segments,_=engine.stt.transcribe(small.astype(np.float32)/32768,language='pt',beam_size=3,vad_filter=True)
  transcript=' '.join(s.text.strip() for s in segments)
  assert all(word in transcript.lower() for word in ['pessoal','voz','jogar','juntos']),transcript
  del engine
  for part in ['input','listen']:
   modules.insert(0,pactl('load-module','module-null-sink','sink_name='+prefix+'_'+part,'rate=48000','channels=1','channel_map=mono','sink_properties=device.description=JocasAISynthetic priority.session=1'))
  modules.insert(0,pactl('load-module','module-remap-source','master='+prefix+'_input.monitor','source_name='+prefix+'_source','channels=1','channel_map=mono','master_channel_map=mono','source_properties=device.description=JocasAISyntheticInput priority.session=1'))
  pactl('set-source-volume',prefix+'_source','100%');pactl('set-source-mute',prefix+'_source','0')
  log=open(d/'native.log','w+');files.append(log)
  native=sp.Popen([str(d/'pipeline'),prefix+'_source',prefix+'_listen',sys.executable,str(ROOT/'ai/worker.py'),str(RUNTIME)],stderr=log,stdout=log);processes.append(native)
  for i in range(150):
   log.flush();log.seek(0);text=log.read()
   if 'IA pronta.' in text:break
   if native.poll() is not None:raise RuntimeError(text)
   time.sleep(.1)
  else:raise RuntimeError('AI startup timed out: '+text)
  starts={}
  for name,device in [('virtual','jocas_ai_test_mic'),('listen',prefix+'_listen.monitor')]:
   f=open(d/(name+'.raw'),'wb');files.append(f)
   starts[name]=time.monotonic()
   processes.append(sp.Popen(['parec','--device='+device,'--raw','--format=float32le','--rate=48000','--channels=1','--latency-msec=20'],stdout=f))
  time.sleep(.5);pactl('set-source-volume','jocas_ai_test_mic','100%')
  data=np.concatenate([np.zeros(48000,dtype=np.float32),samples,np.zeros(48000*10,dtype=np.float32)]).astype('<f4');data.tofile(d/'input.raw')
  f=open(d/'input.raw','rb');files.append(f);start=time.monotonic()
  player=sp.Popen(['pacat','--device='+prefix+'_input','--raw','--format=float32le','--rate=48000','--channels=1','--latency-msec=20','--volume=65536'],stdin=f);processes.append(player)
  player.wait(timeout=22);time.sleep(.3)
  for p in processes[1:]:
   if p.poll() is None:p.terminate();p.wait(timeout=4)
  for f in files:f.flush()
  metrics={'local_repeat_seconds':round(elapsed,3),'synthetic_transcript':transcript,'input_duration_seconds':len(samples)/48000}
  for name in ['virtual','listen']:
   x=np.fromfile(d/(name+'.raw'),dtype='<f4');offset=start-starts[name]
   raw_phase=x[int((offset+1.3)*48000):int((offset+len(samples)/48000+.6)*48000)]
   response=x[int((offset+len(samples)/48000+1.8)*48000):]
   metrics[name]={'during_original_rms':rms(raw_phase),'generated_rms':rms(response),'peak':float(abs(response).max())}
   assert rms(raw_phase)<.0001,('Original input leaked before synthesis',metrics)
   assert rms(response)>.015,('Generated voice missing',metrics)
  ratio=metrics['listen']['generated_rms']/metrics['virtual']['generated_rms'];assert .40<ratio<.60,ratio
  result=native.wait(timeout=12);assert result==0,result
  log.seek(0);print(log.read());assert pactl('get-default-source')==source and pactl('get-default-sink')==sink,'Audio defaults changed'
  metrics['passed']=True;print(json.dumps(metrics,ensure_ascii=False,indent=2))
  if len(sys.argv)>1:Path(sys.argv[1]).write_text(json.dumps(metrics,ensure_ascii=False,indent=2)+'\n')
 finally:
  for p in processes:
   if p.poll() is None:p.terminate()
  for p in processes:
   try:p.wait(timeout=3)
   except sp.TimeoutExpired:p.kill();p.wait()
  for f in files:f.close()
  for module in modules:
   try:pactl('unload-module',module)
   except sp.SubprocessError:pass
  # A crashed synthetic native test cannot leave its private fixtures behind.
  for m in json.loads(pactl('-f','json','list','modules')):
   if 'jocas_ai_test_' in m.get('argument',''):
    try:pactl('unload-module',str(m['index']))
    except sp.SubprocessError:pass
