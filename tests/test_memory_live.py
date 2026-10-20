"""Hardware opt-in: touches only the included memory-lab payload, never a game."""
import json,sys,tempfile,struct,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'host'))
import memory
from files import chunk,upload

def main():
    lab=json.loads(chunk('/data/ps5-devbridge/memory-lab.json',0)[0])
    with tempfile.TemporaryDirectory() as folder:
        s=memory.Session(Path(folder)/'session.json');s.select(lab['pid'])
        assert s.data['target']['name']=='db-memory-lab'
        low,high=lab['base'],lab['base']+lab['size'];health=lab['health']
        assert s.scan('u32','100',low,high)==1
        s.write(health,'u32','75','100');assert s.filter('decreased')==1
        try:s.write(health,'u32','999','100')
        except memory.MemoryError:pass
        else:raise AssertionError('Stale expected value accepted')
        s.scan('u32',None,low,high);s.write(lab['coins'],'u32','30','25');assert s.filter('increased')==1
        s.write(lab['energy'],'f32','9.25','42.5');s.write(lab['energy'],'f32','42.5','9.25')
        found=s.pointers(health,2,0,low,high,lab['size'])
        assert any(p['root']==lab['pointer2'] and p['offsets']==[0,0] for p in found)
        assert s.resolve(lab['pointer2'],[0,0])==health
        s.freeze(lab['coins'],'u32','777',.4,.1);assert memory.unpack('u32',s.read(lab['coins'],4))==30
        s.write(health,'u32','100','75');s.write(lab['coins'],'u32','25','30')
        print('PASS: PS5 lab scan, filters, float edits, comparison rejection, pointers, freeze and restored values')

if __name__=='__main__':main()
