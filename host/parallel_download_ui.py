from pathlib import Path
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
write(r/'src/app/download_metrics.hpp','''#pragma once
#include <string>
#include <cstdint>
namespace store { struct DownloadMetric {std::string id;std::uint64_t done=0,total=0;double speed=0;std::string stage;}; }
''')
for file in ['src/app/store.hpp','src/app/service.hpp']:
 p=r/file;s=p.read_text();s='#include "app/download_metrics.hpp"\n'+s;s=s.replace('    double bytes_per_second = -1;','    double bytes_per_second = -1;\n    std::vector<DownloadMetric> downloads;',1);write(p,s)
p=r/'src/main.cpp';s=p.read_text().replace('std::move(view.waiting),view.bytes_per_second});','std::move(view.waiting),view.bytes_per_second,std::move(view.downloads)});');write(p,s)
p=r/'src/app/jb_service.cpp';s=p.read_text().replace('if(job.phase==jbstore::DownloadPhase::running){if(view.id.empty())','if(job.phase==jbstore::DownloadPhase::running){view.downloads.push_back({id,job.bytes,job.total,job.bytes_per_second,job.stage});if(view.id.empty())');write(p,s)
p=r/'src/app/store.cpp';s=p.read_text();old='''        for (std::size_t i = 0; i < ids.size() && i < 4; ++i)''';new='''        const std::size_t first=queue_focus_>=4?static_cast<std::size_t>(queue_focus_-3):0;
        for (std::size_t i = first; i < ids.size() && i < first+4; ++i)''';assert old in s;s=s.replace(old,new,1)
old='''            const bool working = i == 0 && !activity_.id.empty();
            float progress = -1.0f;
            const char *phase = "Waiting";
            if (working)
                busy_app(progress, phase);'''
new='''            const auto metric=std::find_if(activity_.downloads.begin(),activity_.downloads.end(),[&](const DownloadMetric& job){return job.id==ids[i];});
            const bool primary=i==0&&!activity_.id.empty();
            const bool working=primary||metric!=activity_.downloads.end();
            float progress=metric!=activity_.downloads.end()&&metric->total?tween::clamp01(static_cast<float>(metric->done)/metric->total):-1.0f;
            const char* phase=metric!=activity_.downloads.end()?metric->stage.c_str():"Waiting";
            if(primary)busy_app(progress,phase);''';assert old in s;s=s.replace(old,new)
s=s.replace('size_text(activity_.done) + " of " +\n                        size_text(activity_.total);','size_text(metric!=activity_.downloads.end()?metric->done:activity_.done) + " of " +\n                        size_text(metric!=activity_.downloads.end()?metric->total:activity_.total);',1)
old='''            if(working){std::string metrics=speed_text();const auto left=time_left();if(!left.empty()){if(!metrics.empty())metrics+="  \\xC2\\xB7  ";metrics+=left;}if(!metrics.empty())ui::text(list,fonts.regular,fonts.regular.font->fit(metrics,19,620.0f),tx,row.y+99.0f,19,kAccent);}'''
new='''            if(working){
                std::string metrics=primary?speed_text():std::string{},left=primary?time_left():std::string{};
                if(!primary&&metric!=activity_.downloads.end()){
                    char rate[64];std::snprintf(rate,sizeof(rate),"%.2f MB/s",metric->speed/1000000.0);metrics=rate;
                    if(metric->speed>=1024&&metric->total>metric->done){const double seconds=(metric->total-metric->done)/metric->speed;left=seconds<60?"about "+std::to_string(static_cast<unsigned>(std::ceil(seconds)))+" s left":"about "+std::to_string(static_cast<unsigned>(std::ceil(seconds/60)))+" min left";}
                }
                if(!left.empty()){if(!metrics.empty())metrics+="  \\xC2\\xB7  ";metrics+=left;}if(!metrics.empty())ui::text(list,fonts.regular,fonts.regular.font->fit(metrics,19,620.0f),tx,row.y+99.0f,19,kAccent);
            }''';assert old in s;s=s.replace(old,new)
write(p,s)
print('Each parallel download displays its own progress/rate; list scrolls past four rows')
