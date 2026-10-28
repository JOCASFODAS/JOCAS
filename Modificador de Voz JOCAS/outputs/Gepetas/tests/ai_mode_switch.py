"""Regression: click AI mode during active synthetic capture; retain silent device."""
from pathlib import Path
import json,os,shlex,subprocess as sp,sys,tempfile,time,uuid
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
def pactl(*a):return sp.check_output(['pactl',*a],text=True,timeout=8).strip()
def rms(a):return float(np.sqrt(np.mean(a*a)))
modules=[];processes=[];files=[];prefix='jocas_switch_'+uuid.uuid4().hex[:8]
default_source=pactl('get-default-source');default_sink=pactl('get-default-sink')
with tempfile.TemporaryDirectory(prefix='jocas-switch-') as td:
 d=Path(td)
 try:
  flags=shlex.split(sp.check_output(['pkg-config','--cflags','--libs','Qt6Widgets','libpulse','rubberband'],text=True))
  sp.run(['g++','-std=c++17','-O2','-fPIC',str(ROOT/'tests/ai_mode_switch_test.cpp'),'-o',str(d/'test'),*flags,'-pthread'],check=True)
  for part in ['input','listen']:
   modules.insert(0,pactl('load-module','module-null-sink','sink_name='+prefix+'_'+part,'rate=48000','channels=1','channel_map=mono','sink_properties=device.description=JocasSwitchSynthetic priority.session=1'))
  modules.insert(0,pactl('load-module','module-remap-source','master='+prefix+'_input.monitor','source_name='+prefix+'_source','channels=1','channel_map=mono','master_channel_map=mono','source_properties=device.description=JocasSwitchInput priority.session=1'))
  pactl('set-source-volume',prefix+'_source','100%');pactl('set-source-mute',prefix+'_source','0')
  (.2*np.sin(2*np.pi*220*np.arange(48000*13)/48000)).astype('<f4').tofile(d/'input.raw')
  f=open(d/'input.raw','rb');files.append(f);processes.append(sp.Popen(['pacat','--device='+prefix+'_input','--format=float32le','--rate=48000','--channels=1','--raw','--latency-msec=20','--volume=65536'],stdin=f))
  runtime=d/'runtime';runtime.mkdir(mode=0o700)
  env={**os.environ,'QT_QPA_PLATFORM':'offscreen','XDG_CONFIG_HOME':str(d/'config'),'XDG_RUNTIME_DIR':str(runtime),'PULSE_SERVER':os.environ.get('PULSE_SERVER','unix:'+os.environ.get('XDG_RUNTIME_DIR','/run/user/'+str(os.getuid()))+'/pulse/native')}
  log=open(d/'test.log','w+');files.append(log);begin=time.monotonic()
  native=sp.Popen([str(d/'test'),prefix+'_source',prefix+'_listen'],env=env,stdout=log,stderr=log);processes.append(native)
  for i in range(80):
   if any(s['name']=='jocas_switch_test_mic' for s in json.loads(pactl('-f','json','list','sources'))):break
   if native.poll() is not None:raise RuntimeError('Window failed before capture')
   time.sleep(.05)
  else:raise RuntimeError('Missing virtual input')
  f=open(d/'capture.raw','wb');files.append(f);capture_start=time.monotonic()
  rec=sp.Popen(['parec','--device=jocas_switch_test_mic','--raw','--format=float32le','--rate=48000','--channels=1','--latency-msec=20'],stdout=f);processes.append(rec)
  time.sleep(max(0,1-(time.monotonic()-begin)));pactl('set-source-volume','jocas_switch_test_mic','100%')
  result=native.wait(timeout=16);rec.terminate();rec.wait(timeout=3)
  for f in files:f.flush()
  log.seek(0);text=log.read();print(text);assert result==0,text
  x=np.fromfile(d/'capture.raw',dtype='<f4');metrics={}
  for name,a,b in [('live',1.1,1.7),('paused',3.5,5.6),('resumed',7,8)]:
   offset=capture_start-begin;chunk=x[int((a-offset)*48000):int((b-offset)*48000)];assert len(chunk)>20000
   metrics[name]=rms(chunk)
   assert metrics[name]<.0001 if name=='paused' else metrics[name]>.02,metrics
  assert not any(s['name']=='jocas_switch_test_mic' for s in json.loads(pactl('-f','json','list','sources'))),'Virtual device not cleaned up on close'
  assert pactl('get-default-source')==default_source and pactl('get-default-sink')==default_sink,'Default devices changed'
  metrics['passed']=True;print(json.dumps(metrics,indent=2))
  if len(sys.argv)>1:Path(sys.argv[1]).write_text(json.dumps(metrics,indent=2)+'\n')
 finally:
  for p in processes:
   if p.poll() is None:p.terminate()
  for p in processes:
   try:p.wait(timeout=3)
   except sp.TimeoutExpired:p.kill();p.wait()
  for f in files:f.close()
  for m in json.loads(pactl('-f','json','list','modules')):
   if 'jocas_switch_test_' in m.get('argument',''):
    try:pactl('unload-module',str(m['index']))
    except sp.SubprocessError:pass
  for module in modules:
   try:pactl('unload-module',module)
   except sp.SubprocessError:pass
