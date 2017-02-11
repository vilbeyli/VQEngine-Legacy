# DX11Renderer

![](Screenshots/spot.PNG)

This is the open source part of the Renderer module for my [3D-Animation project](https://www.youtube.com/watch?v=Rt-h-bMA8Xc).

I plan to keep adding features to this Renderer module. 

**Features**:
 - Phong Lighting
 - Diffuse Textures
 - Procedurally Generated Geometry: Cube, Sphere, Cylinder, Grid
 
## Compiling Instructions

The project uses [DirectXTex](https://github.com/Microsoft/DirectXTex) for textures. Current project settings
look for a `DirectXTex.lib` file in `3rdParty\DirectXTex\DirectXTex\Bin\Desktop_2015\$(Platform)\$(Configuration)`. 
You will get the following error message on build unless you have already copmiled the DirectXTex library:

  > `2>LINK : fatal error LNK1104: cannot open file 'DirectXTex.lib'`

You can simply open the project file in `Source\3rdParty\DirectXTex` directory and build it.

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
 
