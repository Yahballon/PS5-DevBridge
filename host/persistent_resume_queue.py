from pathlib import Path
import json
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
p=r/'jb-backend/src/download_queue.hpp';s=p.read_text().replace('const char *ca = "/app0/assets/ca-bundle.crt");','const char *ca = "/app0/assets/ca-bundle.crt",const char* state_root="/data/jb-store/cache");');s=s.replace('    void cancel(unsigned);','    void cancel(unsigned);\n    void suspend_for_shutdown();');s=s.replace('        char url[1024]{},','        char resume_key[65]{};\n        bool discarded=false,password_required=false;\n        char url[1024]{},');s=s.replace('    unsigned count = 0, serial = 0;','    char journal[1024]{};\n    Uint64 saved_at=0;\n    unsigned count = 0, serial = 0;\n    void save();\n    void restore();');write(p,s)
p=r/'jb-backend/src/download_queue.cpp';s=p.read_text();s=r"""#include "core/save_file.hpp"
#include "core/storage_layout.hpp"
#include "../../src/third_party/picosha2/picosha2.h"
#ifdef STORE_NATIVE_CURL
#include "jb_json_symbols.hpp"
#endif
#include <json-c/json.h>
#include <sys/stat.h>
"""+s
s=s.replace('DownloadQueue::DownloadQueue(const char *folder, const char *certificate)','DownloadQueue::DownloadQueue(const char *folder, const char *certificate,const char* state_root)')
s=s.replace('    std::snprintf(ca, sizeof(ca), "%s", certificate);','    std::snprintf(ca, sizeof(ca), "%s", certificate);\n    const std::string identity=folder;const auto key=picosha2::hash256_hex_string(identity.begin(),identity.end());\n    store::storage::directory(state_root);std::snprintf(journal,sizeof(journal),"%s/download-queue-%s.json",state_root,key.c_str());restore();')
s=s.replace('DownloadQueue::~DownloadQueue()\n{','DownloadQueue::~DownloadQueue()\n{\n    save();')
s=s.replace('    unsigned slot = count;','    unsigned slot = count;\n    for(unsigned i=0;i<count;++i)if(!tasks[i].thread&&tasks[i].view.phase==DownloadPhase::stopped&&!tasks[i].discarded&&\n      !std::strcmp(tasks[i].url,entry.download_url)&&!std::strcmp(tasks[i].view.repository_key,entry.repository_key)){slot=i;break;}')
s=s.replace('    t.view = {};','    t.view = {};t.discarded=false;t.password_required=password&&*password;')
s=s.replace('    std::snprintf(t.view.title, sizeof(t.view.title), "%s", entry.title_id);','    std::snprintf(t.view.title, sizeof(t.view.title), "%s", entry.title_id);\n    const std::string identity=std::string(t.url)+"\\n"+t.view.repository_key+"\\n"+t.view.title+"\\n"+t.view.digest+"\\n"+t.view.format;\n    const auto key=picosha2::hash256_hex_string(identity.begin(),identity.end());std::snprintf(t.resume_key,sizeof(t.resume_key),"%s",key.c_str());')
s=s.replace('    std::snprintf(message, capacity, "Added to Downloads: %s", entry.name);','    save();\n    std::snprintf(message, capacity, "Added to Downloads: %s", entry.name);')
s=s.replace('t.view.updating,t.password);','t.view.updating,t.password,t.resume_key);')
s=s.replace('    SDL_AtomicSet(&t.finished, 1);','    SDL_AtomicSet(&t.finished, 1);')
s=s.replace('            t.thread = nullptr;','            t.thread = nullptr;\n            if(t.discarded)discard_partial(base,t.resume_key);')
s=s.replace('    return changed;\n}', '    if(changed||SDL_GetTicks64()-saved_at>=2000){save();saved_at=SDL_GetTicks64();}\n    return changed;\n}',1)
s=s.replace('    auto &t = tasks[index];\n    SDL_AtomicLock(&lock);','    auto &t = tasks[index];\n    SDL_AtomicLock(&lock);\n    t.discarded=true;',1)
s=s.replace('    SDL_AtomicUnlock(&lock);\n}\nbool DownloadQueue::is_paused','    SDL_AtomicUnlock(&lock);\n    save();if(!t.thread)discard_partial(base,t.resume_key);\n}\nbool DownloadQueue::is_paused',1)
# Journal is replaced atomically; passwords are intentionally excluded.
pos=s.index('bool DownloadQueue::pending')
s=s[:pos]+r"""void DownloadQueue::save(){
 auto* document=json_object_new_object();auto* list=json_object_new_array();if(!document||!list){if(document)json_object_put(document);if(list)json_object_put(list);return;}
 json_object_object_add(document,"schema",json_object_new_int(1));json_object_object_add(document,"target",json_object_new_string(base));json_object_object_add(document,"tasks",list);
 SDL_AtomicLock(&lock);
 for(unsigned i=0;i<count;++i){const auto& t=tasks[i];if(t.discarded||t.view.phase==DownloadPhase::installed)continue;auto* job=json_object_new_object();
  auto text=[&](const char* key,const char* value){json_object_object_add(job,key,json_object_new_string(value));};
  text("source",t.url);text("direct",t.direct_url);text("key",t.resume_key);text("name",t.view.name);text("title",t.view.title);text("version",t.view.version);text("format",t.view.format);text("digest",t.view.digest);text("repo",t.view.repository_key);
  json_object_object_add(job,"bytes",json_object_new_int64(t.view.bytes));json_object_object_add(job,"total",json_object_new_int64(t.view.total));json_object_object_add(job,"updating",json_object_new_boolean(t.view.updating));json_object_object_add(job,"password_required",json_object_new_boolean(t.password_required));json_object_array_add(list,job);
 }
 SDL_AtomicUnlock(&lock);
 const std::string encoded=json_object_to_json_string_ext(document,JSON_C_TO_STRING_PLAIN);json_object_put(document);
 if(hui::save::write_atomic(journal,encoded).empty())chmod(journal,0600);
}
void DownloadQueue::restore(){
 std::string text;if(!hui::save::read_file(journal,&text,1024*1024))return;
 auto* root=json_tokener_parse(text.c_str());if(!root)return;json_object *schema=nullptr,*target=nullptr,*list=nullptr;
 if(!json_object_object_get_ex(root,"schema",&schema)||json_object_get_int(schema)!=1||!json_object_object_get_ex(root,"target",&target)||std::strcmp(json_object_get_string(target),base)||!json_object_object_get_ex(root,"tasks",&list)||!json_object_is_type(list,json_type_array)||json_object_array_length(list)>64){json_object_put(root);return;}
 for(size_t i=0;i<json_object_array_length(list);++i){auto* job=json_object_array_get_idx(list,i);Task& t=tasks[count];bool okay=true;
  auto text=[&](const char* key,char* dst,size_t cap){json_object* v=nullptr;if(!json_object_object_get_ex(job,key,&v)||!json_object_is_type(v,json_type_string)||std::strlen(json_object_get_string(v))>=cap){okay=false;return;}std::snprintf(dst,cap,"%s",json_object_get_string(v));};
  text("source",t.url,sizeof(t.url));text("direct",t.direct_url,sizeof(t.direct_url));text("key",t.resume_key,sizeof(t.resume_key));text("name",t.view.name,sizeof(t.view.name));text("title",t.view.title,sizeof(t.view.title));text("version",t.view.version,sizeof(t.view.version));text("format",t.view.format,sizeof(t.view.format));text("digest",t.view.digest,sizeof(t.view.digest));text("repo",t.view.repository_key,sizeof(t.view.repository_key));
  const std::string identity=std::string(t.url)+"\n"+t.view.repository_key+"\n"+t.view.title+"\n"+t.view.digest+"\n"+t.view.format;const auto expected=picosha2::hash256_hex_string(identity.begin(),identity.end());
  if(!okay||std::strcmp(t.resume_key,expected.c_str())||(!std::string(t.direct_url).starts_with("https://")&&!std::string(t.direct_url).starts_with("http://")&&!std::string(t.direct_url).starts_with("file://")))continue;
  auto number=[&](const char* key){json_object* v=nullptr;return json_object_object_get_ex(job,key,&v)?std::max<int64_t>(0,json_object_get_int64(v)):0;};
  t.owner=this;t.serial=++serial;t.format=package_format((std::string("package.")+t.view.format).c_str());t.view.bytes=number("bytes");t.view.total=number("total");t.view.updating=number("updating")!=0;t.password_required=number("password_required")!=0;
  t.view.phase=t.password_required?DownloadPhase::stopped:DownloadPhase::queued;
  std::snprintf(t.view.stage,sizeof(t.view.stage),t.password_required?"Password required":"Restored download");
  std::snprintf(t.view.message,sizeof(t.view.message),t.password_required?"Select Install again and enter the archive password":"Continuing saved download; expired links can be refreshed by selecting Install again");
  SDL_AtomicSet(&t.cancelled,0);SDL_AtomicSet(&t.finished,0);SDL_AtomicSet(&t.paused,0);++count;
 }
 json_object_put(root);
}
void DownloadQueue::suspend_for_shutdown(){save();for(unsigned i=0;i<count;++i)SDL_AtomicSet(&tasks[i].cancelled,1);}
"""+s[pos:]
write(p,s)
p=r/'src/app/jb_service.cpp';s=p.read_text();old=' auto list=jb_->queue->snapshot();for(unsigned i=0;i<list.count;++i)jb_->queue->cancel(i);';assert old in s;s=s.replace(old,' jb_->queue->suspend_for_shutdown();');write(p,s)
p=r/'sce_sys/param.json';j=json.loads(p.read_text());j['contentVersion']='01.000.099';write(p,json.dumps(j,indent=2)+'\n')
print('Persistent queue restoration and stable refreshed-link identity implemented')
