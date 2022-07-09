# SceneConversionTool

The conversion tool takes a model file (e.g. a `.obj` file) and returns a mesh file, a scene file, a material file, and a series of 512x512 textures.

The mesh file contains all the mesh data. The scene file contains the DOD scene graph. The material file contains all the material data. The tool also goes through all the textures, downscales them to 512x512 when necessary, and saves them in RGBA `.png` files. 
