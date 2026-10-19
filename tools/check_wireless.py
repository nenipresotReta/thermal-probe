import json,socket,struct,sys,time,urllib.request
host=sys.argv[1]
def recvn(s,n):
    b=b''
    while len(b)<n:
        x=s.recv(n-len(b))
        if not x: raise RuntimeError('connection closed')
        b+=x
    return b
with socket.create_connection((host,502),3) as s:
    s.settimeout(3)
    for tid in range(1,4):
        q=struct.pack('>HHHBBHH',tid,0,6,1,3,0,8)
        # TCP packet boundaries must not matter.
        s.sendall(q[:5]);time.sleep(.02);s.sendall(q[5:])
        h=recvn(s,7);transaction,protocol,n,unit=struct.unpack('>HHHB',h)
        body=recvn(s,n-1)
        assert transaction==tid and protocol==0 and unit==1
        if body[0]&128: print('Modbus exception:',body[1])
        else:
            assert body[:2]==bytes([3,16]);print(dict(zip(['raw_c','smooth_c','d1_c_per_s','d2_c_per_s2'],struct.unpack('>ffff',body[2:]))))
        time.sleep(.5)
with urllib.request.urlopen('http://'+host+'/api/temperature',timeout=4) as r:
    data=json.load(r);print('HTTP:',data);assert 'valid' in data
