import json
import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scores'))
from compile_scores import load_scores,generate

class ScoreCompilerTests(unittest.TestCase):
    def test_catalog(self):
        scores=load_scores(Path(__file__).resolve().parents[1]/'scores')
        self.assertEqual({s[0]:len(s[4]) for s in scores},{'mary':26,'twinkle':42})
        self.assertEqual(scores[0][4][4],(76,96,2,0))
    def test_fractional_and_errors(self):
        d={'id':'x','title':'X','bpm':100,'time_signature':[3,8],'notes':[{'note':'C#4','beats':.5},{'note':'D4','beats':1},{'note':'E4','beats':.25}]}
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'x.json';p.write_text(json.dumps(d));out=Path(tmp)/'out.c';generate(tmp,out)
            self.assertEqual(load_scores(tmp)[0][4][-1],(64,24,2,0));self.assertIn('practice_scores',out.read_text())
            for invalid in ('C0','H4','REST',['C4','E4']):
                d['notes'][0]['note']=invalid;p.write_text(json.dumps(d));
                with self.assertRaises(ValueError):load_scores(tmp)
            d['notes'][0]={'note':'C4','beats':.001};p.write_text(json.dumps(d));
            with self.assertRaises(ValueError):load_scores(tmp)
            d['notes'][0]={'note':'C4','beats':1};p.write_text(json.dumps(d));(Path(tmp)/'y.json').write_text(json.dumps(d));
            with self.assertRaises(ValueError):load_scores(tmp)
if __name__=='__main__':unittest.main()
