#!/usr/bin/env ruby
#
# $Id$
#
# Mobility test for grid c lib

exit 1 unless system 'ruby makeRubyExtension.rb Grid adj.c gridStruct.h master_header.h'
exit 1 unless system 'ruby makeRubyExtension.rb GridMetric adj.c grid.c gridStruct.h master_header.h'

require 'test/unit'
require 'Grid/Grid'
require 'GridMetric/GridMetric'

class Grid
 include GridMetric
end

class TestGridMetric < Test::Unit::TestCase

 def rightTet
  grid = Grid.new(4,1,0,0)
  grid.addCell( 
	       grid.addNode(0.0,0.0,0.0), 
	       grid.addNode(1.0,0.0,0.0), 
	       grid.addNode(0.0,1.0,0.0), 
	       grid.addNode(0.0,0.0,1.0) )
 end

 def isoTet
  grid = Grid.new(4,1,0,0)
  grid.addNode( 0.000, 0.000, 0.000 )
  grid.addNode( 1.000, 0.000, 0.000 )
  grid.addNode( 0.500, 0.866, 0.000 )
  grid.addNode( 0.500, 0.289, 0.823 ) 
  grid.addCell(0,1,2,3)
 end

 def testIsotropicTet
  assert_in_delta 1.000, isoTet.minAR, 1.0e-4
 end

 def testEdgeLength
  assert_not_nil grid = Grid.new(2,0,0,0)
  assert_equal 0, grid.addNode(0.0,0.0,0.0)
  assert_equal 1, grid.addNode(0.0,0.0,2.0)
  assert_in_delta 2.0, grid.edgeLength(0,1), 1.0e-15
 end

 def testAverageEdgeLength
  assert_not_nil grid = isoTet
  4.times do |i| 
   assert_in_delta 1.0, grid.averageEdgeLength(i), 3.0e-3, "node #{i} length"
  end
 end

 def testSpacingFunction
  assert_not_nil grid = isoTet
  4.times do |i| 
   assert_in_delta 0.0, grid.spacing(i), 1.0e-15, "node #{i} spacing"
  end
  assert_equal grid, grid.resetSpacing
  4.times do |i| 
   assert_in_delta grid.averageEdgeLength(i), grid.spacing(i), 1.0e-15, "n#{i}"
  end
 end

 def testMetrics
  assert_not_nil grid = rightTet
  nodes = [0,1,2,3]
  ar = 0.732050807568877
  assert_in_delta 1.0/6.0, grid.volume(nodes), 1.0e-15
  assert_in_delta 1.0/6.0, grid.minVolume, 1.0e-15
  assert_in_delta ar, grid.ar(nodes), 1.0e-15
  assert_in_delta ar, grid.minAR, 1.0e-15
  assert_in_delta ar, grid.nodeAR(0), 1.0e-15
  assert_in_delta ar, grid.nodeAR(1), 1.0e-15
  assert_in_delta ar, grid.nodeAR(2), 1.0e-15
  assert_in_delta ar, grid.nodeAR(3), 1.0e-15
 end

 def testDerivatives
  assert_not_nil grid = Grid.new(4,1,0,0)

  assert_equal grid, grid.addCell( 
				  grid.addNode(0.0,0.0,0.0), 
				  grid.addNode(1.0,0.0,0.0), 
				  grid.addNode(0.0,1.0,0.0), 
				  grid.addNode(0.0,0.0,1.0) )
  nodes = [0,1,2,3]
  ar = 0.732050807568877
  deriv = -2.0/3.0

  ans = grid.cellARDerivative(nodes)
  assert_in_delta ar,    ans[0], 1.0e-15
  assert_in_delta deriv, ans[1], 1.0e-15
  assert_in_delta deriv, ans[2], 1.0e-15
  assert_in_delta deriv, ans[3], 1.0e-15  

  ans = grid.nodeARDerivative(0)
  assert_in_delta ar,    ans[0], 1.0e-15
  assert_in_delta deriv, ans[1], 1.0e-15
  assert_in_delta deriv, ans[2], 1.0e-15
  assert_in_delta deriv, ans[3], 1.0e-15  

  deriv = 1.0/3.0

  ans = grid.nodeARDerivative(1)
  assert_in_delta ar,    ans[0], 1.0e-15
  assert_in_delta 0.0,   ans[1], 1.0e-15
  assert_in_delta deriv, ans[2], 1.0e-15
  assert_in_delta deriv, ans[3], 1.0e-15  

  ans = grid.nodeARDerivative(2)
  assert_in_delta ar,    ans[0], 1.0e-15
  assert_in_delta deriv, ans[1], 1.0e-15
  assert_in_delta 0.0,   ans[2], 1.0e-15
  assert_in_delta deriv, ans[3], 1.0e-15  

  ans = grid.nodeARDerivative(3)
  assert_in_delta ar,    ans[0], 1.0e-15
  assert_in_delta deriv, ans[1], 1.0e-15
  assert_in_delta deriv, ans[2], 1.0e-15
  assert_in_delta 0.0,   ans[3], 1.0e-15  
 end

 def testRightHandedFaces
  assert_not_nil grid = Grid.new(4,1,2,0)
  assert_equal grid, grid.
   addCell( grid.addNode(0.0,0.0,0.0), grid.addNode(1.0,0.0,0.0), 
	    grid.addNode(0.0,1.0,0.0), grid.addNode(0.0,0.0,1.0) )
  assert_equal grid,  grid.addFace(0,1,2,11)
  assert_equal true,  grid.rightHandedFace(0)
  assert_equal grid,  grid.addFace(0,2,3,11)
  assert_equal true,  grid.rightHandedBoundary
  assert_equal grid,  grid.removeFace(grid.findFace(0,1,2))
  assert_equal grid,  grid.addFace(0,2,1,11)
  assert_equal false, grid.rightHandedFace(0)
  assert_equal false, grid.rightHandedBoundary
 end
 
end
