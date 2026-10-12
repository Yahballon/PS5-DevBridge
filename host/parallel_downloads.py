from pathlib import Path
import json
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
p=r/'src/app/store.hpp';s=p.read_text().replace('    bool check_updates = true;','    unsigned maximum_downloads = 1;\n    bool check_updates = true;',1);write(p,s)
p=r/'src/app/store.cpp';s=p.read_text().replace('constexpr int kRows = 6;','constexpr int kRows = 7;',1)
s=s.replace('    else\n    {\n        if(!input.is_pressed(Action::confirm))return;\n        pending_store_update_check=true;','    else if(setting_focus_==6){\n        constexpr unsigned values[]={1,3,6,9,12};int index=0;for(int n=0;n<5;++n)if(values[n]==settings_.maximum_downloads)index=n;\n        settings_.maximum_downloads=values[(index+5+turn)%5];\n    }\n    else\n    {\n        if(!input.is_pressed(Action::confirm))return;\n        pending_store_update_check=true;',1)
s=s.replace('"\\nsounds=" + (settings.sounds','"\\nmaximum_downloads="+std::to_string(settings.maximum_downloads)+"\\nsounds=" + (settings.sounds',1)
s=s.replace('        else if (key == "sounds")','        else if(key=="maximum_downloads"){\n            if(value=="1"||value=="3"||value=="6"||value=="9"||value=="12")settings.maximum_downloads=static_cast<unsigned>(std::strtoul(std::string(value).c_str(),nullptr,10));\n        }\n        else if (key == "sounds")',1)
at=s.index('            {7, "Check store update now",');end=s.index('};',at)
s=s[:end].rstrip().rstrip(',')+',\n            {3,"Maximum simultaneous downloads","Change with Left / Right or X. Default: 1.",2,std::to_string(settings_.maximum_downloads),true}\n        '+s[end:]
at=s.index('"Maximum simultaneous downloads"');loop=s.index('for (int i = 0; i < 6; ++i)',at);s=s[:loop]+s[loop:].replace('for (int i = 0; i < 6; ++i)','for (int i = 0; i < 7; ++i)',1);write(p,s)
p=r/'jb-backend/src/download_queue.hpp';s=p.read_text();a=s.index('    #ifdef JB_UI_SINGLE_JOB');b=s.index('    explicit DownloadQueue',a);s=s[:a]+'    void set_parallel(unsigned value){parallel=(value==3||value==6||value==9||value==12)?value:1;}\n'+s[b:];s=s.replace('    unsigned count = 0, serial = 0;','    unsigned parallel=1;\n    unsigned count = 0, serial = 0;');write(p,s)
p=r/'src/app/service.hpp';s=p.read_text().replace('    std::atomic<bool> repository_refresh_{false};','    std::atomic<bool> repository_refresh_{false};\n    std::atomic<unsigned> maximum_downloads_{1};');write(p,s)
p=r/'src/app/jb_service.cpp';s=p.read_text().replace('bool Service::start(){if(started_)return true;','bool Service::start(){if(started_)return true;if(!root_.empty())maximum_downloads_=parse_settings(read_settings_file(root_)).maximum_downloads;').replace('settings_=std::move(text);settings_pending_=true;','maximum_downloads_=parse_settings(text).maximum_downloads;settings_=std::move(text);settings_pending_=true;').replace('  bool changed=jb_->queue->pump(', '  jb_->queue->set_parallel(maximum_downloads_.load());\n  bool changed=jb_->queue->pump(');write(p,s)
p=r/'sce_sys/param.json';j=json.loads(p.read_text());j['contentVersion']='01.000.101';write(p,json.dumps(j,indent=2)+'\n')
print('Live persisted download limit: 1 / 3 / 6 / 9 / 12')
