.SUFFIXES:

.PHONY: all
all:
	cd ..; $(MAKE)

CLEAN_PATTERNS := \
  *.o  \
  *~ \
  *.d \
  *.exe \
  *.gen.* \
  *.bin \
  *.tab.* \
  *.output \
  a.out \
  core

.PHONY: clean
clean: idemclean
	rm -f $(CLEAN_PATTERNS)
	rm -f lexer1 lexer1yy.cc
	rm -f lexer2 
	rm -f cc.bin ccgr ccparse libccgr.a 
	rm -f bccgr
	rm -f bccgr.y cc.tok
	rm -f ccparse_twalk

# Check idempotency of ccparse -tr prettyPrint ****************

CHK_CCIN :=
CHK_CCIN += cc.in1
CHK_CCIN += cc.in2
CHK_CCIN += cc.in3
CHK_CCIN += cc.in4
CHK_CCIN += cc.in5
CHK_CCIN += cc.in6
CHK_CCIN += cc.in7
CHK_CCIN += cc.in8
CHK_CCIN += cc.in9
CHK_CCIN += cc.in10
CHK_CCIN += cc.in11
CHK_CCIN += cc.in12
CHK_CCIN += cc.in13
CHK_CCIN += cc.in14
CHK_CCIN += cc.in14a
CHK_CCIN += cc.in15
CHK_CCIN += cc.in16
CHK_CCIN += cc.in17
CHK_CCIN += cc.in18
CHK_CCIN += cc.in19
CHK_CCIN += cc.in20
CHK_CCIN += cc.in21
CHK_CCIN += cc.in22
CHK_CCIN += cc.in23
CHK_CCIN += cc.in24

# Scope operator not being printed before using type.
#  A::B b;
#  class B b;
# CHK_CCIN += cc.in25

CHK_CCIN += cc.in26

# DSW: Templates not being printed correctly when the template
# arguments are used within the template.
#CHK_CCIN += cc.in27
#CHK_CCIN += cc.in28

CHK_CCIN += cc.in29

# scope not printing.
#CHK_CCIN += cc.in30

CHK_CCIN += cc.in30a
CHK_CCIN += cc.in30b

CHK_CCIN += cc.in31
CHK_CCIN += cc.in32
CHK_CCIN += cc.in33
CHK_CCIN += cc.in34

# template <class num>
# class TPoint {
# becomes:
# class TPoint {
# Plus has other template printing problems mentioned above.
#CHK_CCIN += cc.in35

# Same template printing problems.
#CHK_CCIN += cc.in36

CHK_CCIN += cc.in37
CHK_CCIN += cc.in38
CHK_CCIN += cc.in39
CHK_CCIN += cc.in40
CHK_CCIN += cc.in41
CHK_CCIN += cc.in42
CHK_CCIN += cc.in43
CHK_CCIN += cc.in100
CHK_CCIN += cc.in101
CHK_CCIN += cc.in102

CHK_CCEX :=
CHK_CCEX += Box.cc
CHK_CCEX += JustHello.c
CHK_CCEX += JustWorld.c
CHK_CCEX += Test_005true.c
CHK_CCEX += Test_010hello.c
CHK_CCEX += Test_105true.cc
CHK_CCEX += Test_110hello.cc

CHK_CCEX += Test_210box.cc

# template
#CHK_CCEX += Test_220tbox.cc

CHK_CCEX += Test_310hello.c
CHK_CCEX += Test_315hello.c

# template
#CHK_CCEX += Test_320hello_tbox.cc

# template
#CHK_CCEX += Test_325hello_tbox.cc

# This uses STLport, which we don't use anymore.
#CHK_CALC :=
# CHK_CALC += calc/Test_100.cc
# CHK_CALC += calc/Test_110.cc
# CHK_CALC += calc/Test_200Lexer.cc
# CHK_CALC += calc/Test_300Parser.cc
# CHK_CALC += calc/Test_400Eval.cc
# CHK_CALC += calc/calc.cc
# CHK_CALC += calc/lexer.cc
# CHK_CALC += calc/parser.cc
# CHK_CALC += calc/util.cc

#Exception thrown: Assertion failed: !a->isReference(), file cc/cc_type.cc line 884
# Sent to Scott Oct 21
#CHK_CHAP += ch5/ex1.cc

CHK_CHAP += ch5/ex3.cc
CHK_CHAP += ch5/ex4.cc

#Line 5, col 90: Parse error at ;
#Sent to Scott Oct 21
#CHK_CHAP += ch7/overload.cc

#CHK_CHAP += ch7/va_args_ex.c

#CHK_CHAP += ch8/dc_except.cc

#Line 3154, col 1: Parse error at namespace
#CHK_CHAP += ch8/namespaces.cc

CHK_CHAP += ch10/Arena.cc
CHK_CHAP += ch10/NonLocal.cc
CHK_CHAP += ch10/NonLocalB.cc
#Line 3193, col 1: Parse error at namespace
#CHK_CHAP += ch10/NonLocalC.cc

CHK_CHAP += ch10/Person.cc
#CHK_CHAP += ch10/PlacementNew.cc
#CHK_CHAP += ch10/RefBox.cc
#CHK_CHAP += ch10/Test1.cc
CHK_CHAP += ch10/Test2.cc

#outdir/Union.cc.0cppout.cc:3166:12 error: unions cannot have member functions
#CHK_CHAP += ch10/Union.cc

CHK_CHAP += ch10/test_ambig1.cc

#CHK_CHAP += ch11/apply.cc
#CHK_CHAP += ch11/arrow.cc
CHK_CHAP += ch11/complex.cc
#CHK_CHAP += ch11/complex3.cc

#CHK_CHAP += ch11/day.cc
CHK_CHAP += ch11/map.cc
CHK_CHAP += ch11/my_ostream.cc

# CHK_CHAP += ch11/test1.cc
# CHK_CHAP += ch11/test2.cc
# CHK_CHAP += ch11/test3.cc
# CHK_CHAP += ch11/test4.cc
# CHK_CHAP += ch11/test5.cc

#CHK_CHAP += ch12/impl0_IO.cc
#CHK_CHAP += ch12/impl1_IO.cc
#CHK_CHAP += ch12/test1.cc
# CHK_CHAP += ch12/test2.cc
# CHK_CHAP += ch12/test3.cc
# CHK_CHAP += ch12/test4.cc

# CHK_CHAP += ch13/test1.cc
# CHK_CHAP += ch13/test10.cc
# CHK_CHAP += ch13/test11.cc
# CHK_CHAP += ch13/test12.cc
# CHK_CHAP += ch13/test13.cc
# CHK_CHAP += ch13/test2.cc
# CHK_CHAP += ch13/test3.cc
# CHK_CHAP += ch13/test4.cc
# CHK_CHAP += ch13/test5.cc
CHK_CHAP += ch13/test6.cc
# CHK_CHAP += ch13/test7.cc
# CHK_CHAP += ch13/test8.cc
# CHK_CHAP += ch13/test9.cc

# CHK_CHAP += ch14/test1.cc
# CHK_CHAP += ch14/test2.cc
CHK_CHAP += ch14/test3.cc
CHK_CHAP += ch14/test4.cc
# CHK_CHAP += ch14/test5.cc
# CHK_CHAP += ch14/test6.cc
# CHK_CHAP += ch14/test7.cc
# CHK_CHAP += ch14/test8.cc

# CHK_CHAP += ch15/test1.cc
# CHK_CHAP += ch15/test10.cc
# CHK_CHAP += ch15/test11.cc
# CHK_CHAP += ch15/test12.cc
# CHK_CHAP += ch15/test13.cc
#CHK_CHAP += ch15/test14.cc
#CHK_CHAP += ch15/test15.cc
#CHK_CHAP += ch15/test16.cc
#CHK_CHAP += ch15/test2.cc
#CHK_CHAP += ch15/test3.cc
#CHK_CHAP += ch15/test3virtual.cc
#CHK_CHAP += ch15/test4.cc
#CHK_CHAP += ch15/test5.cc
#CHK_CHAP += ch15/test6.cc
#CHK_CHAP += ch15/test7.cc
#CHK_CHAP += ch15/test8.cc
#CHK_CHAP += ch15/test9.cc

# TOCHECK2 += eon/eon.cc
#TOCHECK2 += eon/eonImageCalculator.cc
#TOCHECK2 += eon/ggBRDF.cc
#TOCHECK2 += eon/ggBST.cc
#TOCHECK2 += eon/ggBoardFloorSolidTexture.cc
#TOCHECK2 += eon/ggBox2.cc
#TOCHECK2 += eon/ggBox3.cc
#TOCHECK2 += eon/ggBoxPixelFilter.cc
#TOCHECK2 += eon/ggCamera.cc
#TOCHECK2 += eon/ggConductorMaterial.cc
#TOCHECK2 += eon/ggConstantMaterial.cc
#TOCHECK2 += eon/ggCookSample3.cc
#TOCHECK2 += eon/ggCoverageSolidTexture.cc
#TOCHECK2 += eon/ggDielectricMaterial.cc
#TOCHECK2 += eon/ggDiffuseBRDF.cc
#TOCHECK2 += eon/ggDiffuseMaterial.cc
#TOCHECK2 += eon/ggEdgeDiscrepancy.cc
#TOCHECK2 += eon/ggErr.cc
#TOCHECK2 += eon/ggFineSpectrum.cc
#TOCHECK2 += eon/ggFormat.cc
#TOCHECK2 += eon/ggFrame2.cc
#TOCHECK2 += eon/ggFrame3.cc
#TOCHECK2 += eon/ggFrameInterpolation.cc
#TOCHECK2 += eon/ggGamma.cc
#TOCHECK2 += eon/ggGeometry.cc
#TOCHECK2 += eon/ggGrayPixel_x.cc
#TOCHECK2 += eon/ggHAffineMatrix3.cc
#TOCHECK2 += eon/ggHBoxMatrix3.cc
#TOCHECK2 += eon/ggHMatrix3.cc
#TOCHECK2 += eon/ggHPerspectiveMatrix3.cc
#TOCHECK2 += eon/ggHPoint3.cc
#TOCHECK2 += eon/ggHReflectionMatrix3.cc
#TOCHECK2 += eon/ggHRigidBodyMatrix3.cc
#TOCHECK2 += eon/ggHRotationMatrix3.cc
#TOCHECK2 += eon/ggHScaleMatrix3.cc
#TOCHECK2 += eon/ggHShearMatrix3.cc
#TOCHECK2 += eon/ggHTranslationMatrix3.cc
#TOCHECK2 += eon/ggIO.cc
#TOCHECK2 += eon/ggJRSample2.cc
#TOCHECK2 += eon/ggJRSample3.cc
#TOCHECK2 += eon/ggJitterSample1.cc
#TOCHECK2 += eon/ggJitterSample2.cc
#TOCHECK2 += eon/ggJitterSample3.cc
#TOCHECK2 += eon/ggMaterial.cc
#TOCHECK2 += eon/ggMultiJitterSample2.cc
#TOCHECK2 += eon/ggMultiJitterSample3.cc
#TOCHECK2 += eon/ggNA.cc
#TOCHECK2 += eon/ggNRooksSample2.cc
#TOCHECK2 += eon/ggNRooksSample3.cc
#TOCHECK2 += eon/ggONB2.cc
#TOCHECK2 += eon/ggONB3.cc
#TOCHECK2 += eon/ggOakSolidTexture.cc
#TOCHECK2 += eon/ggOptics.cc
#TOCHECK2 += eon/ggPathDielectricMaterial.cc
#TOCHECK2 += eon/ggPermute.cc
#TOCHECK2 += eon/ggPhongBRDF.cc
#TOCHECK2 += eon/ggPinholeCamera.cc
#TOCHECK2 += eon/ggPixelFilter.cc
#TOCHECK2 += eon/ggPlane.cc
#TOCHECK2 += eon/ggPoint2.cc
#TOCHECK2 += eon/ggPoint3.cc
#TOCHECK2 += eon/ggPoissonDiskSample2.cc
#TOCHECK2 += eon/ggPoissonSphereSample3.cc
#TOCHECK2 += eon/ggPolishedBRDF.cc
#TOCHECK2 += eon/ggPolishedMaterial.cc
#TOCHECK2 += eon/ggPolygon.cc
#TOCHECK2 += eon/ggQuaternion.cc
#TOCHECK2 += eon/ggRGBE.cc
#TOCHECK2 += eon/ggRGBFPixel.cc
#TOCHECK2 += eon/ggRGBPixel.cc
#TOCHECK2 += eon/ggRGBPixel_x.cc
#TOCHECK2 += eon/ggRanNum.cc
#TOCHECK2 += eon/ggRandomSample2.cc
#TOCHECK2 += eon/ggRandomSample3.cc
#TOCHECK2 += eon/ggRange.cc
#TOCHECK2 += eon/ggRaster.cc
#TOCHECK2 += eon/ggRasterSurfaceTexture.cc
#TOCHECK2 += eon/ggRay2.cc
#TOCHECK2 += eon/ggRay3.cc
#TOCHECK2 += eon/ggRegularSample2.cc
#TOCHECK2 += eon/ggRegularSample3.cc
#TOCHECK2 += eon/ggRotatingPinholeCamera.cc
#TOCHECK2 += eon/ggSolidNoise2.cc
#TOCHECK2 += eon/ggSolidNoise3.cc
#TOCHECK2 += eon/ggSolidTexture.cc
#TOCHECK2 += eon/ggSpectrum.cc
#TOCHECK2 += eon/ggSpecularMaterial.cc
#TOCHECK2 += eon/ggSphere.cc
#TOCHECK2 += eon/ggSpline.cc
#TOCHECK2 += eon/ggStaticArray.cc
#TOCHECK2 += eon/ggString.cc
#TOCHECK2 += eon/ggThinLensCamera.cc
#TOCHECK2 += eon/ggTimer.cc
#TOCHECK2 += eon/ggTrain.cc
#TOCHECK2 += eon/ggTrianglePixelFilter.cc
#TOCHECK2 += eon/ggVector2.cc
#TOCHECK2 += eon/ggVector3.cc
#TOCHECK2 += eon/mrBox.cc
#TOCHECK2 += eon/mrBruteForcePixelRenderer.cc
#TOCHECK2 += eon/mrCamera.cc
#TOCHECK2 += eon/mrChiuPixelRenderer.cc
#TOCHECK2 += eon/mrCoarsePixelRenderer.cc
#TOCHECK2 += eon/mrCookPixelRenderer.cc
#TOCHECK2 += eon/mrDiffuseAreaSphereLuminaire.cc
#TOCHECK2 += eon/mrDiffuseAreaTriangleLuminaire.cc
#TOCHECK2 += eon/mrDiffuseAreaXYRectangleLuminaire.cc
#TOCHECK2 += eon/mrDiffuseAreaXZRectangleLuminaire.cc
#TOCHECK2 += eon/mrDiffuseAreaYZRectangleLuminaire.cc
#TOCHECK2 += eon/mrDiffuseAreaZCylinderLuminaire.cc
#TOCHECK2 += eon/mrDiffuseCosineSphereLuminaire.cc
#TOCHECK2 += eon/mrDiffuseCosineZCylinderLuminaire.cc
#TOCHECK2 += eon/mrDiffuseSolidAngleSphereLuminaire.cc
#TOCHECK2 += eon/mrDiffuseVisibleAreaZCylinderLuminaire.cc
#TOCHECK2 += eon/mrEmitter.cc
#TOCHECK2 += eon/mrFastTriangle.cc
#TOCHECK2 += eon/mrGrid.cc
#TOCHECK2 += eon/mrImposter.cc
#TOCHECK2 += eon/mrIndirectPixelRenderer.cc
#TOCHECK2 += eon/mrInstance.cc
#TOCHECK2 += eon/mrKajiyaPixelRenderer.cc
#TOCHECK2 += eon/mrLinkedObjects.cc
#TOCHECK2 += eon/mrMaterial.cc
#TOCHECK2 += eon/mrObjectRecord.cc
#TOCHECK2 += eon/mrPhongAreaTriangleLuminaire.cc
#TOCHECK2 += eon/mrPhongAreaXYRectangleLuminaire.cc
#TOCHECK2 += eon/mrPhongAreaXZRectangleLuminaire.cc
#TOCHECK2 += eon/mrPhongAreaYZRectangleLuminaire.cc
#TOCHECK2 += eon/mrPolygon.cc
#TOCHECK2 += eon/mrRushmeierPixelRenderer.cc
#TOCHECK2 += eon/mrScene.cc
#TOCHECK2 += eon/mrShellLuminaire.cc
#TOCHECK2 += eon/mrSolidTexture.cc
#TOCHECK2 += eon/mrSphere.cc
#TOCHECK2 += eon/mrSpotAreaXYDiskLuminaire.cc
#TOCHECK2 += eon/mrSurface.cc
#TOCHECK2 += eon/mrSurfaceList.cc
#TOCHECK2 += eon/mrSurfaceTexture.cc
#TOCHECK2 += eon/mrTriangle.cc
#TOCHECK2 += eon/mrXCylinder.cc
#TOCHECK2 += eon/mrXEllipticalCylinder.cc
#TOCHECK2 += eon/mrXYDisk.cc
#TOCHECK2 += eon/mrXYRectangle.cc
#TOCHECK2 += eon/mrXZDisk.cc
#TOCHECK2 += eon/mrXZRectangle.cc
#TOCHECK2 += eon/mrYCylinder.cc
#TOCHECK2 += eon/mrYEllipticalCylinder.cc
#TOCHECK2 += eon/mrYZDisk.cc
#TOCHECK2 += eon/mrYZRectangle.cc
#TOCHECK2 += eon/mrZCylinder.cc
#TOCHECK2 += eon/mrZEllipticalCylinder.cc
#TOCHECK2 += eon/myrand.cc

# CHK_OTHER1 += estl/00-1.cc
# CHK_OTHER1 += estl/02-1.cc

#WARNING: there is no action to merge nonterm Expression
#CHK_OTHER1 += estl/02-2.cc

#CHK_OTHER1 += estl/02-3.cc
#CHK_OTHER1 += estl/02-4.cc
#CHK_OTHER1 += estl/02-5.cc

#CHK_OTHER1 += exceptions/except1.cc
#CHK_OTHER1 += exceptions/except2.cc
#CHK_OTHER1 += exceptions/except3.cc

CHK_OTHER1 += infer_graph/test1.c
CHK_OTHER1 += infer_graph/test2.c
CHK_OTHER1 += infer_graph/test3.c
CHK_OTHER1 += infer_graph/test4.c

CHK_OTHER1 += laundering/test1.cc
CHK_OTHER1 += laundering/test2.cc

#CHK_OTHER1 += stl/cout1.cc
#CHK_OTHER1 += stl/encrypt.cc
#CHK_OTHER1 += stl/iostream2.cc
#CHK_OTHER1 += stl/rectangle.cc
#CHK_OTHER1 += stl/sizes.cc
#CHK_OTHER1 += stl/stl1.cc
#CHK_OTHER1 += stl/structs.cc
#CHK_OTHER1 += stl/tinycout.cc
#CHK_OTHER1 += stl/triangle.cc
#CHK_OTHER1 += stl/tstring1.cc
#CHK_OTHER1 += stl/tstring2.cc

#CHK_OTHER1 += tests/Barrier_Test.merged.c

.PHONY: idemcheck
idemcheck: idem/cc.in idem/cc_examples idem/chap idem/other1

DIR_CCIN := ../cc.in/
.PHONY: idem/cc.in
idem/cc.in:; ./idemcheck -d outdir $(addprefix $(DIR_CCIN),$(CHK_CCIN))

DIR_CCEX := ../../cure_cpp/cc_examples/
.PHONY: idem/cc_examples
idem/cc_examples:; ./idemcheck -d outdir $(addprefix $(DIR_CCEX),$(CHK_CCEX))
# .PHONY: idem/calc
# idem/calc:; ./idemcheck -d outdir $(addprefix $(DIR_CCEX),$(CHK_CALC))
.PHONY: idem/chap
idem/chap:; ./idemcheck -d outdir $(addprefix $(DIR_CCEX),$(CHK_CHAP))
.PHONY: idem/other1
idem/other1:; ./idemcheck -d outdir $(addprefix $(DIR_CCEX),$(CHK_OTHER1))

# Clean
TEST_SUFFIXES_TO_CLEAN := 0cppout.cc 1rawout.cc 2out.cc 3rawout.cc 4out.cc 5diff
TEST_PATTERNS_TO_CLEAN := $(patsubst %,-o -name '*.%',$(TEST_SUFFIXES_TO_CLEAN))
.PHONY: idemclean
idemclean:
	find outdir -type f \( -false $(TEST_PATTERNS_TO_CLEAN) \) | xargs rm -f
