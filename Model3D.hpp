#ifndef Model3D_hpp
#define Model3D_hpp

#include "Mesh.hpp"

#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>

namespace gps {

    class Model3D {

    public:
        struct AABB {
            glm::vec3 min;
            glm::vec3 max;
        };

        ~Model3D();

		void LoadModel(std::string fileName);

		void LoadModel(std::string fileName, std::string basePath);

		void Draw(gps::Shader shaderProgram);

        AABB getBounds() const { return modelBounds; }
        bool getHeightAt(float x, float z, float currentY, float& outHeight) const;

    private:
		// Component meshes - group of objects
        std::vector<gps::Mesh> meshes;
		// Associated textures
        std::vector<gps::Texture> loadedTextures;
        AABB modelBounds{};
        bool boundsValid = false;
        struct WalkTriangle {
            glm::vec3 v0;
            glm::vec3 v1;
            glm::vec3 v2;
        };

        float walkCellSize = 0.5f;
        int walkGridWidth = 0;
        int walkGridHeight = 0;
        glm::vec2 walkGridOrigin{};
        std::vector<WalkTriangle> walkTriangles;
        std::vector<std::vector<int>> walkGrid;
        bool walkGridValid = false;

		// Does the parsing of the .obj file and fills in the data structure
		void ReadOBJ(std::string fileName, std::string basePath);

		// Retrieves a texture associated with the object - by its name and type
		gps::Texture LoadTexture(std::string path, std::string type);

		// Reads the pixel data from an image file and loads it into the video memory
		GLuint ReadTextureFromFile(const char* file_name);
    };
}

#endif /* Model3D_hpp */
