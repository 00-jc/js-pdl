#import "@preview/cetz:0.3.4"

// Ablación (sección «Evaluación de las optimizaciones»): efecto de quitar cada optimización en el
// tiempo total (suma de las medianas de los 18 corpus) y en el pico de memoria.
// none: no significativo (≈0); "na": no afecta a la memoria.

// (optimización quitada, motivo, Δ tiempo en %, Δ memoria en %)
#let filas = (
  ([LTO], [integra _xft_ en el analizador], 49, "na"),
  ([Líneas perezosas], [no cuenta líneas por _token_], 8, 19),
  ([_Hash_ perfecto], [una comparación por palabra], 4, "na"),
  ([Lecturas solapadas], [evita un `memcpy` variable], none, "na"),
  ([`__builtin_expect`], [ramas de error frías], none, "na"),
  ([`-march=native`], [AVX-512 en _xft_], none, "na"),
  ([_Prefetch_], [adelanta tabla y texto], none, "na"),
  ([_Computed gotos_], [salto directo del AFD], none, "na"),
  ([Minúsculas], [mitad de entradas en la tabla], none, "na"),
  ([SoA], [el sintáctico lee solo el tipo], none, 11),
  ([*Todas salvo SoA*], [], 54, 19),
)

#let tinta = black
#let tenue = luma(40%)
#let borde = 0.4pt + black
#let relleno = luma(45%)
#let relleno-total = luma(15%)

#let fmt-pct(v) = (if v > 0 { "+" } else { "−" }) + str(calc.abs(v)) + " %"

// Un panel horizontal: una barra por fila, con el cero marcado.
//   ticks: valores con marca; etiquetas: dibuja a la izquierda el nombre y el motivo de cada fila
#let panel(titulo, campo, lo, hi, ticks, ancho, alto-fila, etiquetas: false) = {
  import cetz.draw: *
  let n = filas.len()
  let w = ancho / 1cm
  let hf = alto-fila / 1cm
  let h = hf * n
  let x(v) = (v - lo) / (hi - lo) * w
  let bh = hf * 0.55
  {
    for v in ticks {
      if v != 0 {
        line((x(v), 0), (x(v), h), stroke: (paint: luma(80%), thickness: 0.3pt, dash: "dotted"))
      }
      line((x(v), 0), (x(v), -0.06), stroke: borde)
      content((x(v), -0.1), anchor: "north", text(size: 6pt, fill: tinta, if v == 0 { "0" } else { fmt-pct(v) }))
    }
    line((0, 0), (w, 0), stroke: borde)
    // separación antes de la fila de total
    line((0, hf), (w, hf), stroke: 0.3pt + luma(60%))
    for (i, f) in filas.enumerate() {
      let v = f.at(campo)
      let yc = h - (i + 0.5) * hf
      let total = i == n - 1
      if etiquetas {
        content((-0.15, yc), anchor: "east", {
          set par(justify: false, leading: 0.25em)
          set align(right)
          text(size: 6.5pt, f.at(0))
          if f.at(1) != [] { linebreak(); text(size: 5.5pt, fill: tenue, f.at(1)) }
        })
      }
      if v == "na" {
        content((x(0) + 0.08, yc), anchor: "west", text(size: 5.5pt, fill: tenue, [N/A]))
      } else if v == none {
        circle((x(0), yc), radius: 0.035, fill: black, stroke: none)
        content((x(0) + 0.1, yc), anchor: "west", text(size: 6pt, fill: tinta, $approx 0$))
      } else {
        rect((x(0), yc - bh / 2), (x(v), yc + bh / 2), fill: if total { relleno-total } else { relleno }, stroke: borde)
        let et = text(size: 6pt, fill: tinta, weight: if total { "bold" } else { "regular" }, fmt-pct(v))
        // las negativas son pequeñas: su etiqueta va al otro lado del cero
        content((calc.max(x(v), x(0)) + 0.06, yc), anchor: "west", et)
      }
    }
    line((x(0), 0), (x(0), h), stroke: 0.6pt + black)
    content((w / 2, h + 0.15), anchor: "south", text(size: 7pt, fill: tinta, weight: "bold", top-edge: "cap-height", bottom-edge: "baseline", titulo))
  }
}

#let grafico-ablacion(ancho) = {
  let alto-fila = 0.56cm
  let ancho-etiquetas = 2.9cm
  let resto = ancho - ancho-etiquetas - 0.4cm
  set align(center)
  cetz.canvas(length: 1cm, {
    import cetz.draw: *
    panel([$Delta$ tiempo], 2, 0, 70, (0, 20, 40, 60), resto * 0.64, alto-fila, etiquetas: true)
    translate(x: (resto * 0.64 + 0.4cm) / 1cm)
    panel([$Delta$ memoria], 3, 0, 28, (0, 10, 20), resto * 0.36, alto-fila)
  })
}
