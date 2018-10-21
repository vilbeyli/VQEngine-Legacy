# VQEngine Documentation

This page is Currently to serve as a collection of resources that influenced the engine/renderer design.

# Engine Design

Below is the collection of resources used for building the VQEngine. Items marked with star\* are implemented in the codebase - some are partial implementations.

## Conference Talks & YouTube Content

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

## Ambient Occlusion (SSAO)
 - [**Modified Crysis SSAO w/ Normal Oriented Hemisphere***](https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html)
 - [NVIDIA HBAO](http://developer.download.nvidia.com/presentations/2008/SIGGRAPH/HBAO_SIG08b.pdf)
 - [NVIDIA (Bavoil) Deinterleaved Texturing & Sampling Patterns](http://twvideo01.ubm-us.net/o1/vault/gdc2013/slides/822298Bavoil_Louis_ParticleShadows.pdf)
- [Filion & McNaughton: StarCraft II Effects & Techniques](https://developer.amd.com/wordpress/media/2013/01/Chapter05-Filion-StarCraftII.pdf) / [Powerpoint (PDF)](https://developer.amd.com/wordpress/media/2012/10/S2008-Filion-McNaughton-StarCraftII.pdf)
- [Frederik P. Aalund: A Comparative Study of SSAO Methods](https://www.gamedevs.org/uploads/comparative-study-of-ssao-methods.pdf)
- [Simon Wallner: Ogre3D - 5 SSAO Techniques](https://www.cg.tuwien.ac.at/research/publications/2010/WALLNER-2010-CSSAO/WALLNER-2010-CSSAO-doc.pdf)

## Bloom
 - [**Fabian Giesen: Fast Blurs Part 1***](https://fgiesen.wordpress.com/2012/07/30/fast-blurs-1/)
 - [Fabian Giesen: Fast Blurs Part 2](https://fgiesen.wordpress.com/2012/08/01/fast-blurs-2/)
 - [How to do good bloom for HDR rendering](http://kalogirou.net/2006/05/20/how-to-do-good-bloom-for-hdr-rendering/)
 - [Efficient Gaussian blur with linear sampling](http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/)


## BRDF

- [**Joe DeVayo: LearnOpenGL - PBR Theory\***](https://learnopengl.com/#!PBR/Theory)
- [PBS Physics Math Notes](http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf)
- [Trowbridge-Reitz GGX Distribution - How NDF is defined](http://reedbeta.com/blog/hows-the-ndf-really-defined/)
- [Fresnel-Schliick w/ roughness](https://seblagarde.wordpress.com/2011/08/17/hello-world/)

## Compute
 
 - [**Nick Thibieroz, AMD - Shader Model 5.0 and Compute Shader (DirectX11)***](https://twvideo01.ubm-us.net/o1/vault/gdc09/slides/100_Handout%206.pdf)
 - [Sebastian Aaltonen: Optimizing GPU occupancy and resource usage with large thread groups](https://gpuopen.com/optimizing-gpu-occupancy-resource-usage-large-thread-groups/)
 - [Wolfgang Engel: Compute Shader Optimizations for AMD GPUs - Parallel Reduction](https://diaryofagraphicsprogrammer.blogspot.com/2014/03/compute-shader-optimizations-for-amd.html)
 - [NVIDIA GPU Technology Conference: DirectCompute Optimizations & Best Practices](http://on-demand.gputechconf.com/gtc/2010/presentations/S12312-DirectCompute-Pre-Conference-Tutorial.pdf)
 - [Life of a triangle - NVIDIA's logical pipeline](https://developer.nvidia.com/content/life-triangle-nvidias-logical-pipeline)


## Culling: Shadow View 
 - [Umbra 2011: Shadow Caster Culling for Efficient Shadow Mapping](http://dcgi.felk.cvut.cz/?media=publications%2F2011%2Fbittner-i3d-scc%2Fpaper.pdf&alias=Bittner2011&action=fetch&presenter=Media)
 - [Stefan-S: Shadow caster frustum culling](http://stefan-s.net/?p=92)

## Culling: View Frustum
 - [Universitat Bremen CGVR: View Frustum Culling Tutorial OpenGL](http://cgvr.informatik.uni-bremen.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html)
 - [**Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix\***](http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf)
 - [Fabian Giesen: View frustum culling (2010)](https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/)
 - [**Zeux: View frustum culling optimization\***](https://zeuxcg.org/2009/03/01/view-frustum-culling-optimization-never-let-me-branch/)
 


## Depth Buffer
  - [NVidia - Depth Precision Visualized](https://developer.nvidia.com/content/depth-precision-visualized)
  - [The Devil In The Details: Linear Depth](http://dev.theomader.com/linear-depth/)
  - [Robert Dunlop: Linearized Depth using Vertex Shaders](https://www.mvps.org/directx/articles/linear_z/linearz.htm)
  - [Emil "Humus" Persson: A couple of notes about Z](http://www.humus.name/index.php?ID=255)
  - [MJP: Reconstructing Position From Depth](https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/)
  - [David Lenaerts: Reconstruction Positions From the Depth Buffer](http://www.derschmale.com/2014/01/26/reconstructing-positions-from-the-depth-buffer/)


## Environment Mapping
 - [**Joe DeVayo: LearnOpenGL - Image-Based Lighting I: Diffuse Irradiance\***](https://learnopengl.com/PBR/IBL/Diffuse-irradiance)
 - [**Joe DeVayo: LearnOpenGL - Image-Based Lighting II: Specular IBL\***](https://learnopengl.com/PBR/IBL/Specular-IBL)

## Math
- [gamedev StackExchange QA: Cylindrical Projection UV Coordinates](https://gamedev.stackexchange.com/questions/114412/how-to-get-uv-coordinates-for-sphere-cylindrical-projection)
- [Paul Bourke: Converting to/from Cube Maps](http://paulbourke.net/miscellaneous/cubemaps/)
- [Holger Dammertz - Hammersley Points on the Hemisphere](http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html )
- [Scratchapixel: Introduction to Quasi Monte Carlo](https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/monte-carlo-methods-in-practice/monte-carlo-methods)


## Occlusion Culling
- [GPU Gems - Chaper 29: Efficient Occlusion Culling](http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch29.html)
- [Gamasutra: Occlusion Culling Algorithms](https://www.gamasutra.com/view/feature/131801/occlusion_culling_algorithms.php?page=1)
- [Intel: Software Occlusion Culling](https://software.intel.com/en-us/articles/software-occlusion-culling)
  - Setup a software rasterizer for depth buffer
  - Render the set of 'occluders' into depth buffer
  - Test all the 'occludees' aginst depth buffer
  - SSE/AVX for optimization
- [Fabian Giesen: Optimizing Software Occlusion Culling](https://fgiesen.wordpress.com/2013/02/17/optimizing-sw-occlusion-culling-index/)
 - [Masked Software Occlusion Culling](https://software.intel.com/sites/default/files/managed/ef/61/masked-software-occlusion-culling.pdf)
   - Iteration of previous [Intel: Software Occlusion Culling](https://software.intel.com/en-us/articles/software-occlusion-culling)
  - [Nick Darnell: DirectX11 Hi-Z Occlusion Culling](https://www.nickdarnell.com/hierarchical-z-buffer-occlusion-culling/)

## Performance

 - [The Peak-Performance Analysis Method for Optimizing Any GPU Workload](https://devblogs.nvidia.com/the-peak-performance-analysis-method-for-optimizing-any-gpu-workload/)
 

## Profiling & Debugging 

- [GDC 2016: Jeffrey Kiel - "Raise your Game with NVIDIA GeForce Tools"](https://archive.org/details/GDC2016Kiel)


## Shadow Rendering

- [Docs MS: Common Techniques to Improve Shadow Depth Maps](https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/common-techniques-to-improve-shadow-depth-maps)


# VQEngine Architecture


VQEngine architecture is more or less based on [Nicolas Guillemot's CppCon2017 Talk:  “Design Patterns for Low-Level Real-Time Rendering”](https://www.youtube.com/watch?v=mdPeXJ0eiGc).

![](renderer-design.PNG)
