"""On-device concurrency regression; leaves room for one Artisan connection."""
import concurrent.futures,json,math,socket,statistics,struct,sys,time,urllib.request
if len(sys.argv)<2: raise SystemExit('Usage: check_multiclient.py DEVICE_IP [--stress]')
host=sys.argv[1]
stress='--stress' in sys.argv
def recvn(s,n):
    result=b''
    while len(result)<n:
        chunk=s.recv(n-len(result))
        if not chunk:raise RuntimeError('unexpected disconnect')
        result+=chunk
    return result

def request(tid):return struct.pack('>HHHBBHH',tid,0,6,1,3,0,8)
def reply(s,tid):
    h=recvn(s,7);transaction,protocol,n,unit=struct.unpack('>HHHB',h)
    assert (transaction,protocol,unit)==(tid,0,1),(transaction,tid)
    b=recvn(s,n-1);assert b[:2]==bytes([3,16]),b
    data=struct.unpack('>ffff',b[2:]);assert all(math.isfinite(v) for v in data)
    assert -200<data[0]<850
    return data

def reader(base):
    lat=[]
    with socket.create_connection((host,502),2) as s:
        s.settimeout(2)
        # Fragmented input and two requests in a single TCP write.
        q=request(base);s.sendall(q[:5]);time.sleep(.02);s.sendall(q[5:]);reply(s,base)
        s.sendall(request(base+1)+request(base+2));reply(s,base+1);reply(s,base+2)
        deadline=time.monotonic()
        for i in range(60):
            start=time.monotonic();s.sendall(request(base+3+i));data=reply(s,base+3+i)
            lat.append((time.monotonic()-start)*1000)
            deadline+=.1;time.sleep(max(0,deadline-time.monotonic()))
    return {'requests':63,'median_ms':round(statistics.median(lat),2),'p95_ms':round(sorted(lat)[int(.95*(len(lat)-1))],2),'max_ms':round(max(lat),2),'latest_c':round(data[1],3)}

def http_reader():
    out=[]
    for i in range(12 if stress else 1):
        with socket.create_connection((host,80),2) as s:
            s.settimeout(2)
            # HTTP, like Modbus, is a byte stream; no complete line is required
            # in the first TCP packet.
            s.sendall(b'GET /api/')
            time.sleep(.02)
            s.sendall(b'temperature HTTP/1.1\r\nHost: '+host.encode()+b'\r\nConnection: close\r\n\r\n')
            response=b''
            while True:
                chunk=s.recv(4096)
                if not chunk:break
                response+=chunk
        header,body=response.split(b'\r\n\r\n',1)
        assert header.startswith(b'HTTP/1.1 200 OK')
        try:d=json.loads(body)
        except (ValueError,UnicodeError) as error:
            raise AssertionError(f'Invalid HTTP JSON payload: {body!r}') from error
        assert d['valid'] and d['derivatives_ready'],d
        for field in ('time_s','raw_c','temperature_c','d1_c_per_s','d2_c_per_s2'):
            assert isinstance(d[field],(int,float)) and math.isfinite(d[field]),(field,body)
        if out:assert d['time_s']>=out[-1],('sample clock moved backwards',body)
        out.append(d['time_s'])
        time.sleep(.1)
    if len(out)>1:assert out[-1]>out[0]
    return {'valid_responses':len(out),'sample_time_advanced_s':round(out[-1]-out[0],3)}

# An incomplete MBAP header must only occupy its own worker.
idle=socket.create_connection((host,502),2);idle.settimeout(8);idle.sendall(b'\x00\x01\x00')
# A stalled HTTP client must not prevent a second HTTP request being served.
slow_http=socket.create_connection((host,80),2);slow_http.settimeout(8)
try:
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        fs=[pool.submit(reader,0x1000),pool.submit(reader,0x2000),pool.submit(http_reader)]
        results=[]
        for f in fs:
            result=f.result();results.append(result);print(json.dumps(result),flush=True)
    assert idle.recv(1)==b'', 'incomplete Modbus frame not closed after timeout'
    assert slow_http.recv(1)==b'', 'idle HTTP connection not closed after timeout'
finally:idle.close();slow_http.close()
print(json.dumps({'concurrent':results,'stalled_clients_closed':True},indent=2))
# Malformed frame isolation and slot recovery.
with socket.create_connection((host,502),2) as s:
    s.settimeout(2);s.sendall(b'\x00\x01\x00\x01\x00\x06\x01');assert s.recv(1)==b''
with socket.create_connection((host,502),2) as s:
    s.settimeout(2);s.sendall(request(0x4444));reply(s,0x4444)
print('PASS: concurrent clients, transaction isolation, fragmentation, pipelining, idle timeout, malformed frame and slot reuse')
