import math

# Round x to 'digits' significant digits and return it.
def signif(x, digits=6):
  if x == 0 or not math.isfinite(x):
    return x
  digits -= math.ceil(math.log10(abs(x)))
  return round(x, digits)

# Functions to convert between Kelvin, Celsius, and Fahrenheit.
# Degree argument can be a number or a list of numbers, and
# the return value is a number or list, respectively.
def degFtoC(degF):
  if not isinstance(degF, list): return (degF-32)*5/9
  return [degFtoC(degF[i]) for i in range(len(degF))]
def degCtoF(degC):
  if not isinstance(degC, list): return degC*9/5+32
  return [degCtoF(degC[i]) for i in range(len(degC))]
def degCtoK(degC):
  if not isinstance(degC, list): return degC+273.15
  return [degCtoK(degC[i]) for i in range(len(degC))]
def degKtoC(degK):
  if not isinstance(degK, list): return degK-273.15
  return [degKtoC(degK[i]) for i in range(len(degK))]

#print("20  defFtoC(68) = " + str(degFtoC(68)))
#print("0 100  defFtoC([32, 212]) = " + str(degFtoC([32, 212])))
#print("50  defCtoF(10) = " + str(degCtoF(10)))
#print("32 212  defCtoF([0, 100]) = " + str(degCtoF([0, 100])))
#print("273.15  defCtoK(0) = " + str(degCtoK(0)))
#print("273.15 0  defCtoK([0, -273.15]) = " + str(degCtoK([0, -273.15])))
#print("-273.15  defKtoC(0) = " + str(degKtoC(0)))
#print("-273.15 0  defKtoC([0, 273.15]) = " + str(degKtoC([0, 273.15])))

# Compute temperature T from thermistor resistances (rt, in ohms) and thermistor
# coefficients A/B/C. The rt argument can be a number or a list of numbers, and
# the return value is a number or list, respectively. Print the returned values
# if printVals=True.
#   T_C returns Celsius degrees
#   T_F returns Fahrenheit degrees
def T_C(rt, A, B, C, printVals=True):
  if not isinstance(rt, list):
    TC = degKtoC(1/(A + B*math.log(rt) + C*math.log(rt)**3))
    TC = signif(TC, 4)
    if printVals:
      print(" " + str(TC))
    return TC
  return [T_C(rt[i], A, B, C, printVals) for i in range(len(rt))]
def T_F(rt, A, B, C, printVals=True):
    TF = degCtoF(T_C(rt, A, B, C, False))
    TF = signif(TF, 4)
    if printVals:
        print(" " + str(TF))
    return TF

#print("25°  T_C(10000, A = 0.001029, B = 0.0002391, C = 1.566e-07) = " +
#    str(T_C(10000, A = 0.001029, B = 0.0002391, C = 1.566e-07)))
#print("0°, 25°, 50°  T_C([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07) = " +
#    str(T_C([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07)))
#print("77°  T_F(10000, A = 0.001029, B = 0.0002391, C = 1.566e-07) = " +
#    str(T_F(10000, A = 0.001029, B = 0.0002391, C = 1.566e-07)))
#print("32°, 77°, 122°  T_F([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07) = " +
#    str(T_F([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07)))


# Given Vadc = ADC value(s) of the voltage at the connection between an unknown
# resistance Rt (the thermistor) and a series resistance rs, with a maximum ADC
# reading of Vmax, this returns thermistor resistance Rt. Vout can be a number or
# list of numbers, and the return value is a number or list, respectively, of
# thermistor resistance(s).
# This assumes that the ADC reference voltage is at the other side of the
# thermistor and that the other side of the series resistor is grounded.
def Rt_fromADCvalues(Vout, Vmax, rs):
  if not isinstance(Vout, list):
    return round(rs*(Vmax/Vout - 1))
  return [Rt_fromADCvalues(Vout[i], Vmax, rs) for i in range(len(Vout))]

#print("230  Rt_fromADCvalues(1000, 1023, 10000) = " + str(Rt_fromADCvalues(1000, 1023, 10000)))
#print("12000, 230  Rt_fromADCvalues([465, 1000], 1023, 10000) = " +
#    str(Rt_fromADCvalues([465, 1000], 1023, 10000)))

# Given a thermistor resistance rt, a series resistance rs, and a maximum ADC reading
# of Vmax, this computes the ADC reading at the junction between the resistors.
# The rt argument can be a number or list, and the return value is a number or
# list of numbers, respectively, of ADC junction readings.
def Vout_ADC(rt, rs, Vmax):
  if not isinstance(rt, list):
    return round(Vmax*rs/(rs+rt))
  return [Vout_ADC(rt[i], rs, Vmax) for i in range(len(rt))]

#print("465  Vout_ADC(12000, 10000, 1023) = " + str(Vout_ADC(12000, 10000, 1023)))
#print("1000, 465  Vout_ADC([230, 12000], 10000, 1023) = " + str(Vout_ADC([230, 12000], 10000, 1023)))

# Given temperature TC in Celsius and thermistor A/B/C coefficients, compute
# expected thermistor resistance. The TC argument can be a number or a list of
# numbers, and the return value is a number or list, respectively, of thermistor
# resistance(s). Print returned values if printVals=True.
def Rt_fromTC_ABC(TC, A, B, C, printVals=True):
  if not isinstance(TC, list):
    y = (A-1/degCtoK(TC))/(2*C)
    x = y + math.sqrt((B/(3*C))**3 + y**2)
    x13 = x**(1/3)
    lnR = B/(3*C*x13) - x13
    R = round(math.exp(lnR))
    if printVals:
        print(" " + str(R))
    return R
  return [Rt_fromTC_ABC(TC[i], A, B, C, printVals) for i in range(len(TC))]

#print("10017  Rt_fromTC_ABC(25, A = 0.001029, B = 0.0002391, C = 1.566e-07) = " +
#    str(Rt_fromTC_ABC(25, A = 0.001029, B = 0.0002391, C = 1.566e-07)))
#print("29542, 10017,  3899  Rt_fromTC_ABC([0, 25, 50], A = 0.001029, B = 0.0002391, C = 1.566e-07) = " +
#    str(Rt_fromTC_ABC([0, 25, 50], A = 0.001029, B = 0.0002391, C = 1.566e-07)))

# Given TC[0:2] with three temperatures (in Celsius) and Rt[0:2] with 3 thermistor
# resistances at those temperatures (in ohms), compute thermistor coefficients
# A/B/C, returned in a list. Print values if printVals=True.
def ABCatRT(TC, Rt, printVals=True):
  L = [math.log(Rt[i]) for i in range(len(Rt))]
  Y = [1/degCtoK(TC[i]) for i in range(len(TC))]
  y = [(Y[i]-Y[0])/(L[i]-L[0]) for i in range(1, len(Y))]
  C = (y[1]-y[0])/(L[2]-L[1])/sum(L)
  B = y[0] - C*(L[0]**2+L[0]*L[1]+L[1]**2)
  A = Y[0] - L[0]*(B+C*L[0]**2)
  A = signif(A, 4)
  B = signif(B, 4)
  C = signif(C, 4)
  if printVals:
    print("A = " + str(signif(A, 4)) + ", B = " + str(signif(B, 4)) + ", C = " + str(signif(C, 4)))
  return [A, B, C]

#print("A = 0.001029, B = 0.0002391, C = 1.566e-07" +
#    "   ABCatRT([0, 25, 50], [29490, 10000, 3893]) = ")
#ABCatRT([0, 25, 50], [29490, 10000, 3893])
#


#######################################################
# Use above functions to calculate A/B/C coefficients
# for several thermistors.
#######################################################
print("ABCatRT([0, 25, 50], [29490, 10000, 3893])   # 10K type 3")
ABCatRT([0, 25, 50], [29490, 10000, 3893])
print("T_C([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07)")
T_C([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07)
print("Rt_fromTC_ABC([0, 25, 50], A = 0.001029, B = 0.0002391, C = 1.566e-07)")
Rt_fromTC_ABC([0, 25, 50], A = 0.001029, B = 0.0002391, C = 1.566e-07)

print()

print("ABCatRT([0, 25, 50], [32650, 10000, 3602])   # 10K type 2")
ABCatRT([0, 25, 50], [32650, 10000, 3602])
print("T_C([32650, 10000, 3602], A = 0.001127, B = 0.0002344, C = 8.675e-08)")
T_C([32650, 10000, 3602], A = 0.001127, B = 0.0002344, C = 8.675e-08)
print("Rt_fromTC_ABC([0, 25, 50], A = 0.001127, B = 0.0002344, C = 8.675e-08)")
Rt_fromTC_ABC([0, 25, 50], A = 0.001127, B = 0.0002344, C = 8.675e-08)

print()

print("ABCatRT([0, 25, 50], [31426, 9900, 3515])   # Arduino thermistor, Chinese table first column")
ABCatRT([0, 25, 50], [31426, 9900, 3515])
print("ABCatRT([0, 25, 50], [32116, 10000, 3588])   # Arduino thermistor, Chinese table second column")
ABCatRT([0, 25, 50], [32116, 10000, 3588])
print("ABCatRT([0, 25, 50], [32817, 10100, 3661])   # Arduino thermistor, Chinese table third column")
ABCatRT([0, 25, 50], [32817, 10100, 3661])

print()

print("ABCatRT([0, 25, 50], [321000, 100000, 35800]) # MF52B 3950 100K NTC")
ABCatRT([0, 25, 50], [321000, 100000, 35800])

print()

print("ABCatRT([0, 25, 50], [31770, 10000, 3592])  # M52B 3435 10K NTC")
ABCatRT([0, 25, 50], [31770, 10000, 3592])
print("ABCatRT(degFtoC([36, 70, 115]), [26000, 11800, 4740]) # M52B 3435 10K NTC my measurements")
ABCatRT(degFtoC([36, 70, 115]), [26000, 11800, 4740])
print("T_F(11800, A = 0.0007609, B = 0.0002752, C = 6.933e-08)")
T_F(11800, A = 0.0007609, B = 0.0002752, C = 6.933e-08)

print()

print("ABCatRT([0, 25, 50], [32650, 10000, 3603])  # EPCOS p/n: B57862S103F, NTC 10K Ohms 1% 3988K 60mW")
ABCatRT([0, 25, 50], [32650, 10000, 3603])
