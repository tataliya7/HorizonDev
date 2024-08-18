# Horizon Engine

Horizon Engine is an open-source 3D rendering engine, focusing on modern rendering engine architecture and rendering techniques. Serving a different purpose than game engines, this project aims to build a highly scalable rendering framework to improve the productivity of prototype projects and academic research, but also to serve as an educational tool for teaching rendering engine design and implementation from scratch.

Horizon Engine is currently only supported on Windows and only target modern graphics APIs (Direct3D 12, Vulkan).

<!--
[![Bilibili]()]()
-->

[![Discord](https://badgen.net/badge/icon/discord?icon=discord&label)](https://discord.gg/nepzQHf2jv)

<!--
# [![Patreon](https://badgen.net/badge/icon/patreon?icon=patreon&label)]()
-->

## Introduction

Goals:

* Efficent and Flexible Rendering
* Fast Rendering Techniques Experimentation, eg. Hybrid Rendering with DXR or Vulkan Ray Tracing KHR

## Requirements

* Windows 10 or 11
* NIVIDIA Graphics Cards, and keep your graphics drivers up to date (https://www.nvidia.com/Download/index.aspx)
* Vulkan SDK 1.3.250.1, this repository tries to always be up to date with the latest Vulkan SDK (https://vulkan.lunarg.com/sdk/home)
* Visual Studio 2022, and make sure C++ Modules for v143 build tools (x64/x86 - experimental) is installed
* CUDA SDK 11.7 (Optional)

## Getting Started

1. Clone this repository

`git clone https://github.com/tataliya7/HorizonEngine`

2. Run "Setup.bat"

3. Run "GenerateProjects.bat"

4. Open "Horizon.sln" and build the solution

## Documentation

See [Horizon Documentation](https://tataliya7.github.io/HorizonEngine/).

## Contribution

Contributions are welcome.
