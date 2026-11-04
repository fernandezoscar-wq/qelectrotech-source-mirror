"""Reference values for tst_rebtshortcircuit, computed apart from the C++.

Formulas written out from the sources, not from the program:
  Guia-BT Anexo 3: Icc = 0.8 U / R, R from the CGP, outgoing + return, 20 degC
  IEC 60909-0: R(theta_e) = [1 + 0.004 (theta_e - 20)] R20
  UNE-HD 60364-4-43 table 43A: k Cu PVC 115, Cu XLPE 143; PVC 160 degC, XLPE 250
"""
from math import sqrt

RHO_CU = 1 / 58          # IEC 60228, ohm mm2/m at 20 degC
U0 = 230.0

def r(L, S, rho=RHO_CU):
    return rho * L / S

# --- Guide example as printed (rho 0.018): 2898 A
R_guide = r(2 * 15, 10, 0.018) + r(2 * 25, 95, 0.018)
print("guide example rho 0.018: R=%.6f Icc=%.3f A" % (R_guide, 0.8 * U0 / R_guide))

# --- Case G: LGA 95/50 mm2 25 m, DI 10/10 mm2 15 m, circuit 2.5 mm2 20 m PVC,
#     C16 UNE-EN 60898, Icn 6 kA. LGA/DI XLPE.
lga_ph, lga_n = r(25, 95), r(25, 50)
di_ph, di_n = r(15, 10), r(15, 10)
c_ph = c_n = r(20, 2.5)
imax = 0.8 * U0 / (lga_ph + lga_n + di_ph + di_n)
hot_x = 1 + 0.004 * (250 - 20)
hot_p = 1 + 0.004 * (160 - 20)
rmin = (lga_ph + lga_n) * hot_x + (di_ph + di_n) * hot_x + (c_ph + c_n) * hot_p
imin = 0.8 * U0 / rmin
k = 115
t = (k * 2.5 / imax) ** 2
print("G: Imax=%.6f Imin=%.6f trip=%.1f t=%.6f k2S2=%.1f" % (imax, imin, 160, t, (k*2.5)**2))

# --- Case K: known source, Icc max 10 kA, min 6 kA; feeder 16 mm2 30 m XLPE
#     phase only for max; circuit 4 mm2 25 m XLPE, X 0.08 ohm/km both.
zs_max = U0 / 10000
zs_min = U0 / 6000
f_ph = r(30, 16)
imax_k = U0 / sqrt(zs_max ** 2 + f_ph ** 2)
rl = (2 * r(30, 16) + 2 * r(25, 4)) * hot_x
xl = 2 * 0.08e-3 * 30 + 2 * 0.08e-3 * 25
imin_k = U0 / (zs_min + sqrt(rl ** 2 + xl ** 2))
k2 = 143
t_k = (k2 * 4 / imax_k) ** 2
print("K: Imax=%.6f Imin=%.6f t=%.6f k2S2=%.1f" % (imax_k, imin_k, t_k, (k2*4)**2))

# --- Case H: guide, one feeder Cu 10/10 mm2 120 m XLPE (DI, neutral given),
#     circuit 2.5 mm2 10 m PVC interior (neutral assumed equal), C16
f = r(120, 10)
imax_h = 0.8 * U0 / (2 * f)
imin_h = 0.8 * U0 / (2 * f * hot_x + 2 * r(10, 2.5) * hot_p)
t_h = (115 * 2.5 / imax_h) ** 2
print("H: Imax=%.6f Imin=%.6f t=%.6f" % (imax_h, imin_h, t_h))

# --- Case M (through the model): CPM origin, guide factor; DI Cu 10/10 mm2
#     15 m XLPE; CGMP; circuit C1 Cu 2.5 mm2 20 m PVC, dwelling, C16
imax_m = 0.8 * U0 / (2 * r(15, 10))
imin_m = 0.8 * U0 / (2 * r(15, 10) * hot_x + 2 * r(20, 2.5) * hot_p)
print("M: Imax=%.6f Imin=%.6f" % (imax_m, imin_m))
