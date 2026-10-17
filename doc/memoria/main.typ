#import "@preview/magic-isprs:0.1.0": isprs, isprs-heading

#let impl = sys.inputs.at("impl", default: "true") == "true"
#let anonymous = sys.inputs.at("anonymous", default: "false") == "true"

#show "Keywords": "Palabras clave"
#show "Abstract": "Resumen"
#show "References": "Referencias"
#show "Appendix": "Apéndice"

#show: isprs.with(
  title: [Implementación de un frontend para un subset de javascript],
  abstract: include "content/abstract.typ",
  authors: (
    (
      name: "Jaime Castro Enrique",
      email: "jaime.castro@alumnos.upm.es",
      institutions: "upm",
    ),
    (
      name: "Lucas Winand Osete",
      email: "lucas.winand@alumnos.upm.es",
      institutions: "upm",
    ),
    (
      name: "Chen Yu Zhao",
      email: "chenyu.zhao@alumnos.upm.es",
      institutions: "upm",
    ),
  ),
  institutions: (
    upm: (
      name: [Universidad Politécnica de Madrid],
      location: [Madrid, España],
      email-suffix: "@alumnos.upm.es",
    ),
  ),
  keywords: (
    "Procesadores de lenguajes",
    "Compiladores",
    "Análisis Léxico",
    "Análisis Sintáctico",
    "Análisis Semántico",
  ),
  bibliography: bibliography("biblio.bib", full: true),
  appendix: include "content/appendix.typ",
  anonymous: anonymous,
)

#set text(lang: "es")


#include "content/introduction.typ"

// La introducción ocupa la primera columna entera; la gramática, la segunda.
#colbreak()

#include "content/main_body.typ"

#include "content/conclusions.typ"
