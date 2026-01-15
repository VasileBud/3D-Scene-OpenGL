#include "Model3D.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gps {

	void Model3D::LoadModel(std::string fileName) {

        std::string basePath = fileName.substr(0, fileName.find_last_of('/')) + "/";
		ReadOBJ(fileName, basePath);
	}

    void Model3D::LoadModel(std::string fileName, std::string basePath)	{

		ReadOBJ(fileName, basePath);
	}

	// Draw each mesh from the model
	void Model3D::Draw(gps::Shader shaderProgram) {

		for (int i = 0; i < meshes.size(); i++)
			meshes[i].Draw(shaderProgram);
	}

	// Does the parsing of the .obj file and fills in the data structure
	void Model3D::ReadOBJ(std::string fileName, std::string basePath) {

        std::cout << "Loading : " << fileName << std::endl;
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		int materialId;

		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str(), basePath.c_str(), GL_TRUE);

		if (!err.empty()) {

			// `err` may contain warning message.
			std::cerr << err << std::endl;
		}

		if (!ret) {

			exit(1);
		}

		std::cout << "# of shapes    : " << shapes.size() << std::endl;
		std::cout << "# of materials : " << materials.size() << std::endl;

        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {

			std::vector<gps::Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<gps::Texture> textures;

			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

				int fv = shapes[s].mesh.num_face_vertices[f];

				//gps::Texture currentTexture = LoadTexture("index1.png", "ambientTexture");
				//textures.push_back(currentTexture);

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {

					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

					float vx = attrib.vertices[3 * idx.vertex_index + 0];
					float vy = attrib.vertices[3 * idx.vertex_index + 1];
					float vz = attrib.vertices[3 * idx.vertex_index + 2];
					float nx = attrib.normals[3 * idx.normal_index + 0];
					float ny = attrib.normals[3 * idx.normal_index + 1];
					float nz = attrib.normals[3 * idx.normal_index + 2];
					float tx = 0.0f;
					float ty = 0.0f;

					if (idx.texcoord_index != -1) {

						tx = attrib.texcoords[2 * idx.texcoord_index + 0];
						ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					}

					glm::vec3 vertexPosition(vx, vy, vz);
					glm::vec3 vertexNormal(nx, ny, nz);
					glm::vec2 vertexTexCoords(tx, ty);

					gps::Vertex currentVertex;
					currentVertex.Position = vertexPosition;
					currentVertex.Normal = vertexNormal;
					currentVertex.TexCoords = vertexTexCoords;

                    minBounds = glm::min(minBounds, vertexPosition);
                    maxBounds = glm::max(maxBounds, vertexPosition);

					vertices.push_back(currentVertex);

					indices.push_back((GLuint)(index_offset + v));
				}

				index_offset += fv;
			}

			// get material id
			// Only try to read materials if the .mtl file is present
			size_t a = shapes[s].mesh.material_ids.size();

			if (a > 0 && materials.size()>0) {

				materialId = shapes[s].mesh.material_ids[0];
				if (materialId != -1) {

					gps::Material currentMaterial;
					currentMaterial.ambient = glm::vec3(materials[materialId].ambient[0], materials[materialId].ambient[1], materials[materialId].ambient[2]);
					currentMaterial.diffuse = glm::vec3(materials[materialId].diffuse[0], materials[materialId].diffuse[1], materials[materialId].diffuse[2]);
					currentMaterial.specular = glm::vec3(materials[materialId].specular[0], materials[materialId].specular[1], materials[materialId].specular[2]);

					//ambient texture
					std::string ambientTexturePath = materials[materialId].ambient_texname;

					if (!ambientTexturePath.empty()) {

						gps::Texture currentTexture;
						currentTexture = LoadTexture(basePath + ambientTexturePath, "ambientTexture");
						textures.push_back(currentTexture);
					}

					//diffuse texture
					std::string diffuseTexturePath = materials[materialId].diffuse_texname;

					if (!diffuseTexturePath.empty()) {

						gps::Texture currentTexture;
						currentTexture = LoadTexture(basePath + diffuseTexturePath, "diffuseTexture");
						textures.push_back(currentTexture);
					}

					//specular texture
					std::string specularTexturePath = materials[materialId].specular_texname;

					if (!specularTexturePath.empty()) {

						gps::Texture currentTexture;
						currentTexture = LoadTexture(basePath + specularTexturePath, "specularTexture");
						textures.push_back(currentTexture);
					}
				}
			}

			meshes.push_back(gps::Mesh(vertices, indices, textures));
		}

        modelBounds.min = minBounds;
        modelBounds.max = maxBounds;
        boundsValid = true;

        const float normalThreshold = 0.6f;
        walkTriangles.clear();

        for (const auto& mesh : meshes)
        {
            for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
            {
                const glm::vec3 v0 = mesh.vertices[mesh.indices[i]].Position;
                const glm::vec3 v1 = mesh.vertices[mesh.indices[i + 1]].Position;
                const glm::vec3 v2 = mesh.vertices[mesh.indices[i + 2]].Position;

                const glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                if (n.y < normalThreshold)
                {
                    continue;
                }

                walkTriangles.push_back({v0, v1, v2});
            }
        }

        walkGridOrigin = glm::vec2(modelBounds.min.x, modelBounds.min.z);
        walkGridWidth = static_cast<int>(std::ceil((modelBounds.max.x - modelBounds.min.x) / walkCellSize)) + 1;
        walkGridHeight = static_cast<int>(std::ceil((modelBounds.max.z - modelBounds.min.z) / walkCellSize)) + 1;
        walkGrid.assign(walkGridWidth * walkGridHeight, {});

        for (size_t t = 0; t < walkTriangles.size(); ++t)
        {
            const auto& tri = walkTriangles[t];
            const float minX = std::min({tri.v0.x, tri.v1.x, tri.v2.x});
            const float maxX = std::max({tri.v0.x, tri.v1.x, tri.v2.x});
            const float minZ = std::min({tri.v0.z, tri.v1.z, tri.v2.z});
            const float maxZ = std::max({tri.v0.z, tri.v1.z, tri.v2.z});

            int ix0 = static_cast<int>(std::floor((minX - walkGridOrigin.x) / walkCellSize));
            int ix1 = static_cast<int>(std::floor((maxX - walkGridOrigin.x) / walkCellSize));
            int iz0 = static_cast<int>(std::floor((minZ - walkGridOrigin.y) / walkCellSize));
            int iz1 = static_cast<int>(std::floor((maxZ - walkGridOrigin.y) / walkCellSize));

            ix0 = std::clamp(ix0, 0, walkGridWidth - 1);
            ix1 = std::clamp(ix1, 0, walkGridWidth - 1);
            iz0 = std::clamp(iz0, 0, walkGridHeight - 1);
            iz1 = std::clamp(iz1, 0, walkGridHeight - 1);

            for (int iz = iz0; iz <= iz1; ++iz)
            {
                for (int ix = ix0; ix <= ix1; ++ix)
                {
                    walkGrid[iz * walkGridWidth + ix].push_back(static_cast<int>(t));
                }
            }
        }

        walkGridValid = true;

	}

    bool Model3D::getHeightAt(float x, float z, float currentY, float& outHeight) const
    {
        if (!walkGridValid || walkGridWidth == 0 || walkGridHeight == 0)
        {
            return false;
        }

        int ix = static_cast<int>(std::floor((x - walkGridOrigin.x) / walkCellSize));
        int iz = static_cast<int>(std::floor((z - walkGridOrigin.y) / walkCellSize));

        if (ix < 0 || ix >= walkGridWidth || iz < 0 || iz >= walkGridHeight)
        {
            return false;
        }

        const float eps = 1e-4f;
        float bestDiff = std::numeric_limits<float>::max();
        bool found = false;

        for (int dz = -1; dz <= 1; ++dz)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int cx = ix + dx;
                int cz = iz + dz;
                if (cx < 0 || cx >= walkGridWidth || cz < 0 || cz >= walkGridHeight)
                {
                    continue;
                }

                const auto& cell = walkGrid[cz * walkGridWidth + cx];
                for (int triIndex : cell)
                {
                    const auto& tri = walkTriangles[triIndex];
                    const glm::vec2 a(tri.v0.x, tri.v0.z);
                    const glm::vec2 b(tri.v1.x, tri.v1.z);
                    const glm::vec2 c(tri.v2.x, tri.v2.z);
                    const glm::vec2 p(x, z);

                    const glm::vec2 v0 = b - a;
                    const glm::vec2 v1 = c - a;
                    const glm::vec2 v2 = p - a;

                    float denom = v0.x * v1.y - v1.x * v0.y;
                    if (std::abs(denom) < eps)
                    {
                        continue;
                    }

                    float v = (v2.x * v1.y - v1.x * v2.y) / denom;
                    float w = (v0.x * v2.y - v2.x * v0.y) / denom;
                    float u = 1.0f - v - w;

                    if (u < -eps || v < -eps || w < -eps)
                    {
                        continue;
                    }

                    float y = u * tri.v0.y + v * tri.v1.y + w * tri.v2.y;
                    float diff = std::abs(y - currentY);
                    if (diff < bestDiff)
                    {
                        bestDiff = diff;
                        outHeight = y;
                        found = true;
                    }
                }
            }
        }

        return found;
    }


	// Retrieves a texture associated with the object - by its name and type
	gps::Texture Model3D::LoadTexture(std::string path, std::string type) {

			for (int i = 0; i < loadedTextures.size(); i++) {

				if (loadedTextures[i].path == path)	{

					//already loaded texture
					return loadedTextures[i];
				}
			}

			gps::Texture currentTexture;
			currentTexture.id = ReadTextureFromFile(path.c_str());
			currentTexture.type = std::string(type);
			currentTexture.path = path;

			loadedTextures.push_back(currentTexture);

			return currentTexture;
		}

	// Reads the pixel data from an image file and loads it into the video memory
	GLuint Model3D::ReadTextureFromFile(const char* file_name) {

		int x, y, n;
		int force_channels = 4;
		unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);

		if (!image_data) {
			fprintf(stderr, "ERROR: could not load %s\n", file_name);
			return false;
		}
		// NPOT check
		if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
			fprintf(
				stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
			);
		}

		int width_in_bytes = x * 4;
		unsigned char *top = NULL;
		unsigned char *bottom = NULL;
		unsigned char temp = 0;
		int half_height = y / 2;

		for (int row = 0; row < half_height; row++) {

			top = image_data + row * width_in_bytes;
			bottom = image_data + (y - row - 1) * width_in_bytes;

			for (int col = 0; col < width_in_bytes; col++) {

				temp = *top;
				*top = *bottom;
				*bottom = temp;
				top++;
				bottom++;
			}
		}

		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_SRGB, //GL_SRGB,//GL_RGBA,
			x,
			y,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			image_data
		);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		return textureID;
	}

	Model3D::~Model3D() {

        for (size_t i = 0; i < loadedTextures.size(); i++) {

            glDeleteTextures(1, &loadedTextures.at(i).id);
        }

        for (size_t i = 0; i < meshes.size(); i++) {

            GLuint VBO = meshes.at(i).getBuffers().VBO;
            GLuint EBO = meshes.at(i).getBuffers().EBO;
            GLuint VAO = meshes.at(i).getBuffers().VAO;
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }
	}
}
