# (AI-assisted) Run in the JB_STORE_REPO checkout; generate a content version and catalog ZIP together.
import argparse,hashlib,json,zipfile
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument("root",nargs="?",default=".");args=parser.parse_args()
root=Path(args.root).resolve();repo=root/"repo"
if not repo.is_dir():raise SystemExit("Expected a repo/ folder inside the chosen checkout")
files=sorted(p for p in repo.rglob("*") if p.is_file() and p.name!="version.json")
fingerprint=hashlib.sha256();index=[]
for path in files:
 if path.is_symlink():raise SystemExit("Symlinks are not supported")
 name=path.relative_to(root).as_posix();data=path.read_bytes();fingerprint.update(name.encode()+b"\0"+hashlib.sha256(data).digest())
 if path.suffix.lower()==".json":
  json.loads(data.decode("utf-8-sig"))
  blob=b"blob "+str(len(data)).encode()+b"\0"+data
  index.append({"path":name,"sha":hashlib.sha1(blob).hexdigest()})
manifest={"schema":2,"version":fingerprint.hexdigest(),"files":index}
(repo/"version.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
with zipfile.ZipFile(root/"JB_STORE_REPO.zip","w",zipfile.ZIP_DEFLATED) as archive:
 for path in sorted(files+[repo/"version.json"]):
  info=zipfile.ZipInfo(path.relative_to(root).as_posix(),(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;archive.writestr(info,path.read_bytes())
print("Repository version:",manifest["version"])
print("Publish repo/version.json and JB_STORE_REPO.zip together.")
