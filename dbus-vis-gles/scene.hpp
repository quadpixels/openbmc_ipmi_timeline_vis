// 2022-12-30

#include <unordered_map>
#include <vector>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "camera.hpp"
#include "chunk.hpp"
#include "chunkindex.hpp"
#include "rendertarget.hpp"
#include "sprite.hpp"
#include "util.hpp"

class Scene {
public:
  virtual void Render() = 0;
  virtual void Update(float secs) = 0;
};

class HelloTriangleScene : public Scene {
public:
  unsigned int vbo_solid, vbo_line; // Buffer
  // Shader Program = VS + FS.
  unsigned int shader_program;
  HelloTriangleScene();
  void Render();
  void Update(float secs) {}
};

class PaletteScene : public Scene {
public:
  unsigned int vbo;
  unsigned int shader_program;
  PaletteScene();
  void Render();
  void Update(float secs) {}
};

class RotatingCubeScene : public Scene {
public:
  unsigned int vbo;
  unsigned int ebo;
  unsigned int shader_program;
  unsigned int vbo_lines;
  unsigned int ebo_lines;
  glm::mat4 projection_matrix;
  std::vector<glm::vec3> positions;
  std::vector<glm::mat3> orientations;
  Camera camera;
  RotatingCubeScene();
  void Render();
  void Update(float secs);
};

class OneChunkScene : public Scene {  // Show 1 Vox Chunk
public:
  // Facing upwards (+Y)
  // Backdrop uses Chunk's shader program
  class Backdrop {
  public:
    Backdrop(float half_width, int color_index);
    unsigned int vbo;
    unsigned int ebo;
    glm::vec3 pos;
    void Render();
  };
  Backdrop* backdrop;

  // For dummy texture resource
  unsigned int depth_buffer_tex;

  DepthOnlyFBO* depth_fbo;

  unsigned int shader_program;
  glm::mat4 projection_matrix;
  Camera camera;
  DirectionalLight* directional_light;

  Chunk chunk;
  ChunkIndex* chunkindex;
  ChunkSprite* chunksprite;

  OneChunkScene();
  void Render();
  void Update(float secs);
};

class TextureScene : public Scene {  // Render a texture AND an FBO onto the screen.
public:
  // Draw texture to screen
  unsigned int vbo_tex;
  unsigned int ebo_tex;
  unsigned int tex; // The texture to be drawn
  unsigned int depth_buffer_tex;
  unsigned int shader_program;

  BasicFBO* basic_fbo;
  DepthOnlyFBO* depth_fbo;

  TextureScene();
  void Render();
  void Update(float secs);
};

class DBusPCAPScene : public Scene {  // Putting everything together . . .
public:
  // Load assets
  enum AssetID {
    OpenBMC = 0,
    HwMon = 1,
  };
  std::unordered_map<AssetID, ChunkIndex*> chunk_assets;

  struct SpriteAndProperty {
    ChunkSprite* sprite;
    std::string dbus_service_name;
    ~SpriteAndProperty() {
      delete sprite;
    }
  };

  struct Projectile {
    static ChunkIndex* chunk_index;
    Sprite* sprite;  // The projectile itself
    Sprite *from_sprite, *to_sprite;  // targets
    glm::vec3 pos, dir, p0, p1;
    void Render();
    void Update(float secs);
    float lifetime, total_lifetime;

    Projectile(Sprite* from, Sprite* to);
  };

  std::vector<struct SpriteAndProperty*> sprites;
  std::vector<struct Projectile*> projectiles;

  // 画线用
  unsigned int lines_vbo;
  const static int kNumMaxLines = 10000;
  std::vector<float> lines_buf;
  unsigned int line_drawing_program;

  DBusPCAPScene();

  SpriteAndProperty* CreateSprite(AssetID asset_id, const glm::vec3& pos);
  ChunkSprite* openbmc_sprite;
  
  OneChunkScene::Backdrop* backdrop;
  Camera camera;
  DirectionalLight* directional_light;
  glm::mat4 projection_matrix;
  DepthOnlyFBO* depth_fbo;

  void Render();
  void Update(float secs);
  void Test1();
  void Test2();
};