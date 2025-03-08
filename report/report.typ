#import "template.typ": *
#import "@preview/tablex:0.0.9": tablex, hlinex, vlinex, colspanx, rowspanx
#import "@preview/colorful-boxes:1.3.1": outline-colorbox
#import "@preview/gentle-clues:1.0.0": *
#import "@preview/codly:1.2.0": *
#show: codly-init.with()

#codly(
  languages: (
    cs: (name: "C#", icon: ".NET ", color: rgb("#2b77ce")),
  )
)

#set math.equation(numbering: "(1)")

#show: project.with(
  title: "Práctica 4",
  subtitle: [
    Problema del Viajante de Comercio
  ],
  author: (
    "Pablo Hernández Jiménez",
    "José Ángel Portillo García"
  ),
  affiliation: "Universidad de La Laguna",
  date: datetime.today().display(),
  Licence: "Curso 2024-2025",
  UE: "Diseño y Análisis de Algoritmos",
  logo: "resources/images/ull-logo.png",
  main_color: "#5c068c",
  toc_title: "Índice",
)

= Introducción
La definición del Problema del Viajante de Comercio se define como:

#info(title: "Nota")[
    *_El problema del viajante de comercio (TSP)_* es un problema NP-Duro
    altamente conocido en la literatura científica. El problema parte de la 
    existencia de un *grafo completo, no dirigido y pesado*, donde el peso de 
    cada una de sus aristas corresponde a la distancia a viajar entre sus 2 
    nodos (bidireccional). Existe un comercial que parte de uno de los nodos y 
    para el que se debe obtener la ruta que cumpla lo siguiente:
  - Debe pasar por todos los nodos del grafo *exactamente* una vez.
  - Debe volver al nodo de origen.
  - La distancia total de la ruta debe ser mínima.
]

#figure(
  image("resources/images/graph-example.png"),
  caption: [
    Ejemplo de un *grafo* que cumple el _problema del viajante de comercio_
  ]
)

#v(1em)

#block[
El objetivo principal de esta práctica es implementar en C++ un programa que
compare diferentes soluciones al _problema del viajante de comercio_ 
implementadas a traves de diferentes algoritmos:

  - *_Fuerza Bruta_*: Comprueba todas las posibles permutaciones para encontrar la solución óptima.
  - *_Programación Dinámica_*: Usa el algoritmo Held-Karp para encontrar de manera optima el tour.
  - *_Nearest Neighbour_*: Un algoritmos voráz que siempre escoge la ciudad no visitada más cercana.
  - *_Simulated Annealing_*: Una solución metaheuristica que imita el proceso de annealing en la metalurgia.
  - *_Two Opt_*: Mejora una solución voráz a través de repetidamente invertir los subtours.
]

#v(1em)


= Comparación de Algoritmos
Para evaluar el rendimiento de los algoritmos implementados, se realizaron experimentos con instancias del problema del viajante de comercio (TSP) de diferentes tamaños. Las instancias consisten de el problema con 5, 50 y 100 nodos. A continuación, se presentan los resultados obtenidos para cada algoritmo:

#linebreak()

#figure(
  image("resources/images/algorithm-comparation.png"),
  caption:[
    Comparación de algoritmos con diferentes tamaños de nodos.
  ] 
)

#linebreak()

Se puede observar que como escala la complejidad para algunos algoritmos.

#pagebreak()

Además si observamos la tabla de comparación del los algoritmos se puede ver más explicitamente como escala cada algoritmo

#figure(
  image("resources/images/graphic-algorithm.png"),
  caption:[
    Gráfico de escala de cadá algoritmo.
  ] 
)

Por último también podemos observar las puntuaciones de cada algoritmo para cada cantidad de nodos que se hagan uso

#figure(
  image("resources/images/score-algorithms.png"),
  caption: [
    Puntuación de los algoritmos para cada tamaño
  ]
)

#pagebreak()

= Conclusión

#block[
Los resultados muestran que:

- *_Fuerza Bruta_* es computacionalmente inviable para instancias mayores a 20 nodos debido a su complejidad factorial.
- *_Programación Dinámica (Held-Karp)_* ofrece soluciones óptimas, pero su complejidad O(n²·2ⁿ) limita su aplicabilidad a instancias pequeñas.
- *_Nearest Neighbour_* es extremadamente rápido, pero produce soluciones de menor calidad debido a su naturaleza voraz.
- *_Simulated Annealing_* y *_Two Opt_* ofrecen un buen equilibrio entre tiempo de ejecución y calidad de la solución, siendo adecuados para instancias medianas y grandes.

Estos resultados destacan la importancia de seleccionar el algoritmo adecuado según el tamaño de la instancia y los requisitos de calidad de la solución.
]