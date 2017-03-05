# DX11Renderer

![](Screenshots/spot.PNG)

This is the open source part of the Renderer module for my [3D-Animation project](https://www.youtube.com/watch?v=Rt-h-bMA8Xc).

I plan to keep adding features to this Renderer module to tinker with rendering algorithms and some special effects.

**Features**:
 - Phong Lighting
 - Diffuse Textures
 - Generated Geometry: Cube, Sphere, Cylinder, Grid
 
## Compiling Instructions & Dependencies

Navigate to `Source\3rdParty\DirectXTex` and compile the solution `DirectXTex_Desktop_2015.sln` with the platform settings that match the DX11Renderer's.
This will generate the lib file that is required to compile the DX11Renderer project.

 - The project uses [DirectXTex](https://github.com/Microsoft/DirectXTex) for textures. Current project settings
look for a `DirectXTex.lib` file in `3rdParty\DirectXTex\DirectXTex\Bin\Desktop_2015\$(Platform)\$(Configuration)`. 
You will get the following error message on build unless you have already copmiled the DirectXTex library:  
    > `2>LINK : fatal error LNK1104: cannot open file 'DirectXTex.lib'`


## Controls

| Key | Control |
| :---: | :--- |
| **WASD** |	Camera Movement |
| **IJKL** |	Central Object Movement |
| **UOP** |	Central Object Rotation  |
| **F1** |	TexCoord Shader |
| **F2** |	Normals Shader |
| **F3** |	Toggle Mesh Draw (Inactive) |
| **F4** |	Toggle Normal Draw (Inactive) |
| **F5** |	Unlit Shader |
| **F6** |	Phong Shader |
| **F7** |	BRDF Shader (Inactive) |
| **F9** |	Toggle Gamma Correction |
| **Backspace** | Pause App |
| **ESC** |	Exit App |
 
