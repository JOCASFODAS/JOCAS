"""Synthetic inference process for native mute/failure tests, no model dependencies."""
import base64,json,math,struct,sys,time
json.loads(sys.stdin.readline())
print(json.dumps({'type':'ready','message':'fixture ready'}),flush=True)
for line in sys.stdin:
    o=json.loads(line)
    time.sleep(.25)
    audio=b''.join(struct.pack('<h',int(.25*32767*math.sin(2*math.pi*880*i/24000))) for i in range(12000))
    print(json.dumps({'type':'audio','epoch':o['epoch'],'rate':24000,'pcm':base64.b64encode(audio).decode()}),flush=True)
