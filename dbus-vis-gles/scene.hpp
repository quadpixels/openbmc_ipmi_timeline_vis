// 2022-12-30

#include <mutex>
#include <unordered_map>
#include <vector>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "animator.hpp"
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
  std::mutex mtx;
  constexpr static float kSceneRadius = 60;  // float cannot be const, must be constexpr.
  constexpr static float kRepulsionDistThresh = 20;
  constexpr static float kRepulsionFactor = 100;
  constexpr static float kDampening = 0.95f;

  // Load assets
  enum AssetID {
    OpenBMC = 0,
    HwMon,
    Background,
    DefaultDaemon,
    ObjectMapper,
    IpmiHost,
    EntityManager,
  };
  std::unordered_map<AssetID, ChunkIndex*> chunk_assets;
  std::unordered_map<AssetID, int> assets_occ;

  struct SpriteAndProperty {
    SpriteAndProperty() : marked_for_deletion(false) {}
    ChunkSprite* sprite;
    std::string dbus_service_name;
    ~SpriteAndProperty() {
      delete sprite;
    }
    enum Usage {
      Background = 0,
      MovingSprite = 1,
    };
    Usage usage;
    bool marked_for_deletion;
  };

  struct Projectile {
    constexpr static float kLineLength = 12.0f;
    constexpr static float kSpeed = 100.0f;
    static ChunkIndex* chunk_index;
    Sprite* sprite;  // The projectile itself
    Sprite *from_sprite, *to_sprite;  // targets
    glm::vec3 p0, p1; // 尾迹
    void Render();
    void Update(float secs);
    float projectile_lifetime, projectile_total_lifetime;
    float trail_lifetime, trail_total_lifetime;
    bool IsProjectileVisible();
    bool IsTrailVisible();
    bool Done();
    Projectile(Sprite* from, Sprite* to);
  };

  struct SignalWave {
    constexpr static int kBreaks = 8;
    constexpr static float kDefaultTotalLifetime = 0.5f;
    constexpr static float kRadius = 32.0f;
    glm::vec3 pos;
    float lifetime, total_lifetime;
    SignalWave(Sprite* from);
    bool Done();
    void Update(float secs);
    std::vector<float> GenerateVertexList();
  };

  std::vector<struct SpriteAndProperty*> sprites;
  std::vector<struct Projectile*> projectiles;
  std::vector<SignalWave*> signal_waves;
  std::unordered_map<std::string, struct SpriteAndProperty*> dbus_services;

  // 画线用
  unsigned int lines_vbo;
  const static int kNumMaxLines = 10000;
  std::vector<float> lines_buf;
  unsigned int line_drawing_program;

  DBusPCAPScene();

  SpriteAndProperty* CreateSprite(AssetID asset_id, const glm::vec3& pos);
  ChunkSprite* openbmc_sprite;
  ChunkSprite* bg_sprite;
  
  OneChunkScene::Backdrop* backdrop;
  Camera camera;
  struct CamZoom {
    CamZoom(const Camera& camera);
    glm::vec3 camera_lookdir;
    glm::vec3 camera_focus;
    float min_dist = 150.0f;
    float max_dist = 550.0f;
    float speed = 0.0f;
    float dist = 550.0f;
    float acc = 80.0f;
    float speed_min = -200.0f;
    float speed_max = 100.0f;
    void Update(float secs);
    void OnMessage();
    glm::vec3 GetCamTargetPosition();
    void Push();
  };
  CamZoom* cam_zoom;
  DirectionalLight* directional_light;
  glm::mat4 projection_matrix;
  DepthOnlyFBO* depth_fbo;

  void Render();
  void Update(float secs);
  void Test1();
  void Test2();

  Animator animator;

  // Exercise these two functions with DBus activity
  SpriteAndProperty* GetOrCreateDBusServiceSprite(const std::string& service, const std::string& path, const std::string& iface, const std::string& member, bool is_sig);
  SpriteAndProperty* DBusServiceFadeIn(const std::string& service, const std::string& path, const std::string& iface, const std::string& member, bool is_sig);
  void DBusServiceFadeOut(const std::string& service);
  void DBusMakeMethodCall(const std::string& from, const std::string& to, const std::string& path, const std::string& iface, const std::string& member);
  void DBusEmitSignal(const std::string& from, const std::string& path, const std::string& iface, const std::string& member);
};

class AccompanimentScene : public Scene {
public:
  enum AssetID {
    Scene20230226,
    NarcissusTazetta,
  };

  std::unordered_map<AssetID, ChunkIndex*> chunk_assets;
  OneChunkScene::Backdrop* backdrop;
  Camera camera;
  DirectionalLight* directional_light;
  DepthOnlyFBO* depth_fbo;
  glm::mat4 projection_matrix;
  float seconds;

  struct SpriteAndProperty {
    SpriteAndProperty() : sprite(nullptr), tag(0) {}
    ChunkSprite* sprite;

    char tag;
    float phase; // 跳越时的phase
    glm::vec3 pos0;
  };

  AccompanimentScene();
  void Render();
  void Update(float secs);
  SpriteAndProperty* CreateSprite(AssetID asset_id, const glm::vec3& pos);

  std::vector<struct SpriteAndProperty*> sprites;
};