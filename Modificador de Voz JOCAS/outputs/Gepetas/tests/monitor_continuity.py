#!/usr/bin/env python3
"""Measure monitor dropouts under 35 ms scheduling stalls; uses virtual devices only.

Usage: python3 tests/monitor_continuity.py [test_binary] [--allow-gaps]
Without a binary, compiles monitor_test.cpp against the current src/.
Set JOCAS_TEST_STALL_MS=150 to exercise game-sized stalls.
The synthetic source is recorded separately to reject invalid input fixtures.
Both silence gaps and waveform discontinuities are checked.
"""
import json
import uuid
import os
from pathlib import Path
import shlex
import signal
import subprocess as sp
import sys
import tempfile
import time
import numpy as np

root = Path(__file__).resolve().parents[1]
# Exercise game-sized scheduler stalls without touching other processes.
stall_ms = int(os.environ.get('JOCAS_TEST_STALL_MS', '35'))
assert 1 <= stall_ms <= 250
modules, processes, files = [], [], []
prefix = 'gvcontinuity_' + uuid.uuid4().hex[:10]

def pactl(*args):
    return sp.check_output(['pactl', *args], text=True, timeout=8).strip()

with tempfile.TemporaryDirectory(prefix='gepetas-continuity-') as td:
    tmp = Path(td)
    try:
        if len(sys.argv) > 1 and not sys.argv[1].startswith('--'):
            binary = str(Path(sys.argv[1]).resolve())
        else:
            binary = str(tmp/'monitor-test')
            flags = shlex.split(sp.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core', 'libpulse', 'rubberband'], text=True))
            sp.run(['g++', '-std=c++17', '-O2', '-fPIC', '-I'+str(root/'src'), str(root/'tests/monitor_test.cpp'), '-o', binary, *flags, '-pthread'], check=True)
        for name in ('input', 'listen', 'bus'):
            modules.insert(0, pactl('load-module', 'module-null-sink', f'sink_name={prefix}_{name}',
                                   'rate=48000', 'channels=1', 'channel_map=mono',
                                   'sink_properties=device.description=GepetasContinuityTest priority.session=1'))
        tone = (.2*np.sin(2*np.pi*220*np.arange(48000*20)/48000)).astype('<f4')
        tone.tofile(tmp/'tone.raw')
        tonefile = open(tmp/'tone.raw', 'rb'); files.append(tonefile)
        player = sp.Popen(['pacat', '--client-name=JOCAS-continuity-tone', '--volume=65536', '--property=node.dont-reconnect=true', '--playback', f'--device={prefix}_input', '--raw', '--format=float32le',
                           '--rate=48000', '--channels=1', '--latency-msec=500'], stdin=tonefile)
        processes.append(player)
        reference = open(tmp/'input.raw', 'wb'); files.append(reference)
        reference_recorder = sp.Popen(['parec', '--client-name=JOCAS-reference-recorder', '--property=node.dont-reconnect=true', '--property=node.dont-fallback=true', f'--device={prefix}_input.monitor', '--raw', '--format=float32le', '--rate=48000', '--channels=1', '--latency-msec=100'], stdout=reference)
        processes.append(reference_recorder)
        recording = open(tmp/'monitor.raw', 'wb'); files.append(recording)
        recorder = sp.Popen(['parec', '--client-name=JOCAS-continuity-recorder', '--property=node.dont-reconnect=true', '--property=node.dont-fallback=true', f'--device={prefix}_' + ('bus.monitor' if '--send' in sys.argv else 'listen.monitor'), '--raw', '--format=float32le',
                             '--rate=48000', '--channels=1', '--latency-msec=20'], stdout=recording)
        processes.append(recorder)
        log = open(tmp/'engine.log', 'w+'); files.append(log)
        engine = sp.Popen([binary, prefix+'_input.monitor', prefix+'_listen', prefix+'_bus'], stderr=log)
        processes.append(engine)
        begin = time.monotonic()
        time.sleep(2)
        # Deliberately stall only this test client, never PipeWire or other applications.
        while time.monotonic() - begin < 12:
            if engine.poll() is not None: raise RuntimeError('Engine stopped unexpectedly')
            os.kill(engine.pid, signal.SIGSTOP)
            time.sleep(stall_ms / 1000)
            os.kill(engine.pid, signal.SIGCONT)
            time.sleep(.45 if stall_ms > 35 else .165)
        code = engine.wait(timeout=8)
        recorder.terminate(); recorder.wait(timeout=3); recording.flush()
        reference_recorder.terminate(); reference_recorder.wait(timeout=3); reference.flush()
        log.seek(0); logtext = log.read()
        assert code == 0 and 'READY' in logtext, logtext
        data = np.fromfile(tmp/'monitor.raw', dtype='<f4')[3*48000:11*48000]
        assert len(data) == 8*48000, 'Incomplete recording'
        assert np.sqrt(np.mean(data**2)) > .02, 'Monitor silent'
        refdata = np.fromfile(tmp/'input.raw', dtype='<f4')[3*48000:11*48000]
        assert len(refdata) == 8*48000, 'Incomplete reference recording'
        refedges = np.diff(np.r_[False, np.abs(refdata) < .00001, False].astype(np.int8))
        reflengths = np.flatnonzero(refedges == -1) - np.flatnonzero(refedges == 1)
        reference_gaps = int(np.sum(reflengths >= 96))
        assert reference_gaps == 0, 'Synthetic input itself had gaps; fixture invalid under system load'
        quiet = np.abs(data) < .00001
        edges = np.diff(np.r_[False, quiet, False].astype(np.int8))
        lengths = np.flatnonzero(edges == -1) - np.flatnonzero(edges == 1)
        dropouts = lengths[lengths >= 96]  # Ignore sine zero crossings; count gaps >= 2 ms.
        metrics = {'dropouts_over_2ms':len(dropouts), 'longest_gap_ms':float(max(lengths, default=0)/48),
                   'silent_gap_total_ms':float(sum(dropouts)/48), 'measured_seconds':8,
                   'waveform_jumps_over_0_08':int(np.sum(np.abs(np.diff(data)) > .08)),
                   'injected_stall_ms':stall_ms, 'reference_dropouts':reference_gaps,
                   'worker_scheduling':next((line for line in logtext.splitlines() if line.startswith('READY')), 'unknown'), 'path':'send' if '--send' in sys.argv else 'monitor'}
        print(json.dumps(metrics, indent=2), flush=True)
        if '--allow-gaps' not in sys.argv:
            assert not len(dropouts), 'Audio underruns under scheduling jitter'
            assert metrics['waveform_jumps_over_0_08'] == 0, 'Audio waveform truncated during catch-up'
    finally:
        for p in processes:
            if p.poll() is None:
                p.send_signal(signal.SIGCONT); p.terminate()
        for p in processes:
            try: p.wait(timeout=3)
            except sp.TimeoutExpired: p.kill(); p.wait()
        for f in files: f.close()
        for module in modules:
            try: pactl('unload-module', module)
            except sp.SubprocessError: pass
