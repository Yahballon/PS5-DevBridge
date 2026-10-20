"""Bounded offline process-memory debugger over DevBridge operation 21."""
import argparse,bisect,json,math,os,struct,sys,time,uuid
from pathlib import Path
from bridge import ROOT,request

TYPES={'u8':'B','i8':'b','u16':'H','i16':'h','u32':'I','i32':'i','u64':'Q','i64':'q','f32':'f','f64':'d'}
MAX_RESULTS=100000
PAGE=16384
class MemoryError(RuntimeError):pass

def pack(kind,value):
    value=float(value) if kind.startswith('f') else int(str(value),0)
    if isinstance(value,float) and not math.isfinite(value):raise ValueError('Use a finite numeric value')
    return struct.pack('<'+TYPES[kind],value)

def unpack(kind,blob):return struct.unpack('<'+TYPES[kind],blob)[0]

class Client:
    def __init__(self,transport=request):self.transport=transport
    def command(self,op,pid=0,cookie=0,address=0,length=0,body=b''):
        frame=struct.pack('<IHHiIQQIIQ',0x314d4244,1,op,pid,0,cookie,address,length,int(op==4),0)
        data=self.transport(21,frame+body,timeout=30)
        if len(data)<32:raise MemoryError('Memory endpoint unavailable; start DevBridge v0.2.3 or newer')
        magic,status,observed,size,reserved,actual=struct.unpack('<IiQIIQ',data[:32])
        if magic!=0x31524d44 or reserved or observed!=cookie or actual!=address or size!=len(data)-32:raise MemoryError('Invalid memory response')
        if status:raise MemoryError(data[32:].decode(errors='replace'))
        return data[32:]
    def processes(self):return json.loads(self.command(1))
    def maps(self,pid,cookie):return json.loads(self.command(2,pid,cookie))
    def read(self,pid,cookie,address,length):
        if not 1<=length<=1024*1024:raise ValueError('Read must be 1..1048576 bytes')
        data=self.command(3,pid,cookie,address,length)
        if len(data)!=length:raise MemoryError('Incomplete memory read')
        return data
    def write(self,pid,cookie,address,expected,value):
        if not 1<=len(value)<=16 or len(expected)!=len(value):raise ValueError('Write requires 1..16 expected and replacement bytes')
        data=self.command(4,pid,cookie,address,len(value),expected+value)
        if data!=value:raise MemoryError('Write readback mismatch')
        return data

def atomic(path,data):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_name(path.name+'.part');temp.write_text(json.dumps(data,indent=2));os.replace(temp,path)

def ranges(maps,start=None,end=None):
    for m in maps:
        if m['type'] not in (1,2,9) or m['protection']&3!=3 or m['protection']&4:continue
        low=max(m['start'],start if start is not None else m['start'])
        high=min(m['end'],end if end is not None else m['end'])
        if low<high:yield low,high

class Session:
    def __init__(self,path,client=None):
        self.path=Path(path);self.client=client or Client();self.data=json.loads(self.path.read_text()) if self.path.exists() else {}
    def save(self):atomic(self.path,self.data)
    def select(self,pid):
        targets=[p for p in self.client.processes() if p['pid']==pid and p['eligible'] and p['cookie']]
        if len(targets)!=1:raise MemoryError('Choose an eligible app PID from processes')
        self.data={'schema':1,'target':targets[0]};self.save()
    @property
    def target(self):
        if 'target' not in self.data:raise MemoryError('Select a process first')
        p=self.data['target'];return p['pid'],p['cookie']
    def read(self,address,length):return self.client.read(*self.target,address,length)
    def write(self,address,kind,value,expected=None):
        blob=pack(kind,value);old=self.read(address,len(blob)) if expected is None else pack(kind,expected)
        self.client.write(*self.target,address,old,blob);return old
    def chunks(self,start,end,max_bytes):
        if not 1<=max_bytes<=256*1024*1024:raise ValueError('Scan budget exceeds 256 MiB')
        if start is not None and end is not None and end<=start:raise ValueError('End must be greater than start')
        maps=self.client.maps(*self.target);used=0;skipped=0
        for low,high in ranges(maps,start,end):
            at=low
            while at<high and used<max_bytes:
                size=min(1024*1024,high-at,max_bytes-used)
                if not size:break
                try:blob=self.read(at,size)
                except MemoryError as e:
                    if not any(s in str(e) for s in ['Process read failed','Unmapped, device']):raise
                    # Salvage readable pages instead of silently losing a whole MiB.
                    for page in range(at,at+size,PAGE):
                        count=min(PAGE,at+size-page)
                        try:yield page,self.read(page,count)
                        except MemoryError as err:
                            if not any(s in str(err) for s in ['Process read failed','Unmapped, device']):raise
                            skipped+=count
                else:yield at,blob
                used+=size;at+=size
            if used>=max_bytes:break
        self.coverage={'attempted_bytes':used,'unreadable_bytes':skipped,'budget_bytes':max_bytes,'scope':'writable non-executable mappings; scan is bounded, not whole-process coverage'}
    def scan(self,kind,value=None,start=None,end=None,max_bytes=32*1024*1024):
        if not 1<=max_bytes<=256*1024*1024:raise ValueError('Scan budget must be 1..256 MiB')
        if value is None and (start is None or end is None):raise ValueError('Unknown-value scans require an explicit start/end range')
        width=struct.calcsize(TYPES[kind]);needle=pack(kind,value) if value is not None else None
        candidates=[];snapshots=[]
        folder=self.path.parent/'snapshots'/uuid.uuid4().hex
        for address,blob in self.chunks(start,end,max_bytes):
            if needle is None:
                folder.mkdir(parents=True,exist_ok=True);file=folder/(str(address)+'.bin');file.write_bytes(blob);snapshots.append({'address':address,'file':str(file.resolve()),'size':len(blob)})
            else:
                at=blob.find(needle)
                while at>=0:
                    if (address+at)%width==0:candidates.append([address+at,needle.hex()])
                    if len(candidates)>MAX_RESULTS:raise MemoryError('Too many matches; choose a smaller range or a more specific value')
                    at=blob.find(needle,at+1)
        self.data.update(kind=kind,candidates=candidates,snapshots=snapshots,unknown=needle is None,coverage=self.coverage);self.save()
        return None if needle is None else len(candidates)
    def filter(self,mode,value=None):
        if mode not in ('equal','not-equal','changed','unchanged','increased','decreased'):raise ValueError('Unknown filter mode')
        kind=self.data['kind'];width=struct.calcsize(TYPES[kind]);wanted=pack(kind,value) if value is not None else None
        if mode in ('equal','not-equal') and wanted is None:raise ValueError('Filter needs a value')
        def match(old,new):
            if mode=='equal':return new==wanted
            if mode=='not-equal':return new!=wanted
            if mode=='changed':return old!=new
            if mode=='unchanged':return old==new
            a,b=unpack(kind,old),unpack(kind,new)
            return b>a if mode=='increased' else b<a
        result=[]
        def keep(address,old,new):
            if match(old,new):result.append([address,new.hex()])
            if len(result)>MAX_RESULTS:raise MemoryError('Too many matches; prior scan retained. Filter by an exact value or reduce the range')
        if self.data.get('unknown'):
            for chunk in self.data['snapshots']:
                old=Path(chunk['file']).read_bytes();new=self.read(chunk['address'],chunk['size'])
                for at in range((-chunk['address'])%width,len(old)-width+1,width):keep(chunk['address']+at,old[at:at+width],new[at:at+width])
        else:
            pages={}
            for address,old in self.data['candidates']:
                page=address//PAGE*PAGE
                if page not in pages:pages[page]=self.read(page,PAGE)
                new=pages[page][address-page:address-page+width]
                if len(new)!=width:raise MemoryError('Candidate crosses an inaccessible page')
                keep(address,bytes.fromhex(old),new)
        self.data.update(candidates=result,unknown=False,snapshots=[]);self.save();return len(result)
    def resolve(self,root,offsets):
        address=root
        for offset in offsets:
            address=struct.unpack('<Q',self.read(address,8))[0]+offset
            if not 0x10000<=address<0x800000000000:raise MemoryError('Pointer chain leaves user memory')
        return address
    def pointers(self,address,depth=2,max_offset=4096,start=None,end=None,max_bytes=32*1024*1024):
        if not 1<=depth<=3 or not 0<=max_offset<=65536:raise ValueError('Pointer depth 1..3; offset 0..65536')
        chunks=list(self.chunks(start,end,max_bytes));frontier={address:[([],set([address]))]};result=[]
        for _ in range(depth):
            keys=sorted(frontier);following={}
            for base,blob in chunks:
                for at in range((-base)%8,len(blob)-7,8):
                    pointer=struct.unpack_from('<Q',blob,at)[0]
                    for target in keys[bisect.bisect_left(keys,pointer):bisect.bisect_right(keys,pointer+max_offset)]:
                        root=base+at
                        for suffix,seen in frontier[target]:
                            if root in seen:continue
                            offsets=[target-pointer]+suffix
                            result.append({'root':root,'offsets':offsets,'scope':'absolute root; validate again after restart'})
                            following.setdefault(root,[]).append((offsets,seen|{root}))
                            if len(result)>=2000:return result
            frontier=following
            if not frontier:break
        return result
    def freeze(self,address,kind,value,seconds,interval=.25):
        if not 0<seconds<=3600 or not .1<=interval<=10:raise ValueError('Freeze: 0..3600 seconds and interval .1..10')
        desired=pack(kind,value);original=self.read(address,len(desired));last=None;deadline=time.monotonic()+seconds
        try:
            while time.monotonic()<deadline:
                before=self.read(address,len(desired));self.client.write(*self.target,address,before,desired);last=desired;time.sleep(interval)
        finally:
            # Restore only our final value, and only within the same verified process session.
            if last is not None and self.read(address,len(last))==last:self.client.write(*self.target,address,last,original)
    def profile_save(self,path,label,version,entries):
        if not label or not version or not entries:raise ValueError('Profile needs game label, exact version label and entries')
        for e in entries:pack(e['type'],e['value'])
        atomic(path,{'schema':1,'game':label,'version':version,'target':self.data['target'],'entries':entries,'address_scope':'absolute; valid only for this process session'})
    def profile_apply(self,path,version):
        profile=json.loads(Path(path).read_text())
        if profile.get('schema')!=1 or profile['version']!=version or profile['target']!=self.data['target']:raise MemoryError('Profile version or process session differs; rediscover and validate addresses')
        for e in profile['entries']:self.write(e['address'],e['type'],e['value'])

def number(text):return int(text,0)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--session',default=str(ROOT/'memory-sessions/default.json'))
    sub=parser.add_subparsers(dest='command',required=True)
    p=sub.add_parser('processes');p.add_argument('--all',action='store_true');p=sub.add_parser('select');p.add_argument('pid',type=int)
    sub.add_parser('maps')
    p=sub.add_parser('scan');p.add_argument('--type',choices=TYPES,default='u32');p.add_argument('--value');p.add_argument('--start',type=number);p.add_argument('--end',type=number);p.add_argument('--max-mib',type=int,default=32)
    p=sub.add_parser('filter');p.add_argument('mode',choices=['equal','not-equal','changed','unchanged','increased','decreased']);p.add_argument('--value')
    p=sub.add_parser('results');p.add_argument('--limit',type=int,default=30)
    for name in ['read','write','watch','freeze']:
        p=sub.add_parser(name);p.add_argument('address',type=number);p.add_argument('--type',choices=TYPES,default='u32')
        if name in ('write','freeze'):p.add_argument('value')
        if name=='write':p.add_argument('--expect')
        if name in ('watch','freeze'):p.add_argument('--seconds',type=float,default=10);p.add_argument('--interval',type=float,default=.25)
    p=sub.add_parser('pointers');p.add_argument('address',type=number);p.add_argument('--depth',type=int,default=2);p.add_argument('--max-offset',type=number,default=4096);p.add_argument('--start',type=number);p.add_argument('--end',type=number);p.add_argument('--max-mib',type=int,default=32)
    p=sub.add_parser('resolve');p.add_argument('root',type=number);p.add_argument('offsets',nargs='+',type=number)
    p=sub.add_parser('profile-save');p.add_argument('file');p.add_argument('--game',required=True);p.add_argument('--version',required=True);p.add_argument('--address',type=number,required=True);p.add_argument('--type',choices=TYPES,default='u32');p.add_argument('--value',required=True)
    p=sub.add_parser('profile-apply');p.add_argument('file');p.add_argument('--version',required=True)
    a=parser.parse_args();session=Session(a.session);cmd=a.command
    if cmd=='processes':print(json.dumps([p for p in session.client.processes() if a.all or p['eligible']],indent=2))
    elif cmd=='select':session.select(a.pid);print(json.dumps(session.data['target']))
    elif cmd=='maps':print(json.dumps(session.client.maps(*session.target),indent=2))
    elif cmd=='scan':print('matches:',session.scan(a.type,a.value,a.start,a.end,a.max_mib*1024*1024));print(json.dumps(session.data['coverage']))
    elif cmd=='filter':print('matches:',session.filter(a.mode,a.value))
    elif cmd=='results':
        if session.data.get('unknown'):print('Unknown-value snapshot: filter it before listing matches')
        else:
            for address,value in session.data.get('candidates',[])[:max(0,min(a.limit,1000))]:print(hex(address),unpack(session.data['kind'],bytes.fromhex(value)))
            print('total:',len(session.data.get('candidates',[])))
    elif cmd=='read':print(unpack(a.type,session.read(a.address,struct.calcsize(TYPES[a.type]))))
    elif cmd=='write':print('verified; previous value:',unpack(a.type,session.write(a.address,a.type,a.value,a.expect)))
    elif cmd=='freeze':session.freeze(a.address,a.type,a.value,a.seconds,a.interval);print('Freeze ended; original restored if still our value')
    elif cmd=='watch':
        if not 0<a.seconds<=3600 or not .1<=a.interval<=10:raise ValueError('Invalid watch duration or interval')
        until=time.monotonic()+a.seconds
        while time.monotonic()<until:print(unpack(a.type,session.read(a.address,struct.calcsize(TYPES[a.type]))),flush=True);time.sleep(a.interval)
    elif cmd=='pointers':print(json.dumps(session.pointers(a.address,a.depth,a.max_offset,a.start,a.end,a.max_mib*1024*1024),indent=2))
    elif cmd=='resolve':print(hex(session.resolve(a.root,a.offsets)))
    elif cmd=='profile-save':session.profile_save(a.file,a.game,a.version,[{'address':a.address,'type':a.type,'value':a.value}]);print('Saved session-bound profile')
    elif cmd=='profile-apply':session.profile_apply(a.file,a.version);print('Profile applied with readback verification')
if __name__=='__main__':
    try:main()
    except (MemoryError,ValueError,KeyError,struct.error) as e:print('Memory debugger:',e,file=sys.stderr);sys.exit(1)
