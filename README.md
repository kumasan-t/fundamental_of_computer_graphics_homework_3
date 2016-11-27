# Assignment 4: Animate

Out: Nov 22\. Due: Dec 13.

### Introduction

In your third assignment, you will implement animation in a simple interactive application. You will see that with a small amount of code, we can produce realistic animations.

You are to perform this assignment using C++. To ease your development, we are providing a simple C++ framework to represent the scene, perform basic mathematical calculations, and save your image results. The framework also contain simple test scenes to judge the correctness of your algorithm. These test scenes are encoded in JSON, a readable ASCII format. You can compare to the correct output images we supply. To build the framework, you can use either Visual Studio Express 2013 on Windows or XCode 6 on OS X. We will use external libraries to help us interact with the system, including GLFW for window management and GLEW to access OpenGL functions on Windows.

For this assignment, we will provide executables for Windows and OSX of the solution. This is to help you see the "correct" animation as it is played back.

### Framework Overview

We suggest you use our framework to create your renderer. We have removed from the code all the function implementations your will need to provide, but we have left the function declarations which can aid you in planning your solution. All code in the framework is documented, so please read the documentation for further information. Following is a brief description of the content of the framework.

*   **common.h** includes the basic files from the standard library and contains general utilities such as print messages, handle errors, and enable Python-style foreach loops;
*   **vmath.h** includes various math functions from the standard library, and math types specific to graphics; `vecXX`s are 2d, 3d and 4d tuples, both float and integerers, with related arithmetic options and functions - you should use this type for point, vectors, colors, etc.; `frame3f`s are 3d frames with transformations to and from the frame for points, vectors, normals, etc.; `mat4f` defines a 4x4 matrix with matrix-vector operations and functions to create transform matrices and convert frames
*   **image.h/image.cpp** defines a color image, with pixel access operations and image loading/saving operations
*   **lodepng.h/lodepng.cpp** provide support for the PNG file format
*   **json.h/json.cpp/picojson.h** provide support for the JSON file format
*   **scene.h/scene.cpp** defines the scene data structure and provide JSON scene loading
*   **tesselation.h/tesselation.cpp**: implements smooth curves and surfaces
*   **animation.h/animation.cpp**: implements animation and simulation: _your code goes here_
*   **animate.cpp** implements the OpenGL renderer and the interaction
*   **animate_vertex.glsl/animate_fragment.glsl**: vertex and fragment shaders: _your extra credit code goes here_

In this homework, scenes are becoming more complex. A `Scene` is comprised of a `Camera`, and a list of `Mesh`es, a list of `Surface`s and a list of `Light`s. The `Camera` is defined by its frame, the size and distance of the image plane and the focus distance (used for interaction). Each `Mesh` is a collection of either points, lines or triangles and quads, centered with respect to its frame, and colored according to a Blinn-Phong `Material` with diffuse, specular coefficients. Each `Mesh` is represented as an indexed polygonal mesh, with vertex position normals and texture coordinates. Each `spline` segment is the reference to the four vertices of the Bezier. Each `Surface` can be either a quad or a sphere, as before. Each `Light` is a point light centered with respect to its frame and with given intensity. The scene also includes the background color, the ambient illumination, the image resolution and the samples per pixel.

Animation data is store in auxiliary structures, in `Mesh`es, `Surface`s and the `Scene`. The scene animation data includes the current `time` (measured a frame number), the total animation `length`, the time in seconds for each frame `dt`, the number of simulation steps `simsteps` for each frame, the `gravity` acceleration and the dumpening coefficients for the `bounce_bump` (parallel, ortho respectively).

The `Mesh` animation data is divided into separate structures. If a structure is missing, just skip that mesh. `MeshAnimation` contains keyframe data for the mesh frame, including the initial frame `rest_frame`, the times for each keyframe `keytimes`, and values for `translation` and `rotation`. `MeshSkinning` contains skinning data, including rest position `rest_pos` and normals `rest_norm`, per-vertex bone indices `bone_ids` and weights `bone_weights`. We decide that maximum 4 bonces can be active at any given time, and indicated with -1 if a bone index is unused. Bone animation is included in `bone_xforms` which stored transform matrices for each bone and time frame. `MeshSimulation` defines simulation data, storing initial position and velocities `init_pos`, `init_vel`, `mass` and whether a point is locked into its position `pinned` (just skip these in the simulation loop). During simulation, just update the vertex position in `mesh->pos` and the velocities in `mesh->simulation->vel`. Finally, we inlude a list of `springs`, each with vertex indices `ids`, rest length `restlength` and static `ks` and dynamic `kd` conefficients. We also include collision data in `MeshCollision` that represents the simple primitive, sphere or quad, a `Mesh` is associated with.

In this homework, model geometry is read from RAW files. This is a trivial file format we built for the course to make parsing trivial in C++. This geometry is stored in the `models` directory.

We provide very simple interactions for your viewer. Clicking and moving the mouse lets you rotate the model. If you want to save, please restart the program to avoid issues, and press `s`. For debugging purposes, it is helpful to see face edges. You can do so with wireframing, enabled with `w`. Animation is enabled by hitting space and GPU skinning is enabled with `g` (only for extra credit work). We also provide version of the test cases that only include 1/3 of the frames; these are the ones you should use to compute the images.

Since we perform a lot of computation, we suggest you compile in `Release` mode. You `Debug` mode only when deemed stricly necessary. You can also modify the scenes, including the amount of samples while debugging.

### Requirements

You are to implement the code left blank in `animation.cpp`. You will implement these features.

1.  Keyframe interpolation (`animation.cpp#animate_frame`). Implement keyframe interpolation of the Mesh frame. In this homework, linearly interpolate translation and rotation angles stored in the keyframe, based on the key times. The final transform matrix is written as `translate * rotate_z * rotate_y * rotate_x` and should be multipled by the `rest_frame` to obtain the current `mesh->frame`.

2.  Skinning (`animation.cpp#animate_skin`). Implement mesh skinning by computing both position `pos` and normals `norm` based on the bone transforms, skin weights and rest positions `rest_pos` and normals `rest_norm`. The various variables are described above and in the code.

3.  Simulation (`animation.cpp#simulate`). Implement a particle-based simulator for particle systems, cloth and soft bodies.

    1.  Implement particle dynamics, by first setting the force based on gravity and particle mass, and then using Euler integration to advance the particles. Remember that the simulation is performed `simsteps` times, each of which advancing the time by `dt/simsteps`.
    2.  Add collision after Euler integration, by first checking if a particle is inside a Mesh (using the proxy collision data), and then adjusting position and velocity.
    3.  Add springs to the force computation for cloth and soft bodies.

### Hints

We suggest to implement the renderer following the steps presented above. To debug code, you can use the step by step advancing.

### Submission

Please sent an email to `pellacini@di.uniroma1.it` with your code as well as the images generated as a .zip file. **Upload the results of your animation on YouTube and send the links in the email**. If the animation are not uploaded on YouTube, you will have points detracted. You can capture animation on your systems with a variety of tolls such as Camtasia for Windows and the built in QuickTime on Mac, but any tools is fine. If you want, you can also make a GIF for your video using any online site and attached it in the ZIP.

### Extra Credit

1.  Implement skinning on the GPU. You can implement the same exact algorithm. Just pass the arrays to the vertex shader and execute skinning there.

2.  Implement simple Soft Body Dynamics. Just like cloth, we can implement soft body dynamics putting springs _in the volume_. To do so, create tetrahedra for your mesh, using the open source TetGen library (search on Google). Then attach springs to each tetrahedra edge and your done. Follow the setup for the cloth simulator for this.

3.  Collision Detection. Implement cloth internal collision detection using Bridgon algorithm. If you are intereted in this extra credit, contact Prof. Pellacini for further instructions.