// main project
#let project(
  title: "",
  subtitle: "",
  author: "",
  affiliation: "",
  Licence: none,
  UE: none,
  date: none,
  logo: none,
  main_color: "E94845",
  alpha: 60%,
  toc_title: "Table of contents",
  body,
) = {
  // Set the document's basic properties.
  set document(author: author, title: title)

  // latex looks
  set page(margin: 1.0in)
  set par(leading: 0.55em, justify: true, spacing: 0.55em)
  set text(font: "Libertinus Serif")
  show raw: set text(font: "Libertinus Serif")
  show heading: set block(above: 2em, below: 1.4em)

  // Set colors
  let primary-color = rgb(main_color) // alpha = 100%
  // change alpha of primary color
  let secondary-color = color.mix(color.rgb(100%, 100%, 100%, alpha), primary-color)

  show "highlight" : it => text(fill: primary-color)[#it]

  // figures
  show figure.caption: it => [
    *#it.supplement #context it.counter.display(it.numbering)*: #it.body
  ]
  show figure.where(kind: table): it => {
    set figure.caption(position: top)
    set align(center)
    v(12.5pt, weak: true)
    if it.has("caption") {
      it.caption
      v(0.25em)
    }
    it.body
    v(1em)
  }
  show figure.where(kind: image): it => {
    set align(center)
    show: pad.with(x: 13pt)
    v(12.5pt, weak: true)

    // Display the figure's body.
    it.body

    // Display the figure's caption.
    if it.has("caption") {
      it.caption
    }
    v(1em)
  }

  //customize inline raw code
  show raw.where(block: false) : it => h(0.5em) + box(fill: primary-color.lighten(90%), outset: 0.2em, it) + h(0.5em)

  // Set body font family.
  set text(lang: "es", 12pt)
  show heading: set text(fill: primary-color)

  //heading numbering
set heading(numbering: (..nums) => {
  let level = nums.pos().len()
  // only level 1 and 2 are numbered
  let pattern = if level == 1 {
    "I.1"
  } else if level == 2 or level == 3 {
    "I.1"
  }
  if pattern != none {
    numbering(pattern, ..nums)
  }
})

  // Set link style
  show link: it => text(blue, it)
  show footnote: it => text(blue, it)

  //pagebreak before first heading and space after
  //show heading.where(level:1): it => pagebreak(weak: true) + it + v(0.5em)

  // no pagebreak before heading
  show heading.where(level:1): it => it + v(0.5em)

  //numbered list colored
  set enum(indent: 1em, numbering: n => [#text(fill: primary-color, numbering("1.", n))])

  //unordered list colored
  set list(indent: 1em, marker: n => [#text(fill: primary-color, "•")])


  // display of outline entries
  show outline.entry: it => text(size: 12pt, weight: "regular",it)

  //avoid C++ word break
  show "C++": box

  // Title page.
  // Logo at top right if given
  if logo != none {
    place(top + right, image(logo, width: 35%))
  }
  // decorations at top left
  place(top + left, dx: -35%, dy: -28%, circle(radius: 150pt, fill: primary-color))
  place(top + left, dx: -10%, circle(radius: 75pt, fill: secondary-color))

  // decorations at bottom right
  place(bottom +right, dx: 40%, dy: 30%, circle(radius: 150pt, fill: secondary-color))


  v(2fr)

  align(center, text(3em, weight: 700, title))
  v(2em, weak: true)
  align(center, text(2em, weight: 700, subtitle))
  v(2em, weak: true)
  align(center, text(1.1em, date))

  v(2fr)

  // Author information.
  align(center)[
      #for single-author in author [
        #text(single-author, 12pt, weight: "bold");
        \;
      ] \
      #affiliation \ #Licence\ #emph[#UE]
    ]

  pagebreak()


  // Table of contents.
  set page(
    numbering: "1 / 1",
    number-align: center,
    )
  outline(depth: 3, indent: auto, title: toc_title)
  //useless pagebreak because using pagebreak before first heading and assuming that the page always starts with a heading
  //pagebreak()


  // Main body.
  set page(
    // header: [#emph()[#title #h(1fr) #author]],
    header: [#grid(
      columns: (1fr, 1fr, 1fr),
      rows: (auto),
      [#align(top + left, emph(title))], [#align(top + center, subtitle)],
    )]
  )
  set par(justify: true)

  body


}

//useful functions
//set block-quote
#let blockquote = rect.with(stroke: (left: 2.5pt + luma(170)), inset: (left: 1em))
#let primary-color = rgb("E94845")
#let secondary-color = rgb(255, 80, 69, 60%)