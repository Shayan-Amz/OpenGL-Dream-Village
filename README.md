# 🏡 Dream Village — OpenGL Graphics Simulation

A 2D/3D computer graphics environment built using **C++** and **OpenGL (FreeGLUT/GLEW)** simulating an interactive village landscape with dynamic animations and environmental elements.

---

## 📌 Project Overview

This project renders a village scene featuring terrain, water bodies, village architecture, and automated animations:
- **Atmospheric Animations:** Moving clouds (`cloud1` to `cloud5`) with smooth horizontal drift across the sky.
- **Wildlife Simulation:** Animated flying birds (`birdmove1`, `birdmove2`) following coordinate trajectories.
- **Waterway Mechanics:** River/lake scenery with a moving small boat (`smallboat`).
- **Nature & Village Scenery:** Houses, natural terrain, vegetation, and fruit-bearing trees with individual object rendering (`orange1` to `orange6`).

---

## 🛠️ Tech Stack & Dependencies

- **Language:** C++ (MSVC v143 toolset)
- **Graphics API:** OpenGL
- **Windowing & Utility:** FreeGLUT, GLEW, GLFW
- **Package Management:** NuGet (`nupengl.core`)
- **IDE:** Visual Studio 2022

---

## 📁 Project Structure

```text
Shapes/
│
├── main.cpp                 # Core application logic, drawing routines, and display loops
├── Shapes.sln               # Visual Studio Solution file
├── Shapes.vcxproj           # Visual Studio Project configuration
├── packages.config          # NuGet package dependencies (Nupengl core & redist)
├── .gitignore               # Git build artifact exclusions
└── README.md                # Project documentation
