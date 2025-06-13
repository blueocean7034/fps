# Development Guidelines

- Use **C++17** with **SDL2** for windowing and input, **OpenGL** for rendering and **GLM** for math.
- Integrate **Bullet** for physics and **OpenAL** for audio. Build the project with **CMake** and track everything with **Git**.
- Keep dependencies light and rendering simple so the game runs well on low performance hardware.
- Shaders must imitate N64 visuals – basic vertex colors without modern effects.
- The sample scene should start the player in a large room. Include gravity so walking and jumping work and remove any spinning cube.
- Place all `no_texture` placeholder images inside the `images/` folder.
