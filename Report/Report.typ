#import "@preview/algorithmic:1.0.7"
#import algorithmic: algorithm-figure, style-algorithm
#show: style-algorithm

#let title = [
  Computer Graphics (CSED451-01)
  #linebreak()
  Assignment 3.2: 3D Drawing with OpenGL Shaders
]

#set math.equation(numbering: "(1)")

#show heading.where(level: 1): it => [
  #set align(center)
  #set text(13pt, weight: "regular")
  #block(smallcaps(it))
]

#show heading.where(level: 2): it => [
  #set text(11pt, weight: "regular")
  #block(emph(it))
]

#show heading.where(level: 3): it => {
  set text(weight: "regular")
  box(width: 2em)[]
  emph[#it.body :]
}

#show ref: it => {
  if it.element != none and it.element.func() == math.equation {
    link(it.element.location())[(#counter(math.equation).at(it.element.location()).at(0))]
  } else {
    it
  }
}

#set par(justify: true, first-line-indent: (amount: 1em, all: true))
#set text(10pt, lang: "fr")

#set page(
  paper: "us-letter",
  numbering: "1 / 1",
  columns: 2,
  margin: 4em,
)

#place(
  top + center,
  float: true,
  scope: "parent",
  clearance: 2em,
)[
  #align(center, text(17pt)[
    *#title*
  ])

  #align(center, text(14pt)[
    Team Baguette - _#link("https://github.com/EliottPiccand/ComputerGraphicsAssignment3_2")[Github Repository]_
  ])

  #grid(
    columns: (1fr, 1fr),
    align(center)[
      BLAIS Vladimir \
      CSED \
      49005916 - vblais
    ],
    align(center)[
      PICCAND Eliott \
      CSED \
      49005903 - piccandeliot
    ],
  )
]

#show link: text.with(blue.darken(10%))
#show link: underline

= Development environment
To develop our program, we used VSCodium (with the clangd extension) for code editing, and CMake for compiling. However, instruction to compile with Visual Studio are available in the project `README.md`. To understand the source code, the reader must be familiar with modern C++ features (C++ 23) and OpenGL latest version (4.6).

In addition, we used the #link("https://github.com/wolfpld/tracy")[Tracy] library to profile our game when we faced performances issues. This library was only used during the development and is not needed when compiling the game in Debug or Release mode.

= Program design and implementation
Our program contains several features, including:
// requirements
- basic player controls;
- cannon balls;
- collisions;
- radar;
- enemies;
- damage system;
- victory / defeat display;
- multiple views;
// additional goals
- explosion effects;
- camera shaking;
- foam trails;
- cannon recoil;
- flapping flag;
// custom additional goals
- water splash;
- sparks on cannon balls;
- red vignette on player hit.

and some features not visible by the players, but useful for development:
- hierarchical system;
- component system;
- event system;
- input system;
- resource loader;
- physics system;
- particle system.

In order to compile our program, there are some additional requirements#footnote[Please check the `README.md` for detailed instructions.]:
- enabling C++23 features. This choice was made to be able to use the C++23 `std::ranges` and `std::views` features, as well as the `<print>` header, to shorten development time and code readability.
- the `stb` (single-file public domain (or MIT licensed) libraries for C/C++) library. This allow us to load images for the game (we only use the `stb_image.h` header).
- the `TinyGLTF` library. This allow us to load glTF 3d models.

To summarize how our program works, we can use the following pseudo-code (see alg:main-loop)

#algorithm-figure(
  "Game Main Loop",
  vstroke: .5pt + luma(200),
  {
    import algorithmic: *
    let CallMath(name) = arraify(CallInline.with(name)[]).join()()

    Function(
      "Main-Loop",
      (),
      {
        While(
          $not #CallMath("Window-Should-Close")$,
          {
            Call.with("Update-Input-System")[]()
            Call.with("Process-Events")[]()
            Comment[update the different elements of the game, recursively from the scene root]
            Call.with("Update-Scene")[]()
            Call.with("Update-Physics-System")[]()
            Call.with("Update-Particle-System")[]()

            LineBreak

            Comment[render everything (this is a constant function, no update occurs here)]
            Call.with("Render-Scene")[]()
            Call.with("Render-Particles")[]()

            LineBreak

            Comment[display the rendered content on the screen]
            Call.with("Swap-Framebuffers")[]()
          },
        )
      },
    )
  },
) <alg:main-loop>

This is a usual game main loop. Sometime, games place the `Process-Events()` part at the end of the loop, but we decided to place it at the beginning, since it has no impact on the behavior of the program, and allow to defer calls during the initialization (even if we do not need that so far, we might need it in the future).

== Technical Features

=== Hierarchical System
We defined our scene as a tree of `GameObject`.
By default, each game object only handle its relation with its parent/children and do nothing particular beside propagating the `update()` and `render()` methods.

=== Component System
Each `GameObject` have a list of components attached to it.
Each component is a small brick of behavior.
Here are some of the main components:
- `Transform`: handle the position, orientation and scale of a `GameObject` relatively to its parent;
- `Collider`: represent a region of space. Allow collision detection;
- `RigidBody`: represent an element of the Physics engine. Allow movement and collision solving; 
- `ModelInstance`: attach a `Model` to the `GameObject`.

=== Event System
To allow every part of our code to interact between each other, the event system allow to register callbacks and post events.
Those events are then processed at the same time as the window events.

=== Input System
This is a simple abstraction layer that allow developers to get keyboard and mouse input without having to deal with glfw heavy syntax and duplicate bindings.

=== Resource Loader
This is a wrapping around a hash map that only load each 3d model / texture once, even if it is used at multiple places in the code base.
For instance, since each ship's model has a lot of polygons, we do not want to load the model for each player and enemy.
Instead, we use the resource loader to load the model once in memory, and then use references to this memory to draw each ship.

=== Physics System
This system allow to apply forces to each rigid body in the scene.
Each rigid body is then moved and rotated accordingly to Newton's laws.
In addition, this part also handle collision detection for triggers, and collision solving when needed.

=== Particle System
This system is used extensively in the game to render huge amount of colored points.
It handle the display part but also the motion of each particle.
To be able to render a huge amount of particles, we used a combination of 2 methods.

==== Particles Rendering
Particles are rendered using instanced rendering: the particle model is bound to the shader, then another buffer containing the data of each particle is bound.
Then, we call `glDrawElementsInstanced()` to draw the same model multiple time, each instance with an different entry of the data buffer.

Moreover, because particles do not need to be a complex 3D model, we simply rendered each particle as a billboard: a plane that always faces the camera.

==== Particles Update
Because updating tens of thousands of particles on each frame is heavy for the CPU, we used a compute shader to parallelize all those computation.
Now, the CPU part almost never access the particles' data (only when spawning new particles).
On each update, the `ParticleSystem` dispatch all the particle motion and killing logic into multiple GPU threads.

== Visual Features

=== Player Ship
The player has a ship (see @fig:player-ship) that it can control.

=== Cannon
Each ship has a cannon (see @fig:player-ship-annotated & @fig:cannon) that is made of 2 parts: the stand that rotate toward the target, and the barrel that move up/down depending on how far the target is.
The maths used to compute the cannon inclination can be found inside `Ballistic.pdf`.

=== Cannon Balls
Each cannon can fire cannon balls (see @fig:cannon-ball).
Cannon balls follow a parabolic trajectory (see @fig:cannon-ball-trajectory & @fig:top-view-debug) matching Newton's laws thanks to the physics engine.

=== Collisions
The game use a lot of collision, for triggers like when a cannon ball hit a ship and also for interaction between ships and walls.
Different types are implemented:
- Axis Aligned Bounding Box (AABB): this collider is used mostly as an optimization, for pre-checking if a collision occurs between two collider  (see @fig:collider-aabb). However, some simple objects such as the cannon balls have only this type of collider and no more precise one.
- Convex Polyhedron: This is the main collider used in the game for the ship (see @fig:collider-convex-polyhedron). Its a low resolution mesh, edited to be convex.
- Sphere: We mainly used this type of collider for explosions damage area (see @fig:collider-sphere).

Also, because we did not wanted the ships and cannon balls to hit an invisible wall when reaching the border of the map, we added some rocks all around the ocean (see @fig:rocks & @fig:top-view).
Those rocks are purely visual and the collisions are handle by an AABB collider on each map edge (see @fig:rocks-collider & @fig:top-view-debug).

=== Radar
Each ship feature a rotating radar made of a brown cylinder and a red cone (see @fig:player-ship-annotated & @fig:radar).

=== Enemies
When starting, the game spawn some enemies (see @fig:enemy-ship).
Each enemy motion follow this process: the enemy pick a random point of the map (not too close to the current ship position), then rotate and move in a straight line toward this waypoint.
Once reached, another waypoint is selected and the process restart.
We also use the same process for the enemies' cannon's target: each target has its own waypoint, and the target move toward the waypoint.
Those waypoints can be seen in debug mode: the waypoint itself is a 1m side red cube on the water, and its link to the ship or the ship's target by a green dashed line (see @fig:top-view-debug).

=== Damage System
Each ship collider detects if a cannon ball enter collides with it.
If this event occurs, the cannon ball is destroyed and is replaced by an explosion.
This explosion has its own collider.
If it touch the ship's one, the ship takes randomly between 3k and 15k damages.
Each ship has 24k hit points.
If the hit points reach 0, the ship disappear.

Health bars are displayed over each ship (see @fig:top-view).
To render them, we use a simple trick: instead of implementing an entire UI system, we simply drew 2 quads high in the sky, and by enabling back face culling, those are only visible from the top view, and not from the cannon/cannon ball view (see @fig:ui-tricks).

=== Victory / Defeat Display
On victory or defeat, we used the same tricks as for the health bar to render a simple hand drawn texture with the texts (see @fig:victory & @fig:defeat & @fig:ui-tricks).

=== Multiple Views
The game feature 3 view modes:
- The Top view (see @fig:top-view);
- The Free View (for debug purposes, toggled by clicking `Enter`);
- The Cannon/Cannon ball view: if at least one cannon ball fired by the player is still flying, the camera follow the last fired one (see @fig:cannon-ball-view). Otherwise, it is located near the player's cannon, facing toward the target (see @fig:cannon-view).

=== Explosion Effects
When a cannon ball hit anything but water, it spawn an explosion (see @fig:explosion).
To render it, we used 2500 particles with different colors and velocity.

=== Camera Shaking
Whenever a the player ship takes damages, the camera shakes for a short amount of time.
The shaking is done by creating an `offset` 2D vector using the @alg:camera-shake, and use this offset to move the camera eyes (from the "look at" view matrix) in a plan normal to the camera forward, without changing the `look at` position.

#algorithm-figure(
  "Camera Shake",
  vstroke: .5pt + luma(200),
  {
    import algorithmic: *
    let CallMath(name) = arraify(CallInline.with(name)[]).join()()

    Function(
      "Shake",
      (),
      {

        Comment[Crate a vector of length INTENSITY with a random orientation]
        Assign("offset", $#CallMath("Random-Unit-Vector") times "INTENSITY"$)

        LineBreak

        While(
          $#Fn.with("length")[offset]().join() > "MIN_SHAKING"$,
          {
            Comment[Flip the offset]
            Assign("offset", Call.with("Rotate")[offset, 180°]())

            LineBreak

            Comment[Slightly Rotate the offset by some random angle]
            Assign("angle", Fn.with("random")[-60°, 60°]())
            Assign("offset", Call.with("Rotate")[offset, angle]())

            LineBreak

            Comment[Decrease the offset intensity]
            Assign("offset", $"offset" times "INTENSITY_DECAY"$)

            
          },
        )
      },
    )
  }
) <alg:camera-shake>

=== Foam Trails
When a ship is moving, it spawn behind it a foam trail (see @fig:foam-trail).
To render it, we used 50 particles per frame with different position, velocity and lifetime, subject to gravity.

=== Cannon Recoil
Whenever a canon fires, the barrel have a recoil effect.
However, because the views instantly swap to the cannon ball's (because of the assignment requirements), this is only visible in cannon/cannon ball view when enemies are close enough, or in free camera view.

=== Flapping Flag
Each ship has its topmost flag (see @fig:player-ship-annotated) flapping.
However, since the health bar is above the ship, this is only visible in cannon/cannon ball view when enemies are close enough, or in free camera view.
The model actually remains always flat on the memory, the flapping part is handle completely by displacing the vertices inside the vertex shader.

=== Water Splash
When a cannon ball hit the water plane, it spawn an water splash (see @fig:water-splash).
To render it, we used 500 particles with different colors, position, velocity and lifetime, subject to gravity.

=== Sparks on Cannon Balls
Each cannon ball has animated sparks on its stem (see @fig:cannon-ball).
This is done by spawning a few particle each frames.
This is also that which allow the player to see the cannon balls in top view.
Otherwise, they would be too small and only appear as a 1 pixel wide black dot.

=== Red Vignette on Player Hit
Whenever the player gets hit, a red vignette effect covers the screen for a small duration (see @fig:red-vignette).

=== Debug Mode
Not really a player-oriented feature, but we added a debug mode to the game (it can be toggled by clicking `F3`) to display useful debug information such as:
- perspective cameras (see @fig:camera).

= End-user guide
The game starts immediately on running the executable.
The goal is to sunk the enemies' ships (the ones with black sails).

The player (ship with white sails) can change its ship speed with the `W` and `S` keys (respectively increasing and decreasing the ship speed), and rotate it using the `A` and `D` keys (turing the ship respectively left and right by 15°).
The player can also switch between 2 views by clicking (press and release) `V`:
- Top view: in this view, the player sees the entire world, and can use its mouse to fire cannon balls. Holding pressed the `Left Mouse Button` enable aiming mode. In aiming mode, the player can either fire and exit the aiming mode (by releasing the `Left Mouse Button`), or exit without firing by clicking (pressing then releasing) the `Right Mouse Button`.
- Cannon / Cannon ball view: in this view, the player can no longer fire, but can still move its ship.

When a cannon ball hit a ship, it deals randomly between 3000 and 15000 damages.
Each ship has 24000 hit points.
When a ship reach 0 or less hit points, it sink.
The player win if every enemies are sunk, and lose if its ship get sunk.

Additionally, at any time, the player can:
- click `ESC` to quit the game
- click `G` to restart the game
- click `F11` to toggle fullscreen

== Debug view and controls
To help with debugging, a few debug options are available :
- clicking `R` cycle between the different rendering mode. For now, the available rendering modes are:
    - Default
    - Wireframe
- clicking `Enter` toggle a free view camera that can be moved with `W` `A` `S` `D` `Space` `Left Shift` and moving the mouse. (note that here, `W` `A` `S` `D` no longer move the ship).
- clicking `P` toggle on/off the physics and the particle engines (not the animations tho)
- clicking `F` when in another view than Top View fire a cannon ball from the player ship
- the arrow keys `Up` `Left` `Down` `Right` move the player's cannon's target respectively North, West, South and East.
- clicking `F3` toggle the display of a lot of debug information such as :
    - colliders: AABB with red boxes, Convex Polyhedron with green wireframe and Sphere with cyan circles
    - cannon's predicted trajectories: blue line
    - ship's targets: represented by a 1x1x1 aabb collider at the end of the blue line
    - enemy ships' waypoints: represented by a 1x1x1 aabb collider. A green dashed line goes from the ship to this waypoint
    - enemy ships' targets' targets: same as the enemy ships' waypoint, but starting from the current ship's target.

== Game Screenshots

#let imageWidth = 96%;

#figure(
  image("Images/PlayerShip.png", width: imageWidth),
  caption: [Player Ship],
) <fig:player-ship>

#figure(
  image("Images/PlayerShipAnnotated.png", width: imageWidth),
  caption: [Player Ship with its radar (red), its flapping flag (green) and its cannon (cyan)],
) <fig:player-ship-annotated>

#figure(
  image("Images/Cannon.png", width: imageWidth),
  caption: [Cannon stand (wooden part and wheels) and barrel (metallic part)],
) <fig:cannon>

#figure(
  image("Images/CannonBall.png", width: imageWidth),
  caption: [Cannon ball and sparks on its stem#footnote[The amount of spark's particles on the screenshot is bigger than the one seen in game because of the time needed to take the screenshot.]],
) <fig:cannon-ball>

#figure(
  image("Images/CannonBallTrajectory.png", width: imageWidth),
  caption: [Cannon ball trajectory preview (blue line) visible with the debug mode (`F3`) enabled],
) <fig:cannon-ball-trajectory>

#figure(
  image("Images/ColliderAABB.png", width: imageWidth),
  caption: [AABB collider (in red) around a cannon ball],
) <fig:collider-aabb>

#figure(
  image("Images/ColliderConvexPolyhedron.png", width: imageWidth),
  caption: [Convex polyhedron collider (in green) and its AABB (in red) around a ship],
) <fig:collider-convex-polyhedron>

#figure(
  image("Images/ColliderSphere.png", width: imageWidth),
  caption: [Sphere collider (in cyan) and its AABB (in red) inside an explosion],
) <fig:collider-sphere>

#figure(
  image("Images/Rocks.png", width: imageWidth),
  caption: [Rocks at the edge of the map],
) <fig:rocks>

#figure(
  image("Images/RocksCollider.png", width: imageWidth),
  caption: [Rocks at the edge of the map and the map edge collider],
) <fig:rocks-collider>

#figure(
  image("Images/TopView.png", width: imageWidth),
  caption: [Top view at the start of the game],
) <fig:top-view>

#figure(
  image("Images/TopViewDebug.png", width: imageWidth),
  caption: [Top view at the start of the game with debug mode enabled],
) <fig:top-view-debug>

#figure(
  image("Images/Radar.png", width: imageWidth),
  caption: [Radar at the back of each ships],
) <fig:radar>

#figure(
  image("Images/EnemyShip.png", width: imageWidth),
  caption: [Enemy ship#footnote[The player and enemies ships share the same 3d model. Only the sails texture is updated on the player to have a different look. Thus, only one model is loaded into the memory.]],
) <fig:enemy-ship>

#figure(
  image("Images/UITricks.png", width: imageWidth),
  caption: [Health bar and victory menu viewed from the free view camera],
) <fig:ui-tricks>

#figure(
  image("Images/VictoryScreen.png", width: imageWidth),
  caption: [Victory screen],
) <fig:victory>

#figure(
  image("Images/DefeatScreen.png", width: imageWidth),
  caption: [Defeat screen],
) <fig:defeat>

#figure(
  image("Images/CannonView.png", width: imageWidth),
  caption: [Cannon view],
) <fig:cannon-view>

#figure(
  image("Images/CannonBallView.png", width: imageWidth),
  caption: [Cannon ball view],
) <fig:cannon-ball-view>

#figure(
  image("Images/Explosion.png", width: imageWidth),
  caption: [Explosion],
) <fig:explosion>

#figure(
  image("Images/FoamTrail.png", width: imageWidth),
  caption: [Foam trail],
) <fig:foam-trail>

#figure(
  image("Images/WaterSplash.png", width: imageWidth),
  caption: [Water splash],
) <fig:water-splash>

#figure(
  image("Images/RedVignette.png", width: imageWidth),
  caption: [Red vignette effect when the player ship get hit],
) <fig:red-vignette>

#figure(
  image("Images/Axis.png", width: imageWidth),
  caption: [Debug view of a cannon ball with on the middle of the screen, the game's axis and on the cannon ball, its model axis (red = X, green = Y and blue = Z)],
) <fig:axis>

#figure(
  image("Images/Camera.png", width: imageWidth),
  caption: [Debug view of the camera following the cannon ball],
) <fig:camera>

= Discussions/Conclusions
During the development, we didn't encountered much issues. We just spent a lot of time finding good assets for the game and solving orientation issues with those models.

= References
Every part of the code is original, but we use several tutorials or references during the development :
- the camera shaking part mechanic is greatly inspired by #link("https://gamedev.stackexchange.com/a/47565")[\miklatov answer on this Stack Exchange discussion]\;
- the component system was inspired by #link("https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/03_component_systems.html")[this Vulkan tutorial].

= AI-assisted coding references
During the development, we used generative AI when some part require learning a lot of non-graphical related topic to implement. So we always designed everything without AI, but sometime use AI to implement some designs such as :
- Copilot helped implementing the collision solving system (not detection);
- Copilot helped implementing the glTF model loading.

Thus, we estimate that 8% of the code was generated by an LLM.
