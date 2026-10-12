from pathlib import Path
import json
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
p=r/'jb-backend/src/install.hpp';s=p.read_text().replace('DownloadInfo* info=nullptr);','DownloadInfo* info=nullptr,bool persistent=false);').replace('const char* password = nullptr);','const char* password = nullptr,const char* resume_key = nullptr);');s=s.replace('bool install_url(', 'void discard_partial(const char* base,const char* key);\nbool install_url(');write(p,s)
p=r/'jb-backend/src/install.cpp';s=p.read_text().replace('std::uint64_t max_bytes,DownloadInfo* info)','std::uint64_t max_bytes,DownloadInfo* info,bool persistent)')
s=s.replace('O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600','O_RDWR | O_CREAT | (persistent?0:O_TRUNC) | O_NOFOLLOW, 0600',1).replace('fdopen(download_fd, "wb")','fdopen(download_fd, persistent?"r+b":"wb")',1)
s=s.replace('    Transfer t{f, progress, max_bytes};',r"""    Transfer t{f, progress, max_bytes};
    t.persistent=persistent;
    if(persistent){
        struct stat partial{};if(fstat(fileno(f),&partial)||!S_ISREG(partial.st_mode)||partial.st_size<0||(uint64_t)partial.st_size>max_bytes){fclose(f);std::snprintf(msg,cap,"Invalid partial file");return false;}
        unsigned long long complete=0;FILE* marker=fopen((std::string(file)+".done").c_str(),"r");if(marker){std::fscanf(marker,"%llu",&complete);fclose(marker);}
        if(complete&&complete==(uint64_t)partial.st_size){fclose(f);if(info)info->status=200;progress.note("Using completed package saved before shutdown");return true;}
        FILE* state=fopen((std::string(file)+".resume").c_str(),"r");char value[512]{};if(state){fgets(value,sizeof(value),state);fclose(state);t.validator=value;while(!t.validator.empty()&&(t.validator.back()=='\n'||t.validator.back()=='\r'))t.validator.pop_back();}
        if(!t.validator.empty()&&t.validator.find_first_of("\r\n")==t.validator.npos){t.written=partial.st_size;fseeko(f,0,SEEK_END);}
        else{t.validator.clear();if(ftruncate(fileno(f),0)||fseeko(f,0,SEEK_SET)){fclose(f);return false;}}
        t.offset=t.written;
    }
""")
s=s.replace('    std::uint64_t offset = 0;','    std::uint64_t offset = 0;\n    bool persistent=false;\n    std::uint64_t synced=0;')
s=s.replace('    t.written += actual;','    t.written += actual;\n    if(t.persistent&&t.written-t.synced>=8u*1024*1024){if(fflush(t.file)||fsync(fileno(t.file))){t.failed=true;return 0;}t.synced=t.written;}')
s=s.replace('struct Headers{Transfer* transfer;DownloadInfo* info;};Headers headers{&t,info};','struct Headers{Transfer* transfer;DownloadInfo* info;const char* file;bool persistent;};Headers headers{&t,info,file,persistent};')
s=s.replace('        if(state.info){long long n=',r"""        if(state.persistent&&!t.offset&&!t.validator.empty()&&(key=="etag"||key=="last-modified")){
            const std::string saved=std::string(state.file)+".resume",temp=saved+".tmp";
            if(FILE* f=fopen(temp.c_str(),"w")){const bool okay=std::fprintf(f,"%s\n",t.validator.c_str())>0&&std::fflush(f)==0&&fsync(fileno(f))==0;const bool closed=std::fclose(f)==0;if(okay&&closed)std::rename(temp.c_str(),saved.c_str());else unlink(temp.c_str());}
        }
        if(state.info){long long n=""")
s=s.replace('    bool closed = fclose(f) == 0;','    bool closed = (!persistent||fflush(f)==0)&&(!persistent||fsync(fileno(f))==0);\n    closed=(fclose(f)==0)&&closed;')
a=s.index('    if (rc != CURLE_OK || !closed || !t.written)');b=s.index('bool uninstall(',a)
part=s[a:b].replace('        unlink(file);','        if(!persistent)unlink(file);')
part=part.replace('    return true;\n}',r"""    if(persistent){const std::string marker=std::string(file)+".done";if(FILE* done=fopen(marker.c_str(),"w")){std::fprintf(done,"%llu\n",(unsigned long long)t.written);std::fflush(done);fsync(fileno(done));fclose(done);}}
    return true;
}""")
s=s[:a]+part+s[b:]
s=s.replace('bool updating, const char* password)','bool updating, const char* password,const char* resume_key)')
old='    if (std::snprintf(job, sizeof(job), "%s/download-XXXXXX", temporary_root) >= int(sizeof(job)))'
new='    const bool durable=resume_key&&std::strlen(resume_key)==64&&std::strspn(resume_key,"0123456789abcdef")==64;\n    if (std::snprintf(job, sizeof(job), durable?"%s/download-%s":"%s/download-XXXXXX", temporary_root,durable?resume_key:"") >= int(sizeof(job)))'
assert old in s;s=s.replace(old,new).replace('    if (!mkdtemp(job))','    if (durable?!(mkdir(job,0700)==0||(lstat(job,&st)==0&&S_ISDIR(st.st_mode))):!mkdtemp(job))')
s=s.replace('format == PackageFormat::pkg ? 64ULL * 1024 * 1024 * 1024 : limit));','format == PackageFormat::pkg ? 64ULL * 1024 * 1024 * 1024 : limit),nullptr,durable);')
s=s.replace('    bool retain = false;','    bool retain = false;\n    bool fetched=false;')
s=s.replace('    if(ok){char beginning[513]','    fetched=ok;\n    if(ok){char beginning[513]')
s=s.replace('    if (!retain)\n        remove_tree(job);','    if (!retain && (ok||!durable))\n        remove_tree(job);\n    else if(durable&&!ok){\n        if(fetched&&!retain){unlink(zip);unlink((std::string(zip)+".done").c_str());unlink((std::string(zip)+".resume").c_str());}\n        progress.note("Partial package preserved; restart Store or refresh this download link to continue");\n    }')
# Discard only an explicitly identified Store download, never a caller-provided directory.
pos=s.index('bool install_url(')
s=s[:pos]+r"""void discard_partial(const char* base,const char* key){
 if(!base||!key||std::strlen(key)!=64||std::strspn(key,"0123456789abcdef")!=64)return;
 const std::string root=std::string(base)+"/.jb-store/tmp/download-"+key;
 struct stat st{};if(lstat(root.c_str(),&st)==0&&S_ISDIR(st.st_mode))remove_tree(root.c_str());
}
"""+s[pos:]
write(p,s)
print('Durable partial packages and validators implemented')
