# Computer Graphics (CSED451-01) - Assignment 3.2: 3D Drawing with OpenGL Shaders

by _Team Baguette_

## Controls
The game starts immediately on running the executable. The player can change the ship speed with the `W` and `S` keys (respectively increasing and decreasing the boat speed), and rotate it using the `A` and `D` keys (turing the boat respectively left and right by 15°)

The game starts in a top view. Its possible to cycle between views by pressing `V`. Two views are available :
- Top view : Here, the player can shoot missiles with his mouse : holding the `Left Click` enable aiming mode. In aiming mode, 2 actions are possible : cancel fire by clicking (press and release) the `Right Click`, or fire by releasing the `Left Click`.
- Cannon / Cannon Ball view : This is only a display view and can help to aim. The player can still control the ship with `W` `A` `S` `D` but can no longer aim / fire. 

At any time, other keybinds are available:
- releasing `Esc` close the game
- releasing `G` restart the game
- releasing `F11` toggle fullscreen mode.

### Debug Controls
To help with debugging, a few debug options are available :
- releasing `R` cycle between the different rendering mode. For now, the available rendering modes are :
    - Default
    - Wireframe
- releasing `Enter` toggle a free view camera that can be moved with `W` `A` `S` `D` `Space` and `Left Shift`. (note that here, `W` `A` `S` `D` no longer move the ship).
- releasing `P` toggle on/off the physics engine
- releasing `F` when in another view than Top View fire a cannon ball from the player ship
- the arrow keys `Up` `Left` `Down` `Right` move the player's cannon's target respectively North, West, South and East.
- releasing `F3` toggle the display of a lot of debug information such as :
    - colliders: AABB with red boxes, Convex Polyhedron with green wireframe and Sphere with cyan circles
    - cannon's predicted trajectories: blue line
    - ship's targets: represented by a 1x1x1 aabb collider at the end of the blue line
    - enemy ships' waypoints: represented by a 1x1x1 aabb collider. A green dashed line goes from the ship to this waypoint
    - enemy ships' targets' targets: same as the enemy ships' waypoint, but starting from the current ship's target.

## Building the game
This project use CMake for compiling, and Python for moving assets.
If you don't have Python installed, you can remove everything inside `CMakeLists.txt` after the line `# --- Assets ---`, but you will have to manually copy the `Assets` folder next to the executable.

### With pure CMake
+ Setup the dependancies:
    ```cmd
    git clone https://github.com/syoyo/tinygltf Lib/tinygltf
    git clone https://github.com/nothings/stb Lib/stb
    ```

+ Setup the CMake project with one of:
    ```cmd
    cmake -B Build/Release -DCMAKE_BUILD_TYPE=Release
    cmake -B Build/Debug -DCMAKE_BUILD_TYPE=Debug
    cmake -B Build/Profiling -DCMAKE_BUILD_TYPE=Profiling
    ```

+ Compile with:
    ```cmake
    cmake --build Build/Release
    cmake --build Build/Debug
    cmake --build Build/Profiling
    ```

### With VisualStudio
+ Create an empty solution
+ Recreate the project structure with filters (Src/... goes into Source files/...  and Include/ into Headers files/...);
+ Add every .cpp and .h to their proper filter;
+ Open the project properties (right click on the project then properties) :
    + Change `Configuration Properties > General > Platform Toolset` to `LLVM (clang-cl)` (you might have to install it from VS Installer : `C++ Clang Compiler for Windows` + `MSBuild support for LLVM (clang-cl) toolset`);
    + Change `Configuration Properties > General > C++ Language Standard` to `c++23`
    + Add to `Configuration Properties > C/C++ > General > Additional Include Directories` :
        - `path/to/repo/Include`;
        - `path/to/repo/Lib`;
    + Add to `Configuration Properties > C/C++ > Preprocessor > Preprocessor Definitions` : `OE_RELEASE`
    + Change `Configuration Properties > C/C++ > Output Files > Object File Names` to `$(IntDir)%(RelativeDir)%(Filename).obj`;
    + Add to `Configuration Properties > C/C++ > Command Line > Additional Options` : `-Xclang -std=c++23`;
    + Add to `Configuration Properties > Linker > General > Additional Library Directories` : `path/to/repo/Lib`;
    + Add to `Configuration Properties > Linker > Input > Additional Dependencies` : `glew32.lib; glfw3.lib; opengl32.lib; user32.lib; gdi32.lib; shell32.lib; glu32.lib`;
+ Switch the Solution Configuration to `Release`;
+ Build without launching by pressing ``F7`;
+ Locate the created `.exe` file, and copy the `Assets` folder next to it. It should looks like
    ```cmd
    SomeDir
    ├───MyProgram.exe
    └───Assets
        ├───Models
        │   └───...
        └───Textures
            └───...
    ```
+ Launch the game by pressing `Ctrl + F5`. (The window might stay blank for a few seconds the time for the assets to load).

### Using the Profiling Profile
[Tracy](https://github.com/wolfpld/tracy) must be added as a Git submodule (see `.gitmodules`) to be able to compile with the Profiling profile. Note that to use Tracy, the submodule should be checkout to the last stable version like
```cmd
git clone https://github.com/wolfpld/tracy Lib/tracy
cd Lib/tracy
git checkout v0.13.1
```

## Assets References
- Ship & Cannon models : [Stylized Pirate Ship by _Comeback_](https://assetstore.unity.com/packages/3d/vehicles/sea/stylized-pirate-ship-200192) (Player's Ship Sails by _Team Baguette_)
- Cannon ball model : [Ball Pack by _YounGen Tech_](https://assetstore.unity.com/packages/3d/props/ball-pack-446)
- Rocks : [Fantasy landscape by 
_Pxltiger_](https://assetstore.unity.com/packages/3d/environments/fantasy-landscape-103573)
