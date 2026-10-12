from pathlib import Path
import re
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
p=r/'src/core/storage_layout.hpp';s=p.read_text();s=s.replace('inline void prepare(',r"""inline std::string temporary(const std::string& path){
 if(!path.starts_with("/data/jb-store/"))return path+".tmp";
 directory("/data/jb-store/tmp");unsigned long long hash=1469598103934665603ULL;for(unsigned char c:path){hash^=c;hash*=1099511628211ULL;}
 return "/data/jb-store/tmp/"+std::to_string(hash)+".tmp";
}
inline void prepare(""");write(p,s)
for sub in ['src','jb-backend/src']:
 for p in (r/sub).rglob('*.cpp'):
  s=p.read_text();old=s
  # Only string-valued path variables used for cache/save atomic writes.
  s=re.sub(r'\b(path|file|response_cache|budget_file|cached)\s*\+\s*"\.tmp"',r'store::storage::temporary(\1)',s)
  if s!=old:
   s='#include "core/storage_layout.hpp"\n'+s;write(p,s)
p=r/'src/app/jb_service.cpp';s=p.read_text().replace('mkdir((cache+"/cache").c_str(),0777);','store::storage::prepare(cache);');s='#include "core/storage_layout.hpp"\n'+s;write(p,s)
p=r/'jb-backend/src/install.cpp';s=p.read_text();s=s.replace('    std::uint64_t offset = 0;','    std::uint64_t offset = 0;\n    std::string validator;\n    long response_status=0;\n    std::uint64_t range_start=UINT64_MAX;')
s=s.replace('    size_t bytes = s * n;','    size_t bytes = s * n;\n    if(t.response_status>=300&&t.response_status<400)return bytes;\n    if(t.offset&&(t.response_status!=206||t.range_start!=t.offset))return 0;')
a=s.index('    if(info){*info=DownloadInfo{};');b=s.index('    curl_easy_setopt(c, CURLOPT_URL, url);',a)
s=s[:a]+r"""    struct Headers{Transfer* transfer;DownloadInfo* info;};Headers headers{&t,info};
    if(info)*info=DownloadInfo{};
    curl_easy_setopt(c,CURLOPT_HEADERDATA,&headers);
    curl_easy_setopt(c,CURLOPT_HEADERFUNCTION,+[](char* data,size_t size,size_t count,void* opaque)->size_t{
        if(size&&count>SIZE_MAX/size)return 0;const size_t length=size*count;auto& state=*static_cast<Headers*>(opaque);auto& t=*state.transfer;std::string line(data,length);
        if(line.starts_with("HTTP/")){t.range_start=UINT64_MAX;const auto space=line.find(' ');t.response_status=space==line.npos?0:std::strtol(line.c_str()+space+1,nullptr,10);if(state.info)*state.info=DownloadInfo{};return length;}
        const auto colon=line.find(':');if(colon==line.npos)return length;std::string key=line.substr(0,colon),value=line.substr(colon+1);for(char& c:key)if(c>='A'&&c<='Z')c+=32;
        const auto first=value.find_first_not_of(" \t");value=first==value.npos?"":value.substr(first);while(!value.empty()&&(value.back()=='\r'||value.back()=='\n'||value.back()==' '))value.pop_back();
        if(key=="content-range"){unsigned long long start=0;if(std::sscanf(value.c_str(),"bytes %llu-",&start)==1)t.range_start=start;}
        if(!t.offset&&(t.response_status==200||t.response_status==206)&&(key=="etag"||(key=="last-modified"&&t.validator.empty()))&&value.size()<512&&value.find_first_of("\r\n")==value.npos)t.validator=value;
        if(state.info){long long n=std::strtoll(value.c_str(),nullptr,10);if(key=="x-ratelimit-remaining")state.info->remaining=n;else if(key=="x-ratelimit-reset")state.info->reset=n;else if(key=="retry-after")state.info->retry_after=n;}
        return length;
    });
"""+s[b:]
s=s.replace('    for(unsigned attempt=0;attempt<31;++attempt){','    curl_slist* range_headers=nullptr;\n    for(unsigned attempt=0;attempt<31;++attempt){')
s=s.replace('        error[0]=0;','        curl_slist_free_all(range_headers);range_headers=nullptr;\n        if(t.offset&&!t.validator.empty())range_headers=curl_slist_append(nullptr,("If-Range: "+t.validator).c_str());\n        curl_easy_setopt(c,CURLOPT_HTTPHEADER,range_headers);\n        error[0]=0;')
s=s.replace('    progress.note(rc == CURLE_OK ?', '    curl_easy_setopt(c,CURLOPT_HTTPHEADER,nullptr);curl_slist_free_all(range_headers);\n    progress.note(rc == CURLE_OK ?')
# No validator: reconnect safely from zero rather than joining possibly changed data.
s=s.replace('        if(rc==CURLE_RANGE_ERROR&&t.offset){','        if((rc==CURLE_RANGE_ERROR||t.validator.empty())&&t.offset){')
s=s.replace('            clearerr(f);t.written=t.offset=0;','            clearerr(f);t.written=t.offset=0;t.validator.clear();')
write(p,s)
print('Validator-protected resume and temporary-file layout completed')
