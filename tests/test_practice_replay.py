"""Insert a deliberate wrong note into a monophonic practice take and verify recovery."""
from array import array
import math
import subprocess
import sys
from score_fixtures import SCORES

def tone(midi,seconds):
    n=round(seconds*16000);f=440*2**((midi-69)/12);fade=160
    values=((.2*math.sin(math.tau*f*i/16000)+.08*math.sin(2*math.tau*f*i/16000))*min(1,i/fade,(n-1-i)/fade) for i in range(n))
    return array('f',values)

def main():
    sequence=list(SCORES['twinkle']);sequence.insert(2,(65,1))
    audio=array('f')
    for midi,beats in sequence:audio.extend(tone(midi,beats*.6))
    audio.extend(array('f',[0])*16000)
    result=subprocess.check_output([sys.argv[1],'twinkle'],input=audio.tobytes()).decode()
    errors=[line for line in result.splitlines() if line.startswith('ERROR ')]
    assert errors==['ERROR song=twinkle note=3/42 bar=1 beat=3 expected=G4 actual=F4'],result
    assert 'SUMMARY 42/42 errors=1 complete=1' in result,result
    print(errors[0]);print('Corrected performance completes with exactly one recorded error')
if __name__=='__main__':main()
