---
name: swingtree
description: >
  Write Swing desktop UIs with the SwingTree library and its companion property
  library Sprouts. Use this skill whenever you are building, editing, reviewing,
  or debugging Java Swing GUI code that imports `swingtree.UI` or `sprouts.*` —
  declarative component trees, convergent/responsive/reactive layouts, the
  functional style API, SVG icons, animations, and MVI/MVL or MVVM view models.
---

# Writing SwingTree Applications

SwingTree is a Java library for building **Swing** desktop GUIs **declaratively**,
the way Flutter / SwiftUI / Jetpack Compose / JetBrains' Kotlin UI DSL build
theirs. You describe the component tree with **method chaining + nesting**, bind
it to state with the **Sprouts** property library (`Var`/`Val`/`Vars`/`Tuple`),
and paint it with a **functional, immutable style API**. There is no XML, no FXML,
no separate template language — it is all plain Java, fully type-safe and
debuggable.

This document gives you the intuition to write *any* SwingTree app: the builder,
layout, properties & lenses, the two architecture patterns (MVI/MVL and classic
MVVM), events, styling, animation, tables, icons & SVG, dialogs, and the
non-obvious gotchas that bite people. Read it top to bottom once; thereafter use the cheat sheet at
the end.

> **One library-wide preference up front: SwingTree views are expected to be
> *convergent*** — usable whether the window is maximised on an ultrawide or
> tiled into a tall, 500-pixel strip. Users run tiling window managers, snap
> windows to halves, and rotate monitors into portrait; a view that only works
> at the size you developed it in is considered broken. §2c is the short
> version and the checklist; apply it to every view you write or review.

---

## 0. The one import and the mental model

```java
import swingtree.UI;
import static swingtree.UI.*;   // brings panel(), button(), FILL, WRAP, GROW, ...
```

A UI is a **tree of components**. Every node is built by a `UI.xxx(..)` **factory**
that returns a **builder** (`UIForPanel`, `UIForButton`, `UIForLabel`, … all
subtypes of `UIForAnySwing`). On a builder you:

- **configure** it with chained `withXyz(..)` / `isXyzIf(..)` calls,
- **nest children** with `.add(..)`,
- **bind** it to `Var`/`Val` properties for reactivity,
- **style** it with `.withStyle(it -> ...)`,
- **wire events** with `.onXyz(..)`,
- and finally **unwrap** it with `.get(JPanel.class)` or hand it to `UI.show(..)`.

Crucial idea: **a builder is a recipe, not the component.** It produces a real
`JComponent` underneath. You can escape to the raw component with `.peek(c -> ...)`
or unwrap with `.get(Type.class)` — but treat `peek` as a last resort (§12): reach
for a SwingTree `with*`/`is*If`/`on*` method first.

The smallest complete program:

```java
import static swingtree.UI.*;

public static void main(String[] args) {
    UI.show(
        panel("wrap 1")
        .add(label("Welcome to SwingTree!"))
        .add(button("Click me").onClick(it -> System.out.println("clicked")))
    );
}
```

---

## 1. Growing the tree — factories, nesting, `add`

`UI.show(component | builder | title, builder | Function<JFrame,Component>)`
opens a window. Inside it you compose nodes:

```java
UI.show(
    panel("wrap 2")                              // a JPanel, MigLayout "wrap 2"
    .add(label("Name:"))
    .add("grow", textField("John"))              // first String arg = per-child layout constraint
    .add(label("Age:"))
    .add("grow", textField("42"))
    .add("span", separator())                    // span all columns, then wrap
    .add(button("Save"))
);
```

Rules of `.add(..)`:

- `.add(childBuilder)` — add with no constraint.
- `.add("growx, span 2", childBuilder)` — first arg is a **MigLayout add-constraint string**.
- `.add(GROW.and(SPAN), childBuilder)` — or a **type-safe constraint** (see §2).
- `.add(a, b, c)` — add several children at once (same constraint applies to each).

### Common factories (each returns a builder)

| Factory | Component |
|---|---|
| `panel(...)`, `box(...)` | `JPanel` / `JBox` (a transparent, insets-free panel — perfect for grouping) |
| `label(text)`, `html("<h1>..</h1>")` | `JLabel` (html(..) renders HTML) |
| `button(text)`, `toggleButton(text)`, `checkBox(text)`, `radioButton(text)` | buttons |
| `textField(text)`, `textArea(text)`, `passwordField()`, `numericTextField(var)` | text inputs |
| `comboBox(...)`, `slider(Align, min, max)`, `spinner(...)`, `progressBar(...)` | value pickers |
| `separator()`, `scrollPane()`, `scrollPanels()`, `splitPane(Align)`, `tabbedPane()` | structure |
| `table(Var<TableData>)`, `table()`, `list(...)`, `menu(...)`, `menuItem(...)`, `splitButton(text)` | data / menus (bind a `TableData` value — see §10) |
| `icon(path)`, `icon(w,h,path)` | `JIcon` (supports SVG, see §10) |

`box(...)` vs `panel(...)`: a `JBox` is non-opaque with zero default insets — use it
for invisible structural grouping; use `panel` when you want a real surface to
style. **Never call `setOpaque(..)` yourself on a styled component — the style
engine owns opacity and will fight you.**

### Wrapping a custom / third-party component

```java
.add( UI.of(new MyCustomJComponent()).onMouseClick(it -> ...) )
```

`UI.of(jcomponent)` wraps any `JComponent` so the declaration keeps flowing.
`UI.of(this)` is the standard way to start a `View extends JPanel` (see §5).

---

## 2. Layout — MigLayout, type-safe constants, and convergence

SwingTree's default layout manager is **MigLayout**; §2a and §2b are the two ways
to drive it. §2c onwards is about making the result work at **any** window size,
which SwingTree treats as the default expectation rather than a nice-to-have.

### 2a. String constraints (most common, terse)

The **container** constraint goes in the factory; **per-child** constraints go as
the first `add(..)` arg:

```java
panel("fill, wrap 3, insets 12, gap 8")   // container: fill space, 3 cols, 12px insets
.add("growx", a)
.add("span 2, growx", b)                   // this child spans 2 columns
.add("wrap", c)                            // force a new row after c
```

Memorize these MigLayout keywords:
- Container: `fill`, `fillx`, `filly`, `wrap N` (N columns), `insets T L B R` / `ins N`, `gap`, `debug` (draws guide borders — great for diagnosing layout).
- Per-child: `grow`, `growx`, `growy`, `push`, `pushx`, `pushy`, `span` / `span N`, `wrap`, `align center/left/right`, `top/bottom`, `width 60px::`, `w 180!`, `h 90!`.
- `60px::` means "min 60, no max"; `180!` means "exactly 180".

`withLayout("fill, wrap 2")` sets the container constraint after the fact, and
`withLayout(layout, colConstraints, rowConstraints)` gives full control, e.g.
`.withLayout("fill, wrap 2", "[grow 60][grow 40]")`.

Full keyword reference: http://www.miglayout.com/

### 2b. Type-safe constants (refactor-safe, composable)

`import static swingtree.UI.*` exposes constants that compose with `.and(..)`:

```java
of(this).withLayout(FILL.and(WRAP(1)).and(INS(16)))
.add(GROW.and(PUSH), child)
.add(CENTER.and(SPAN), html("<h2>Title</h2>"))
.add(RIGHT, button("OK"));
```

Container constants: `FILL`, `FILL_X`, `FILL_Y`, `WRAP(n)`, `INS(n)` / `INS(t,l,b,r)`, `GAP_REL(n)`, `FLOW_X`, `DEBUG`.
Per-child constants: `GROW`, `GROW_X`, `GROW_Y`, `PUSH`, `PUSH_X`, `PUSH_Y`, `SPAN`, `SPAN(n)`, `WRAP`, `SHRINK`, `CENTER`, `LEFT`, `RIGHT`, `TOP`, `BOTTOM`, `ALIGN_CENTER`, `ALIGN_LEFT`, `ALIGN_X_CENTER`, `ALIGN_Y_TOP`, `GAP_LEFT(n)`, …

String constraints and constants are interchangeable — pick whichever reads
clearer locally. (Examples in this codebase mix both freely.)

### 2c. Convergence — a SwingTree view is expected to survive any window shape

**This is a strong preference of the library, not an optional polish step. Write
convergent views by default; treat "it only works maximised on a landscape
monitor" as a bug.**

Desktop windows are not a fixed size any more. Users run tiling window managers
(i3, sway, Hyprland, yabai, AeroSpace, FancyZones), snap windows to halves and
thirds, put four windows across an ultrawide, rotate a monitor into portrait,
and drag your app onto a smaller second screen. A view that assumes ~1400×900
is broken for a large fraction of real users.

**Convergent** ≠ merely responsive. Responsive means nothing *overlaps*;
convergent means nothing is *lost* — the layout rearranges, the content
re-prioritises, and the primary action stays reachable at every size.

#### The default recipe (start every page like this)

```java
UI.scrollPane( conf -> conf.fitWidth(true) )         // the page may outgrow the window
.withHorizontalScrollBarPolicy(UI.Active.NEVER)      // never scroll sideways
.withVerticalScrollIncrement(24)
.add(
    UI.panel().withFlowLayout(UI.HorizontalAlignment.LEFT, 18, 18)
    .withMinSize(0, 0)                               // may shrink to whatever it is given
    .withPrefSize(PAGE_REFERENCE_WIDTH, 0)           // the reference width (see 2d)
    .add(SIDEBAR_SPAN, sidebar(vm))
    .add(CONTENT_SPAN, content(vm))
);
```

#### Four mechanisms, in the order you should reach for them

| Gear | Mechanism | Use when | Costs |
|---|---|---|---|
| **0** | `"wmin 0"` + `withMinSize(0,0)` | **always** — a prerequisite for all of the others | nothing |
| **1** | `withFlowLayout()` + `AUTO_SPAN` (§2d) | the same regions want a different number of columns | **no state at all** |
| **2** | `Var<Layout>` reflow (§2e) | the same widgets want a genuinely different arrangement (a toolbar folding into rows) | one property; **nothing is rebuilt** |
| **3** | form-factor state + view swap (§2f) | the two shapes want different component trees (split pane ⇄ scrolling column) | rebuilds — loses focus/caret/scroll |
| **4** | `isVisibleIf` + shorter bound labels (below) | content, not layout, must re-prioritise | nothing |

Most views need **gear 0 + gear 1**. Escalate only when the shape of the problem
demands it — gear 3 is the only one that destroys component state.

#### Gear 0 — minimum sizes are a hard floor (the #1 cause of "it won't narrow")

A `JLabel`'s minimum width is its full text; containers propagate child minimums
upward; a flow grid reports the **sum** of its children's minimums. One
forgotten label deep in the tree gives the whole *window* a minimum width, and
the responsive bands are then unreachable — the layout never even gets to try.

```java
.add("growx, wmin 0", label("A long descriptive caption"))   // ellipsizes instead of strutting
.withMinSize(0, 0)                                           // on every flow-grid panel
```

Prefer ranges over hard sizes: `width 90::200`, not `width 200!`.
**Rule:** drag the window as narrow as it goes. If it stops at an arbitrary
width, that is a minimum-size bug — fix it before touching anything else.

#### Gear 4 — the content converges too

```java
label("Live timetable · click any train to see its route").isVisibleIf(isWide)  // + "hidemode 3"
Val<String> btn = Viewable.of(String.class, theme, formfactor,
        (t, f) -> f.isTall() ? "☾" : "☾  Dark mode");
```

`hidemode 3` on the container constraint makes a hidden child stop reserving its
cell. Drop what is **redundant** (already shown elsewhere) before what is unique.

#### The convergence checklist (apply this in every review)

- [ ] Window narrows freely — no arbitrary floor (`wmin 0`, `withMinSize(0,0)`).
- [ ] Multi-column regions collapse to one column rather than becoming slivers.
- [ ] A stacked page sits in `scrollPane(conf -> conf.fitWidth(true))`.
- [ ] Everything with no natural preferred size (`scrollPane`, `scrollPanels`,
      empty `textField`) has been given one.
- [ ] A nested grid lives inside another **grid**, never in a MigLayout cell (§2d).
- [ ] The primary action is reachable at every size.
- [ ] What vanishes when narrow is redundant, not unique.
- [ ] A view swap (gear 3) has hysteresis.
- [ ] Every `Var<Layout>` variant spells out a constraint for **every** child.

Full prose: [Convergent-Design.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Convergent-Design.md).

### 2d. Gear 1 — the responsive flow grid (`ResponsiveGridFlowLayout`, Bootstrap-style 12 columns)

The workhorse, and **stateless**: no breakpoint field, no resize listener, no
view-model change. Each child declares how many of 12 virtual columns it occupies
per size category; the lambda re-runs on every resize.

```java
private static final FlowCell ROSTER_SPAN = AUTO_SPAN( it -> it.fill(true)
        .verySmall(12).small(12).medium(12).large(5).veryLarge(4).oversize(4) );
private static final FlowCell EDITOR_SPAN = AUTO_SPAN( it -> it.fill(true)
        .verySmall(12).small(12).medium(12).large(7).veryLarge(8).oversize(8) );

panel().withFlowLayout(UI.HorizontalAlignment.LEFT, 18, 18)
.withMinSize(0, 0).withPrefSize(900, 0)
.add(ROSTER_SPAN, roster(vm))
.add(EDITOR_SPAN, editor(vm));
```

That span table **is** the responsive design — read it out loud: *side by side
from LARGE up, one stacked column below.*

**Size categories are exact fifths of the grid's reference width** (not pixels):

| `VERY_SMALL` | `SMALL` | `MEDIUM` | `LARGE` | `VERY_LARGE` | `OVERSIZE` |
|---|---|---|---|---|---|
| 0…⅕ | ⅕…⅖ | ⅖…⅗ | ⅗…⅘ | ⅘…1 | ≥1 |

An undeclared category falls back to the **nearest declared** one, so
`AUTO_SPAN(it -> it.large(12))` means "always full width". Spell all six out in
shared code anyway — the table then documents the design.

**Reference width** = the explicitly set preferred width if there is one, else
the ideal single-row sum of all children. Declaring it (`withPrefSize(w, 0)`) is
how you *move* the breakpoints, and it is mandatory for a nested grid — otherwise
the nested grid reports "all children in one row" upward and silently rewrites
the parent's bands.

**Other cell options:** `.fill(true)` stretches the cell to the row height (how a
short sidebar card ends up flush with a tall content card; a MigLayout child with
a `fill`/`filly` container constraint gets this automatically), and
`.align(UI.VerticalAlignment.TOP|CENTER|BOTTOM)` positions a non-filling cell.

**Heights: a row is as tall as its tallest child's *preferred* height** — a flow
grid never stretches a row to fill a tall window. Hence two rules:
1. Give a preferred height to anything that has none:
   `scrollPanels().withPrefSize(340, 470)`, or it collapses to one line.
2. Put a page-level grid in a `scrollPane(conf -> conf.fitWidth(true))`, or the
   stacked layout is clipped at the bottom.

> ⚠️ **THE NESTING TRAP — a grid nests in a grid, NOT in a MigLayout cell.**
> A wrapping grid is a *width-for-height* layout, so `ResponsiveGridFlowLayout`
> asks each child "how tall at the width you are about to get?" — and only
> another `ResponsiveGridFlowLayout` can answer. Meanwhile
> `JComponent.getPreferredSize()` short-circuits the layout manager once a
> preferred size is set, so a **MigLayout** parent reads the literal `0` from
> `withPrefSize(w, 0)` and **the nested grid collapses to zero height**, silently
> clipping everything in it.
> ```java
> // ❌ form laid out at height 0
> panel("fill, wrap 1").add("grow, push", panel().withFlowLayout(..).withPrefSize(620,0)...)
> // ✅ make the card a grid too
> panel().withFlowLayout(UI.HorizontalAlignment.LEFT, 0, 0).withMinSize(0,0).withPrefSize(620,0)
> .add(FULL_ROW, titleStrip()).add(FULL_ROW, form())
> ```
> A grid declaring a reference width must live inside another grid or directly
> inside a `scrollPane(fitWidth(true))`. Anywhere else, drop the explicit
> preferred size.

### 2e. Gear 2 — reactive layout: bind the layout itself to a `Var<Layout>`

To swap the *entire layout manager* at runtime (one toolbar row ↔ three rows,
compact ↔ wide, edit ↔ read mode) **without destroying or rebuilding any
child** — so focus, caret, selection and scroll offsets all survive:

```java
import swingtree.api.Layout;
import swingtree.layout.MigAddConstraint;

Var<Layout> layout = Var.of(Layout.class, Layout.mig("fill, wrap 1"));

panel(layout)                                  // == panel().withLayout(layout)
.add("growx", a).add("growx", b);

// later, anywhere — atomic reflow, no rebuild:
layout.set(Layout.mig("fill, wrap 2").withChildConstraints(
    MigAddConstraint.of("growx"),
    MigAddConstraint.of("growx, span 2")       // positional: index 0, 1, ...
));
```

`Layout` factories: `Layout.mig(constraints)`, `Layout.flow(FlowCell...)`,
`Layout.border()`, `Layout.grid(rows,cols)`, `Layout.box(UI.Axis.X)`,
`Layout.none()` (absolute positioning — `setLayout(null)`), `Layout.unspecific()`
(no-op, leaves current manager alone). `withChildConstraints(...)` maps
positionally to children. This is how `SalesDashboard`, `AlmanackView` and
`CelestialScribe` work — see §5.4 for deriving a layout from data.

Two rules that cost debug time:
- **Every variant must supply a constraint for *every* child.** They apply
  positionally and are only overwritten where a new layout supplies one, so a gap
  leaves the previous variant's constraint (a stray `"wrap"`) in place after
  switching back.
- **Add `nogrid`** to a wrapped MigLayout variant, or every row's columns line up
  with every other row's and the second row inherits the first column's width.

### 2f. Gear 3 — form-factor state and view swapping

When the two shapes want genuinely different component trees (a split pane is a
good landscape design and a bad portrait one), classify the shape into a small
enum, keep it in the **view model** like any other state, and swap the body with
the property-bound `add(Val, ViewSupplier)`:

```java
public enum Formfactor {
    WIDE, TALL;
    public boolean isTall(){ return this == TALL; }
    /** 10% dead band — without hysteresis, dragging along the diagonal strobes. */
    public static Formfactor of( int width, int height, Formfactor current ) {
        double slack = 1.1;
        return current == TALL ? (width  > height * slack ? WIDE : TALL)
                               : (height > width  * slack ? TALL : WIDE);
    }
}
```
```java
of(this).withLayout(FILL.and(WRAP(1)))
.onResize( it -> formfactor.update(From.VIEW, f -> Formfactor.of(it.getWidth(), it.getHeight(), f)) )
.add(GROW.and(PUSH), formfactor, this::body);   // rebuilds only on an actual shape change
```

- **Always add hysteresis** (gears 1 and 2 don't need it — reflowing never
  changes the width it was measured against; a view *swap* can).
- `onResize` fires per pixel of a drag, but `Var.update(..)` is a no-op when the
  value is unchanged, so the rebuild happens once per shape change.
- The form factor is ordinary, Swing-free state ⇒ unit-testable without a GUI.
- If the swapped sub-view is built under a `StyleSheet`, re-enter the scope:
  `UI.of(UI.use(sheet, () -> tallBody().get(JScrollPane.class)))` — `UI.use`
  consumes the builder and returns the component (§7).

---

## 3. State — Sprouts properties (`Var`, `Val`) and binding

Reactivity comes from the **Sprouts** library. The whole point: **the view never
holds Swing state; it binds to properties, and the property system keeps the two
in sync bidirectionally.** Your business logic never imports a Swing class.

- `Var<T>` — a **mutable** property. `get()`, `set(value)`, `update(fn)`, `onChange(..)`.
- `Val<T>` — a **read-only** view of a property. `Var extends Val`, so you can
  expose `Val` from a view model to prevent the view from writing.
- `Vars<T>` / `Vals<T>` — observable **lists** of properties (classic MVVM).
- `Tuple<T>` — an **immutable** ordered collection (functional MVI/MVL).

```java
Var<String>  name  = Var.of("Joseph");
Var<Boolean> ok    = Var.of(true);
Var<Integer> count = Var.of(0);
Var<Layout>  lay   = Var.of(Layout.class, Layout.mig("fill"));  // explicit type when value could be null/ambiguous
```

### Binding properties to components

Pass the property to the factory and the binding is automatic and bidirectional:

```java
textField(name)                 // user typing -> name.set(..); name.set(..) -> field text
checkBox("Agree", ok)           // toggling <-> ok
slider(Align.HORIZONTAL, 0.0, 1.0, ratio)   // generic over Number: int OR double
comboBox(selectedEnum, e -> prettyLabel(e)) // selection <-> Var<MyEnum>
label(name)                     // one-way: label text follows name
progressBar(Align.HORIZONTAL, ratioVal)     // one-way Val<Double> 0..1
```

Flags bind through `isXyzIf(Val<Boolean>)`:

```java
textField(name).isEnabledIf(ok).isVisibleIf(showAdvanced)
button("Go").isEnabledIf(canSubmit)
checkBox("edit").isSelectedIf(...)  // and isEditableIf on text components
```

### Derived (computed) read-only views

`view*` methods produce a `Val` that recomputes when the source changes — perfect
for labels and computed flags:

```java
Val<String> caption = count.viewAsString(n -> "Items: " + n);
Val<Boolean> isEmpty = name.viewAs(Boolean.class, s -> s.isBlank());
Val<Double>  asD     = count.viewAsDouble(n -> n / 100.0);
label(caption);
```

`viewAsString/Int/Double()` with **no mapper** just stringify/convert the value;
the `nullObject`-first overloads (`viewAsString("", fn)`) define what to show when
the source is null — null-safe by construction. To derive from **two** sources at
once, combine them — the result recomputes when *either* input changes:

```java
Viewable<Double> total = Viewable.of(price, taxRate, (p, tr) -> p * (1 + tr));   // Val<Double>, updates live
```

To merge **any number** of sources (not just two) into one value without nesting,
use the **composite view builder** (Sprouts ≥ 2.7.0): a seed plus one
`join(property, wither)` per input, each folding that property's item into the seed.
It recomputes as a whole on any input change — ideal for feeding a *single*
`withStyle` from a whole cluster of view-model properties (§8):

```java
Viewable<Weather> weather = Viewable.of(Weather.blank(), it -> it
    .join(city,        Weather::withCity)
    .join(temperature, Weather::withTemperature)
    .join(humidity,    Weather::withHumidity));   // Val<Weather>, recomputed on any change
```

> All `view*`/`viewAs*` results are `Viewable` (a `Val` you may listen on). They
> are held **weakly** by their source — see the GC gotcha in §9c: if you only
> register an `onChange` on one, keep it in a field or it is collected.

### The two change channels (`From.VIEW` vs `From.VIEW_MODEL`)

Every `Var` distinguishes who caused a change:

- `set(From.VIEW, v)` — the **user/view** changed it (SwingTree calls this for you when the user types/clicks).
- `set(From.VIEW_MODEL, v)` / plain `set(v)` — your **application logic** changed it.

Register listeners per channel via `Viewable.cast(prop).onChange(From.VIEW_MODEL, it -> ...)`
(or `From.VIEW`, or `From.ALL`). This split prevents infinite feedback loops and
lets you react only to user input or only to logic. Inside a listener,
`it.currentValue()` is the new value.

> **`prop.view()` vs `Viewable.cast(prop)`.** You cannot listen on a raw
> `Var`/`Val` directly — you need a `Viewable`. Two ways to get one, and the
> difference is lifecycle: `prop.view()` returns a **new, weakly-held** view
> (the sprouts-preferred default) — store it in a field so it isn't GC'd.
> `Viewable.cast(prop)` reinterprets the property *itself* as `Viewable`, so the
> listener lives exactly as long as that property object. Both are safe **only
> when the thing you listen on is reachable**: for a lens (which its parent holds
> *weakly*) you must keep the lens — or its `view()` — in a field either way (§9c).

```java
Viewable.cast(firstName).onChange(From.ALL, it ->
    fullName.set(it.currentValue().orElseThrowUnchecked() + " " + lastName.get())
);
```
**Warning:** The approach above can lead to memory leaks due to change listeners
never being garbage collected and still holding strong references to captured variables.

→ So the prefer custom change listener registration on views instead of directly!

---

## 4. Lenses — `zoomTo` and immutable view models

This is the heart of the **recommended** SwingTree architecture (MVI/MVL). A
**lens** focuses a root `Var<BigImmutableRecord>` down onto one field, giving you
a `Var<Field>` that reads via a getter and writes via a **wither** (a method that
returns a *new* record with that field changed).

```java
record Person(String forename, String surname, Address address) {
    Person withForename(String f){ return new Person(f, surname, address); }
    Person withSurname(String s){ return new Person(forename, s, address); }
    Person withAddress(Address a){ return new Person(forename, surname, a); }
}

Var<Person>  person   = Var.of(new Person("Tom","Schultz", addr));
Var<String>  forename = person.zoomTo(Person::forename, Person::withForename);
Var<Address> address  = person.zoomTo(Person::address,  Person::withAddress);
Var<String>  street   = address.zoomTo(Address::street, Address::withStreet);  // lenses nest!
```

Now `textField(forename)` edits the forename, and a keystroke produces a brand-new
`Person` (and `Team`, etc., all the way up) inside `person`. Lenses are **smart**:
they fire change events only when *their own slice* actually changes, even if the
whole root record was replaced.

Other lens flavors:
- `viewAs(Type.class, getter)` / `viewAsString/Double/Int(getter)` — **read-only** derived `Val`.
- `zoomToNullable(Type.class, getter, wither)` — when the focused value may be null.
- `zoomTo(defaultValue, getter, wither)` — supply a fallback for null parents.
- `zoomTo(Lens<S,T>)` — a hand-written lens (implement `Lens.getter`/`wither`, or
  `Lens.of(getter, wither)`) when the focus needs **logic** — clamping, derived
  fields, or zooming into a collection entry (see below).

**Tip:** Generate withers with Lombok `@With` on records to avoid boilerplate
(this is also how you stay on **Java 8** — records need 16+, but `@With @Getter`
on a `final class` gives the same value semantics):
```java
@With record Person(String forename, String surname, Address address) {}
// person.zoomTo(Person::forename, Person::withForename)  // withForename generated by @With
```

### Sprouts immutable collections — `Tuple`, `Association`, `ValueSet`, `Pair`

Records model *fixed* shape; for *variable-size* state inside a view model, use
Sprouts' **persistent** (structural-sharing) collections instead of
`java.util` — they are immutable value objects, so they fit record fields and
withers, and SwingTree binds to several of them directly. Every "mutation"
returns a **new** instance.

| Type | `java.util` analogue | Make it | Key ops (all return a new instance) |
|---|---|---|---|
| `Tuple<T>` | `List<T>` | `Tuple.of(a,b,c)`, `Tuple.of(T.class)` (empty), `Tuple.of(T.class, iterable)` | `add`, `remove`, `removeAt`, `setAt(i,x)`, `map`, `retainIf`/`removeIf`, `slice`, `sort`, `first`/`last` |
| `Association<K,V>` | `Map<K,V>` | `Association.between(K.class, V.class)` (empty!), `.ofLinked(..)` (insertion-ordered) | `put`, `putAll(Pair...)`, `get(k) → Optional`, `remove`, `removeIf(pair->..)` |
| `ValueSet<E>` | `Set<E>` | `ValueSet.of(E.class)`, `ValueSet.of(a,b,..)`, `.ofLinked(..)` | `add`, `addAll`, `remove`, `retainAll`, `retainIf`, `any(pred)` |
| `Pair<A,B>` | `Map.Entry` | `Pair.of(a, b)` | `.first()`, `.second()` |

> ⚠️ The empty-map factory is **`Association.between(K.class, V.class)`**, *not*
> `Association.of(..)` — `of(key, value)` builds a one-entry map (and
> `of(String.class, Integer.class)` would silently make an `Association<Class,Class>`).

A field of one of these *is* part of the immutable value, so it composes with
lenses and withers like any other field:

```java
@With record PartyPlan(
    Tuple<Guest>                 guests,       // ordered, may repeat
    Association<String,Integer>  drinkStock,   // name -> quantity
    ValueSet<String>             decorations   // unique, unordered
) {}

Var<PartyPlan>                    plan  = Var.of(initialPlan);
Var<Tuple<Guest>>                 guests = plan.zoomTo(PartyPlan::guests, PartyPlan::withGuests);
Var<Association<String,Integer>>  stock  = plan.zoomTo(PartyPlan::drinkStock, PartyPlan::withDrinkStock);

guests.update(g -> g.add(new Guest("Gimli")));      // immutable add, fires change
stock.update(s -> s.put("Ale", 12));                // immutable put
```

You can even **lens into a single entry** of a collection with logic lenses —
the write rebuilds the whole collection immutably, but the property behaves like
a plain `Var<V>` (great for binding one map value to one field):

```java
Var<Integer> aleStock = stock.zoomTo(
    s -> s.get("Ale").orElse(0),                    // getter: read the entry
    (s, qty) -> s.put("Ale", qty)                   // wither: return a new map
);
aleStock.set(20);   // updates the entire association inside `plan`
```

`Tuple` is the one most wired into SwingTree: `addAll(..)` renders one sub-view
per element (§5.2), and `Var<Tuple<Item>>` is the canonical MVI list.

---

## 5. Architecture — how to structure a real app

A SwingTree **view** is conventionally a `class extends JPanel` whose constructor
takes the view model (or a `Var` of it) and builds itself with `UI.of(this)`:

```java
public final class MyView extends JPanel {
    public MyView(Var<MyViewModel> vm) {
        UI.of(this).withLayout("fill, wrap 1")
        .add(...)
        .add(...);
    }
    public static void main(String[] args) {
        Var<MyViewModel> vm = Var.of(new MyViewModel());
        UI.show(f -> new MyView(vm));
        EventProcessor.DECOUPLED.join();   // keep the app thread alive (see §11), processes events forever (blocks)
    }
}
```

Pull repeated fragments into `private static UIForAnySwing<?,?> someSection(...)`
methods that return builders — this is the standard way large views (TeamView,
BreathingView, CelestialScribe) stay readable.

### 5.1 MVI / MVL — the recommended pattern (immutable records + lenses)

The whole UI state lives in **one immutable record** (the view model). The view is
a pure function of it; every change produces a new record via withers; the view
reaches fields through `zoomTo`. There are no Swing references and no mutable
fields in the view model — it is unit-testable in isolation.

**View model** (note: `static empty()` / no-arg constructor for the initial state,
withers for every field, and *business methods* that return new instances):

```java
public record CalculatorViewModel(CalculatorInputs inputs, CalculatorOutput output) {
    public static CalculatorViewModel empty(){ return new CalculatorViewModel(CalculatorInputs.empty(), CalculatorOutput.empty()); }
    public CalculatorViewModel withInputs(CalculatorInputs i){ return new CalculatorViewModel(i, output); }
    public CalculatorViewModel withOutput(CalculatorOutput o){ return new CalculatorViewModel(inputs, o); }
    public CalculatorViewModel runCalculation(){            // business logic = pure function returning new VM
        try {
            double l = Double.parseDouble(inputs.left()), r = Double.parseDouble(inputs.right());
            double res = switch (inputs.operator()) {
                case ADD -> l+r; case SUBTRACT -> l-r; case MULTIPLY -> l*r; case DIVIDE -> l/r;
            };
            return withOutput(output.withResult(res).withValid(true));
        } catch (NumberFormatException e) { return withOutput(output.withError("Invalid number").withValid(false)); }
    }
}
```

For business logic that can **fail** (parsing, validation, IO), Sprouts'
`Result<T>` is a cleaner alternative to ad-hoc error fields: it is a `Maybe<T>`
(present-or-empty, like `Optional`) that *also* carries a `Tuple<Problem>`
describing what went wrong. `Result.ofTry(T.class, () -> risky())` runs a
throwing supplier and captures any exception as a `Problem` instead of
propagating it — ideal inside a pure view-model method. The view then renders
`result.problems()` (e.g. an error label) and `result.orElse(fallback)` for the
value. (SwingTree itself returns `Result` from table-cell conversions.)

**View** zooms in and triggers business methods with `vm.set(vm.get().runCalculation())`
or, more idiomatically, `vm.update(CalculatorViewModel::runCalculation)`:

```java
public final class CalculatorView extends JPanel {
    public CalculatorView(Var<CalculatorViewModel> vm) {
        Var<CalculatorInputs> inputs = vm.zoomTo(CalculatorViewModel::inputs, CalculatorViewModel::withInputs);
        Var<CalculatorOutput> output = vm.zoomTo(CalculatorViewModel::output, CalculatorViewModel::withOutput);
        UI.of(this).withLayout("fill")
        .add("growx", textField(inputs.zoomTo(CalculatorInputs::left, CalculatorInputs::withLeft)))
        .add(comboBox(inputs.zoomTo(CalculatorInputs::operator, CalculatorInputs::withOperator), Operator::symbol))
        .add("growx", textField(inputs.zoomTo(CalculatorInputs::right, CalculatorInputs::withRight)))
        .add("wrap", button("Run!").onClick(e -> vm.update(CalculatorViewModel::runCalculation)))
        .add("span", label(output.viewAsString(o -> o.valid() ? "= " + o.result() : o.error())));
    }
}
```

`vm.update(fn)` is shorthand for `vm.set(fn.apply(vm.get()))` — prefer it for
applying a business method.

### 5.2 Lists in MVI/MVL — `Tuple` + `addAll` + `HasId`

Model a collection as a `Tuple<T>` field; zoom to it; render with `addAll`:

```java
record ChatVM(Tuple<Message> allMessages, String draft) {
    record Message(UUID id, String text, LocalDateTime sentAt, boolean editing) implements HasId<UUID> {
        Message(){ this(UUID.randomUUID(), "", LocalDateTime.now(), false); }
    }
}

Var<Tuple<Message>> messages = vm.zoomTo(ChatVM::allMessages, ChatVM::withAllMessages);

scrollPanels()
.addAll(messages, (Var<Message> entry) -> {              // one sub-view per item; entry is a per-item lens
    Var<String> text = entry.zoomTo(Message::text, Message::withText);
    return panel(FILL)
        .add(GROW_X.and(WRAP), textArea(text))
        .add(RIGHT, button("✕").onClick(it -> messages.update(t -> t.remove(entry))));
});

// add an item:
messages.update(t -> t.add(new Message().withText(draft.get())));
```

> **CRITICAL: when you bind a *mutable* `Var<Tuple<M>>` and want a per-item lens,
> the item type MUST implement `sprouts.HasId<IdType>`** (carry a `UUID`/stable
> id). That overload — `addAll(Var<Tuple<M>>, entry -> ...)`, where `entry` is a
> `Var<M>` lens — is the one above, and it is `<M extends HasId<?>>`. Value
> records define identity by *content*, so two equal records would confuse the
> component binding; `HasId.id()` gives each item a stable identity so SwingTree
> knows which sub-view maps to which item, which item-lens to hand it, and which
> rows to reuse vs. rebuild on change. Add a `UUID id` field and `implements
> HasId<UUID>`.
>
> The **read-only** overloads do *not* require `HasId`: `addAll(Val<Tuple<M>>,
> m -> view)` and `addAll(Tuple<M>, m -> view)` (and the `Vals<M>` MVVM overload)
> hand the supplier the **value** `M`, not a lens — use these when items aren't
> individually editable. `HasId` is the price of admission for per-item editing.

> **A bound `addAll` OWNS its container — give it a panel of its own.** The
> binding manages every child, so a component that already had children added by
> hand is **cleared** when `addAll` binds to it (SwingTree logs "Trying to bind
> multiple sub-views to component … Clearing component now"). A heading plus a
> bound list is therefore two components, not one:
> ```java
> // ❌ the heading is silently deleted when the binding attaches
> panel().add(FULL_ROW, label("ROOMS")).addAll(CHIP_SPAN, rooms, this::roomChip)
> // ✅ the list gets a container to itself
> panel().add(FULL_ROW, label("ROOMS")).add(FULL_ROW, roomRail())
> //  where roomRail() == panel().withFlowLayout(..).addAll(CHIP_SPAN, rooms, this::roomChip)
> ```

> **A row supplier runs *later*, so under a `StyleSheet` it must re-enter the
> scope.** `UI.use(sheet, ..)` only binds what is built inside its lambda, and
> `addAll` rebuilds rows whenever the tuple changes — long after the constructor
> returned. Initial rows then look right and every row built after the first
> model change comes out unstyled (§7):
> ```java
> private UIForAnySwing<?,?> row( Var<M> entry ) {   // the supplier passed to addAll
>     return UI.of(UI.use(sheet, () -> rowBody(entry).get(JPanel.class)));
> }
> ```

`Tuple` is functional: `.add(x)`, `.remove(x)`, `.map(fn)`, `.setAt(i, x)`,
`.get(i)`, `.size()`, `.isEmpty()` — all return new tuples (or values).
`Tuple.of(Message.class)` makes an empty typed tuple; `Tuple.of(a, b, c)` a
populated one.

### 5.3 Classic MVVM — mutable view models (the alternative)

If you prefer mutable view models: the view model holds `Var<X>` *fields* directly
(no root record, no lenses), exposes them through getters, and uses `Vars<T>` for
observable lists. The view binds straight to those fields.

```java
public class PersonVM {
    private final Var<String> firstName = Var.of("Joseph");
    private final Var<String> lastName  = Var.of("Armstrong");
    private final Var<String> fullName  = Var.of("");
    public PersonVM() {
        Viewable.cast(firstName).onChange(From.ALL, it -> recompute());
        Viewable.cast(lastName ).onChange(From.ALL, it -> recompute());
        recompute();
    }
    private void recompute(){ fullName.set(firstName.get() + " " + lastName.get()); }
    public Var<String> firstName(){ return firstName; }   // mutable out
    public Var<String> lastName(){ return lastName; }
    public Val<String> fullName(){ return fullName; }     // read-only out
}
```

**Polymorphic / dynamic sub-views** work in both patterns via the property-bound
`add` overload — when the property changes, SwingTree swaps the sub-view:

```java
// MVVM: Var<Object> subVM, view supplier dispatches on type
.add(vm.subViewModel(), subVM ->
    subVM instanceof SubVM1 s ? new SubView1(s) : new SubView2((SubVM2) subVM))

// MVI: Val<Boolean> + supplier picks which fragment to (re)build
.add("grow, push", hasSelection, has -> has ? editorBody(vm) : emptyState())
```

A `Vars<T>` (MVVM) and a `Var<Tuple<T>>` (MVI) are both rendered with
`addAll(list, viewSupplier)`. **TeamView exists in the SwingTree repo in both flavors**
(`examples.team.mvi` and `examples.team.mvvm`) — the clearest side-by-side
contrast. Choose **MVI/MVL for new code**; reach for MVVM only when integrating
with existing mutable models.

### 5.4 Deriving a layout from data (advanced reactive)

`CelestialScribe` derives the entire child layout from a tuple of model objects —
positions are a pure function of state, so dragging a star just updates the model:

```java
Val<Layout> layout = stars.viewAs(Layout.class, tuple -> {
    Layout.None none = Layout.none();
    for (int i = 0; i < tuple.size(); i++)
        none = none.withChildBound(i, tuple.get(i).bounds());
    return none;
});
box().withLayout(layout).withRepaintOn(stars).addAll(stars, this::starPanel);
```

---

## 6. Events

Every component supports the same base events; the handler receives a delegate
(conventionally `it`) that wraps **both the component and the event state** and
offers query/animation helpers.

```java
button("Go")
.onClick(it -> doThing())                      // also: onClick(Runnable) for no-arg
.onMouseClick(it -> ...).onMousePress(it -> ...).onMouseRelease(it -> ...)
.onMouseEnter(it -> ...).onMouseExit(it -> ...).onMouseMove(it -> ...).onMouseDrag(it -> ...)
.onFocusGain(it -> ...).onFocusLoss(it -> ...)
.onKeyPress(it -> ...).onKeyRelease(it -> ...).onKeyTyped(it -> ...)
.onResize(it -> ...).onShown(it -> ...).onHidden(it -> ...);
```

Useful delegate methods: `it.get()` / `it.getComponent()` (the component),
`it.getParent()`, `it.mouseX()` / `it.mouseY()`, `it.animateFor(..)` (§9),
`it.paint(status, g -> ...)` (custom rendering), drag deltas
(`it.deltaXSinceStart()`, `it.initialComponentPosition()`). **All geometry these
return is in DPI-agnostic "developer pixels"** (except `mouse*OnScreen()`, which
is raw screen pixels) — see §13.

### Custom / model-driven events: `on(..)` vs `onView(..)`

Both attach an `Action` to any `sprouts.Observable` (e.g. an `Event` from
`Event.create()`, or a property). The difference is **which thread runs the
handler**:

| Method | Handler runs on | Use for |
|---|---|---|
| `onView(observable, it -> ...)` | **EDT** (Swing thread) | reacting to model changes that **touch the view** — resize a label, animate a colour |
| `on(observable, it -> ...)` | **application thread** | reacting to external/business events that **update your model** — network, custom input |

Rule: if your handler sets Swing properties → `onView`; if it mutates the view
model or does non-UI work → `on`.

---

## 7. Styling — the functional `withStyle` API

`.withStyle(it -> it. ... )` receives a `ComponentStyleDelegate` (`it`) and returns
a configured one. It is **immutable and re-run on every paint**, so styles can
depend on live state (selection, animation progress, model fields). This is how
SwingTree paints shadows, gradients, rounded borders, etc. *on top of* the current
Look-and-Feel — things plain Swing cannot do.

```java
panel("fill")
.withStyle(it -> it
    .margin(8).padding(24)
    .backgroundColor(new Color(57,221,255))
    .foregroundColor(Color.WHITE)
    .borderRadius(32)
    .border(2, Color.DARK_GRAY)                 // width + color
    .borderAt(Edge.LEFT, 5, accent)             // one edge only (great for accent bars)
    .shadowColor(new Color(0,0,0,128)).shadowBlurRadius(5).shadowSpreadRadius(1).shadowOffset(0,2)
    .shadowIsInset(false)
);
```

Frequently used delegate methods (all chainable, all DPI/HiDPI aware):

- Box: `margin`, `padding`, `borderRadius`, `borderRadiusAt(Corner, w, h)`, `border`, `borderAt(Edge, w, color)`, `prefSize`, `size`.
- Fill: `backgroundColor` / `foundationColor`, `foregroundColor`, `gradient(...)`, `noise(...)`, `image(img -> ...)`.
- Shadow: `shadowColor`, `shadowBlurRadius`, `shadowSpreadRadius`, `shadowOffset`, `shadowIsInset`. Named shadows: `.shadow("name", s -> s.color(..).offset(..))`.
- Layered painting: `.painter(Layer.CONTENT, g -> ...)` for raw `Graphics2D`.
- `component()` returns the live component, so you can branch on its state (e.g. `it.component().isSelected()`). **Deprecated for reading geometry** — its sizes are in *component pixels* and double-scale if fed back in; use `componentWidth/Height()` / `componentPrefWidth/Height()` instead (§13).

Gradients and named layers:

```java
.gradient(Layer.BACKGROUND, "glow", g -> g
    .type(GradientType.RADIAL)                  // or LINEAR
    .boundary(ComponentBoundary.BORDER_TO_INTERIOR)
    .span(Span.TOP_LEFT_TO_BOTTOM_RIGHT)
    .offset(cx, cy).size(radius)
    .colors(color(0.75,1,0.5,0.5), color(0.5,1,1,0))   // UI.color(r,g,b[,a]) -> UI.Color
    .clipTo(ComponentArea.BODY)
)
```

`UI.Color` (via `color(...)`, `Color.ofRgb(...)`, `Color.ofHsb(...)`) adds
`.blend(other, t)`, `.shade(amount)`, `.brighter()`, alpha helpers — handy for
deriving palettes.

### Font styling (`componentFont`)

```java
.withStyle(it -> it.componentFont(f -> f
    .size(32).family("Arial").weight(2f).color(Color.WHITE).posture(0.1f).spacing(0.12f)
    .gradient(grad -> grad.colors(Color.GREEN, Color.BLUE).span(UI.Span.LEFT_TO_RIGHT))
    .noise(n -> n.colors(Color.DARK_GRAY, Color.CYAN).function(UI.NoiseType.CELLS).scale(1.25))
))
```

There are also `.withFontSize(n)`, `.withForeground(color)`, `.withBackground(color)`
shortcuts directly on the builder for simple cases.

### Background filtering (frosted glass)

A non-opaque child can blur/scale the parent's pixels behind it:

```java
.withStyle(it -> it
    .backgroundColor(Color.TRANSPARENT)         // must be non-opaque for the filter to show
    .parentFilter(f -> f.area(ComponentArea.BODY).blur(16).scale(1.25, 1.25))
)
```

### Central style sheets + semantic groups (CSS-like, hot-swappable themes)

For app-wide styling, pull rules into a `StyleSheet` and tag components with
`.group(EnumTag)` / `.id("name")`. This is how the **Theme Garden** swaps five
complete themes at runtime with zero changes to the view skeleton.

```java
enum Skin { PRIMARY, SECONDARY }

final class MySheet extends StyleSheet {
    @Override protected void configure() {
        add(type(JButton.class),                it -> it.borderRadius(8).padding(6,14,6,14));
        add(type(JButton.class).group(Skin.PRIMARY), it -> it.backgroundColor(BLUE).foregroundColor(WHITE));
        add(id("ok-button"),                    it -> it.shadowBlurRadius(8));
    }
}
```

Traits: `id("x")` (most specific), `group(tag)` (prefer **enum** tags over
strings — type-safe), `type(Class)`. They compose:
`type(JButton.class).group(Skin.PRIMARY)`. Specificity: `id` > `type+group` >
`group` > `type`; later `add(..)` wins ties.

Install a sheet either globally —
`SwingTree.initializeUsing(cfg -> cfg.styleSheet(new MySheet()))` — or for a scope:

```java
UI.use(new MySheet(), () -> UI.show(f -> new MyView()));   // only components built INSIDE the lambda bind
```

> `UI.use(sheet, supplier)` **consumes** the builder it is handed and returns the
> finished component. And it only binds what is built *inside* the lambda — so a
> sub-view built later (a property-bound `add(Val, ViewSupplier)`, a lazy tab)
> must re-enter the scope itself, or it comes out unstyled:
> `UI.of(UI.use(sheet, () -> tallBody().get(JScrollPane.class)))`.

**Hot-swap themes**: keep mutable state in the sheet and call `reconfigure()` to
re-run `configure()` and instantly repaint every component in the `UI.use` scope:

```java
final class ThemedSheet extends StyleSheet {
    private Theme theme = Theme.LIGHT;
    public void setTheme(Theme t){ if (t != theme){ theme = t; reconfigure(); } }
    @Override protected void configure(){ switch (theme){ case LIGHT -> light(); case DARK -> dark(); } }
}
// in the view: bind a Var<Theme> to the sheet
Viewable.cast(theme).onChange(From.ALL, it -> sheet.setTheme(theme.get()));
UI.use(sheet, () -> of(this).group(Skin.FRAME). ... .add(comboBox(theme)));
```

---

## 8. Property-driven styles — `withStyle(prop, styler)` (and `withRepaintOn`)

Style lambdas are evaluated by the **UI thread**, as part of the paint cycle. So when
a style depends on property state, don't read the property inside a plain `withStyle`
lambda — hand the property to the style and receive its item as an argument:

```java
box()
.withStyle(orbScale, (scale, it) -> it.shadowBlurRadius((int)(16 + 78 * scale)). ...)
```

The item is captured on the property's owning thread and passed to the lambda
explicitly, and the component re-styles and repaints **automatically** on every
change. This is the thread-safe and preferred way to use property state in styles:
a plain `withStyle(it -> ... someVal.get() ...)` reads application-thread state from
the UI thread (unsafe under `EventProcessor.DECOUPLED`) and doesn't refresh by
itself either. Styles driven by several properties compose by chaining:
`.withStyle(a, ..).withStyle(b, ..)`.

### Merging *many* properties into **one** `withStyle` (Sprouts ≥ 2.7.0)

When one style rule genuinely depends on **several** properties at once, you don't
have to chain a `withStyle` per property. Declare a small **record in the view** that
holds everything the style needs, and merge all the source properties into a single
`Viewable<ThatRecord>` with the Sprouts **composite view builder**
`Viewable.of(seed, it -> it.join(p, combiner)...)` — a seed record plus one
`join(property, wither)` per input, each folding that property's item into the record.
A *single* `withStyle` then drives the whole style from the merged item, for **any**
number of inputs:

```java
record Avatar(Color accent, int diameter, boolean online) {
    Avatar withAccent(Color c)   { return new Avatar(c, diameter, online); }
    Avatar withDiameter(int d)   { return new Avatar(accent, d, online); }
    Avatar withOnline(boolean o) { return new Avatar(accent, diameter, o); }
}

label(initials)
.withStyle(
    Viewable.of(new Avatar(Color.GRAY, 38, false), it -> it
        .join(accentColor, Avatar::withAccent)     // Val<Color>
        .join(diameter,    Avatar::withDiameter)   // Val<Integer>
        .join(isOnline,    Avatar::withOnline)),   // Val<Boolean>
    (a, it) -> it
        .prefSize(a.diameter(), a.diameter())
        .backgroundColor(a.accent())
        .borderRadius(1000)
        .border(a.online() ? 2 : 0, Color.GREEN)
);
```

The composite item is recomputed **as a whole** whenever *any* joined property
changes (fold starts at the seed, applies each combiner in join order, reads the
*current* item of every input), so one `withStyle` stays in sync with all of its
inputs. It scales to any number of properties without nesting, and a property may be
joined more than once. **This is the idiomatic way to capture multiple reactive
view-model properties in a single thread-safe styler.** Requires **Sprouts 2.7.0+**
(`Viewable.of(seed, configurator)` — the composite builder — was added there). Use the
`Viewable.of(Type.class, seed, ..)` overload when the record type is polymorphic.

> **No field needed — build it inline.** A composite is a *view*
> (`isView() == true`), and SwingTree's property bindings hold **views (and lenses)
> strongly** internally (§9c), so the inline `Viewable.of(..)` above is safe from GC
> even though views are otherwise only weakly held by their sources. (Chaining
> separate `withStyle(a,..).withStyle(b,..)` calls is still fine and reads clearer when
> the rules are independent; reach for the composite when one rule needs several
> inputs together, or when you want a single styler for a whole cluster of state.)

An animated flavor transitions towards each new item over a `LifeTime`
(`anim.progress()` runs 0→1 on every item change):

```java
label("status")
.withStyle(status, LifeTime.of(0.5, TimeUnit.SECONDS), (s, anim, it) -> it
    .backgroundColor(mix(s.color(), anim.progress())))
```

The same **composite merge** works here: hand a merged `Viewable<Record>` (built with
`Viewable.of(seed, it -> it.join(...)...)`, Sprouts ≥ 2.7) as the property, and every
change of *any* joined input restarts the transition towards the newly merged item.

The full family of property/animation styling entry points (all cross-linked in their
Javadocs):

| Method | Driven by | Use for |
|---|---|---|
| `withStyle(it -> ..)` | nothing (plain) | static style, or live state you read *safely* (no app-thread props) |
| `withStyle(prop, (item, it) -> ..)` | a property **item** | thread-safe property-driven style, auto-repaint |
| `withStyle(prop, LifeTime, (item, anim, it) -> ..)` | a property **item** + transition | *animate towards* each new item |
| `withTransitionalStyle(boolVar, LifeTime, (state, it) -> ..)` | a **boolean** property | bidirectional 0↔1 transition as the flag flips (§9b) |
| `withTransitoryStyle(observable, LifeTime, (state, it) -> ..)` | an `Observable`/`Event` | a one-shot temporary style animation on each fire |

The two item-driven rows (`ItemStyler`/`AnimatedItemStyler`) are the ones that benefit
from the composite merge — collapse *N* properties into one record and feed a single call.

`withRepaintOn(observableOrEvent, ...)` remains the right tool for repaint triggers
that are *not* property-item-driven styles — e.g. repainting a custom painter when
an `Event` fires, or a bound custom layout (§5) whose inputs changed.

---

## 9. Animation

Animations are timer-driven lambdas invoked ~60×/s on the EDT. Two levels:

### 9a. View-side, fire-and-forget (`it.animateFor` / `UI.animateFor`)

```java
button("hover me")
.onMouseEnter(it -> it.animateFor(0.5, TimeUnit.SECONDS, status -> {
    double h = 1 - status.progress() * 0.5;
    it.setBackgroundColor(h, 1, h);
}));
```

The `AnimationStatus status` gives you `progress()` (0→1), `fadeIn()`, `fadeOut()`,
`pulse()`, `cycle()`. Drive *anything* from it: colors, bounds (`setBounds`),
text, or custom rendering via `it.paint(status, g -> ...)`:

```java
.onMouseClick(it -> it.animateFor(1.2, TimeUnit.SECONDS, s -> it.paint(s, g -> {
    g.setColor(new Color(120,176,238,(int)(200*s.fadeOut())));
    for (int i=0;i<5;i++){ double r=280*s.fadeIn()*(1-i*0.18);
        g.drawOval((int)(it.mouseX()-r/2),(int)(it.mouseY()-r/2),(int)r,(int)r); }
})));
```

`UI.animateFor(dur, unit).go(s -> someVar.set(s.progress()))` runs an animation not
tied to an event; `.asLongAs(s -> true).go(...)` loops forever (ambient effects).
A common idiom: animate a `Var<Double>` and let a property-bound
`withStyle(progress, (p, it) -> ..)` (§8) render the frames.

### 9b. View-side transition between two states (`withTransitionalStyle`)

Given a `Var<Boolean>` and a duration, SwingTree interpolates `progress` 0↔1 every
time the flag flips. Multiply style props by `state.progress()`:

```java
label("toggle me")
.withTransitionalStyle(isOn, LifeTime.of(2, TimeUnit.SECONDS), (state, it) -> it
    .borderRadius(38 * state.progress())
    .backgroundColor(200/255d, 210/255d, 220/255d, state.progress())
    .shadowBlurRadius(10 * state.progress())
);
// elsewhere: toggleButton("toggle").onClick(it -> isOn.set(it.get().isSelected()));
```

### 9c. Modelled animation (MVI-friendly — state lives in the view model)

For testable, multi-phase animation, the view model exposes an `Animatable` (a pure
function of `AnimationStatus` → new model). The view hands it to `UI.animate(vm, vm::xxx)`
and **re-arms** the next phase by listening for the model's phase change.

```java
// view model
public Animatable<BreathingViewModel> breathAnimation() {
    BreathPhase ph = this.phase; double secs = settings.secondsFor(ph);
    return Animatable.of(LifeTime.of(secs, TimeUnit.SECONDS), this,
        new AnimationTransformation<>() {
            public BreathingViewModel run(AnimationStatus s, BreathingViewModel m){   // pure, every frame
                return m.withPhase(ph).withPhaseProgress(s.progress()).withOrbScale(ph.scaleAt(s));
            }
            public BreathingViewModel finish(AnimationStatus s, BreathingViewModel m){ // once, at end
                return m.advancePhase();
            }
        });
}

// view: chain phases by re-arming on phase change
Viewable.cast(phase).onChange(From.VIEW_MODEL, it -> {
    if (vm.get().running()) UI.animate(vm, BreathingViewModel::breathAnimation);
});
// start it:
button.onClick(it -> { vm.update(BreathingViewModel::begin); UI.animate(vm, BreathingViewModel::breathAnimation); });
```

> **GC GOTCHA (this WILL bite you):** Sprouts lenses/views observe their parent
> **weakly**. SwingTree's own bindings (`label`, `slider`, `withRepaintOn`, …)
> hold a strong ref internally, so lenses you pass *to them* are safe as locals.
> But a lens consumed **only** by a raw `Viewable.cast(lens).onChange(..)`
> subscription (like the `phase` re-arming lens above) is **not** retained — it
> gets garbage-collected and the animation silently freezes after one phase.
> **Fix: keep that lens as a `private final` field of the view.** (See the
> `BreathingView.phase` field and its Javadoc.)

---

## 10. Tables, lists, icons, dialogs

### Tables — model them as **data** (`TableData`), never as a `TableModel`

`TableData` (`swingtree.api.model`) is an **immutable value describing a whole
table**: cells + column names + column classes + a `UI.ListData` layout. Put it in a
`Var`, bind it, done — no model subclass, no `updateTableOn(..)`, no event to fire,
and thread-safe by construction (§11). **This is the preferred way to build tables.**

```java
Var<TableData> data = Var.of(
    TableData.of(UI.ListData.ROW_MAJOR, "Name", "Age")   // columns first, no rows yet
        .addRow("Alice", 30)
        .addRow("Bob",   42)
);

UI.table(data);                                // that's the whole binding
data.update(it -> it.addRow("Carol", 55));     // ...and the table follows
```

Every method returns a **new** `TableData` (verbs mirror `Tuple`, §4):

| | |
|---|---|
| read | `getValueAt(r,c)`, `getRow(r)`, `getColumn(c)`, `getRowCount()`, `getColumnCount()`, `isEmpty()`, `indexOfColumn(name)`, `getColumnName(i)`, `getColumnClass(i)`, `isEditable()`, `layout()`, `cells()`, `columnNames()`, `columnClasses()` |
| cell | `setCellAt(r, c, value)` — **not** `setValueAt` (that is `TableModel`'s *mutator*) |
| rows | `addRow(vals…)`, `addRowAt(i, vals…)`, `addRows(t)`, `addRowsAt(i, t)`, `setRowAt(i, vals…)`, `setRowsAt(i, t)`, `removeRowAt(i)`, `removeRowsAt(i, n)`, `removeAllRows()` |
| columns | `addColumn(name, cls, vals)`, `addColumnAt(i, ..)`, `setColumnAt(i, vals)`, `removeColumnAt(i)`, `removeColumnsAt(i, n)`, `setColumnNameAt(i, name)`, `setColumnClassAt(i, cls)`, `setColumnNames(..)`, `setColumnClasses(..)` |
| whole | `setCells(t)`, `withLayout(listData)`, `TableData.empty()`, `TableData.row(vals…)` |

Rows, columns, names, classes and both counts may **all change at any time** —
reshaping a table is just another value, not a special case. Indices rot when columns
move, so address columns by meaning: `it.setCellAt(0, it.indexOfColumn("Age"), 31)`.

**Performance — do not hand-roll around it.** `Tuple`s are persistent (structural
sharing: adding a row to a 1000-row table copies no rows), and a `ROW_MAJOR` table
forwards the tuple's change-diff to the `JTable` as **targeted** row events — a row
add repaints that row, not the table. **Prefer range ops**: `addRows(..)` /
`removeRowsAt(..)` / `setRowsAt(..)` emit **one** event instead of N.
(`COLUMN_MAJOR` stores columns, so a change never maps onto a row range and it must
rebuild — use `ROW_MAJOR` for big/lively tables. All methods still speak
`(row, column)` in either layout.)

**Editable needs BOTH** a `*_EDITABLE` layout **and** a mutable `Var` — a `Val`, or a
`Var` with a non-editable layout, yields a read-only table. Edits flow back into the
property as a new value. Flip it live with `it.withLayout(ROW_MAJOR_EDITABLE)`.

`getColumnClass` drives the `JTable`'s renderer/editor, so
`setColumnClassAt(i, Boolean.class)` buys you check boxes for free.

Custom cell rendering: `.withCell(cell -> cell.view(c -> c.orGetUi(() -> textField()).updateIf(JTextField.class, tf -> { tf.setText(cell.entryAsString()); return tf; })))`.

#### Legacy table sources — still supported, but prefer `TableData`

All of these are **pull-based**: they need `updateTableOn(..)`/`updateOn(..)`, they
cannot say *what* changed (so every refresh rebuilds the whole table), and they are
read live — which forces SwingTree to copy the whole table on every refresh under
`DECOUPLED`.

```java
UI.table().withModel(m -> m.colName(i -> headers[i]).colCount(() -> headers.length)
    .rowCount(() -> data.length).getsEntryAt((r,c) -> data[r][c])
    .setsEntryAt((r,c,val) -> data[r][c] = (int) val)
    .isEditableIf(() -> true).updateOn(dataChangedEvent));   // must .fire() by hand
UI.table(UI.ListData.ROW_MAJOR_EDITABLE, () -> listOfRows).updateTableOn(evt);
UI.table(UI.MapData.EDITABLE, () -> mapOfColumns).updateTableOn(evt);
UI.table(UI.ListData.ROW_MAJOR, tupleVar);   // Var<Tuple<Tuple<E>>>: TableData minus the
                                             // column metadata; keeps the diff fast-path
```
`BasicTableModel` is only a *description of where the data lives* — SwingTree wraps it
in a thread-safe model of its own, so `JTable.getModel()` does **not** return the
object you passed to `withModel(..)`.

Full prose: [Writing-Tables.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Writing-Tables.md).
Executable catalogue of the whole `TableData` API:
[Table_Data_Spec.groovy](https://github.com/globaltcad/swing-tree/blob/main/src/test/groovy/swingtree/Table_Data_Spec.groovy).

### Icons & SVG (first-class, HiDPI-crisp)

SVG works **everywhere an icon can appear** (icons, buttons, labels, tabs,
menus, dialogs, style-API images), rendered via JSVG + Java2D, re-rendered at
the current UI scale — never blurry. All icon sizes are developer px (§13).

**`IconDeclaration` — the right type for view models.** A lightweight immutable
value (path or SVG text + preferred size); loading is lazy + cached, and a
missing resource logs instead of throwing. It is a functional interface over
`source()`:

```java
IconDeclaration funnel = () -> "img/funnel.svg";        // simplest: lambda
enum Icons implements IconDeclaration {                  // idiomatic: constants
    FUNNEL("img/funnel.svg"), SEED("img/seed.png");
    private final String path;
    Icons(String p){ this.path = p; }
    @Override public String source(){ return path; }
}
Icons.FUNNEL.withSize(24, 24) / .withWidth(24)           // sizing withers
IconDeclaration.ofSvg(svgText)                           // SVG string; reports the size declared in the SVG
IconDeclaration.ofAutoScaledSvg(svgText)                 // SVG string; size -1 -> stretches to its component
```

**Using them:** `icon(decl)`, `icon(48, 48, decl)`, `button(decl)`,
`label("x").withIcon(decl)`, `tab("t").withIcon(decl)`. **Dynamic:** bind a
`Var<IconDeclaration>` — `icon(iconProp)`, `labelWithIcon(iconProp)`,
`buttonWithIcon(iconProp)`, `menuItem("Connect", iconProp)`; set the property
and the icon swaps. View models hold `IconDeclaration`s, never `ImageIcon`s.

**Loading by hand:** `UI.findIcon("path")` → `Optional<ImageIcon>` (classpath →
file system → cache; returns an `SvgIcon` for `.svg`); `UI.findSvgIcon(..)` →
`Optional<SvgIcon>`. Cache lives in `SwingTree.get().getIconCache()`, keyed by
declaration — prefer declarations over hand-built `SvgIcon`s so equal
declarations share one instance.

**`SvgIcon`** (`swingtree.style`) — immutable `ImageIcon` subclass; construct
directly only when the SVG text is dynamic (editors, server-sent graphics):
`SvgIcon.of(svgString | stream | document)` / `SvgIcon.at(path | url)`, then
`.withIconSize(w,h)`, `.withIconSizeFromWidth(w)` (height from aspect ratio),
`.withOpacity(f)`, `.withFitComponent(..)`, `.withPreferredPlacement(..)`.
Reported size (`getIconWidth()/getIconHeight()`, DPI-scaled): an explicit size
wins; else a **directly constructed** `SvgIcon.at/of(..)` adopts the px
`width`/`height` declared in the SVG text; **-1** (= unknown → icon adapts to
its component) when those are missing, `%`-based, or non-px units — **and for
every declaration-pipeline load** (`findIcon`, `icon(path)`,
`IconDeclaration.of(path)`): the declaration's default `Size.unknown()`
deliberately resets the icon to flexible. While a dimension is unknown, two
policies control rendering: `UI.FitComponent` — `NO`, `WIDTH`, `HEIGHT`,
`WIDTH_AND_HEIGHT` (these three may distort), `MIN_DIM`/`MAX_DIM` (fit
smaller/larger dimension, keep aspect ratio — usually what you want) — and
`UI.Placement` (`CENTER`, `TOP_LEFT`, … 9 positions). `.getImage()` rasterizes
to a `BufferedImage` (loses scalability — visibly blurry when stretched).

**Style API images:** `.image(img -> img.svg(svgText).fitMode(..).placement(..))`
or `img.image(iconDeclOrImageIcon)`; plus `opacity`, `size`, `offset`, `repeat`,
`primer(color)`, `clipTo(ComponentArea.BODY|BORDER|INTERIOR|..)`. Layer via the
outer overload `image(Layer.BACKGROUND, img -> ..)`. If the SVG text/config comes
from a property, use the property-bound `withStyle(prop, (svg, it) -> ..)` (§8).
Playground example covering all of this:
[SvgViewer.java](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/stylish/SvgViewer.java).

### Dialogs (`JOptionPane` wrappers)

```java
ConfirmAnswer a = UI.confirmation("Continue?").titled("Confirm").show();   // YES/NO/CANCEL/CLOSE
UI.confirmation("Heads up!").showAsWarning();   // .showAsError() .showAsInfo()
UI.message("Saved.").showAsInfo();              // no return value
// customize buttons: .yesOption("OK").noOption("").cancelOption("")  (empty hides a button)
```

---

## 11. Threading & lifecycle

- SwingTree binding/animation callbacks run on the **EDT**. Business logic that you
  trigger via `on(..)` runs on the **application thread**.
- `UI.run(r)` runs on EDT now; `UI.runLater(r)` / `runLater(delay, r)` defer to EDT.
- In a `main`, after `UI.show(...)`, call `EventProcessor.DECOUPLED.join()` to keep
  the (decoupled) application thread alive so the program doesn't exit.
- **Under `DECOUPLED`, never let the EDT read mutable application state.** Bind
  *values* (immutable records, `Tuple`s, `TableData` — §10) rather than live data
  sources: an immutable value cannot be seen half-updated, so no locking, no torn
  reads. Pull-based sources (lambda/collection table models, §10) force SwingTree to
  copy the whole thing on every refresh to get the same guarantee.
- Set a Look-and-Feel before showing if desired (examples use FlatLaf:
  `FlatDarkLaf.setup();` / `FlatLightLaf.setup();`).

---

## 12. Escape hatches & error containment

SwingTree wraps **every lambda it invokes for you** in try/catch + SLF4J logging,
so a thrown exception in one fragment doesn't tear down the whole UI ("the show
must go on"). Caught: `peek`, `apply`, `applyIf`, `applyIfPresent`, `withStyle`,
all `onXyz` handlers, and `zoomTo` map/wither lambdas. **NOT** caught: code at the
top level of your declaration (your own `for`/`if`/arithmetic *outside* a captured
lambda) — push risky top-level code into `apply(ui -> ...)` or `peek(c -> ...)`.

| Hatch | Use |
|---|---|
| `.peek(c -> ...)` | **last resort** — reach into the raw Swing component only when SwingTree wraps no equivalent (see the caution below) |
| `.apply(ui -> ...)` | imperative loop that `add(..)`s many children (the lambda gets the builder) |
| `.applyIf(boolean, ui -> ...)` | inline conditional sub-tree (static shape decisions) |
| `.applyIfPresent(Optional<Consumer<I>>)` | inline `Optional`-driven sub-tree |
| `.get(JPanel.class)` | unwrap the builder to the real component |
| `UI.of(jcomponent)` | wrap a hand-rolled/3rd-party component into the tree |

> **Prefer reactivity over hatches.** If a condition depends on app state, bind it
> (`isVisibleIf`, `isEnabledIf`, property-bound `add`) instead of `applyIf`, so the
> UI updates automatically. The hatches are for *construction-time* decisions.

> **`peek(..)` is a code smell — always look for a SwingTree method first.** It hands
> you the raw component and steps *outside* SwingTree's control, forfeiting what the
> library gives you for free: HiDPI "developer-pixel" scaling (§13), the style
> engine's ownership of colours/opacity/borders (§7), decoupled-thread safety (§11),
> and any usability fixes SwingTree layers over raw Swing. So before writing `peek`,
> look for the SwingTree variant — a `with*` setter (e.g. `withPrefSize`,
> `withBackground`, `withTooltip`), an `is*If(Val<Boolean>)` binding, an `on*(..)`
> event handler, `withStyle(..)`, or `withProperty(key, value)` for a client
> property. `peek` is legitimate **only** when no such method exists — a niche Swing
> setter SwingTree genuinely does not wrap (say `JTable#setRowHeight`), or capturing
> a third-party component — and then keep it to that one imperative line.

---

## 13. HiDPI scaling — "developer pixels" vs "component pixels"

SwingTree maintains one **UI scale factor** (`UI.scale()`, a `float`, derived
from the system font) and applies it everywhere, because vanilla Swing + the
JDK's bundled Look-and-Feels do **not** scale for HiDPI. This creates two
coordinate spaces:

- **Developer pixels** — the DPI-agnostic numbers *you* write (`withPrefSize(100,50)`).
- **Component pixels** — the real scaled numbers Swing lays out/paints (at scale `2.0` → `200×100`).

**The symmetry you can rely on:** everything you pass *into* the SwingTree API is
in developer pixels and gets scaled **up** for you; everything SwingTree reads
*back* for you is scaled **down** into developer pixels. So values round-trip
cleanly — you almost never call `UI.scale(..)` yourself.

- **Inputs scaled up:** all builder dims (`withPrefSize/withMinSize/withWidth/withSizeExactly/...`)
  and all style dims (`prefSize`, `minHeight`, `margin`, `padding`, `borderWidth`,
  `borderRadius`, gradient/shadow offsets & sizes, …).
- **Outputs scaled down (already in developer px):**
  - Style delegate: `it.componentWidth()`, `it.componentHeight()`,
    `it.componentPrefWidth()`, `it.componentPrefHeight()`.
  - Event delegates (`onClick`, `onResize`, `onMouseMove`, `onDrag`, …):
    `it.getX/getY/getPosition`, `it.getWidth/getHeight/getSize`, `it.getPrefSize`,
    `it.getBounds`; setters like `it.setBounds/setPrefSize/setMinSize` take
    developer px. Mouse: `it.mouseX()/mouseY()/mousePosition()`. Drag:
    `it.initialComponentPosition()`, `it.dragPositions()`, `it.deltaXSinceStart()`.

> **THE DOUBLE-SCALING TRAP (this is why `component()` is deprecated):** the raw
> Swing component returns **component pixels**. If you read
> `it.component().getPreferredSize().height` (already scaled) and pass it back
> into a scaling method like `minHeight(..)`, it is scaled **twice** — min height
> becomes `200` when you meant `100`, and the error grows with the scale factor.
> **Fix:** use the developer-pixel accessor instead:
> ```java
> .withStyle( it -> it.minHeight(it.componentPrefHeight()) )   // ✅ round-trips; NOT it.component().getPreferredSize().height ❌
> ```

> **THE ONE EXCEPTION:** absolute on-screen coords are **raw**, not unscaled —
> `it.mouseXOnScreen()`, `it.mouseYOnScreen()`, `it.mousePositionOnScreen()` are
> in real screen pixels (they're desktop-absolute, possibly multi-monitor).

Only call the raw helpers when working **against raw Swing** (custom `Graphics2D`
painting, a peeked component, a third-party widget): `UI.scale(int|float|double)`
(developer→component), `UI.unscale(int|float|Dimension)`
(component→developer), `UI.scale(Graphics2D)` (scales a context in place),
`UI.scale()` (the raw factor). Override the factor with
`SwingTree.get().setUiScaleFactor(2.0f)` or
`SwingTree.initializeUsing(cfg -> cfg.uiScaleFactor(2.0f))`. Full prose:
[HiDPI-Scaling.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/HiDPI-Scaling.md).

## 14. Hard-won gotchas (check these in any review)

1. **A view that only works at one window size is a bug.** Build convergent by
   default (§2c): `wmin 0` / `withMinSize(0,0)` everywhere, a 12-column
   `AUTO_SPAN` grid for the page, a `scrollPane(conf -> conf.fitWidth(true))`
   around it so the stacked arrangement can outgrow the window.
2. **Minimum sizes are a hard floor and propagate upward.** A label's minimum
   width is its full text and a flow grid's minimum is the **sum** of its
   children's — one forgotten row gives the whole *window* a minimum width and
   the responsive bands become unreachable. `"wmin 0"` on rows, `withMinSize(0,0)`
   on grids, `width 90::200` instead of `width 200!`.
3. **A responsive grid nests inside another grid — never inside a MigLayout
   cell.** `withPrefSize(w, 0)` declares the reference width, but
   `getPreferredSize()` short-circuits the layout manager, so a MigLayout parent
   reads that literal `0` and the nested grid **collapses to zero height**,
   silently clipping its content. Make the containing card a grid too (§2d). Also
   give a preferred height to anything that has none (`scrollPane`,
   `scrollPanels`, empty `textField`) — a grid row is only as tall as its
   tallest child *prefers* to be.
4. **Never `setOpaque(..)`** on a styled component — the style engine controls
   opacity; manual calls fight it. Use `backgroundColor(Color.TRANSPARENT)` / a real
   color in `withStyle` instead.
5. **`Tuple` items bound for *per-item editing* (`addAll(Var<Tuple<M>>, entry -> ..)`,
   where `entry` is a `Var<M>` lens) must `implement HasId<UUID>`** with a stable id —
   otherwise equal value-records collide and bindings target the wrong sub-view. The
   read-only `addAll(Val<Tuple<M>>/Tuple<M>, m -> ..)` overloads pass the value and
   need no `HasId` (§5.2).
5b. **A bound `addAll` owns its container and clears hand-added children, and its
   row supplier runs *later*** — so give the list a panel of its own, and under a
   `StyleSheet` wrap the supplier in `UI.use(sheet, ..)` or every row built after
   the first model change comes out unstyled (§5.2, §7).
6. **Hold a strong reference (a view field) to any lens used only by a raw
   `onChange` subscription** — weak observation will GC it and silently break (§9c).
7. **Tables: bind a `Var<TableData>`; don't reach for a `TableModel` or a pull-based
   data source** (§10). An editable table needs **both** a `*_EDITABLE` layout **and**
   a mutable `Var` — either alone is silently read-only. Use **`ROW_MAJOR`** (the
   diff-driven, incremental path) and **range ops** (`addRows`/`removeRowsAt`/
   `setRowsAt`) for bulk changes; per-row loops emit one event each.
8. **Never read property values inside a plain `withStyle` lambda** — use the
   property-bound `withStyle(prop, (item, it) -> ..)` (§8), which captures the item
   thread-safely and repaints automatically. (`withRepaintOn(props) + prop.get()`
   is the legacy version of this pattern.) When one style depends on **several**
   properties, merge them into one record with the Sprouts ≥2.7 composite view builder
   `Viewable.of(seed, it -> it.join(p, wither)…)` and drive it from a single
   `withStyle` — no need to chain one per property (§8).
9. **Pick the right thread:** `onView` for view-touching handlers, `on` for
   model/business handlers; respect `From.VIEW` vs `From.VIEW_MODEL` to avoid
   feedback loops.
10. **View models import zero Swing classes.** If you find a `JComponent` in a view
   model, the architecture is wrong.
11. Expose **`Val`** (not `Var`) from a view model for fields the view must not write.
12. Use **enum** group tags and the type-safe layout constants for refactor safety.
13. Withers must be **pure** and return **new** instances (Lombok `@With` on records
   is the cleanest path); never mutate `this`.
14. **Never feed a raw Swing size/position back into the SwingTree API** — values
    from `it.component().getPreferredSize()`/`getBounds()`/`getWidth()` are in
    *component pixels* (already scaled); passing them to `minHeight(..)`/`size(..)`/etc.
    double-scales them. Read geometry through the delegate accessors
    (`componentPrefHeight()`, `getWidth()`, `mouseX()`, …) which give developer pixels. (§13)
15. **`peek(..)` is a code smell — prefer a SwingTree method.** Raw-component tweaks
    step outside HiDPI scaling, the style engine and decoupled-thread safety; reach
    for a `with*`/`is*If`/`on*`/`withStyle`/`withProperty` method first. `peek` is
    legitimate only when SwingTree wraps no equivalent (§12).

---

## 15. Cheat sheet

```java
import static swingtree.UI.*;
import sprouts.*;                        // Var, Val, Vars, Vals, Tuple, From, Viewable, HasId, Event

// build + show
UI.show(panel("fill, wrap 2").add("growx", textField(name)).add(button("Go").onClick(it -> ...)));
UI.show(f -> new MyView(vm)); EventProcessor.DECOUPLED.join();

// view skeleton
UI.of(this).withLayout(FILL.and(WRAP(1)).and(INS(16))).add(GROW, child);

// state
Var<T> v = Var.of(value);  v.get(); v.set(x); v.update(fn);  Val<U> d = v.viewAsString(fn);
v.isEnabledIf / isVisibleIf / isSelectedIf / isEditableIf (Val<Boolean>)
Viewable<T> c = Viewable.of(a, b, (x,y) -> combine);     // derived from 2 sources; result type = a's type
Viewable<R> r = Viewable.of(R.class, a, b, (x,y) -> ..); // ...or with an explicitly different result type
Viewable<C> m = Viewable.of(seed, it -> it.join(a,C::withA).join(b,C::withB).join(c,C::withC)); // N sources → 1 record (Sprouts ≥2.7)
Viewable<T> w = v.view();                                // weakly-held listenable view (store in a field!)

// sprouts immutable collections (persistent; every op returns a new instance)
Tuple<T> t = Tuple.of(a,b,c) / Tuple.of(T.class);        // List-like: add/remove/setAt/map/retainIf/sort
Association<K,V> m = Association.between(K.class,V.class);// Map-like: put / get(k)->Optional / remove   (NOT .of!)
ValueSet<E> s = ValueSet.of(E.class);                    // Set-like: add/addAll/retainAll/any
Result<T> res = Result.ofTry(T.class, () -> risky());    // Maybe<T> + Tuple<Problem>; res.problems()/orElse(x)

// lenses (MVI/MVL)
Var<F> f = root.zoomTo(Root::f, Root::withF);             // mutable lens
Val<F> r = root.viewAs(F.class, Root::f);                 // read-only view
Var<V> e = root.zoomTo(c -> c.get(k).orElse(d), (c,x) -> c.put(k,x));  // lens into a collection entry
Var<Tuple<Item>> items = root.zoomTo(Root::items, Root::withItems);
panel.addAll(items, (Var<Item> it) -> itemView(it));      // per-item lens ⇒ Item implements HasId<UUID>!
panel.addAll(roTuple, (Item it) -> itemView(it));         // read-only value ⇒ no HasId needed

// tables (§10) — an immutable value describing the WHOLE table; bind it and it follows
Var<TableData> d = Var.of(TableData.of(UI.ListData.ROW_MAJOR, "Name","Age").addRow("Alice",30));
UI.table(d);  d.update(it -> it.addRow("Bob", 42));      // no updateTableOn/Event needed
it.setCellAt(r,c,v) / .addRowAt(i,vals…) / .removeRowAt(i) / .setColumnClassAt(i,Boolean.class)
it.addRows(t) / .removeRowsAt(i,n) / .setRowsAt(i,t)     // range ops ⇒ ONE table event, not N
// editable ⇔ *_EDITABLE layout AND a mutable Var; ROW_MAJOR ⇒ incremental (diff) updates

// events
.onClick / .onMouseEnter / .onMouseClick / .onKeyPress / .onResize (it -> ...)
.on(observable, it -> appWork)        .onView(observable, it -> viewWork)

// style
.withStyle(it -> it.padding(8).borderRadius(12).backgroundColor(c).shadowBlurRadius(6)
                   .gradient(Layer.BACKGROUND,"g",g->g.type(GradientType.RADIAL).colors(a,b))
                   .componentFont(fc -> fc.size(14).family("Serif")))
.withStyle(prop, (item, it) -> it.backgroundColor(item.color()))  // property-driven, auto-repaint (§8)
.withStyle(Viewable.of(seed, it -> it.join(a,Seed::withA).join(b,Seed::withB)), (m,it)->..) // N props → 1 styler (Sprouts ≥2.7; §8)
.withRepaintOn(eventA, eventB)
.withTransitionalStyle(boolVar, LifeTime.of(0.4, SECONDS), (state, it) -> it. ...progress()...)

// animation
it.animateFor(0.5, TimeUnit.SECONDS, s -> ... s.progress() / s.fadeIn() / it.paint(s, g->...));
UI.animateFor(2, SECONDS).go(s -> p.set(s.progress()));
UI.animate(vm, ViewModel::someAnimatable);

// convergence — the default page skeleton (§2c). Categories are FIFTHS of the reference width.
scrollPane(conf -> conf.fitWidth(true)).withHorizontalScrollBarPolicy(UI.Active.NEVER).add(
  panel().withFlowLayout(UI.HorizontalAlignment.LEFT, 18, 18)
  .withMinSize(0,0)                    // a grid's minimum is the SUM of its children's — kill it
  .withPrefSize(REFERENCE_WIDTH, 0)    // declares where the bands sit; MANDATORY for a nested grid
  .add(AUTO_SPAN(it->it.fill(true).verySmall(12).small(12).medium(12).large(5).veryLarge(4).oversize(4)), sidebar)
  .add(AUTO_SPAN(it->it.fill(true).verySmall(12).small(12).medium(12).large(7).veryLarge(8).oversize(8)), content));
.add("growx, wmin 0", label(..))       // or its text becomes the window's minimum width
scrollPanels().withPrefSize(340, 470)  // a grid row is only as tall as its tallest child PREFERS
// ⚠ a grid with withPrefSize(w,0) must sit in a GRID or a fitWidth scrollPane — a MigLayout
//   cell reads the literal 0 and the grid renders at zero height (§2d)
label(..).isVisibleIf(isWide)           // + "hidemode 3" on the container ⇒ content converges too

// reactive layout (gear 2 — reflow, nothing rebuilt: focus/caret/scroll survive)
Var<Layout> L = Var.of(Layout.class, Layout.mig("fill, wrap 1"));
panel(L)...;  L.set(Layout.mig("fill, wrap 2, nogrid").withChildConstraints(MigAddConstraint.of("growx, span 2")));
// every variant must give EVERY child a constraint (positional, only overwritten where supplied)

// form factor (gear 3 — swaps the tree; needs hysteresis, loses component state)
.onResize(it -> ff.update(From.VIEW, f -> Formfactor.of(it.getWidth(), it.getHeight(), f)))
.add(GROW.and(PUSH), ff, this::body);

// icons & SVG (crisp at any DPI; sizes in developer px)
IconDeclaration ic = () -> "img/x.svg";                  // value object -> belongs in view models
IconDeclaration.ofSvg(svgText) / .ofAutoScaledSvg(svgText) / ic.withSize(24,24)
icon(ic) / button(ic) / label("x").withIcon(ic) / tab("t").withIcon(ic)
icon(iconProp) / labelWithIcon(iconProp) / buttonWithIcon(iconProp)   // Val<IconDeclaration> -> swaps live
UI.findIcon("img/x.svg") / UI.findSvgIcon(..)            // Optional<..>, classpath + cache
SvgIcon.of(svgText).withIconSizeFromWidth(64).withFitComponent(FitComponent.MIN_DIM)
.withStyle(it -> it.image(img -> img.svg(svgText).fitMode(..).placement(..)))

// style sheet + theme
UI.use(sheet, () -> UI.show(f -> new View()));   // sheet.reconfigure() hot-swaps

// escape hatches  (peek = last resort; prefer a with*/is*If/on*/withStyle method — §12)
.peek(c -> c.setX(..)).apply(ui -> {for(..) ui.add(..);}).applyIf(cond, ui -> ui.add(..)).get(JPanel.class)

// HiDPI scaling — you write developer px (scaled up), delegates return developer px (scaled down)
.withStyle(it -> it.minHeight(it.componentPrefHeight()))   // ✅ round-trips; NOT it.component().getPreferredSize().height ❌
it.getWidth()/getHeight()/getBounds()/mouseX()/mouseY()    // all developer px;  mouse*OnScreen() = raw screen px
UI.scale(int|float|double) / UI.unscale(..) / UI.scale(g2d)  // only when working against RAW Swing
```

### Runnable examples in the SwingTree repo (read these for full context)

All example sources live under [`src/test/java/examples/`](https://github.com/globaltcad/swing-tree/tree/main/src/test/java/examples)
in the repo; the links below open each on GitHub.

- [`calculator/mvi/CalculatorView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/calculator/mvi/CalculatorView.java) — canonical MVI/MVL.
- [`team/mvi/TeamView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/team/mvi/TeamView.java) **vs** [`team/mvvm/TeamView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/team/mvvm/TeamView.java) — same UI, both architectures.
- [`chat/mvi/ChatView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/chat/mvi/ChatView.java) (+ `ChatViewModel`, `Room`, `Message`, `ChatStyle`, `ChatArt`) — **the reference for `Tuple` + `addAll` + `HasId`**, inside a whole messenger: a room rail, a roster, message bubbles editable in place, and emoji reactions, all bound off one immutable root. Three less obvious ideas live here too: a **lens onto a *computed* projection** (`vm.zoomTo(ChatViewModel::visibleMessages, ChatViewModel::withVisibleMessages)` — the getter filters the selected room by the search box, the wither merges edits and deletions back by `id`, so one lens reacts to three inputs with zero listeners); **generated SVG as a value** (`ChatArt` builds the room sigils and a "conversation ribbon" as SVG *text*, fed to `withStyle(svgVal, (svg, it) -> it.image(img -> img.svg(svg)))`); and a hot-swapped `StyleSheet` whose row suppliers **re-enter the `UI.use(..)` scope** — the gotcha that otherwise leaves every dynamically added row unstyled (§5.2).
- [`trains/mvi/TrainsView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/trains/mvi/TrainsView.java) (+ `TrainsViewModel`, `TransitClient`) — real-world MVI: `Tuple`-valued state, a Swing-free data layer doing blocking IO off the EDT, and Lombok `@With`/`@Getter` value objects (records-free, **Java 8**-clean).
- [`budget/mvi/BudgetView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/budget/mvi/BudgetView.java) (+ `BudgetViewModel`, `Budget`, `BudgetHealth`) — **the reference for convergence (§2c/2d): four arrangements of three cards from one span table, with zero state.** It also showcases three other ideas at once: a **value-model table** bound with `UI.table(Var<TableData>)` (editable, edits flow back as a new value; a `withCellForColumn` renderer/editor euro-formats the Amount column yet commits back a `Double`), a **value-capturing SVG style** `withStyle(svgText, (svg, it) -> it.image(img -> img.svg(svg)))` driving a donut chart generated from the data, and a **composite view** `Viewable.of(seed, it -> it.join(a, ..).join(b, ..)…)` (Sprouts ≥2.7) merging three properties into one item for a single `withStyle`.
- [`breathing/mvi/BreathingView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/breathing/mvi/BreathingView.java) (+ `BreathingViewModel`) — modelled animation, re-arming, the GC gotcha.
- [`animated/AnimatedView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/animated/AnimatedView.java) / [`TransitionalAnimation.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/animated/TransitionalAnimation.java) — the full animation primitive tour.
- [`zen/ThemeGardenView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/zen/ThemeGardenView.java) (+ `ThemedStyleSheet`) — style sheets, groups, runtime theme swap.
- [`scribe/CelestialScribe.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/scribe/CelestialScribe.java) — `Layout.none()` derived from data, styled text flowing around children.
- [`dashboard/SalesDashboard.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/dashboard/SalesDashboard.java) — reactive `Var<Layout>` reflow.
- [`almanack/mvi/AlmanackView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/almanack/mvi/AlmanackView.java) (+ `AlmanackViewModel`) — every tab binding mechanism in one field-notebook app: a two-way `Var<Integer>` selection index that may point at tabs which don't exist yet (deferred selection), `addAll(Val<Tuple<M>>, TabSupplier)` dynamic tabs, enum⇄index lenses, bound tab titles/tooltips/enabled flags.
- [`stylish/SoftUIView.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/stylish/SoftUIView.java) — soft-UI style sheet, custom paint.
- [`stylish/SvgViewer.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/stylish/SvgViewer.java) — SVG playground: one SVG rendered through four pipelines (`SvgIcon` in style API, `img.svg(..)` string, rasterized `getImage()`, component icon) with live `Placement`/`FitComponent` switching.
- [`simple/ResponsiveLayout.java`](https://github.com/globaltcad/swing-tree/blob/main/src/test/java/examples/simple/ResponsiveLayout.java) (+ `ResponsiveLayoutAlign`, `ResponsiveLayoutFill`) — the smallest `AUTO_SPAN` responsive flow demo.

**Convergent examples, by which gears they use (§2c):** `budget/mvi/BudgetView`
and `zen/ThemeGardenView` (gears 0+1, pure span tables); `animated/AnimatedView`
(0+1 with a **nested** grid — the recipe list is a column as a sidebar, a chip
grid when stacked); `team/mvi/TeamView` + its `mvvm` twin (0+1, master–detail
with a nested responsive *form*, and the grid-in-a-grid card that makes it
measure correctly); `breathing/mvi/BreathingView` (0+1 plus size-relative
*painting* — the orb is sized from its box, not in pixels);
`almanack/mvi/AlmanackView` (0+2+4, four breakpoints feeding four `Val<Layout>`
properties, nothing ever rebuilt); `trains/mvi/TrainsView` (0+2+3+4 — a
`Formfactor` in the view model swapping a split pane for a scrolling column,
plus a reactive toolbar and bound labels that shorten);
`chat/mvi/ChatView` (0+1+2+4 and **deliberately no gear 3** — a chat is full of
state you must not destroy, so every shape is reached by reflowing: nested grids
turn the room rail and the roster from sidebars into banners, a `Val<Layout>`
composer measures *its own* width rather than the window's, and the conversation's
preferred height is derived from the window inside the view model, because a flow
grid gives a row the height its tallest child *prefers* and never stretches it).

The wiki ([`docs/markdown/`](https://github.com/globaltcad/swing-tree/tree/main/docs/markdown)) is the prose
companion; start at [README.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/README.md) →
[Climbing-Swing-Tree.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Climbing-Swing-Tree.md) →
[Functional-MVVM.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Functional-MVVM.md).
For layout specifically:
[Convergent-Design.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Convergent-Design.md)
(strategy + checklist) →
[Responsive-Layouts.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Responsive-Layouts.md)
(grid mechanics, nesting rules, a debugging table) →
[Reactive-Layouts.md](https://github.com/globaltcad/swing-tree/blob/main/docs/markdown/Reactive-Layouts.md).