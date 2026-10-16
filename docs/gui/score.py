import verovio, sys
NAMES=['C','C','D','D','E','F','F','G','G','A','A','B']
ALT=[0,1,0,1,0,0,1,0,1,0,1,0]
def pitch(m):
    s=NAMES[m%12]; a=ALT[m%12]; o=m//12-1
    return f"<pitch><step>{s}</step>{'<alter>1</alter>' if a else ''}<octave>{o}</octave></pitch>"
TYPES={1:'eighth',2:'quarter',3:'quarter',4:'half',8:'whole'}
def notes(events,staff,tuning,with_tab):
    out=[]
    for dur,pitches in events:
        for i,m in enumerate(pitches):
            chord='<chord/>' if i else ''
            dot='<dot/>' if dur==3 else ''
            tech=''
            if with_tab and m is not None:
                # lowest string that can play it, fret<=12 preferring low frets
                best=None
                for sidx,open_ in enumerate(tuning):
                    f=m-open_
                    if 0<=f<=12 and (best is None or f<best[1]): best=(len(tuning)-sidx,f)
                tech=f"<notations><technical><string>{best[0]}</string><fret>{best[1]}</fret></technical></notations>"
            if m is None:
                out.append(f"<note><rest/><duration>{dur}</duration><type>{TYPES[dur]}</type>{dot}<staff>{staff}</staff></note>")
            else:
                out.append(f"<note>{chord}{pitch(m + (12 if staff == 1 else 0))}<duration>{dur}</duration><type>{TYPES[dur]}</type>{dot}<staff>{staff}</staff>{tech}</note>")
    return ''.join(out)
def part(pid,name,clef,tuning,bars):
    steps=''.join(f'<staff-tuning line="{i+1}">{pitch(t).replace("pitch>","x>").replace("<x>","").replace("</x>","").replace("step>","tuning-step>").replace("alter>","tuning-alter>").replace("octave>","tuning-octave>")}</staff-tuning>' for i,t in enumerate(tuning))
    meas=[]
    for n,ev in enumerate(bars,1):
        attrs=''
        if n==1:
            attrs=f'''<attributes><divisions>2</divisions><key><fifths>0</fifths></key><time><beats>4</beats><beat-type>4</beat-type></time><staves>2</staves>{clef}<clef number="2"><sign>TAB</sign><line>5</line></clef><staff-details number="2"><staff-lines>{len(tuning)}</staff-lines>{steps}</staff-details></attributes>'''
            if pid=='P1': attrs+='<direction placement="above"><direction-type><words>≈112</words></direction-type><sound tempo="112"/></direction>'
        meas.append(f'<measure number="{n}">{attrs}{notes(ev,1,tuning,False)}<backup><duration>8</duration></backup>{notes(ev,2,tuning,True)}</measure>')
    return f'<part id="{pid}">{"".join(meas)}</part>'
G8='<clef number="1"><sign>G</sign><line>2</line><clef-octave-change>-1</clef-octave-change></clef>'
F8='<clef number="1"><sign>F</sign><line>4</line><clef-octave-change>-1</clef-octave-change></clef>'
g7=[32,37,42,47,52,56,61]; g6=[40,45,50,55,59,64]; b5=[20,25,30,35,40]
E=lambda d,*p:(d,list(p))
gtr1=[[E(1,32),E(1,32),E(1,35),E(1,32),E(1,32),E(1,37),E(1,36),E(1,32)],
      [E(1,32),E(1,32),E(1,35),E(1,32),E(2,32,39,44),E(2,35,42,47)],
      [E(1,32),E(1,32),E(1,35),E(1,32),E(1,32),E(1,37),E(1,36),E(1,32)],
      [E(3,37,44,49),E(1,37,44,49),E(4,32,39,44)]]
gtr2=[[E(4,None),E(2,52),E(1,55),E(1,57)],
      [E(3,59),E(1,57),E(1,55),E(1,52),E(2,50)],
      [E(2,52),E(1,52),E(1,55),E(2,57),E(1,59),E(1,62)],
      [E(8,59)]]
bass=[[E(2,20),E(2,20),E(2,23),E(1,20),E(1,25)],
      [E(2,20),E(2,20),E(2,27),E(2,23)],
      [E(2,20),E(2,20),E(2,23),E(1,20),E(1,25)],
      [E(3,25),E(1,25),E(4,20)]]
xml=f'''<?xml version="1.0" encoding="UTF-8"?>
<score-partwise version="4.0"><part-list>
<score-part id="P1"><part-name>Gitarre 1</part-name></score-part>
<score-part id="P2"><part-name>Gitarre 2</part-name></score-part>
<score-part id="P3"><part-name>Bass</part-name></score-part></part-list>
{part("P1","Gitarre 1",G8,g7,gtr1)}{part("P2","Gitarre 2",G8,g6,gtr2)}{part("P3","Bass",F8,b5,bass)}
</score-partwise>'''
open('riff.musicxml','w').write(xml)
tk=verovio.toolkit()
tk.setOptions({"pageWidth":int(sys.argv[1]),"pageHeight":int(sys.argv[2]),"scale":int(sys.argv[3]),"adjustPageHeight":True,"adjustPageWidth":True,"header":"none","footer":"none","pageMarginLeft":30,"pageMarginRight":30,"pageMarginTop":20,"pageMarginBottom":20,"svgViewBox":True,"breaks":"auto","spacingSystem":10,"spacingStaff":6})
assert tk.loadData(xml), "load failed"
print(tk.getPageCount(), file=sys.stderr)
open('score.svg','w').write(tk.renderToSVG(1))
