# 2D SPH Fluid Simulation with Vulkan

This project is a real-time 2D fluid simulation built from scratch using the Vulkan API. It simulates the behavior of thousands of interacting particles using the **Smoothed Particle Hydrodynamics (SPH)** method. The entire physics pipeline is offloaded to the GPU using compute shaders for performance.

The core of the simulation is an efficient spatial hash grid that accelerates the neighbor search process, allowing the  simulation to scale to a large number of particles while maintaining a high framerate.

___
### Key Features
* **Real-time SPH simulation**: implements density, pressure, viscosity, and external forces (gravity, mouse interaction)
* **GPU-Accelerated**: The entire simulation, from neighbor searching to physics calculations are offloaded to run within compute shaders.
* **Efficient Neighbor Search**: Uses a multi-pass compute pipeline to build a spatial hash grid on the GPU each frame.
* **Parallel Sorting**: Features a **Bitonic Merge Sort** implemented in a compute shader to efficiently group particles by their grid cell.
* **Interactive**: Click and drag the mouse to apply forces to the fluid particles.
* **Customizable**: Easily tweak simulation parameters like gravity, viscosity, particle count, search radius, colors, etc.

___

### Technnology Stack
* **Language**: C++
* **Graphics API**: Vulkan
* **Shaders**: GLSL (compiled to SPIR-V)
* **Windowing API**: GLFW
___

### How It Works
The simulation avoids the slow, brute-force $O(n^2)$ approach for particle interaction by using a GPU-driven spatial grid. Each frame, a sequence of compute shaders is dispatched to perform the following steps:
1. **Hashing**: Each particle's position is converted into a grid cell coordinate, and a single hash key is generated from it.
2. **Sorting**: The buffer of hash keys (and a parallel buffer of particle indices) is sorted using the parallel Bitonic Sort. This groups all particles within the same grid cell together in memory.
3. **Indexing**: A quick lookup table is generated that marks the start and end index for each cell's particle group in the sorted buffer.
4. **Physics Simulation**: The main SPH compute shader runs. For each particle, it uses the lookup table to query only its own grid cell and the * adjacent cells, drastically reducing the number of particles it needs to check for calculating density forces.

The simulation uses a double-buffering ("ping-pong") system for particle data, reading from one buffer and writing the updated results to another each frame to prevent data hazards.

___
### Building
**Prerequisites**
* A Vulkan-compatible GPU with up-to-date drivers.
* The official Vulkan SDK
* VS2022 (with MSVC C++ compiler)


As of now, there is no cmake.