# Horizon Engine

## Introduction

Horizon Engine is an open-source 3D rendering engine, focusing on modern rendering engine architecture and rendering techniques. Serving a different purpose than game engines, this project aims to build a highly scalable rendering framework to improve the productivity of prototype projects and academic research.

Horizon Engine is currently only supported on Windows and only target modern graphics APIs (Direct3D 12, Vulkan).

Goals:

* Efficient and Flexible Rendering
* Fast Rendering Techniques Experimentation, eg. Hybrid Rendering with DXR or Vulkan Ray Tracing KHR

Features:

* Bindless Resources
* Support for OpenUSD
* Skeletal Animation
* Render Graph
* Path Tracing Renderer (In development)
* Rasterization Renderer
    * Meshlet Rendering
    * Visibility Buffer
    * Deferred Material Shading
    * Light Grid
    * Atmosphere Rendering
    * Shadow Rendering
        * Cascaded Shadow Maps
        * Virtual Shadow Maps
        * Ray Traced Shadow Maps
        * Screen Space Shadows (Bend SSS)
    * Ambient Occlusion
        * Screen Space Ambient Occlusion (GTAO)
    * Volumetric Fog
    * Image Based Lighting
    * Reflections
        * Screen Space Reflections (In development)
    * Subsurface Scattering (In development)
    * Post Processing Pipeline
        * Bloom
        * Lens Flare
        * Motion Blur
        * Auto Exposure
        * Local Tone Mapping
        * Temporal Super Sampling (FSR3, DLSS)

## Gallery

![image](/ScreenShots/Editor.png)

![image](/ScreenShots/Meshlet.png)

![image](/ScreenShots/SkyAtmosphere.png)

![image](/ScreenShots/GTAO.png)

![image](/ScreenShots/SkeletalAnimation.png)

## Requirements
* Operating System: Windows
    * Windows 11 (All versions)
    * Windows 10 Version 1909 and newer (Build 18363)
* GPU: NIVIDIA Graphics Cards, and keep your graphics drivers up to date (https://www.nvidia.com/Download/index.aspx)
* IDE: Visual Studio 2022, and make sure C++ Modules for v143 build tools (x64/x86 - experimental) is installed

## Getting Started

1. Clone this repository

`git clone https://github.com/tataliya7/HorizonEngine`

2. Run "Setup.bat"

3. Run "GenerateProjects.bat"

4. Open "Horizon.sln" and build the solution

## Documentation

See [Horizon Documentation](https://tataliya7.github.io/HorizonEngine/).
