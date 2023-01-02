#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "util.hpp"
#include "scene.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <fstream>
#include <functional>
#include <thread>
#include <pcap/pcap.h>
#include "pcap_analyzer.hpp"
#endif

GLFWwindow* g_window;
double g_last_secs = 0;

int WIN_W = 1024, WIN_H = 640;

Scene* g_curr_scene;
HelloTriangleScene* g_hellotriangle;
OneChunkScene*      g_onechunk;
PaletteScene*       g_paletteview;
RotatingCubeScene*  g_rotating_cube;
TextureScene*       g_texture_scene;
DBusPCAPScene*      g_dbuspcap_scene;

void Update(float delta_secs) {
  g_curr_scene->Update(delta_secs);
}

void Render() {
  glViewport(0, 0, WIN_W, WIN_H);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(1.0f, 1.0f, 0.9f, 1.0f);
  glDisable(GL_DEPTH_TEST);

  g_curr_scene->Render();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    switch (key) {
    case GLFW_KEY_ESCAPE: {
      printf("Done.\n");
      exit(0);
    }
    case GLFW_KEY_1: g_curr_scene = g_hellotriangle; break;
    case GLFW_KEY_2: g_curr_scene = g_paletteview; break;
    case GLFW_KEY_3: g_curr_scene = g_rotating_cube; break;
    case GLFW_KEY_4: g_curr_scene = g_onechunk; break;
    case GLFW_KEY_5: g_curr_scene = g_texture_scene; break;
    case GLFW_KEY_6: g_curr_scene = g_dbuspcap_scene; break;
    case GLFW_KEY_Q: {
      if (g_curr_scene == g_dbuspcap_scene) {
        g_dbuspcap_scene->Test1();
      }
    }
    case GLFW_KEY_W: {
      if (g_curr_scene == g_dbuspcap_scene) {
        g_dbuspcap_scene->Test2();
      }
    }
    default: break;
    }
  }
}

void MyInit() {
  g_hellotriangle = new HelloTriangleScene();
  g_paletteview = new PaletteScene();
  g_rotating_cube = new RotatingCubeScene();
  g_onechunk = new OneChunkScene();
  g_texture_scene = new TextureScene();
  g_dbuspcap_scene = new DBusPCAPScene();

  Chunk::shader_program = BuildShaderProgram("shaders/vert_norm_data_ao.vs", "shaders/vert_norm_data_ao.fs");
  MyCheckError("Build Chunk's shader program");
}

void emscriptenLoop() {
  double secs = glfwGetTime();
  glfwPollEvents();
  Update(secs - g_last_secs);
  Render();
  glfwSwapBuffers(g_window);
  g_last_secs = secs;
}

#ifndef __EMSCRIPTEN__
struct DBusVisEvent {
  enum MessageType {
    MethodCall,
    Signal,
    ServiceFadeIn, // FadeIn 和 FadeOut 要事后计算出来
    ServiceFadeOut,
  };
  double timestamp;
  MessageType type;
  std::string arg1, arg2;
};
static std::vector<struct DBusVisEvent> g_dbus_vis_events;
// For replay
void MyCallback(unsigned char* user_data, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
	const struct timeval& ts = pkthdr->ts;
	double sec = ts.tv_sec * 1.0 + ts.tv_usec / 1000000.0;

	int caplen = pkthdr->caplen, len = pkthdr->len;

	if (caplen >= 12) {
		AlignedStream s;
		MessageEndian endian;
		FixedHeader fixed;
		DBusMessageFields fields;
		std::vector<DBusType> body;

		ParseHeaderAndBody(packet, caplen, &fixed, &fields, &body);

		std::string path, iface, member, destination, sender;
		uint32_t reply_serial = 0xFFFFFFFF;
		for (const auto& entry : fields) {
			switch (entry.first) {
				case MessageHeaderType::PATH: {
					path = std::get<object_path>(entry.second).str;
					break;
				}
				case MessageHeaderType::INTERFACE: {
					iface = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::MEMBER: {
					member = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::DESTINATION: {
					destination = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::SENDER: {
					sender = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::REPLY_SERIAL: {
					reply_serial = std::get<uint32_t>(entry.second);
					break;
				}
				default: break;
			}
		}
    
    if (fixed.type == MessageType::METHOD_CALL) {
      printf("MC @ %g: %s -> %s\n", sec, sender.c_str(), destination.c_str());
      DBusVisEvent evt;
      evt.type = DBusVisEvent::MessageType::MethodCall;
      evt.arg1 = sender; evt.arg2 = destination;
      evt.timestamp = sec;
      g_dbus_vis_events.push_back(evt);
    }
	}
}

void StartReplayingMessages();
void StartPCAPReplayThread(const char* filename) {
  std::vector<uint8_t> buf;
  std::ifstream is(filename);
  while (is.good()) {
    buf.push_back(is.get());
  }
  SetPCAPCallback(MyCallback);
  ProcessByteArray(buf);

  printf("%zu events. Sleeping for 2s before replaying . . .\n",
    g_dbus_vis_events.size());
  sleep(2);

  StartReplayingMessages();
}

void StartReplayingMessages() {
  if (g_dbus_vis_events.empty()) return;
  double replay_t0 = glfwGetTime();
  double msg_t0 = g_dbus_vis_events[0].timestamp;
  int idx = 0;
  while (idx < int(g_dbus_vis_events.size())) {
    double replay_elapsed = glfwGetTime() - replay_t0;
    struct DBusVisEvent& evt = g_dbus_vis_events[idx];
    double diff_sec = (evt.timestamp - msg_t0) - replay_elapsed;
    if (diff_sec < 0) diff_sec = 0;
    printf("idx=%d, sleep time=%g\n", idx, diff_sec);
    if (diff_sec > 0)
      usleep(diff_sec * 1000000.0f);

    switch (evt.type) {
      case DBusVisEvent::MessageType::MethodCall: {
        g_dbuspcap_scene->DBusMakeMethodCall(evt.arg1, evt.arg2);
        break;
      }
    }
    idx++;
  }
}
#endif

int main(int argc, char** argv) {
  if (glfwInit() == GLFW_FALSE) {
    printf("glfwInit returned false\n");
    exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  g_window = glfwCreateWindow(WIN_W, WIN_H, __FILE__, NULL, NULL);

  // https://discourse.glfw.org/t/invalid-enum-after-glfwmakecontextcurrent/170
  // Happens on ZX C960, ignoring
  glfwMakeContextCurrent(g_window);
  MyCheckError("make context current", true);

  glfwSetKeyCallback(g_window, KeyCallback);

  int major, minor, revision;
  glfwGetVersion(&major, &minor, &revision);
  printf("GLFW version: %d.%d.%d\n", major, minor, revision);
  const char* sz = (const char*)glGetString(GL_VERSION);
  printf("OpenGL Version String: %s\n", sz);
  const char* sz1 = (const char*)glGetString(GL_VENDOR);
  printf("OpenGL Vendor String: %s\n", sz1);
  const char* sz2 = (const char*)glGetString(GL_RENDERER);
  printf("OpenGL Renderer String: %s\n", sz2);
  const char* sz3 = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  printf("OpenGL Shading Language Version: %s\n", sz3);

  MyInit();
  g_curr_scene = g_dbuspcap_scene;

  #ifndef __EMSCRIPTEN__
  std::thread* pcap_replay_thread = nullptr;
  if (argc > 1) {
    pcap_replay_thread = new std::thread(std::bind(StartPCAPReplayThread, argv[1]));
  }
  while (!glfwWindowShouldClose(g_window)) {
    emscriptenLoop();
  }
  if (pcap_replay_thread) pcap_replay_thread->join();
  #else
  emscripten_set_main_loop(emscriptenLoop, 0, 1);
  #endif

  printf("Done!\n");
  exit(0);
}