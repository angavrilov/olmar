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
clean: idem/clean idem/clean2
	rm -f $(CLEAN_PATTERNS)
	rm -f lexer1 lexer1yy.cc
	rm -f lexer2 
	rm -f cc.bin ccgr ccparse libccgr.a 
	rm -f bccgr
	rm -f bccgr.y cc.tok
	rm -f ccparse_twalk

# ****************

# Check in the things I work on.
DSW_STUFF := Makefile cc_twalk.cc cc.ast check_idempotency chop_out

.PHONY: commit
commit:
	ci -l -m"no message" $(DSW_STUFF)

dsw_changes.tar.gz:
	rm -f $@;
	tar cvzf $@ $(DSW_STUFF)

# ****************

.PHONY: check
check: idem/check idem/check2

# Check idempotency of ccparse -tr prettyPrint on files in parsgen/cc.in.

TOCHECK :=
TOCHECK += cc.in1
TOCHECK += cc.in2
TOCHECK += cc.in3
TOCHECK += cc.in4
TOCHECK += cc.in5
TOCHECK += cc.in6
TOCHECK += cc.in7
TOCHECK += cc.in8
TOCHECK += cc.in9
TOCHECK += cc.in10
TOCHECK += cc.in11
TOCHECK += cc.in12
TOCHECK += cc.in13
TOCHECK += cc.in14
TOCHECK += cc.in14a
TOCHECK += cc.in15
TOCHECK += cc.in16
TOCHECK += cc.in17
TOCHECK += cc.in18
TOCHECK += cc.in19
TOCHECK += cc.in20
TOCHECK += cc.in21
TOCHECK += cc.in22
TOCHECK += cc.in23
TOCHECK += cc.in24

# Scope operator not being printed before using type.
#  A::B b;
#  class B b;
# TOCHECK += cc.in25

# DSW: Templates not being printed correctly when the template
# arguments are used within the template.
#TOCHECK += cc.in26
#TOCHECK += cc.in27
#TOCHECK += cc.in28

TOCHECK += cc.in29

# Anonymous classes and enums being printed incorrectly in different
# ways.  Wrote Scott if this is the parser or the printer.
#TOCHECK += cc.in30
# Smaller example of same problem.
#TOCHECK += cc.in30a

TOCHECK += cc.in31
TOCHECK += cc.in32
TOCHECK += cc.in33
TOCHECK += cc.in34

# template <class num>
# class TPoint {
# becomes:
# class TPoint {
# Plus has other template printing problems mentioned above.
#TOCHECK += cc.in35

# Same template printing problems.
#TOCHECK += cc.in36

TOCHECK += cc.in37
TOCHECK += cc.in38
TOCHECK += cc.in39
TOCHECK += cc.in40
TOCHECK += cc.in41
TOCHECK += cc.in42
TOCHECK += cc.in43

# That's all for parsgen/cc.in.

.PHONY: idem/check
idem/check: idem/clean
	./check_idempotency $(addprefix ../cc.in/,$(TOCHECK))

.PHONY: idem/clean
idem/clean:
	find ../cc.in -type f \
	\( -name '*.out_cpp'  \
	-o -name '*.out_raw'  -o -name '*.out' \
	-o -name '*.out2_raw' -o -name '*.out2' \
	-o -name '*.diff' \
	-o -name '*.out_raw'  -o -name '*.out' \
	-o -name '*.out2_raw' -o -name '*.out2' \
	-o -name '*.diff' \) | xargs rm -f

# Check idempotency of ccparse -tr prettyPrint on C and C++ files in
# cure_cpp/cc_examples.

# NOTE: Most of these fail because standard header files don't parse.

TOCHECK2 :=

TOCHECK2 += Box.cc

# Why this should be a parse error on the first pass, I don't know.
# Line 1322, col 18: Parse error at wchar_t
# typedef long int wchar_t;
# TOCHECK2 += JustHello.c

#TOCHECK2 += JustWorld.c
TOCHECK2 += Test_005true.c
#TOCHECK2 += Test_010hello.c
TOCHECK2 += Test_105true.cc

# anonymous structs not printing with {}.
#TOCHECK2 += Test_110hello.cc

#TOCHECK2 += Test_210box.cc
#TOCHECK2 += Test_220tbox.cc
#TOCHECK2 += Test_310hello.c
#TOCHECK2 += Test_315hello.c
#TOCHECK2 += Test_320hello_tbox.cc
#TOCHECK2 += Test_325hello_tbox.cc

#TOCHECK2 += calc/Test_100.cc
#TOCHECK2 += calc/Test_110.cc
#TOCHECK2 += calc/Test_200Lexer.cc
#TOCHECK2 += calc/Test_300Parser.cc
#TOCHECK2 += calc/Test_400Eval.cc
#TOCHECK2 += calc/calc.cc
#TOCHECK2 += calc/lexer.cc
#TOCHECK2 += calc/parser.cc
#TOCHECK2 += calc/util.cc

#TOCHECK2 += ch5/ex1.cc
#TOCHECK2 += ch5/ex3.cc
#TOCHECK2 += ch5/ex4.cc

#TOCHECK2 += ch7/overload.cc
#TOCHECK2 += ch7/va_args_ex.c

#TOCHECK2 += ch8/dc_except.cc
#TOCHECK2 += ch8/namespaces.cc

#TOCHECK2 += ch10/Arena.cc
#TOCHECK2 += ch10/NonLocal.cc
#TOCHECK2 += ch10/NonLocalB.cc
#TOCHECK2 += ch10/NonLocalC.cc
#TOCHECK2 += ch10/Person.cc
#TOCHECK2 += ch10/PlacementNew.cc
#TOCHECK2 += ch10/RefBox.cc
#TOCHECK2 += ch10/Test1.cc
TOCHECK2 += ch10/Test2.cc
#TOCHECK2 += ch10/Union.cc
#TOCHECK2 += ch10/test_ambig1.cc

#TOCHECK2 += ch11/apply.cc
#TOCHECK2 += ch11/arrow.cc
#TOCHECK2 += ch11/complex.cc
#TOCHECK2 += ch11/complex3.cc
#TOCHECK2 += ch11/day.cc
#TOCHECK2 += ch11/map.cc
#TOCHECK2 += ch11/my_ostream.cc
#TOCHECK2 += ch11/test1.cc
#TOCHECK2 += ch11/test2.cc
#TOCHECK2 += ch11/test3.cc
#TOCHECK2 += ch11/test4.cc
#TOCHECK2 += ch11/test5.cc

#TOCHECK2 += ch12/impl0_IO.cc
#TOCHECK2 += ch12/impl1_IO.cc
#TOCHECK2 += ch12/test1.cc
#TOCHECK2 += ch12/test2.cc
#TOCHECK2 += ch12/test3.cc
#TOCHECK2 += ch12/test4.cc

#TOCHECK2 += ch13/test1.cc
#TOCHECK2 += ch13/test10.cc
#TOCHECK2 += ch13/test11.cc
#TOCHECK2 += ch13/test12.cc
#TOCHECK2 += ch13/test13.cc
#TOCHECK2 += ch13/test2.cc
#TOCHECK2 += ch13/test3.cc
#TOCHECK2 += ch13/test4.cc
#TOCHECK2 += ch13/test5.cc
#TOCHECK2 += ch13/test6.cc
#TOCHECK2 += ch13/test7.cc
#TOCHECK2 += ch13/test8.cc
#TOCHECK2 += ch13/test9.cc

#TOCHECK2 += ch14/test1.cc
#TOCHECK2 += ch14/test2.cc
#TOCHECK2 += ch14/test3.cc
#TOCHECK2 += ch14/test4.cc
#TOCHECK2 += ch14/test5.cc
#TOCHECK2 += ch14/test6.cc
#TOCHECK2 += ch14/test7.cc
#TOCHECK2 += ch14/test8.cc

#TOCHECK2 += ch15/test1.cc
#TOCHECK2 += ch15/test10.cc
#TOCHECK2 += ch15/test11.cc
#TOCHECK2 += ch15/test12.cc
#TOCHECK2 += ch15/test13.cc
#TOCHECK2 += ch15/test14.cc
#TOCHECK2 += ch15/test15.cc
#TOCHECK2 += ch15/test16.cc
#TOCHECK2 += ch15/test2.cc
#TOCHECK2 += ch15/test3.cc
#TOCHECK2 += ch15/test3virtual.cc
#TOCHECK2 += ch15/test4.cc
#TOCHECK2 += ch15/test5.cc
#TOCHECK2 += ch15/test6.cc
#TOCHECK2 += ch15/test7.cc
#TOCHECK2 += ch15/test8.cc
#TOCHECK2 += ch15/test9.cc

#TOCHECK2 += eon/eon.cc
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

#TOCHECK2 += estl/00-1.cc
#TOCHECK2 += estl/02-1.cc
#TOCHECK2 += estl/02-2.cc
#TOCHECK2 += estl/02-3.cc
#TOCHECK2 += estl/02-4.cc
#TOCHECK2 += estl/02-5.cc

#TOCHECK2 += exceptions/except1.cc
#TOCHECK2 += exceptions/except2.cc
#TOCHECK2 += exceptions/except3.cc

#TOCHECK2 += infer_graph/test1.c
#TOCHECK2 += infer_graph/test2.c
#TOCHECK2 += infer_graph/test3.c
#TOCHECK2 += infer_graph/test4.c

#TOCHECK2 += laundering/test1.cc
#TOCHECK2 += laundering/test2.cc

#TOCHECK2 += stl/cout1.cc
#TOCHECK2 += stl/encrypt.cc
#TOCHECK2 += stl/iostream2.cc
#TOCHECK2 += stl/rectangle.cc
#TOCHECK2 += stl/sizes.cc
#TOCHECK2 += stl/stl1.cc
#TOCHECK2 += stl/structs.cc
#TOCHECK2 += stl/tinycout.cc
#TOCHECK2 += stl/triangle.cc
#TOCHECK2 += stl/tstring1.cc
#TOCHECK2 += stl/tstring2.cc

#TOCHECK2 += tests/Barrier_Test.merged.c

TESTDIR2 := ../../cure_cpp/cc_examples/
idem/check2: idem/clean2
	./check_idempotency $(addprefix $(TESTDIR2),$(TOCHECK2))

.PHONY: idem/clean2
idem/clean2:
	find $(TESTDIR2) -type f \
	\( -name '*.out_cpp'  \
	   -name '*.c.out_raw'   -o -name '*.c.out' \
	-o -name '*.c.out2_raw'  -o -name '*.c.out2' \
	-o -name '*.c.diff' \
	-o -name '*.cc.out_raw'  -o -name '*.cc.out' \
	-o -name '*.cc.out2_raw' -o -name '*.cc.out2' \
	-o -name '*.cc.diff' \) | xargs rm -f
