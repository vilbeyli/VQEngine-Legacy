# VQEngine Documentation

This page is Currently to serve as a collection of resources that influenced the engine/renderer design.

# Engine Design

## Youtube

- [CppCon2017: Nicolas Guillemot “Design Patterns for Low-Level Real-Time Rendering”](https://www.youtube.com/watch?v=mdPeXJ0eiGc)
- [GCAP 2016: Parallel Game Engine Design - Brooke Hodgman](https://www.youtube.com/watch?v=JpmK0zu4Mts)
- [CppCon 2016: Jason Jurecka “Game engine using STD C++ 11"](https://www.youtube.com/watch?v=8AjRD6mU96s)
- [C++Now 2018: Allan Deutsch "Game Engine API Design"](https://www.youtube.com/watch?v=W3ViIBnTTKA)
- [CppCon 2014: Mike Acton "Data-Oriented Design and C++"](https://www.youtube.com/watch?v=rX0ItVEVjHc)
- [Jonathan Blow: Data-Oriented Demo: SOA, composition](https://www.youtube.com/watch?v=ZHqFrNyLlpA)
- [Code Blacksmith: Thread Pool Tutorial](https://www.youtube.com/watch?v=eWTGtp3HXiw)

## Text

- [Game Programming Patterns](http://gameprogrammingpatterns.com/contents.html)
  - [Data Locality](http://gameprogrammingpatterns.com/data-locality.html)
  - [Object Pool](http://gameprogrammingpatterns.com/object-pool.html)

# Rendering

## BRDF

- [Joe DeVayo: LearnOpenGL - PBR Theory](https://learnopengl.com/#!PBR/Theory)
- [PBS Physics Math Notes](http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf)
- [Trowbridge-Reitz GGX Distribution - How NDF is defined](http://reedbeta.com/blog/hows-the-ndf-really-defined/)
- [Fresnel-Schliick w/ roughness](https://seblagarde.wordpress.com/2011/08/17/hello-world/)

## Rendering Math

- Linearized Depth Buffer
  - [NVidia - Depth Precision Visualized](https://developer.nvidia.com/content/depth-precision-visualized)
	- [Linear Depth](http://dev.theomader.com/linear-depth/)
	- [LinearZ](https://www.mvps.org/directx/articles/linear_z/linearz.htm)
	- [link1](http://www.humus.name/index.php?ID=255)

- Reconstructing position from depth
	- [link1](https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/)
	- [link2](http://www.derschmale.com/2014/01/26/reconstructing-positions-from-the-depth-buffer/)

- Hammersley Sequence - Quasi-Monte Carlo
   - [link1](http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html )
   - [link2](https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/monte-carlo-methods-in-practice/introduction-quas-monte-carlo)

## Screenshots

![](renderer-design.PNG)
![](commands.PNG)
![](mem-man-discrete.PNG)
![](mem-man-integrated.PNG)
