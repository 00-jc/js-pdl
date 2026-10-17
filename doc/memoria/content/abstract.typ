#import "@preview/magic-isprs:0.1.0": isprs-heading
#show: isprs-heading

#set smartquote(quotes: (double: ("«", "»"), single: ("“", "”")))

En esta memoria se describe la implementación de un _frontend_ para *_js-pdl_*,
un subconjunto de _JavaScript_, escrito en C23 sin dependencias de libc. El
diseño es orientado a datos y especializa cada analizador dentro de las
restricciones del modelo visto en teoría. Se detallan el diseño de cada
analizador, las optimizaciones aplicadas al analizador léxico y la batería de
pruebas utilizada para validarlo.
