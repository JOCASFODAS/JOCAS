"""Exercise complete bus -> remap -> two changing game/Discord-like consumers.

Synthetic devices only; defaults are never changed. The real microphone is not read.
Unlike monitor_continuity.py, records ONLY the exported source, not the internal
bus monitor, so the observer cannot change the scheduling of that bus directly.
"""
from pathlib import Path
import json,os,shlex,signal,subprocess as sp,tempfile,time,uuid
import numpy as np
root=Path(__file__).resolve().parents[1]
modules=[];processes=[];files=[]
def pactl(*a):return sp.check_output(['pactl',*a],text=True,timeout=8).strip()
def spawn(args,**kw):
 p=sp.Popen(args,**kw);processes.append(p);return p
def file(p,mode):
 f=open(p,mode);files.append(f);return f
original_source=pactl('get-default-source');original_sink=pactl('get-default-sink')
metadata_before=sp.check_output(['pw-metadata','-n','settings'],text=True)
assert "key:'clock.force-quantum' value:'0'" in metadata_before,'Clear temporary global overrides before this test'
prefix='jocas_graph_'+uuid.uuid4().hex[:8]
with tempfile.TemporaryDirectory(prefix='jocas-graph-') as td:
 d=Path(td)
 try:
  flags=shlex.split(sp.check_output(['pkg-config','--cflags','--libs','Qt6Core','libpulse','rubberband'],text=True))
  sp.run(['g++','-std=c++17','-O2','-fPIC',str(root/'tests/game_graph_test.cpp'),'-o',str(d/'fixture'),*flags,'-pthread'],check=True)
  for part in ('input','listen'):
   modules.insert(0,pactl('load-module','module-null-sink','sink_name='+prefix+'_'+part,'rate=48000','channels=1','channel_map=mono','sink_properties=device.description=JocasGraphSynthetic priority.session=1'))
  (.2*np.sin(2*np.pi*220*np.arange(48000*46)/48000)).astype('<f4').tofile(d/'tone.raw')
  spawn(['pacat','--client-name=JOCAS-graph-tone','--volume=65536','--property=node.dont-reconnect=true','--playback','--device='+prefix+'_input','--raw','--format=float32le','--rate=48000','--channels=1','--latency-msec=500'],stdin=file(d/'tone.raw','rb'))
  def record(source,output,latency):
   return spawn(['parec','--client-name=JOCAS-graph-recorder','--property=node.dont-reconnect=true','--property=node.dont-fallback=true','--device='+source,'--raw','--format=float32le','--rate=48000','--channels=1','--latency-msec='+str(latency)],stdout=output)
  reference=file(d/'reference.raw','wb');record(prefix+'_input.monitor',reference,100)
  log=file(d/'engine.log','w+')
  engine=spawn([str(d/'fixture'),prefix+'_input.monitor',prefix+'_listen'],stderr=log)
  for _ in range(80):
   log.seek(0)
   if 'READY' in log.read():break
   assert engine.poll() is None,'Engine stopped before ready'
   time.sleep(.1)
  else:raise AssertionError('Engine startup timed out')
  nodes=json.loads(sp.check_output(['pw-dump'],text=True))
  guarded=[o['info']['props'] for o in nodes if o.get('info',{}).get('props',{}).get('application.name')=='JOCAS Graph Synthetic Test' and o.get('type','').endswith('Node')]
  assert len(guarded)>=2 and all(int(n.get('node.force-quantum',0))==1024 for n in guarded),'Active streams lack scoped protection'
  record('jocas_graph_test_mic',file(d/'sent.raw','wb'),5)
  record(prefix+'_listen.monitor',file(d/'heard.raw','wb'),20)
  # Add/remove the second consumer with different requested latencies, while
  # the 5 ms game-like consumer and the 20 ms monitor remain active.
  start=time.monotonic();extra=None;step=0
  while time.monotonic()-start<36:
   elapsed=time.monotonic()-start
   if elapsed>=step*4:
    if extra is not None:extra.terminate();extra.wait(timeout=3);extra=None
    if step%2==0:extra=record('jocas_graph_test_mic',sp.DEVNULL,(10,40,5)[step//2%3])
    step+=1
   assert engine.poll() is None,'Engine stopped under changing consumers'
   time.sleep(.1)
  engine.wait(timeout=8);assert engine.returncode==0
  for p in processes:
   if p.poll() is None:p.terminate();p.wait(timeout=3)
  for f in files:f.flush()
  def gaps(path):
   x=np.fromfile(path,dtype='<f4')[3*48000:33*48000]
   assert len(x)==30*48000,'Incomplete recording'
   assert np.sqrt(np.mean(x*x))>.02,'Silent recording'
   edges=np.diff(np.r_[False,np.abs(x)<.00001,False].astype(np.int8))
   lengths=np.flatnonzero(edges==-1)-np.flatnonzero(edges==1)
   return {'dropouts_over_2ms':int(np.sum(lengths>=96)),'longest_gap_ms':float(max(lengths,default=0)/48),'waveform_jumps_over_0_08':int(np.sum(np.abs(np.diff(x))>.08))}
  metrics={name:gaps(d/path) for name,path in [('reference','reference.raw'),('sent','sent.raw'),('heard','heard.raw')]}
  metrics['global_force_quantum']=0
  metrics['scoped_quantum']=1024
  metrics['measured_seconds']=30
  print(json.dumps(metrics,indent=2),flush=True)
  assert metrics['reference']['dropouts_over_2ms']==0,'Synthetic source invalid'
  for name in ('sent','heard'):
   assert metrics[name]['dropouts_over_2ms']==0,(name,'dropouts')
   assert metrics[name]['waveform_jumps_over_0_08']==0,(name,'truncated waveform')
  assert "key:'clock.force-quantum' value:'0'" in sp.check_output(['pw-metadata','-n','settings'],text=True),'Global override was changed'
  assert not any(o.get('info',{}).get('props',{}).get('application.name')=='JOCAS Graph Synthetic Test' for o in json.loads(sp.check_output(['pw-dump'],text=True))),'Worker protection not released after stop'
  assert pactl('get-default-source')==original_source and pactl('get-default-sink')==original_sink,'Default device changed'
 finally:
  for p in processes:
   if p.poll() is None:p.terminate()
  for p in processes:
   try:p.wait(timeout=3)
   except sp.TimeoutExpired:p.kill();p.wait()
  for f in files:f.close()
  for m in json.loads(pactl('-f','json','list','modules')):
   if 'gepetas.owner=jocas_graph_test-voz-v1' in m.get('argument',''):
    try:pactl('unload-module',str(m['index']))
    except sp.SubprocessError:pass
  for m in modules:
   try:pactl('unload-module',m)
   except sp.SubprocessError:pass
