#!/usr/bin/env python3
"""Exercise actual GUI + PipeWire with synthetic audio, never a hardware microphone."""
import json
import uuid
import os
from pathlib import Path
import shlex
import subprocess as sp
import sys
import tempfile
import time
import shutil
import numpy as np

ROOT = Path(__file__).resolve().parents[1]

def pactl(*args):
    return sp.check_output(['pactl', *args], text=True, timeout=8).strip()

modules, processes, files = [], [], []
prefix = 'gvtest_' + uuid.uuid4().hex[:10]
default_input = pactl('get-default-source')
default_output = pactl('get-default-sink')
if any(s['name'] == 'jocas_integration_mic' for s in json.loads(pactl('-f', 'json', 'list', 'sources'))):
    raise RuntimeError('Feche o Gepetas antes de executar o teste de integração.')

def load(*args):
    module = pactl('load-module', *args)
    modules.insert(0, module)
    return module

def capture(device, path):
    f = open(path, 'wb'); files.append(f)
    p = sp.Popen(['parec', '--client-name=JOCAS-test-recorder', '--property=node.dont-reconnect=true', '--property=node.dont-fallback=true', '--device='+device, '--raw', '--format=float32le', '--rate=48000', '--channels=1', '--latency-msec=20'], stdout=f)
    processes.append(p)
    return time.monotonic()

def segment(path, start, end):
    data = np.fromfile(path, dtype='<f4')
    chunk = data[int(start*48000):int(end*48000)]
    assert len(chunk) > 24000, f'Missing audio: {path}'
    return chunk

def rms(chunk):
    return float(np.sqrt(np.mean(chunk**2)))

with tempfile.TemporaryDirectory(prefix='gepetas-test-') as td:
    temp = Path(td)
    try:
        flags = shlex.split(sp.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'libpulse', 'rubberband'], text=True))
        sp.run(['g++', '-std=c++17', '-O2', '-fPIC', str(ROOT/'tests/window_test.cpp'), '-o', str(temp/'window-test'), *flags, '-pthread'], check=True)
        for suffix in ('input', 'listen'):
            load('module-null-sink', f'sink_name={prefix}_{suffix}', 'rate=48000', 'channels=1', 'channel_map=mono',
                 'sink_properties=device.description=GepetasTest priority.session=1')
        load('module-remap-source', f'master={prefix}_input.monitor', f'source_name={prefix}_source',
             'channels=1', 'channel_map=mono', 'master_channel_map=mono',
             'source_properties=device.description=GepetasTestInput priority.session=1')
        pactl('set-source-volume', prefix+'_source', '100%')
        pactl('set-source-mute', prefix+'_source', '0')
        tone = (.2*np.sin(2*np.pi*220*np.arange(48000*20)/48000)).astype('<f4')
        tone.tofile(temp/'tone.raw')
        tonefile = open(temp/'tone.raw', 'rb'); files.append(tonefile)
        player = sp.Popen(['pacat', '--client-name=JOCAS-test-tone', '--stream-name=JOCAS-synthetic-tone', '--volume=65536', '--property=node.dont-reconnect=true', '--playback', f'--device={prefix}_input', '--raw', '--format=float32le', '--rate=48000', '--channels=1', '--latency-msec=20'], stdin=tonefile)
        processes.append(player)
        listen_start = capture(prefix+'_listen.monitor', temp/'listen.raw')
        runtime = temp/'runtime'; runtime.mkdir(mode=0o700)
        server = os.environ.get('PULSE_SERVER', 'unix:'+os.environ.get('XDG_RUNTIME_DIR', f'/run/user/{os.getuid()}')+'/pulse/native')
        env = {**os.environ, 'QT_QPA_PLATFORM':'offscreen', 'XDG_CONFIG_HOME':str(temp/'config'),
               'XDG_RUNTIME_DIR':str(runtime), 'PULSE_SERVER':server}
        log = open(temp/'window.log', 'w+'); files.append(log)
        begin = time.monotonic()
        gui = sp.Popen([str(temp/'window-test'), prefix+'_source', prefix+'_listen'], env=env, stderr=log)
        processes.append(gui)
        for _ in range(80):
            if any(s['name'] == 'jocas_integration_mic' for s in json.loads(pactl('-f', 'json', 'list', 'sources'))): break
            if gui.poll() is not None:
                log.seek(0); print(log.read(), flush=True)
                raise RuntimeError(f'GUI exited before creating source (code {gui.returncode})')
            time.sleep(.1)
        else: raise RuntimeError('Virtual microphone missing')
        virtual = next(s for s in json.loads(pactl('-f', 'json', 'list', 'sources')) if s['name'] == 'jocas_integration_mic')
        assert virtual['description'] == 'JOCAS Microfone', 'Virtual microphone display name incorrect'
        virtual_start = capture('jocas_integration_mic', temp/'virtual.raw')
        # PipeWire may restore remap-source volume when the first capture stream
        # activates, after module creation. Normalize this synthetic fixture only.
        time.sleep(max(0, 1.0 - (time.monotonic()-begin)))
        pactl('set-source-volume', prefix+'_source', '100%')
        pactl('set-source-mute', prefix+'_source', '0')
        if os.environ.get('JOCAS_TEST_LEVELS'):
            for kind in ('sink-inputs', 'source-outputs', 'sinks', 'sources'):
                for stream in json.loads(pactl('-f', 'json', 'list', kind)):
                    name = stream.get('name') or stream.get('properties', {}).get('application.name', '')
                    if any(part in name.lower() for part in ('jocas', 'gvtest', 'gepetas')):
                        print(kind, name, stream.get('volume'), stream.get('mute'), flush=True)
        time.sleep(max(0, 11.7 - (time.monotonic()-begin)))
        for recorder in processes:
            if recorder.args[0] == 'parec' and recorder.poll() is None:
                recorder.terminate(); recorder.wait(timeout=3)
        result = gui.wait(timeout=20)
        for p in processes:
            if p.poll() is None: p.terminate()
        for p in processes:
            try: p.wait(timeout=3)
            except sp.TimeoutExpired: p.kill(); p.wait()
        for f in files: f.flush()
        log.seek(0); logtext = log.read(); print(logtext)
        assert result == 0, 'GUI integration failed'
        assert 'CLEANUP OK' in logtext, 'Missing cleanup confirmation'
        metrics = {}
        for phase, a, b in [('off_before',1.5,2.5), ('on',4.5,5.5), ('off_after',7.5,8.5), ('muted',10.5,11.5)]:
            voice = segment(temp/'virtual.raw', a-(virtual_start-begin), b-(virtual_start-begin))
            monitor = segment(temp/'listen.raw', a-(listen_start-begin), b-(listen_start-begin))
            spectrum = abs(np.fft.rfft(voice * np.hanning(len(voice))))
            freq = float(np.fft.rfftfreq(len(voice), 1/48000)[np.argmax(spectrum)])
            metrics[phase] = {'virtual_rms':rms(voice), 'listen_rms':rms(monitor), 'frequency_hz':freq}
            print(phase, metrics[phase], flush=True)
            if phase != 'muted':
                assert rms(voice) > .02, f'Virtual mic lost audio during {phase}'
                assert abs(freq-220*2**(-5/12)) < 4, f'Incorrect deep pitch: {freq}'
                frequencies = np.fft.rfftfreq(len(voice), 1/48000)
                original = float(spectrum[abs(frequencies-220)<2].max())
                assert original/max(float(spectrum.max()),1e-12) < .01, 'Original tone leaked into virtual mic'
            else: assert rms(voice) < .0001, 'Mute leaked audio'
            if phase == 'on': assert rms(monitor) > .01, 'Monitoring did not activate'
            else: assert rms(monitor) < .0001, f'Monitor leaked audio during {phase}'
        assert pactl('get-default-source') == default_input, 'Default input changed'
        assert pactl('get-default-sink') == default_output, 'Default output changed'
        print(json.dumps(metrics, indent=2))
        print('PASS: pitch, virtual input, independent monitor, mute, cleanup, unchanged defaults.')
        if len(sys.argv) > 1: Path(sys.argv[1]).write_text(json.dumps({'passed':True, 'metrics':metrics}, indent=2)+'\n')
    finally:
        if 'log' in locals() and not log.closed:
            log.seek(0); print(log.read(), flush=True)
        for p in processes:
            if p.poll() is None: p.terminate()
        for p in processes:
            try: p.wait(timeout=3)
            except sp.TimeoutExpired: p.kill(); p.wait()
        for f in files: f.close()
        if os.environ.get('JOCAS_TEST_ARTIFACTS'):
            shutil.copytree(temp, os.environ['JOCAS_TEST_ARTIFACTS'], dirs_exist_ok=True)
        for module in modules:
            try: pactl('unload-module', module)
            except sp.SubprocessError: pass
