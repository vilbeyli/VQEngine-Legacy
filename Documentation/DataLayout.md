# Types

| | |
| :-- | :-- |
| f | float |
| f# | float# <sub>e.g. f2 <-> float2</sub> |
| b | bool |
| i | int |
| ui | unsigned |
| d | double |
| e | enum |

# Engine Primitive Data Types

| DataType | Size(short) | Size         |  --------     | DataType | Size(short) | Size |
| :-- | :--: | :--:                     | :--:  | :-- | :--: | :--: |
| **Transform** | **f10** | **40B**     |       | **Bounding Box** | **f6** | **24B**   |
| : pos | f3 |                            |       |  : hi | f3                               |
| : quat | f4 |                           |       |  : lo | f3                               |
| : scale | f3 |                          |       |   
| |                                     |       | 
| **Light**   | ? | **124B**                   | 
|  : LightType   | e |                     |
|  : Color       | f3 |                    |
|  : brightness  | f |                     |
|  : shadows?    | b |                     |
|  : depthBias   | f |                     |
|  : near        | f |                     |
|  : far(range)  | f |                     |
|  : Transform   | f10 |                   |
|  : meshID      | i  |                    |
|  : enabled?    | b  |                    |
|  : lightData   | f3 |                    |


## Model
  - Model : i * N<mesh, tmesh, mat>
    - meshIDs   : i * N<mesh>
    - tmeshIDs  : i * N<tmesh>
    - matLookup : i * N<mat/mesh>


## GameObject

- GameObject
    - Transform   : f10
    - Model       : i * N<mesh, tmes, mat>
    - AABB        : f6
    - MeshAABBs   : f6 * N



# Scene

## Vectors
- Meshes[]    | ???B / Mesh
- Cameras[]   | ???B / Camera
- Lights[]    | 126B / Light
  - LightType   : e
  - Color       : f3
  - brightness  : f
  - shadows?    : b
  - depthBias   : f
  - near        : f
  - far(range)  : f
  - Transform   : f10
    - pos             : f3
    - quat            : f4
    - scale           : f3
  - meshID      : i
  - enabled?    : b
  - lightData   : f3
- pObjects[]






