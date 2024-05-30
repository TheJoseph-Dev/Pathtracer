
# Pathtracer

A personal project with the aim of creating a GPU Accelerated Pathtracer using OpenGL


## Features

- Diffuse Rendering
- Specular Rendering
- Transmission and Refraction
- Polygons (Ray-Triangle Intersection)
- GPU Accelerated

## How it developed

First it started with some simple Ray-Sphere Intersections and Basic Diffuse Lighting in Shadertoy

Raytracing without Accumulation, Dot Product Diffuse Lighting
![Archaic](Renders/pathtracing-archaic.png)

Accumulation Without Tone Mapping
![Shadertoy-Pathtracer-1](Renders/pathtracer-shadertoy-1.png) 

Specular Reflections
![Shadertoy-Pathtracer-2](Renders/pathtracer-shadertoy-2.png)

Diffuse Light Bounces
![Shadertoy-Pathtracer-3](Renders/pathtracer-shadertoy-3.png)

---

Then I moved it to OpenGL, allowing much more advanced calculations, performance and controll

Sky and Ambient Light
![OpenGL-Pathtracer](Renders/pathtracer-opengl.jpg)

Ray-Triangle Intersection, Polygons
![OpenGL-Pathtracer-RayTriangle](Renders/pathtracer-opengl-ratri-good-normals.jpg)

Bloom
![OpenGL-Pathtracer-NightBloom](Renders/pahtracer-bloom-night.jpg)

Transmission and Refraction, ImGUI
![OpenGL-Pathtracer-Advanced](Renders/knightPathtracer.jpg)

---

At last, there is a more composed scene viewed in many ways

Scene without Bloom
![OpenGL-FinalScene-NB](Renders/knightPathtracer3.jpg)

Scene with Bloom
![OpenGL-FinalScene-B](Renders/knightPathtracerBloom4.jpg)

Scene without Bloom with Blender Denoise and Glare applied
![OpenGL-FinalScene-BDNB](Renders/knightPathtracerDenoisedBBloom.png)

Scene with Bloom and Blender Denoise
![OpenGL-FinalScene-BDB](Renders/knightPathtracerDenoised.png)


## The Future

I consider the results as satisfactory, there are still some features missing such as caustics, proper light transmission/refraction and Skybox, but the main objective has been achieved for now

## What I've Learned

- The Rendering Equation
- PBR
- Shader programming skills
- Understanding of how a modern renderer work
- Light interactions with objects and nature

## Resources

 - [Sebastian Lague Code Adventure](https://www.youtube.com/watch?v=Qz0KTGYJtUk)
 - [The Cherno Series](https://www.youtube.com/playlist?list=PLlrATfBNZ98edc5GshdBtREv5asFW3yXl)
 - [Eduard0110 OpenGL Pathtracer](https://github.com/Eduard0110/Path-tracer-using-OpenGL)
 - [Shadertoy Pathtracer Series](https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/)
