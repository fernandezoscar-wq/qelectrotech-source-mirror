import math
kT=lambda tmax,ta,tr: math.sqrt((tmax-ta)/(tmax-tr))
pvc=dict(zip([1.5,2.5,4,6,10],[10,20,30,40,60]))
print("kT PVC 40: %.6f  XLPE 40: %.6f  PVC 25: %.6f"%(kT(70,40,30),kT(90,40,30),kT(70,25,30)))
print("A Iz 2.5 PVC 40 g1: %.6f"%(20*kT(70,40,30)))
print("C Iz 2.5 kT .87 g .8: %.6f"%(20*.87*.8))
print("D Iz XLPE3 E 4mm 40: %.6f"%(35*kT(90,40,30)))
print("E Iz 2.5 30C g.8 other .9: %.6f"%(20*1*.8*.9))
Ib=16
for L in (25,40):
    for s in [1.5,2.5,4,6,10]:
        iz=pvc[s]*kT(70,40,30); du=2*L*Ib/(48*s); pct=100*du/230
        print("L",L,"s",s,"Iz %.4f"%iz,"ok_iz",iz>=Ib,"pct %.4f"%pct)
