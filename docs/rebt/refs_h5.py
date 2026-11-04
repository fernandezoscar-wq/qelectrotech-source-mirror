import math
r3=math.sqrt(3)
# Feeder F: three phase 400 V, 30 A given, 20 m, 10 mm2, gamma 48, cos 0.9
dUF=r3*20*30*0.9/(48*10); pF=100*dUF/400
# Circuit C: single phase 230 V, 16 A, 25 m, 2.5 mm2, gamma 48, cos 1
dUC=2*25*16/(48*2.5); pC=100*dUC/230
print("F dU %.6f V  %.6f %%"%(dUF,pF))
print("C dU %.6f V  %.6f %%"%(dUC,pC))
print("C total %.6f %%  (<3: %s, <5: %s)"%(pC+pF,pC+pF<3,pC+pF<5))
# two levels: F2 feeding a third board, same as F
print("C total two feeders %.6f %%"%(pC+2*pF))
# coordination: I2 checks
for name,i2,iz in [("P1",1.45*16,17.320508),("P5",1.6*16,17)]:
    print(name,"I2 %.4f  1.45Iz %.4f"%(i2,1.45*iz))
