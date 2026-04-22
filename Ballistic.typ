#import "@preview/cetz-plot:0.1.3": plot
#import "@preview/cetz:0.4.2" as cetz

Let $arrow(x)$ be the position of the Cannonball, represented as a point of mass $m$.
Let assume only gravity ($arrow(g) = -g arrow("UP")$) applies to the Cannonball.

Then
$
  & m arrow(a)  && = m arrow(g) \
  & => arrow(a) && = arrow(g) \
  & => arrow(v) && = arrow(g) t + arrow(v_0) \
  & => arrow(x) && = 1/2 arrow(g) t^2 + arrow(v_0) t + arrow(x_0)
$

Lets search $arrow(v_0)$ such that $norm(arrow(v_0)) = k$ and at $t = t_1$, $arrow(x)(t_1) = arrow(x_1) => arrow(x_1) dot arrow("UP") = arrow(0)$ (assuming $arrow(x_0) dot arrow("UP") > 0$ and $t >= 0$).

Then
$
  cases(
    arrow(x_1) & = 1/2 arrow(g) t_1^2 + arrow(v_0) t_1 + arrow(x_0),
    norm(arrow(v_0)) & = k,
  )
  \ <=> cases(
    arrow(v_0) & = (arrow(x_1) - arrow(x_0)) / t_1 - 1/2 arrow(g) t_1,
    arrow(v_0) dot arrow(v_0) & = k^2,
  )
$

Then (with $T = t_1^2$ and $delta arrow(x) = arrow(x_1) - arrow(x_0)$)

$
  & ((delta arrow(x)) / t_1 - 1/2 arrow(g) t_1) dot ((delta arrow(x)) / t_1 - 1/2 arrow(g) t_1) &&= k^2
  \ <=> & 1 / t_1^2 norm(delta arrow(x))^2 - delta arrow(x) dot arrow(g) + t_1^2/4 norm(arrow(g))^2 &&= k^2
  \ <=> & 1/4 norm(arrow(g))^2 T^2 - (delta arrow(x) dot arrow(g) + k^2) T + norm(delta arrow(x))^2 &&= 0
  \ <=> & ((delta arrow(x) dot arrow(g) + k^2) plus.minus sqrt((delta arrow(x) dot arrow(g) + k^2)^2 - norm(arrow(g))^2 norm(delta arrow(x))^2)) / (1/2 norm(arrow(g))^2) &&= T space^*
$

\* if $(delta arrow(x) dot arrow(g) + k^2)^2 - norm(arrow(g))^2 norm(delta arrow(x))^2 < 0$, then $k$ is not enough for the target to be reached.

Thus $t_plus.minus = sqrt(T_plus.minus)$ and $arrow(v_0) = (delta arrow(x))/t_plus.minus - 1/2 arrow(g) t_plus.minus$


Lets plot the 2 results to see the difference :

#let x0 = 0
#let y0 = 4
#let x1 = 10
#let y1 = 0
#let k = 7
#let g = -9.81
#let dt = 0.01

#let a = g * g / 4
#let b = -((y1 - y0) * g + k * k)
#let c = (y1 - y0) * (y1 - y0) + (x1 - x0) * (x1 - x0)

#let T_plus = (-b + calc.sqrt(b * b - 1 * a * c)) / (2 * a)
#let T_minus = (-b - calc.sqrt(b * b - 1 * a * c)) / (2 * a)

#let t_plus = calc.sqrt(T_plus)
#let t_minus = calc.sqrt(T_minus)

#let vx0_t_plus = (x1 - x0) / t_plus
#let vy0_t_plus = (y1 - y0) / t_plus - 1 / 2 * g * t_plus

#let vx0_t_minus = (x1 - x0) / t_minus
#let vy0_t_minus = (y1 - y0) / t_minus - 1 / 2 * g * t_minus

($arrow(x_0) = vec(#[#x0], #[#y0], 0, delim: "[")$, $arrow(x_1) = vec(#[#x1], #[#y1], 0, delim: "[")$, $k = #k$)

#let compute_position(x0, y0, vx0, vy0) = {
  let positions = ((x0, y0),)
  let x = x0
  let y = y0
  let vx = vx0
  let vy = vy0

  while y > 0 {
    let ax = 0
    let ay = g
    vx += ax * dt
    vy += ay * dt
    x += vx * dt
    y += vy * dt
    positions.push((x, y))
  }

  positions
}

#cetz.canvas({
  let trajectory_plus = compute_position(x0, y0, vx0_t_plus, vy0_t_plus)
  let trajectory_minus = compute_position(x0, y0, vx0_t_minus, vy0_t_minus)

  let x_max = calc.max(calc.max(..trajectory_plus.map(xy => xy.at(0))), calc.max(..trajectory_minus.map(xy => xy.at(
    0,
  ))))

  let y_max = calc.max(calc.max(..trajectory_plus.map(xy => xy.at(1))), calc.max(..trajectory_minus.map(xy => xy.at(
    1,
  ))))

  plot.plot(
    size: (10, 5),
    axis-style: "school-book",
    x-equal: "y",
    x-tick-step: 5,
    y-tick-step: 5,
    x-max: x_max * 1.02,
    y-max: y_max * 1.02,
    {
      plot.add(trajectory_plus, label: $t_+$)
      plot.add(trajectory_minus, label: $t_-$)
    },
  )
})

Thus, for the game, to have interesting parabolic trajectories, we will use $t_+$ for $arrow(v_0)$ computation.

Now, we also want to know which $k$ to set. We want to have the entire map at range. Thus, let $d$ be the longest distance of the map (its diagonal on our case).

We can now reduce the problem in 2 dimension because the trajectory is contained into a plan.

let $K = k^2$
$
  & 0 &&= (k^2 - y_0 g)^2 - g^2 (d^2 + y_0^2)
  \ <=> & k^4 - 2 k^2 y_0 g + y_0^2 g^2 &&= g^2 (d^2 + y_0^2)
  \ <=> & K^2 - 2 K y_0 g - g^2 d^2 &&= 0
  \ <=> & K = y_0 g plus.minus sqrt(y_0^2 g^2 + g^2 d^2)
  \ <=> & K = g (y_0 plus.minus sqrt(y_0^2 + d^2))
$

But $sqrt(y_0^2 + d^2) > y_0$ so there is only one $K$ that is positive : $K = g (y_0 + sqrt(y_0^2 + d^2))$
Thus, $k = sqrt(g (y_0 + sqrt(y_0^2 + d^2)))$