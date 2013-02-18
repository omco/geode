#!/usr/bin/env python

from __future__ import division
from other.core import *
from other.core.exact import *
import sys

def test_predicates():
  predicate_tests(4096)

def test_delaunay(benchmark=False,cgal=False):
  random.seed(8711)
  for n in range(3,10)+[50,100,563,1025,2000,-2000]+benchmark*[1<<20,-1<<20]:
    if n<0: continue
    X = random.randn(n,2) if n>0 else zeros((-n,2))
    with Log.scope('delaunay %d'%n):
      mesh = delaunay_points(X,validate=not benchmark)
      if not benchmark:
        mesh.assert_consistent()
        assert mesh.n_vertices==abs(n)
        assert len(mesh.boundary_loops())==1
      if 0:
        print 'tris = %s'%compact_str(mesh.elements())
        print 'X = %s'%compact_str(X)
    if cgal:
      from other.tim import cgal
      with Log.scope('cgal delaunay %d'%n):
        nf = cgal.cgal_time_delaunay_points(X)
        if n>0:
          assert nf==mesh.n_faces

if __name__=='__main__':
  Log.configure('exact tests',0,0,100)
  if '-b' in sys.argv:
    test_delaunay(True)
  else:
    test_predicates()
    test_delaunay()
