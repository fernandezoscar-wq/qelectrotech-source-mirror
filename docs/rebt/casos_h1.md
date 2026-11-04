# Casos de referencia del hito 1 (motor REBT)

Los valores esperados de `tests/qttest/tst_rebtcalc.cpp` se calcularon **aparte del
código C++**, con un script de Python independiente (al final) y comprobando a
mano los casos más sencillos. La prueba admite una tolerancia relativa del
0,1 %.

## Fórmulas

| Magnitud | Monofásico | Trifásico (equilibrado) |
|---|---|---|
| Intensidad de cálculo | I = P / (η · U · cos φ) | I = P / (η · √3 · U · cos φ) |
| Caída de tensión de un tramo | ΔU = 2 · L · I · (R·cos φ + X·sen φ) | ΔU = √3 · L · I · (R·cos φ + X·sen φ) |
| Caída porcentual | 100 · ΔU / U (U fase-neutro) | 100 · ΔU / U (U entre fases) |

R = 1 / (γ · S) en Ω/m. X se da en Ω/km por conductor. Si no se indica η, se
toma P como potencia absorbida y queda anotado como hipótesis.

Resistividad (si no se da la conductividad γ directamente):
ρ(T) = ρ20 · (1 + α20 · (T − 20)), con Cu ρ20 = 1/58 Ω·mm²/m y α20 = 0,00393 1/K
(IEC 60028), y Al ρ20 = 0,028264 Ω·mm²/m y α20 = 0,00403 1/K (IEC 60889). La
temperatura del conductor **no se supone**: es un dato de entrada.

## Intensidad de cálculo

| Caso | Sistema | P (W) | U (V) | cos φ | η | I esperada (A) | Comprobación a mano |
|---|---|---|---|---|---|---|---|
| M1 | 1F | 3680 | 230 | 1 | — | 16,000000 | 3680/230 = 16 |
| M2 | 1F | 2300 | 230 | 0,8 | — | 12,500000 | 2300/184 = 12,5 |
| M3 | 1F | 5750 | 230 | 0,9 | — | 27,777778 | 5750/207 |
| T1 | 3F | 10000 | 400 | 0,85 | — | 16,980890 | 10000/(692,82·0,85) |
| T2 | 3F | 15000 | 400 | 0,82 | 0,9 | 29,336904 | 16666,7/(692,82·0,82) |
| T3 | 3F | 22000 | 400 | 1 | — | 31,754265 | 22000/692,82 |

## Caída de tensión

| Caso | Datos | ΔU (V) | ΔU (%) |
|---|---|---|---|
| CDT‑M1 | 1F, 230 V, I = 16 A, L = 25 m, S = 2,5 mm², γ = 48, cos φ = 1, X no dada | 6,666667 | 2,898551 |
| CDT‑T1 | 3F, 400 V, I de T1, L = 50 m, S = 6 mm², γ = 44, X = 0,08 Ω/km, cos φ = 0,85 | 4,796823 | 1,199206 |
| CDT‑M2 | 1F, 230 V, I = 10 A, L = 30 m, S = 1,5 mm², Cu a 70 °C, cos φ = 0,9 | 7,426552 | 3,228936 |
| CDT‑T2 | 3F, 400 V, I = 100 A, L = 80 m, S = 50 mm², Al a 90 °C, cos φ = 0,9 | 9,038131 | 2,259533 |

Comprobación a mano de CDT‑M1: 2 · 25 · 16 / (48 · 2,5) = 800/120 = 6,667 V →
6,667/230 = 2,899 %.

## Límites (ITC‑BT‑19, apartado 2.2.2)

La ITC‑BT‑19 dice «menor del» 3 % / 5 %, por lo que el límite es **estricto**:
una caída exactamente igual al límite **no** lo supera. Prueba de frontera:
1F, 230 V, 10 A, 2,5 mm², γ = 50, cos φ = 1 → ΔU = 0,16 · L.

| L (m) | ΔU (%) | Límite 3 % estricto | Límite 3 % no estricto |
|---|---|---|---|
| 43,000 | 2,991304 | superado | superado |
| 43,125 | 3,000000 | **no superado** | superado |
| 43,250 | 3,008696 | no superado | no superado |

El límite se aplica desde el origen de la instalación interior. Comprobar un
solo circuito es una comprobación parcial y la regla lo deja indicado en sus
limitaciones. La caída acumulada llega en el hito 5.

## Datos insuficientes y entradas no válidas

- Si falta un dato (sistema, tensión, potencia, cos φ, longitud, sección, material
  o temperatura): estado `NotComputable`, **sin valor** y con la lista de
  todo lo que falta, cada cosa una sola vez.
- Si un dato está fuera de rango (cos φ o η fuera de (0, 1], cero o
  negativo, NaN o infinito, reactancia negativa): estado `InvalidInput`, sin valor.
  Un dato no válido tiene prioridad sobre uno que falta.

## Script de referencia

```python
import math
r3 = math.sqrt(3)
for P, U, c in [(3680, 230, 1.0), (2300, 230, 0.8), (5750, 230, 0.9)]:
    print(P / (U * c))
for P, U, c, eta in [(10000, 400, .85, None), (15000, 400, .82, .9), (22000, 400, 1.0, None)]:
    print((P / eta if eta else P) / (r3 * U * c))
dU = 2 * 25 * 16 / (48 * 2.5); print(dU, 100 * dU / 230)
I = 10000 / (r3 * 400 * .85); c = .85; s = math.sqrt(1 - c * c)
dU = r3 * 50 * I * (c / (44 * 6) + 0.08e-3 * s); print(dU, 100 * dU / 400)
rho = (1 / 58) * (1 + 0.00393 * 50); dU = 2 * 30 * 10 * .9 * rho / 1.5; print(dU, 100 * dU / 230)
rho = 0.028264 * (1 + 0.00403 * 70); dU = r3 * 80 * 100 * .9 * rho / 50; print(dU, 100 * dU / 400)
for L in (43.125, 43.0, 43.25): dU = 2 * L * 10 / (50 * 2.5); print(L, 100 * dU / 230)
```
