import sys,unittest,tempfile,struct,json
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'host'))
import memory

class Fake:
    def __init__(self):self.pid=42;self.cookie=123;self.base=0x10000;self.data=bytearray(memory.PAGE*2);self.writes=[]
    def processes(self):return [{'pid':self.pid,'cookie':self.cookie,'name':'db-memory-lab','eligible':True}]
    def check(self,pid,cookie):
        if pid!=self.pid or cookie!=self.cookie:raise memory.MemoryError('Target missing, restarted or not an eligible app')
    def maps(self,pid,cookie):self.check(pid,cookie);return [{'start':self.base,'end':self.base+len(self.data),'type':1,'protection':3,'path':''}]
    def read(self,pid,cookie,address,length):
        self.check(pid,cookie);at=address-self.base
        if at<0 or at+length>len(self.data):raise memory.MemoryError('Unmapped, device, protected or executable write range')
        return bytes(self.data[at:at+length])
    def write(self,pid,cookie,address,expected,value):
        before=self.read(pid,cookie,address,len(value))
        if before!=expected:raise memory.MemoryError('Value changed before write; no write performed')
        self.writes.append((address,before,value));at=address-self.base;self.data[at:at+len(value)]=value;return value
    def put(self,address,kind,value):at=address-self.base;self.data[at:at+struct.calcsize(memory.TYPES[kind])]=memory.pack(kind,value)

class Tests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.fake=Fake();self.session=memory.Session(Path(self.temp.name)/'session.json',self.fake);self.session.select(42);self.health=0x10064;self.fake.put(self.health,'u32','100')
    def tearDown(self):self.temp.cleanup()
    def test_exact_filter_and_compare(self):
        self.assertEqual(self.session.scan('u32','100',0x10000,0x10100),1)
        self.session.write(self.health,'u32','75','100');self.assertEqual(self.session.filter('decreased'),1)
        with self.assertRaises(memory.MemoryError):self.session.write(self.health,'u32','999','100')
        self.assertEqual(memory.unpack('u32',self.session.read(self.health,4)),75)
    def test_unknown_and_float(self):
        energy=self.health+4;self.fake.put(energy,'f32','42.5')
        self.session.scan('f32',None,0x10000,0x10100);self.session.write(energy,'f32','50.25')
        self.assertEqual(self.session.filter('increased'),1)
        self.assertEqual(self.session.data['candidates'][0][0],energy)
        self.assertEqual(self.session.filter('unchanged'),1)
        self.assertEqual(self.session.filter('equal','99'),0)
    def test_pid_restart_and_profile(self):
        file=Path(self.temp.name)/'profile.json';self.session.profile_save(file,'Lab','1.0',[{'address':self.health,'type':'u32','value':'125'}])
        with self.assertRaises(memory.MemoryError):self.session.profile_apply(file,'2.0')
        self.session.profile_apply(file,'1.0');self.assertEqual(memory.unpack('u32',self.session.read(self.health,4)),125)
        self.fake.cookie+=1
        with self.assertRaises(memory.MemoryError):self.session.write(self.health,'u32','999')
        self.session.select(42)
        with self.assertRaises(memory.MemoryError):self.session.profile_apply(file,'1.0')
    def test_pointer_search_and_resolve(self):
        self.fake.put(0x10080,'u64',hex(self.health));self.fake.put(0x10088,'u64','0x10080')
        found=self.session.pointers(self.health,2,0,0x10000,0x10100,256)
        self.assertTrue(any(p['root']==0x10088 and p['offsets']==[0,0] for p in found))
        self.assertEqual(self.session.resolve(0x10088,[0,0]),self.health)
    def test_freeze_restores_on_interrupt(self):
        with patch.object(memory.time,'monotonic',side_effect=[0,0]),patch.object(memory.time,'sleep',side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):self.session.freeze(self.health,'u32','999',1)
        self.assertEqual(memory.unpack('u32',self.session.read(self.health,4)),100)
        self.assertEqual(len(self.fake.writes),2)
    def test_limits_and_types(self):
        with self.assertRaises(ValueError):self.session.scan('u32',None)
        with self.assertRaises(ValueError):self.session.pointers(self.health,4)
        with self.assertRaises(ValueError):list(self.session.chunks(None,None,1024**3))
        with self.assertRaises(struct.error):memory.pack('u8','256')
        with self.assertRaises(ValueError):memory.pack('f32','nan')
        self.assertEqual(memory.pack('i16','-2'),b'\xfe\xff')
    def test_wire_rejects_wrong_identity_and_short_reply(self):
        def peer(op,data,timeout):
            self.assertEqual(op,21);self.assertEqual(len(data),48)
            return struct.pack('<IiQIIQ',0x31524d44,0,999,4,0,self.health)+b'1234'
        with self.assertRaises(memory.MemoryError):memory.Client(peer).read(42,123,self.health,4)
        with self.assertRaises(memory.MemoryError):memory.Client(lambda *a,**k:b'').processes()

if __name__=='__main__':unittest.main()
