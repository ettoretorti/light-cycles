// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <iostream>
#include <stdexcept>
#include <string>

#include <mathfu/glsl_mappings.h>
#include <mathfu/quaternion.h>

#include "gl/Buffer.hpp"
#include "gl/Program.hpp"
#include "gl/Shader.hpp"
#include "gl/VArray.hpp"

#include "input/KeyState.hpp"

#include "lcycle/Cycle.hpp"
#include "lcycle/RollbackWorld.hpp"
#include "lcycle/World.hpp"

#include "gfx/WorldRenderer.hpp"

#include "gui/GuiState.hpp"
#include "gui/nuklear.h"

#include "network/PeerConnection.hpp"
#include "capnp/packet.capnp.h"

constexpr char vshaderSrc[] = R"glsl(
    #version 330
    
    uniform mat4 model;
    uniform mat4 viewProjection;
    
    in vec4 pos;
    in vec4 color;
    
    out vec4 fColor;
    
    void main() {
      gl_Position = viewProjection * model * pos;
      fColor = color;
    }
)glsl";

constexpr char fshaderSrc[] = R"glsl(
    #version 330
    
    in vec4 fColor;
    out vec4 color;
    
    void main() {
      color = fColor;
    }
)glsl";

constexpr double kTimePerFrame = 1.0 / 60.0;

void glfw_error(int error, const char* msg) {
    std::cerr << "GLFW error with code: " << error << std::endl;
    std::cerr << msg << std::endl;
}

#ifndef NDEBUG
void APIENTRY gl_error(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg,
                       const void* uParam) {
    (void)id;
    (void)length;
    (void)uParam;
    auto srcToStr = [=]() -> const char* {
        switch (src) {
            case GL_DEBUG_SOURCE_API:
                return "SOURCE_API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "WINDOW_SYSTEM";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "SHADER_COMPILER";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "THIRD_PARTY";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "APPLICATION";
            case GL_DEBUG_SOURCE_OTHER:
                return "OTHER";
            default:
                return "UNKNOWN";
        }
    };
    auto typeToStr = [=]() -> const char* {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                return "ERROR";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "DEPRECATED_BEHAVIOR";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "UNDEFINED_BEHAVIOR";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "PORTABILITY";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "PERFORMANCE";
            case GL_DEBUG_TYPE_MARKER:
                return "MARKER";
            case GL_DEBUG_TYPE_PUSH_GROUP:
                return "PUSH_GROUP";
            case GL_DEBUG_TYPE_POP_GROUP:
                return "POP_GROUP";
            case GL_DEBUG_TYPE_OTHER:
                return "OTHER";
            default:
                return "UNKNOWN";
        }
    };
    auto sevToStr = [=]() -> const char* {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                return "HIGH";
            case GL_DEBUG_SEVERITY_MEDIUM:
                return "MEDIUM";
            case GL_DEBUG_SEVERITY_LOW:
                return "LOW";
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return "NOTIFICATION";
            default:
                return "UNKNOWN";
        }
    };
    std::cerr << "[GL_" << srcToStr() << "][" << typeToStr() << "][" << sevToStr() << "]: " << msg << std::endl;
}
#endif

struct WindowState {
    input::KeyState keys;
    gui::GuiState gui;
    gfx::WorldRenderer wr;
};

void glfwUpdateNkMouse(GLFWwindow* win, nk_context* ctx) {
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y,
                    glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y,
                    glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
}

nk_keys glfwKeyToNkKey(int key) {
    switch (key) {
        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
            return NK_KEY_SHIFT;
        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL:
            return NK_KEY_CTRL;
        case GLFW_KEY_DELETE:
            return NK_KEY_DEL;
        case GLFW_KEY_ENTER:
            return NK_KEY_ENTER;
        case GLFW_KEY_TAB:
            return NK_KEY_TAB;
        case GLFW_KEY_BACKSPACE:
            return NK_KEY_BACKSPACE;
        case GLFW_KEY_UP:
            return NK_KEY_UP;
        case GLFW_KEY_DOWN:
            return NK_KEY_DOWN;
        case GLFW_KEY_LEFT:
            return NK_KEY_LEFT;
        case GLFW_KEY_RIGHT:
            return NK_KEY_RIGHT;
        default:
            return NK_KEY_NONE;
    }
}

void glfwKeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_UNKNOWN) return;

    auto* winState = static_cast<WindowState*>(glfwGetWindowUserPointer(win));
    const bool pressed = (action == GLFW_PRESS);
    winState->keys.updateState(key, pressed);
    const nk_keys nkKey = glfwKeyToNkKey(key);
    if (winState->gui.active && nkKey != NK_KEY_NONE) {
        nk_input_key(&winState->gui.ctx, nkKey, pressed);
    }
}

/*!
 * Preconditions
 * - None
 *
 * Postconditions
 * - glew has been initialized
 * - a valid opengl context has been created and made current
 * - a valid window is returned
 */
GLFWwindow* init(int width, int height) {
    glfwSetErrorCallback(glfw_error);

    if (!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 8);  // 8xMSAA

    GLFWwindow* win = glfwCreateWindow(width, height, "Light cycles", nullptr, nullptr);
    if (!win) return nullptr;

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);  // vsync

    // make sure we get what we want
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(win);
        return nullptr;
    }
    glEnable(GL_MULTISAMPLE);

#ifndef NDEBUG
    if (GLAD_GL_KHR_debug) {
        glDebugMessageCallback(gl_error, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        std::cout << "Registered gl error callback" << std::endl;
    }
#endif

    return win;
}

void shutdown() { glfwTerminate(); }

std::function<lcycle::CycleInput()> mkInputFunc(GLFWwindow* win, int lKey, int rKey) {
    const auto* ws = static_cast<WindowState*>(glfwGetWindowUserPointer(win));
    return [ws, lKey, rKey]() -> lcycle::CycleInput {
        bool l = ws->keys.isPressed(lKey);
        bool r = ws->keys.isPressed(rKey);

        return {(float)(-l + r)};
    };
}

std::function<lcycle::CycleInput()> jsInputFunc(int joystick, int axis) {
    return [joystick, axis]() -> lcycle::CycleInput {
        int count;
        float val = glfwGetJoystickAxes(joystick, &count)[axis];
        if (val < 0.15 && val > -0.15) {
            val = 0;
        }
        return {val};
    };
}

mathfu::mat4 projection(int winWidth, int winHeight, double worldSize) {
    double side = worldSize * 3.0 / 5.0;

    if (winWidth >= winHeight) {
        double ratio = winWidth / (double)winHeight;
        return mathfu::mat4::Ortho(-side * ratio, side * ratio, -side, side, 0.1, 2.0);
    } else {
        double ratio = winHeight / (double)winWidth;
        return mathfu::mat4::Ortho(-side, side, -side * ratio, side * ratio, 0.1, 2.0);
    }
}

struct GameState {
    lcycle::World initial;
    lcycle::RollbackWorld rbw;
    std::vector<std::function<lcycle::CycleInput()>> inputs;
    std::vector<lcycle::World::PlayerInputs> replay;
    bool running;
};

struct ReplayState {
    lcycle::World world;
    std::vector<lcycle::World::PlayerInputs> replayInputs;
    float replaySpeed = 1.0;
    int replayFrame = 0;
    bool running = false;
};

void replay(GLFWwindow* win, gl::Program& p, ReplayState s) {
    using namespace mathfu;

    auto* ws = static_cast<WindowState*>(glfwGetWindowUserPointer(win));

    int w, h;
    glfwGetFramebufferSize(win, &w, &h);

    double timeSinceLastFrame = 0.0;
    double time = glfwGetTime();
    ws->gui.active = true;
    s.running = false;
    s.replayFrame = 0;
    s.replaySpeed = 1.0;

    p.use();
    mat4 mdl = mathfu::mat4::Identity();
    glUniformMatrix4fv(p.getUniform("model"), 1, false, &mdl[0]);

    mat4 view = mathfu::mat4::LookAt(mathfu::vec3(0.0), mathfu::vec3(0.0, 0.0, 1.0), mathfu::vec3(0.0, 1.0, 0.0), 1.0);
    bool exit = false;
    while (!exit && !glfwWindowShouldClose(win)) {
        ws->keys.step();
        if (ws->gui.active) {
            ws->gui.startInput();
        }
        glfwPollEvents();
        if (ws->gui.active) {
            glfwUpdateNkMouse(win, &ws->gui.ctx);
            ws->gui.endInput();
        }

        double curTime = glfwGetTime();
        if (s.running) {
            timeSinceLastFrame += s.replaySpeed * (curTime - time);
        }
        time = curTime;

        glfwGetFramebufferSize(win, &w, &h);

        if (ws->keys.isPressed(GLFW_KEY_ESCAPE)) {
            exit = true;
        }
        if (ws->keys.isPosEdge(GLFW_KEY_P)) {
            s.running = !s.running;
        }

        // Update game
        if (timeSinceLastFrame >= 0.5 * kTimePerFrame) {
            while (timeSinceLastFrame >= 0.5 * kTimePerFrame && s.replayFrame < s.replayInputs.size()) {
                timeSinceLastFrame -= kTimePerFrame;
                s.world.runFor(kTimePerFrame, s.replayInputs[s.replayFrame]);
                s.replayFrame++;
            }
        }

        // Update UI
        if (ws->gui.active) {
            auto* ctx = &ws->gui.ctx;
        }

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render game
        p.use();
        mat4 proj = projection(w, h, s.world.size());
        glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);

        ws->wr.render(s.world);

        // Render UI
        if (ws->gui.active) {
            ws->gui.render(win);
        }
        ws->gui.active = !s.running;
        glfwSwapBuffers(win);
    }
}

// Returns true if the players want a rematch
bool mainloop(GLFWwindow* win, gl::Program& p, size_t nPlayers) {
    using namespace lcycle;
    using namespace gfx;
    using namespace mathfu;

    const vec4 P_COLORS[] = {
        vec4(1.0, 0.0, 0.0, 1.0),  // red
        vec4(0.0, 0.0, 1.0, 1.0),  // blue
        vec4(1.0, 1.0, 0.0, 1.0),  // yellow
        vec4(0.0, 1.0, 0.0, 1.0),  // green
    };

    const vec4 T_COLORS[] = {
        vec4(1.0, 0.4, 0.4, 1.0),   // red
        vec4(0.0, 0.75, 1.0, 1.0),  // blue
        vec4(1.0, 1.0, 0.5, 1.0),   // yellow
        vec4(0.6, 1.0, 0.5, 1.0),   // green
    };

    const std::function<CycleInput()> INPUTS[] = {
        mkInputFunc(win, GLFW_KEY_LEFT, GLFW_KEY_RIGHT),
        mkInputFunc(win, GLFW_KEY_A, GLFW_KEY_D),
        mkInputFunc(win, GLFW_KEY_J, GLFW_KEY_L),
        mkInputFunc(win, GLFW_KEY_F, GLFW_KEY_H),
    };

    const std::string NAMES[] = {
        "RED",
        "BLUE",
        "YELLOW",
        "GREEN",
    };

    const size_t MAX_PLAYERS = sizeof(P_COLORS) / sizeof(P_COLORS[0]);
    if (nPlayers > MAX_PLAYERS) {
        throw std::range_error("Maximum " + std::to_string(MAX_PLAYERS) + " players allowed");
    }

    auto* windowState = static_cast<WindowState*>(glfwGetWindowUserPointer(win));
    GameState gs;
    for (auto i = 0u; i < nPlayers; i++) {
        gs.inputs.push_back(INPUTS[i]);
    }

    std::vector<std::pair<int, CycleInput>> playerInputs;
    playerInputs.reserve(nPlayers);

    const double WORLD_SIZE = 50.0;
    {
        std::vector<Player> players;
        players.reserve(nPlayers);

        const double anglePerPlayer = 2 * 3.14159 / nPlayers;
        double curAngle = 0.0;
        for (size_t i = 0; i < nPlayers; i++) {
            Cycle c({(float)cos(curAngle), (float)sin(curAngle)}, curAngle);
            players.push_back({c, (int)i, NAMES[i], P_COLORS[i], T_COLORS[i]});
            curAngle += anglePerPlayer;
            playerInputs.push_back(std::make_pair<int, CycleInput>(i, {}));
        }

        gs.initial = World(WORLD_SIZE, 0.14, players);
        gs.rbw = RollbackWorld(gs.initial);
    }

    int w, h;
    glfwGetFramebufferSize(win, &w, &h);

    p.use();
    mat4 mdl = mathfu::mat4::Identity();
    glUniformMatrix4fv(p.getUniform("model"), 1, false, &mdl[0]);

    mat4 view = mathfu::mat4::LookAt(mathfu::vec3(0.0), mathfu::vec3(0.0, 0.0, 1.0), mathfu::vec3(0.0, 1.0, 0.0), 1.0);

    windowState->gui.active = true;

    gs.running = false;
    double timeSinceLastFrame = 0.0;
    double time = glfwGetTime();
    bool first_frame = true;
    while (!glfwWindowShouldClose(win)) {
        windowState->keys.step();
        if (windowState->gui.active) {
            windowState->gui.startInput();
        }
        glfwPollEvents();
        if (windowState->gui.active) {
            glfwUpdateNkMouse(win, &windowState->gui.ctx);
            windowState->gui.endInput();
        }

        double curTime = glfwGetTime();
        if (gs.running) {
            timeSinceLastFrame += curTime - time;
        }
        time = curTime;

        glfwGetFramebufferSize(win, &w, &h);

        if (windowState->keys.isPressed(GLFW_KEY_ESCAPE)) {
            break;
        }
        if (gs.rbw.latest()->players().size() == 1) {
            std::cout << "The winner is: " << gs.rbw.latest()->players()[0].name << std::endl;
            gs.running = false;
            break;
        } else if (gs.rbw.latest()->players().size() == 0) {
            std::cout << "Draw" << std::endl;
            gs.running = false;
            break;
        }

        if (windowState->keys.isPosEdge(GLFW_KEY_P)) {
            gs.running = !gs.running;
        }

        if (windowState->keys.isPosEdge(GLFW_KEY_V)) {
            gs.rbw.rollback(30);
            std::cout << "r-r-rollback" << std::endl;
        }

        // Update game
        if (timeSinceLastFrame >= 0.5 * kTimePerFrame) {
            for (auto i = 0u; i < nPlayers; i++) {
                playerInputs[i].second = gs.inputs[i]();
            }
            while (timeSinceLastFrame >= 0.5 * kTimePerFrame) {
                timeSinceLastFrame -= kTimePerFrame;
                gs.rbw.advance(playerInputs);
                gs.replay.push_back(playerInputs);
                first_frame = false;
            }
        }

        // Update UI
        if (windowState->gui.active) {
            auto* ctx = &windowState->gui.ctx;

            auto layout = [&](auto f) {
                nk_layout_space_begin(ctx, NK_DYNAMIC, 0, 1);
                // std::cout << nk_layout_space_bounds(ctx).h << std::endl;
                nk_layout_space_push(ctx, nk_rect(0.25, 0.1, 0.5, 0.8));
                f();
                nk_layout_space_end(ctx);
            };
            if (nk_begin(ctx, "main window", {0, 0, (float)w, (float)h}, 0)) {
                // h = 2*vertical pad + 3*min_row_height
                // (h - 3*min_row_height)/2 = vertical_pad
                auto bounds = ctx->current->layout->bounds;
                int vertical_pad =
                    (bounds.h - (2 * (ctx->current->layout->row.min_height + ctx->style.window.spacing.y))) / 2;
                nk_layout_space_begin(ctx, NK_DYNAMIC, vertical_pad, 1);
                nk_layout_space_end(ctx);

                layout([&]() {
                    if (nk_button_label(ctx, first_frame ? "Start" : "Resume")) {
                        gs.running = !gs.running;
                    }
                });
                layout([&]() {
                    if (nk_button_label(ctx, "Exit")) {
                        glfwSetWindowShouldClose(win, true);
                    }
                });
            }
            nk_end(ctx);
        }

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render game
        p.use();
        mat4 proj = projection(w, h, WORLD_SIZE);
        glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);

        windowState->wr.render(*gs.rbw.latest());

        // Render UI
        if (windowState->gui.active) {
            windowState->gui.render(win);
        }
        windowState->gui.active = !gs.running;

        glfwSwapBuffers(win);
    }

    bool rematch = false;
    while (!glfwWindowShouldClose(win) && !rematch) {
        windowState->gui.active = true;
        windowState->keys.step();
        windowState->gui.startInput();
        glfwPollEvents();
        glfwUpdateNkMouse(win, &windowState->gui.ctx);
        windowState->gui.endInput();

        if (windowState->keys.isPosEdge(GLFW_KEY_ESCAPE)) {
            break;
        }
        bool watch_replay = windowState->keys.isPosEdge(GLFW_KEY_Q);
        rematch = windowState->keys.isPressed(GLFW_KEY_R);

        // Update UI
        {
            auto* ctx = &windowState->gui.ctx;

            auto layout = [&](auto f) {
                nk_layout_space_begin(ctx, NK_DYNAMIC, 0, 1);
                nk_layout_space_push(ctx, nk_rect(0.25, 0.1, 0.5, 0.8));
                f();
                nk_layout_space_end(ctx);
            };
            if (nk_begin(ctx, "main window", {0, 0, (float)w, (float)h}, 0)) {
                // h = 2*vertical pad + 3*min_row_height
                // (h - 3*min_row_height)/2 = vertical_pad
                auto bounds = ctx->current->layout->bounds;
                int vertical_pad =
                    (bounds.h - (3 * (ctx->current->layout->row.min_height + ctx->style.window.spacing.y))) / 2;
                nk_layout_space_begin(ctx, NK_DYNAMIC, vertical_pad, 1);
                nk_layout_space_end(ctx);

                layout([&]() {
                    if (nk_button_label(ctx, "Rematch")) {
                        rematch = true;
                    }
                });
                layout([&]() {
                    if (nk_button_label(ctx, "Watch Replay")) {
                        watch_replay = true;
                    }
                });
                layout([&]() {
                    if (nk_button_label(ctx, "Exit")) {
                        glfwSetWindowShouldClose(win, true);
                    }
                });
            }
            nk_end(ctx);
        }

        // Render
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render game
        p.use();
        mat4 proj = projection(w, h, WORLD_SIZE);
        glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);

        windowState->wr.render(*gs.rbw.latest());

        // Render UI
        windowState->gui.render(win);

        glfwSwapBuffers(win);

        if (watch_replay) {
            ReplayState rs;
            rs.replayInputs = gs.replay;
            rs.world = gs.initial;
            replay(win, p, rs);
        }
    }

    return rematch;
}

int main(int argc, char** argv) {
    using namespace std;
    using namespace gl;

    int width = 600, height = 600;

    if (argc >= 3) {
        try {
            int w = stoi(argv[1]);
            int h = stoi(argv[2]);

            width = w;
            height = h;
        } catch (...) {
            cerr << "Could not parse dimensions from first 2 command line args" << endl;
        }
    }

    GLFWwindow* win = init(width, height);
    if (!win) {
        cerr << "Could not create window" << endl;
        return -1;
    }

    // OpenGL objects must not outlive window
    {
        Shader vs(GL_VERTEX_SHADER);
        vs.source(vshaderSrc);
        if (!vs.compile()) {
            cerr << "Could not compile vertex shader\n\n" << vs.infoLog() << endl;
            return -1;
        }

        Shader fs(GL_FRAGMENT_SHADER);
        fs.source(fshaderSrc);
        if (!fs.compile()) {
            cerr << "Could not compile fragment shader\n\n" << fs.infoLog() << endl;
            return -1;
        }

        Program p;
        p.setVertexShader(vs);
        p.setFragmentShader(fs);
        p.bindAttrib(0, "pos");
        p.bindAttrib(1, "color");
        if (!p.link()) {
            cerr << "Could not link program\n\n" << p.infoLog() << endl;
            return -1;
        }
        p.use();

        WindowState ws;
        glfwSetWindowUserPointer(win, &ws);
        glfwSetKeyCallback(win, glfwKeyCallback);

        try {
            while (mainloop(win, p, 2))
                ;
        } catch (exception& ex) {
            cerr << ex.what() << endl;
        }
    }

    glfwDestroyWindow(win);

    shutdown();

    return 0;
}
