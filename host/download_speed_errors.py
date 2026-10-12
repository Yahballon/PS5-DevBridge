from pathlib import Path
import json
r=Path(r'C:/Users/GETRICH/Desktop/JB-STORE-ProsperoUI')
def write(p,s):p.write_text(s,encoding='utf-8',newline='\n')
p=r/'src/app/service.hpp';s=p.read_text().replace('    std::vector<std::string> waiting; // titles queued behind it','    std::vector<std::string> waiting; // titles queued behind it\n    double bytes_per_second = -1;');write(p,s)
p=r/'src/app/store.hpp';s=p.read_text().replace('    std::vector<std::string> waiting;\n};','    std::vector<std::string> waiting;\n    double bytes_per_second = -1;\n};',1).replace('    std::string time_left() const;','    std::string time_left() const;\n    std::string speed_text() const;');write(p,s)
p=r/'src/main.cpp';s=p.read_text().replace('                                 std::move(view.waiting)});','                                 std::move(view.waiting),view.bytes_per_second});');write(p,s)
p=r/'src/app/jb_service.cpp';s=p.read_text().replace('view.done=job.bytes;view.total=job.total;view.phase=','view.done=job.bytes;view.total=job.total;view.bytes_per_second=job.bytes_per_second;view.phase=').replace('std::strstr(job.stage,"Downloading")?install::Phase::downloading','(std::strstr(job.stage,"Downloading")||std::strstr(job.stage,"Paused")||std::strstr(job.stage,"Reconnecting"))?install::Phase::downloading');write(p,s)
p=r/'src/app/store.cpp';s=p.read_text().replace('    activity_ = std::move(activity);','    if(activity.bytes_per_second>=0)rate_=static_cast<float>(activity.bytes_per_second);\n    activity_ = std::move(activity);',1)
pos=s.index('// "about 40 s left"');s=s[:pos]+'''std::string Screen::speed_text() const
{
    if(activity_.id.empty()||activity_.phase!=static_cast<int>(install::Phase::downloading))return {};
    char text[64];std::snprintf(text,sizeof(text),"%.2f MB/s",std::max(0.0f,rate_)/1000000.0f);return text;
}

'''+s[pos:]
s=s.replace('            out.note = megabytes(activity_.done) + " of " + megabytes(activity_.total);','            out.note = megabytes(activity_.done) + " of " + megabytes(activity_.total);\n            if(!waiting){const auto speed=speed_text();if(!speed.empty())out.note+="  \\xC2\\xB7  "+speed;}')
s=s.replace('            out.note = waiting ? "Queued behind another app" : "One moment";','            out.note = waiting ? "Queued behind another app" : speed_text().empty()?"One moment":speed_text();')
s=s.replace('    if (const auto left = time_left(); !repo&&!left.empty() && progress >= 0.0f)','    if(!repo){const auto speed=speed_text();if(!speed.empty())line+="  \\xC2\\xB7  "+speed;}\n    if (const auto left = time_left(); !repo&&!left.empty() && progress >= 0.0f)')
old='''                if (const auto left = time_left(); !left.empty())
                    line += "  \\xC2\\xB7  " + left;
''';assert old in s;s=s.replace(old,'',1)
needle='            const Rect track{px + pw - 300.0f, row.cy() - 3.0f, 220.0f, 6.0f};'
s=s.replace(needle,'            if(working){std::string metrics=speed_text();const auto left=time_left();if(!left.empty()){if(!metrics.empty())metrics+="  \\xC2\\xB7  ";metrics+=left;}if(!metrics.empty())ui::text(list,fonts.regular,fonts.regular.font->fit(metrics,19,620.0f),tx,row.y+99.0f,19,kAccent);}\n'+needle)
s=s.replace('        if (ids.empty())\n        {','        if (ids.empty() && (history_.empty() || history_.back().ok))\n        {',1).replace('"FINISHED", px, y + 52.0f','"DOWNLOAD ACTIVITY", px, y + 52.0f',1)
old='''                ui::text(list, fonts.semibold, fonts.semibold.font->fit(done->title, 24, 520.0f),
                         px + 46.0f, y + 6.0f, 24, kInk);
                ui::text(list, fonts.regular, fonts.regular.font->fit(done->body, 21, pw - 600.0f),
                         px + 600.0f, y + 6.0f, 21, kInk.with_alpha(0.6f));
                y += 58.0f;'''
new='''                if(!done->ok){
                    ui::text(list,fonts.semibold,fonts.semibold.font->fit("Error: "+done->title,24,pw-46.0f),px+46.0f,y+6.0f,24,tone);
                    ui::paragraph(list,fonts.regular,done->body,px+46.0f,y+36.0f,20,pw-46.0f,27,kInk.with_alpha(0.88f),3);
                    y+=126.0f;
                }else{
                    ui::text(list, fonts.semibold, fonts.semibold.font->fit(done->title, 24, 520.0f),px + 46.0f, y + 6.0f, 24, kInk);
                    ui::text(list, fonts.regular, fonts.regular.font->fit(done->body, 21, pw - 600.0f),px + 600.0f, y + 6.0f, 21, kInk.with_alpha(0.6f));
                    y+=58.0f;
                }''';assert old in s;s=s.replace(old,new);write(p,s)
p=r/'sce_sys/param.json';j=json.loads(p.read_text());j['contentVersion']='01.000.100';write(p,json.dumps(j,indent=2)+'\n')
print('Download speed and visible error details added; version 01.000.100')
