.SUFFIX:

# DIFF := diff -u
DIFF := diff -c -b

PR := ./ccparse

.PHONY: all
all: clean
# all: check_ast
all: check_type

TEST1 :=
TEST1 += t0001.cc
TEST1 += t0002.cc
TEST1 += t0003.cc
TEST1 += t0004.cc
TEST1 += t0005.cc
TEST1 += t0006.cc
TEST1 += t0007.cc
TEST1 += t0008.cc
TEST1 += t0009.cc
TEST1 += t0010.cc
TEST1 += t0011.cc
TEST1 += t0012.cc
TEST1 += t0013.cc
TEST1 += t0014.cc
TEST1 += t0014a.cc
TEST1 += t0015.cc
TEST1 += t0016.cc
TEST1 += t0017.cc
TEST1 += t0018.cc
TEST1 += t0019.cc
TEST1 += t0020.cc
TEST1 += t0021.cc
TEST1 += t0022.cc
TEST1 += t0023.cc
TEST1 += t0024.cc
TEST1 += t0025.cc
TEST1 += t0026.cc
TEST1 += t0027.cc
TEST1 += t0028.cc
TEST1 += t0029.cc
TEST1 += t0030.cc
TEST1 += t0030a.cc
TEST1 += t0030b.cc
TEST1 += t0031.cc
TEST1 += t0032.cc
TEST1 += t0033.cc
TEST1 += t0034.cc
TEST1 += t0035.cc
TEST1 += t0036.cc
TEST1 += t0037.cc
TEST1 += t0038.cc
TEST1 += t0039.cc
TEST1 += t0040.cc
TEST1 += t0041.cc
TEST1 += t0042.cc
TEST1 += t0043.cc
TEST1 += t0044.cc
TEST1 += t0045.cc
TEST1 += t0046.cc
TEST1 += t0047.cc
TEST1 += t0048.cc
TEST1 += t0049.cc
TEST1 += t0050.cc
TEST1 += t0051.cc
TEST1 += t0052.cc
TEST1 += t0053.cc
TEST1 += t0054.cc
TEST1 += t0055.cc
TEST1 += t0056.cc
TEST1 += t0057.cc
TEST1 += t0058.cc
TEST1 += t0059.cc
TEST1 += t0060.cc
TEST1 += t0061.cc
TEST1 += t0062.cc
TEST1 += t0063.cc
TEST1 += t0064.cc
TEST1 += t0065.cc
TEST1 += t0066.cc
TEST1 += t0067.cc
TEST1 += t0068.cc
TEST1 += t0069.cc
TEST1 += t0070.cc
TEST1 += t0071.cc
TEST1 += t0072.cc
TEST1 += t0073.cc
TEST1 += t0074.cc
TEST1 += t0075.cc
TEST1 += t0076.cc
TEST1 += t0077.cc
TEST1 += t0078.cc
TEST1 += t0079.cc
TEST1 += t0080.cc
TEST1 += t0081.cc
TEST1 += t0082.cc
TEST1 += t0083.cc
TEST1 += t0084.cc
TEST1 += t0085.cc
TEST1 += t0086.cc
TEST1 += t0087.cc
TEST1 += t0088.cc
TEST1 += t0089.cc
TEST1 += t0090.cc
TEST1 += t0091.cc
TEST1 += t0092.cc
TEST1 += t0093.cc
TEST1 += t0094.cc
TEST1 += t0095.cc
TEST1 += t0096.cc
TEST1 += t0097.cc
TEST1 += t0098.cc
TEST1 += t0099.cc
TEST1 += t0100.cc

TEST2 :=

TEST2 += t0001.cc
TEST2 += t0002.cc
TEST2 += t0003.cc
TEST2 += t0004.cc
TEST2 += t0005.cc
TEST2 += t0006.cc
TEST2 += t0007.cc
TEST2 += t0008.cc
TEST2 += t0009.cc
TEST2 += t0010.cc
TEST2 += t0011.cc
TEST2 += t0012.cc
TEST2 += t0013.cc
TEST2 += t0014.cc
TEST2 += t0014a.cc
TEST2 += t0015.cc
TEST2 += t0016.cc
TEST2 += t0017.cc
TEST2 += t0018.cc
TEST2 += t0019.cc
TEST2 += t0020.cc
TEST2 += t0021.cc
TEST2 += t0022.cc
TEST2 += t0023.cc
TEST2 += t0024.cc
TEST2 += t0025.cc
TEST2 += t0026.cc
TEST2 += t0027.cc
TEST2 += t0028.cc
TEST2 += t0029.cc
TEST2 += t0030.cc
TEST2 += t0030a.cc
TEST2 += t0030b.cc
TEST2 += t0031.cc
TEST2 += t0032.cc
TEST2 += t0033.cc
TEST2 += t0034.cc
TEST2 += t0035.cc
TEST2 += t0036.cc
TEST2 += t0037.cc
TEST2 += t0038.cc
TEST2 += t0039.cc
TEST2 += t0040.cc
TEST2 += t0041.cc
TEST2 += t0042.cc
TEST2 += t0043.cc
TEST2 += t0044.cc
TEST2 += t0045.cc
TEST2 += t0046.cc
TEST2 += t0047.cc
TEST2 += t0048.cc
TEST2 += t0049.cc
TEST2 += t0050.cc
TEST2 += t0051.cc
TEST2 += t0052.cc
TEST2 += t0053.cc
TEST2 += t0054.cc
TEST2 += t0055.cc
TEST2 += t0056.cc
TEST2 += t0057.cc
TEST2 += t0058.cc
TEST2 += t0059.cc
TEST2 += t0060.cc
TEST2 += t0061.cc
TEST2 += t0062.cc
TEST2 += t0063.cc
TEST2 += t0064.cc
TEST2 += t0065.cc
TEST2 += t0066.cc
TEST2 += t0067.cc
TEST2 += t0068.cc
TEST2 += t0069.cc
TEST2 += t0070.cc
TEST2 += t0071.cc
TEST2 += t0072.cc
TEST2 += t0073.cc
TEST2 += t0074.cc
TEST2 += t0075.cc
TEST2 += t0076.cc
TEST2 += t0077.cc
TEST2 += t0078.cc
TEST2 += t0079.cc
TEST2 += t0080.cc
TEST2 += t0081.cc
TEST2 += t0082.cc
TEST2 += t0083.cc
TEST2 += t0084.cc
TEST2 += t0085.cc
TEST2 += t0086.cc
TEST2 += t0087.cc
TEST2 += t0088.cc
TEST2 += t0089.cc
TEST2 += t0090.cc
TEST2 += t0091.cc
TEST2 += t0092.cc
TEST2 += t0093.cc
TEST2 += t0094.cc
TEST2 += t0095.cc
TEST2 += t0096.cc
TEST2 += t0097.cc
TEST2 += t0098.cc
TEST2 += t0099.cc
TEST2 += t0100.cc
TEST2 += t0101.cc
TEST2 += t0102.cc
TEST2 += t0103.cc
TEST2 += t0104.cc
TEST2 += t0105.cc
TEST2 += t0106.cc
TEST2 += t0107.cc
TEST2 += t0108.cc
TEST2 += t0108b.cc
TEST2 += t0109.cc
TEST2 += t0110.cc
TEST2 += t0111.cc
TEST2 += t0112.cc
TEST2 += t0113.cc
TEST2 += t0114.cc
TEST2 += t0115.cc
TEST2 += t0116.cc
TEST2 += t0117.cc
TEST2 += t0118.cc
TEST2 += t0119.cc
TEST2 += t0120.cc
TEST2 += t0121.cc
TEST2 += t0122.cc
TEST2 += t0123.cc
TEST2 += t0124.cc
TEST2 += t0125.cc
TEST2 += t0126.cc
TEST2 += t0127.cc
TEST2 += t0128.cc
TEST2 += t0129.cc
TEST2 += t0130.cc
TEST2 += t0131.cc
TEST2 += t0132.cc
TEST2 += t0133.cc
TEST2 += t0134.cc
TEST2 += t0135.cc
TEST2 += t0136.cc
TEST2 += t0137.cc
TEST2 += t0138.cc
TEST2 += t0139.cc
TEST2 += t0140.cc
TEST2 += t0141.cc
TEST2 += t0142.cc
TEST2 += t0143.cc
TEST2 += t0144.cc
TEST2 += t0145.cc
TEST2 += t0146.cc
TEST2 += t0147.cc
TEST2 += t0148.cc
TEST2 += t0149.cc
TEST2 += t0150.cc
TEST2 += t0151.cc
TEST2 += t0152.cc
TEST2 += t0153.cc
TEST2 += t0154.cc
TEST2 += t0155.cc
TEST2 += t0156.cc
TEST2 += t0157.cc
TEST2 += t0158.cc
TEST2 += t0159.cc
TEST2 += t0160.cc
TEST2 += t0161.cc
TEST2 += t0162.cc
TEST2 += t0163.cc
TEST2 += t0164.cc
TEST2 += t0165.cc
TEST2 += t0166.cc
TEST2 += t0167.cc
TEST2 += t0168.cc
TEST2 += t0169.cc
TEST2 += t0170.cc
TEST2 += t0171.cc
TEST2 += t0172.cc
TEST2 += t0173.cc
TEST2 += t0174.cc
TEST2 += t0175.cc
TEST2 += t0176.cc
TEST2 += t0177.cc
TEST2 += t0178.cc
TEST2 += t0179.cc
TEST2 += t0180.cc
TEST2 += t0181.cc
TEST2 += t0182.cc
TEST2 += t0183.cc
TEST2 += t0184.cc
TEST2 += t0185.cc
TEST2 += t0186.cc
TEST2 += t0187.cc
TEST2 += t0188.cc
TEST2 += t0189.cc
TEST2 += t0190.cc
TEST2 += t0191.cc
TEST2 += t0192.cc
TEST2 += t0193.cc
TEST2 += t0194.cc
TEST2 += t0195.cc
TEST2 += t0196.cc
TEST2 += t0197.cc
TEST2 += t0198.cc
TEST2 += t0200.cc
TEST2 += t0201.cc
TEST2 += t0202.cc
TEST2 += t0203.cc
TEST2 += t0204.cc
TEST2 += t0205.cc
TEST2 += t0206.cc
TEST2 += t0207.cc
TEST2 += t0208.cc
TEST2 += t0209.cc
TEST2 += t0210.cc
TEST2 += t0211.cc
TEST2 += t0212.cc
TEST2 += t0213.cc
TEST2 += t0214.cc
TEST2 += t0216.cc
TEST2 += t0217.cc
TEST2 += t0218.cc
TEST2 += t0219.cc
TEST2 += t0220.cc
TEST2 += t0221.cc
TEST2 += t0222.cc
TEST2 += t0223.cc
TEST2 += t0224.cc
TEST2 += t0225.cc
TEST2 += t0226.cc
TEST2 += t0227.cc
TEST2 += t0228.cc
TEST2 += t0228b.cc
TEST2 += t0229.cc
TEST2 += t0230.cc
TEST2 += t0231.cc
TEST2 += t0232.cc
TEST2 += t0233.cc
TEST2 += t0234.cc
TEST2 += t0235.cc
TEST2 += t0236.cc
TEST2 += t0237.cc
TEST2 += t0238.cc
TEST2 += t0239.cc
TEST2 += t0240.cc
TEST2 += t0241.cc
TEST2 += t0242.cc
TEST2 += t0243.cc
TEST2 += t0244.cc
TEST2 += t0245.cc
TEST2 += t0246.cc
TEST2 += t0247.cc
TEST2 += t0248.cc
TEST2 += t0249.cc
TEST2 += t0250.cc
TEST2 += t0251.cc
TEST2 += t0252.cc
TEST2 += t0253.cc

# FIX:
# TEST2 += t0254.cc

TEST2 += t0255.cc
TEST2 += t0256.cc
TEST2 += t0257.cc
TEST2 += t0258.cc
TEST2 += t0259.cc
TEST2 += t0260.cc
TEST2 += t0261.cc
TEST2 += t0262.cc
TEST2 += t0263.cc
TEST2 += t0264.cc
TEST2 += t0265.cc
TEST2 += t0266.cc
TEST2 += t0268.cc
TEST2 += t0268a.cc
TEST2 += t0269.cc
TEST2 += t0270.cc
TEST2 += t0271.cc
TEST2 += t0272.cc
TEST2 += t0273.cc
TEST2 += t0274.cc
TEST2 += t0275.cc
TEST2 += t0276.cc
TEST2 += t0277.cc
TEST2 += t0278.cc
TEST2 += t0280.cc
TEST2 += t0281.cc
TEST2 += t0282.cc
TEST2 += t0283.cc
TEST2 += t0284.cc
TEST2 += t0285.cc
TEST2 += t0286.cc
TEST2 += t0287.cc
TEST2 += t0288.cc
TEST2 += t0289.cc
TEST2 += t0290.cc
TEST2 += t0290a.cc
TEST2 += t0291.cc
TEST2 += t0292.cc
TEST2 += t0293.cc
TEST2 += t0294.cc
TEST2 += t0295.cc
TEST2 += t0296.cc
TEST2 += t0297.cc
TEST2 += t0298.cc
TEST2 += t0299.cc
TEST2 += t0300.cc
TEST2 += t0301.cc
TEST2 += t0302.cc
TEST2 += t0303.cc
TEST2 += t0304.cc
TEST2 += t0305.cc
TEST2 += t0306.cc
TEST2 += t0307.cc
TEST2 += t0308.cc
TEST2 += t0309.cc
TEST2 += t0310.cc
TEST2 += t0311.cc
TEST2 += t0312.cc
TEST2 += t0313.cc
TEST2 += t0314.cc
TEST2 += t0315.cc
TEST2 += t0316.cc
TEST2 += t0317.cc
TEST2 += t0318.cc
TEST2 += t0319.cc
TEST2 += t0320.cc
TEST2 += t0321.cc
TEST2 += t0322.cc
TEST2 += t0323.cc
TEST2 += t0324.cc
TEST2 += t0325.cc
TEST2 += t0326.cc
TEST2 += t0327.cc
TEST2 += t0328.cc
TEST2 += t0329.cc
TEST2 += t0330.cc
TEST2 += t0331.cc
TEST2 += t0332.cc
TEST2 += t0333.cc
TEST2 += t0334.cc
TEST2 += t0335.cc
TEST2 += t0336.cc
TEST2 += t0337.cc
TEST2 += t0338.cc
TEST2 += t0339.cc
TEST2 += t0340.cc
TEST2 += t0341.cc
TEST2 += t0342.cc
TEST2 += t0343.cc
TEST2 += t0344.cc
TEST2 += t0345.cc
TEST2 += t0346.cc
TEST2 += t0347.cc
TEST2 += t0348.cc
TEST2 += t0349.cc
TEST2 += t0350.cc
TEST2 += t0351.cc

# FIX:
# TEST2 += t0352.cc
# TEST2 += t0353.cc

TEST2 += t0354.cc
TEST2 += t0355.cc
TEST2 += t0356.cc
TEST2 += t0357.cc
TEST2 += t0358.cc
TEST2 += t0359.cc
TEST2 += t0360.cc
TEST2 += t0361.cc
TEST2 += t0362.cc
TEST2 += t0363.cc
TEST2 += t0364.cc
TEST2 += t0365.cc
TEST2 += t0366.cc
TEST2 += t0367.cc
TEST2 += t0368.cc
TEST2 += t0369.cc
TEST2 += t0370.cc
TEST2 += t0371.cc
TEST2 += t0372.cc
TEST2 += t0373.cc
TEST2 += t0374.cc
TEST2 += t0375.cc
TEST2 += t0376.cc
TEST2 += t0377.cc
TEST2 += t0378.cc
TEST2 += t0379.cc
TEST2 += t0380.cc
TEST2 += t0381.cc
TEST2 += t0382.cc
TEST2 += t0383.cc
TEST2 += t0384.cc
TEST2 += t0385.cc
TEST2 += t0386.cc
TEST2 += t0387.cc
TEST2 += t0388.cc
TEST2 += t0389.cc
TEST2 += t0390.cc
TEST2 += t0391.cc
TEST2 += t0392.cc
TEST2 += t0393.cc
TEST2 += t0394.cc
TEST2 += t0395.cc
TEST2 += t0396.cc
TEST2 += t0397.cc
TEST2 += t0398.cc
TEST2 += t0399.cc
TEST2 += t0400.cc
TEST2 += t0401.cc
TEST2 += t0402.cc
TEST2 += t0403.cc
TEST2 += t0404.cc
TEST2 += t0405.cc
TEST2 += t0406.cc
TEST2 += t0407.cc
TEST2 += t0408.cc
TEST2 += t0409.cc
TEST2 += t0410.cc
TEST2 += t0411.cc
TEST2 += t0412.cc
TEST2 += t0413.cc
TEST2 += t0414.cc
TEST2 += t0415.cc
TEST2 += t0416.cc
TEST2 += t0417.error2.cc
TEST2 += t0418.cc
TEST2 += t0419.cc
TEST2 += t0420.cc
TEST2 += t0421.cc
TEST2 += t0422.cc
TEST2 += t0422a.cc
TEST2 += t0423.cc
TEST2 += t0424.cc
TEST2 += t0425.cc
TEST2 += t0426.cc
TEST2 += t0427.cc
TEST2 += t0428.cc
TEST2 += t0429.cc
TEST2 += t0430.cc
TEST2 += t0431.cc
TEST2 += t0432.cc
TEST2 += t0433.cc
TEST2 += t0434.cc
TEST2 += t0437.cc
TEST2 += t0438.cc
TEST2 += t0438a.cc
TEST2 += t0441.cc
TEST2 += t0441a.cc
TEST2 += t0442.cc
TEST2 += t0443.cc
TEST2 += t0444.cc
TEST2 += t0445.cc
TEST2 += t0446.cc
TEST2 += t0447.cc
TEST2 += t0448.cc
TEST2 += t0449.cc
TEST2 += t0450.cc
TEST2 += t0451.cc
TEST2 += t0452.cc
TEST2 += t0453.cc
TEST2 += t0454.cc
TEST2 += t0455.cc
TEST2 += t0456.cc
TEST2 += t0457.cc
TEST2 += t0458.cc
TEST2 += t0459.cc
TEST2 += t0460.cc
TEST2 += t0461.cc
TEST2 += t0462.cc
TEST2 += t0463.cc
TEST2 += t0467.cc
TEST2 += t0468.cc
TEST2 += t0469.cc
TEST2 += t0470.cc
TEST2 += t0471.cc
TEST2 += t0472.error1.cc
TEST2 += t0473.cc
TEST2 += t0474.cc
TEST2 += t0475.cc
TEST2 += t0476.cc
TEST2 += t0477.cc
TEST2 += t0478.cc
TEST2 += t0479.cc
TEST2 += t0480.cc
TEST2 += t0481.cc
TEST2 += t0482.cc
TEST2 += t0483.cc
TEST2 += t0484.cc
TEST2 += t0485.cc
TEST2 += t0486.cc
TEST2 += t0487.cc
TEST2 += t0487b.cc
TEST2 += t0488.cc
TEST2 += t0489.cc
TEST2 += t0490.cc
TEST2 += t0491.cc
TEST2 += t0492.cc
TEST2 += t0493.cc
TEST2 += t0494.cc
TEST2 += t0495.cc
TEST2 += t0496.cc
TEST2 += t0497.cc
TEST2 += t0498.cc
TEST2 += t0499.cc
TEST2 += t0501.cc
TEST2 += t0502.cc
TEST2 += t0503.cc
TEST2 += t0504.cc
TEST2 += t0505.cc

# FIX:
# TEST2 += t0506.cc

# TEST2 += t0507.cc
# TEST2 += t0508.cc
# TEST2 += t0510.cc

# FIX:
# TEST2 += t0511.cc

# TEST2 += t0512.cc
# TEST2 += t0513.cc
# TEST2 += t0514.cc

# FIX:
# TEST2 += t0515.cc

# TEST2 += t0516.cc
# TEST2 += t0517.cc
# TEST2 += t0518.cc
# TEST2 += t0519.cc
# TEST2 += t0520.cc
# TEST2 += t0521.cc
# TEST2 += t0522.cc
# TEST2 += t0523.cc
# TEST2 += t0524.cc
# TEST2 += t0525.cc
# TEST2 += t0526.cc
# TEST2 += t0527.cc
# TEST2 += t0528.cc
# TEST2 += t0529.cc
# TEST2 += t0530.cc
# TEST2 += t0531.cc
# TEST2 += t0532.cc
# TEST2 += t0533.cc
# TEST2 += t0534.cc
# TEST2 += t0535.cc
# TEST2 += t0536.cc
# TEST2 += t0537.cc
# TEST2 += t0538.cc
# TEST2 += t0539.cc
# TEST2 += t0539_1.cc
# TEST2 += t0539_10.cc
# TEST2 += t0539_11.cc
# TEST2 += t0539_2.cc
# TEST2 += t0539_3.cc
# TEST2 += t0539_4.cc
# TEST2 += t0539_5.cc
# TEST2 += t0539_6.cc
# TEST2 += t0539_7.cc
# TEST2 += t0539_8.cc
# TEST2 += t0539_9.cc
# TEST2 += t0540.cc
# TEST2 += t0541.cc
# TEST2 += t0542.cc
# TEST2 += t0543.cc
# TEST2 += t0544.cc
# TEST2 += t0545.cc
# TEST2 += t0546.cc
# TEST2 += t0547.cc
# TEST2 += t0548.cc
# TEST2 += t0549.cc
# TEST2 += t0550.cc
# TEST2 += t0551.cc
# TEST2 += t0552.cc
# TEST2 += t0553.cc
# TEST2 += t0555.cc
# TEST2 += t0557.cc
# TEST2 += t0558.cc
# TEST2 += t0559.cc
# TEST2 += t0560.cc
# TEST2 += t0561.cc
# TEST2 += t0562.cc
# TEST2 += t0563.cc
# TEST2 += t0564.cc
# TEST2 += t0566.cc
# TEST2 += t0567.cc
# TEST2 += t0568.cc
# TEST2 += t0569.cc

TEST2 += d0001.cc
TEST2 += d0002.cc
TEST2 += d0003.cc
TEST2 += d0004.cc
TEST2 += d0005.cc
TEST2 += d0006.cc
TEST2 += d0007.cc
TEST2 += d0008.cc
TEST2 += d0009.cc
TEST2 += d0010.cc
TEST2 += d0011.cc
TEST2 += d0012.cc
TEST2 += d0013.cc
TEST2 += d0014.cc
TEST2 += d0015.cc
TEST2 += d0016.cc
TEST2 += d0017.cc
TEST2 += d0018.cc
TEST2 += d0019.cc
TEST2 += d0020.cc
TEST2 += d0021.cc
TEST2 += d0022.cc
TEST2 += d0023.cc
TEST2 += d0024.cc
TEST2 += d0025.cc
TEST2 += d0026.cc
TEST2 += d0027.cc
TEST2 += d0028.cc
TEST2 += d0029.cc
TEST2 += d0032.cc
TEST2 += d0034.cc
TEST2 += d0035.cc
TEST2 += d0036.cc
TEST2 += d0037.cc
TEST2 += d0038.cc
TEST2 += d0039.cc
TEST2 += d0046.cc
TEST2 += d0046elab.cc
TEST2 += d0047.cc
TEST2 += d0048.cc
TEST2 += d0048elab.cc
TEST2 += d0049.cc
TEST2 += d0050.cc
TEST2 += d0050elab.cc
TEST2 += d0051.cc
TEST2 += d0051elab.cc
TEST2 += d0052.cc
TEST2 += d0053.cc
TEST2 += d0054.cc
TEST2 += d0055.cc
TEST2 += d0056.cc
TEST2 += d0057.cc
TEST2 += d0058.cc
TEST2 += d0059.cc
TEST2 += d0060.cc
TEST2 += d0061.cc
TEST2 += d0064.cc
TEST2 += d0065.cc
TEST2 += d0066.cc
TEST2 += d0067.cc
TEST2 += d0068.cc
TEST2 += d0069.cc
TEST2 += d0070.cc
TEST2 += d0071.cc
TEST2 += d0072.cc
TEST2 += d0073.cc
TEST2 += d0074.cc
TEST2 += d0075.cc
TEST2 += d0079.cc
TEST2 += d0080.cc
TEST2 += d0084.cc
TEST2 += d0088.cc
TEST2 += d0089.cc
TEST2 += d0090.cc
TEST2 += d0091.cc
TEST2 += d0097.cc
TEST2 += d0098.cc
TEST2 += d0099.cc
TEST2 += d0100.cc
TEST2 += d0101.cc
TEST2 += d0102.cc
TEST2 += d0103.cc
TEST2 += d0104.cc
TEST2 += d0105.cc
TEST2 += d0106.cc
TEST2 += d0107.cc
TEST2 += d0108.cc
TEST2 += d0109.cc
TEST2 += d0110.cc

# FIX:
# TEST2 += d0111.cc
# TEST2 += d0112.cc

# TEST2 += d0113.cc
# TEST2 += d0114.cc
# TEST2 += d0116.cc
# TEST2 += d0117.cc
# TEST2 += d0118.cc
# TEST2 += d0119.cc
# TEST2 += d0120.cc
# TEST2 += d0124.cc
# TEST2 += k0001.cc
# TEST2 += k0002.cc
# TEST2 += k0003.cc
# TEST2 += k0004.cc
# TEST2 += k0005.cc
# TEST2 += k0005a.cc
# TEST2 += k0006.cc
# TEST2 += k0007.cc
# TEST2 += k0009.cc
# TEST2 += k0011.cc
# TEST2 += k0012.cc
# TEST2 += k0013.cc
# TEST2 += k0014.cc
# TEST2 += k0015.cc
# TEST2 += k0016.cc
# TEST2 += k0017.cc
# TEST2 += k0018.cc
# TEST2 += k0019.cc
# TEST2 += k0020.cc
# TEST2 += k0021.cc
# TEST2 += k0022.cc
# TEST2 += k0023.cc
# TEST2 += k0024.cc
# TEST2 += k0025.cc
# TEST2 += k0026.cc
# TEST2 += k0027.cc
# TEST2 += k0029.cc
# TEST2 += k0030.cc
# TEST2 += k0031.cc
# TEST2 += k0032.cc
# TEST2 += k0033.cc
# TEST2 += k0035.cc
# TEST2 += k0036.cc
# TEST2 += k0037.cc
# TEST2 += k0038.cc
# TEST2 += k0039.cc
# TEST2 += k0040.cc
# TEST2 += k0041.cc
# TEST2 += k0042.cc
# TEST2 += k0043.cc
# TEST2 += k0045.cc
# TEST2 += k0046.cc
# TEST2 += k0046a.cc
# TEST2 += k0047.cc
# TEST2 += k0048.cc
# TEST2 += k0049.cc
# TEST2 += k0050.cc
# TEST2 += k0051.cc
# TEST2 += k0052.cc
# TEST2 += k0053.cc
# TEST2 += k0054.cc
# TEST2 += k0055.cc
# TEST2 += k0056.cc
# TEST2 += k0057.cc
# TEST2 += k0058.cc
# TEST2 += sg0001.cc

TOCLEAN :=

# check parsing commutes with xml serialization
T1D := $(addprefix outdir/,$(TEST1))
TOCLEAN += outdir/*.B0.dp outdir/*.B0.dp_filtered outdir/*.B1.xml outdir/*.B1.xml_filtered outdir/*.B2.xml.dp outdir/*.B2.xml.dp_filtered outdir/*.B3.diff

# generate initial debug-print
$(addsuffix .B0.dp,$(T1D)): outdir/%.B0.dp: in/%
	$(PR) -tr no-elaborate,prettyPrint $< > $@
$(addsuffix .B0.dp_filtered,$(T1D)): outdir/%.B0.dp_filtered: outdir/%.B0.dp
	./chop_out < $< > $@

# generate xml print
$(addsuffix .B1.xml,$(T1D)): outdir/%.B1.xml: in/%
	$(PR) -tr no-elaborate,xmlPrintAST,xmlPrintAST-indent $< > $@
$(addsuffix .B1.xml_filtered,$(T1D)): outdir/%.B1.xml_filtered: outdir/%.B1.xml
	./chop_out < $< > $@

# parse xml and generate second debug-print
$(addsuffix .B2.xml.dp,$(T1D)): outdir/%.B2.xml.dp: outdir/%.B1.xml_filtered
	$(PR) -tr parseXml,no-elaborate,prettyPrint $< > $@
$(addsuffix .B2.xml.dp_filtered,$(T1D)): outdir/%.B2.xml.dp_filtered: outdir/%.B2.xml.dp
	./chop_out < $< > $@

# diff the two debug-prints
$(addsuffix .B3.diff,$(T1D)): outdir/%.B3.diff: outdir/%.B0.dp_filtered outdir/%.B2.xml.dp_filtered
# NOTE: do not, say, replace this with a pipe into 'tee' because that
# masks the return code and prevents make from stopping if there is a
# difference
	$(DIFF) $^ > $@

check_ast: $(addsuffix .B3.diff,$(T1D))

# check typechecking commutes with xml serialization
T2D := $(addprefix outdir/,$(TEST2))
TOCLEAN += outdir/*.C0.dp outdir/*.C0.dp_filtered outdir/*.C1.xml outdir/*.C1.xml_filtered outdir/*.C2.xml.dp outdir/*.C2.xml.dp_filtered outdir/*.C3.diff outdir/*.C4.xml

# generate initial debug-print
$(addsuffix .C0.dp,$(T2D)): outdir/%.C0.dp: in/%
	$(PR) -tr no-elaborate,printTypedAST $< > $@
$(addsuffix .C0.dp_filtered,$(T2D)): outdir/%.C0.dp_filtered: outdir/%.C0.dp
	./filter_loc < $< > $@

# generate xml print
$(addsuffix .C1.xml,$(T2D)): outdir/%.C1.xml: in/%
	$(PR) -tr no-elaborate,xmlPrintAST,xmlPrintAST-indent,xmlPrintAST-types $< > $@
$(addsuffix .C1.xml_filtered,$(T2D)): outdir/%.C1.xml_filtered: outdir/%.C1.xml
	./chop_out < $< > $@

# parse xml and generate second debug-print
$(addsuffix .C2.xml.dp,$(T2D)): outdir/%.C2.xml.dp: outdir/%.C1.xml_filtered
	$(PR) -tr parseXml,no-typecheck,no-elaborate,printAST $< > $@
$(addsuffix .C2.xml.dp_filtered,$(T2D)): outdir/%.C2.xml.dp_filtered: outdir/%.C2.xml.dp
	./filter_loc < $< > $@

# diff the two debug-prints
$(addsuffix .C3.diff,$(T2D)): outdir/%.C3.diff: outdir/%.C0.dp_filtered outdir/%.C2.xml.dp_filtered
# NOTE: do not, say, replace this with a pipe into 'tee' because that
# masks the return code and prevents make from stopping if there is a
# difference
	$(DIFF) $^ > $@

# # parse xml and generate second xml print
# $(addsuffix .C4.xml,$(T2D)): outdir/%.C4.xml: outdir/%.C1.xml_filtered
# 	$(PR) -tr parseXml,no-typecheck,no-elaborate,xmlPrintAST,xmlPrintAST-indent,xmlPrintAST-types $< > $@
# $(addsuffix .C4.xml_filtered,$(T2D)): outdir/%.C4.xml_filtered: outdir/%.C4.xml
# 	./chop_out < $< > $@

# # diff the two xml-prints
# $(addsuffix .C5.diff,$(T2D)): outdir/%.C5.diff: outdir/%.C1.xml_filtered outdir/%.C4.xml_filtered
# # NOTE: do not, say, replace this with a pipe into 'tee' because that
# # masks the return code and prevents make from stopping if there is a
# # difference
# 	./filter_ids < $(filter %.C1.xml_filtered,$^) > outdir/a1.xml
# 	./filter_ids < $(filter %.C4.xml_filtered,$^) > outdir/a4.xml
# 	$(DIFF) outdir/a1.xml outdir/a4.xml > $@

.PHONY: check_type
check_type: $(addsuffix .C3.diff,$(T2D))

# when it works, this test will find things that are being written but
# not read.  Right now, the order of printing is not canonical so it
# is not yet useful.
# check_type: $(addsuffix .C5.diff,$(T2D))

.PHONY: clean
clean:
	find outdir -type f -maxdepth 1 | grep -v .cvsignore | xargs rm -f

# this probably results in an arg too long error
#	rm -f $(TOCLEAN)
