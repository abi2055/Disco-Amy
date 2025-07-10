# Disco-Amy

[Watch the video](https://www.youtube.com/watch?v=8tNiUWqQ300)

This project renders a 3D scene using modern OpenGL, featuring multiple textured models and dynamic colored spotlights. Built with C++, GLSL shaders, and supporting libraries like GLFW, GLAD, and GLM, the scene includes a rotating lighting system and real-time rendering of OBJ models with diffuse and ambient shading.

Key features:

- ✅ Loaded and textured 3D models (.obj) using TinyOBJLoader
- 🎨 Realistic surface detail with texture mapping via stb_image
- 💡 Three dynamic colored spotlights (red, green, blue) with attenuation and cutoff
- 📐 Model-View-Projection transformations with GLM
- 🖱️ Interactive controls with GLFW (escape to quit, P to screenshot)
- 📸 Framebuffer capture feature saving .ppm screenshots
- ⚙️ Shaders written in GLSL with real-time lighting and rotation logic
