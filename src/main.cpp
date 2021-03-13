#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <stdexcept>

#include <mathfu/glsl_mappings.h>
#include <mathfu/quaternion.h>

#include "gl/Buffer.hpp"
#include "gl/VArray.hpp"
#include "gl/Shader.hpp"
#include "gl/Program.hpp"

#include "input/KeyState.hpp"

#include "lcycle/World.hpp"
#include "lcycle/Cycle.hpp"
#include "lcycle/RollbackWorld.hpp"

#include "gfx/WorldRenderer.hpp"

constexpr char vshaderSrc[] =
    "#version 330\n"
    ""
    "uniform mat4 model;\n"
    "uniform mat4 viewProjection;\n"
    ""
    "in vec4 pos;\n"
    "in vec4 color;\n"
    ""
    "out vec4 fColor;\n"
    ""
    "void main() {\n"
    "  gl_Position = viewProjection * model * pos;\n"
    "  fColor = color;\n"
    "}\n";

constexpr char fshaderSrc[] =
    "#version 330\n"
    ""
    "in vec4 fColor;\n"
    "out vec4 color;\n"
    ""
    "void main() {\n"
    "  color = fColor;\n"
    "}\n";


void glfw_error(int error, const char* msg) {
    std::cerr << "GLFW error with code: " << error << std::endl;
    std::cerr << msg << std::endl;
}

#ifndef NDEBUG
void APIENTRY gl_error(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* uParam) {
    (void) src; (void) type; (void) id; (void) severity; (void) length; (void) uParam;
    std::cerr << msg << std::endl;
}
#endif

struct WindowState {
        input::KeyState keys;
};

void glfwKeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    (void) scancode; (void) mods;
    if (key == GLFW_KEY_UNKNOWN) return;

    auto* winState = static_cast<WindowState*>(glfwGetWindowUserPointer(win));
    if (action == GLFW_PRESS) {
            winState->keys.updateState(key, true);
    } else if (action == GLFW_RELEASE) {
            winState->keys.updateState(key, false);
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

    if(!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 8);  // 8xMSAA

    GLFWwindow* win = glfwCreateWindow(width, height, "Light cycles", nullptr, nullptr);
    if (!win) return nullptr;

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);  // vsync

    // make sure we get what we want
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
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

void shutdown() {
    glfwTerminate();
}

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

    if(winWidth >= winHeight) {
        double ratio = winWidth / (double) winHeight;
        return mathfu::mat4::Ortho(-side * ratio, side * ratio, -side, side, 0.1, 2.0);
    } else {
        double ratio = winHeight / (double) winWidth;
        return mathfu::mat4::Ortho(-side, side, -side * ratio, side * ratio, 0.1, 2.0);
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
        vec4(1.0, 0.4, 0.4, 1.0),  // red
        vec4(0.0, 0.75, 1.0, 1.0), // blue
        vec4(1.0, 1.0, 0.5, 1.0),  // yellow
        vec4(0.6, 1.0, 0.5, 1.0),  // green
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
    if(nPlayers > MAX_PLAYERS) {
        throw std::range_error("Maximum " + std::to_string(MAX_PLAYERS) + " players allowed");
    }

    auto* windowState = static_cast<WindowState*>(glfwGetWindowUserPointer(win));

    std::vector<Player> players;
    players.reserve(nPlayers);
    std::vector<std::pair<int, CycleInput>> playerInputs;
    playerInputs.reserve(nPlayers);

    const double anglePerPlayer = 2*3.14159 / nPlayers;
    double curAngle = 0.0;
    for(size_t i = 0; i < nPlayers; i++) {
        Cycle c({(float)cos(curAngle), (float)sin(curAngle)}, curAngle);
        players.push_back({c, (int)i, NAMES[i], P_COLORS[i], T_COLORS[i]});
        curAngle += anglePerPlayer;
        playerInputs.push_back(std::make_pair<int, CycleInput>(i, {}));
    }

    const double WORLD_SIZE = 50.0;
    WorldRenderer wr;
    RollbackWorld rbw(World(WORLD_SIZE, 0.14, players));

    int w, h;
    glfwGetFramebufferSize(win, &w, &h);
    glViewport(0, 0, w, h);

    mat4 mdl  = mathfu::mat4::Identity();
    glUniformMatrix4fv(p.getUniform("model"), 1, false, &mdl[0]);

    mat4 view = mathfu::mat4::LookAt(mathfu::vec3(0.0), mathfu::vec3(0.0, 0.0, 1.0),
                                     mathfu::vec3(0.0, 1.0, 0.0), 1.0);
    mat4 proj = projection(w, h, WORLD_SIZE);

    glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);


    bool running  = false;
    constexpr double timePerFrame = 1.0 / 60.0;
    double timeSinceLastFrame = 0.0;
    double time = glfwGetTime();
    while(!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        double curTime = glfwGetTime();

        if(windowState->keys.isPressed(GLFW_KEY_ESCAPE)) { break; }
        if(rbw.latest()->players().size() == 1) {
            std::cout << "The winner is: " << rbw.latest()->players()[0].name << std::endl;
            running = false;
            break;
        } else if(rbw.latest()->players().size() == 0) {
            std::cout << "Draw" << std::endl;
            running = false;
            break;
        }

        if(windowState->keys.isPosEdge(GLFW_KEY_P)) {
            running = !running;
        }

        if(windowState->keys.isPosEdge(GLFW_KEY_V)) {
            rbw.rollback(30);
            std::cout << "r-r-rollback" << std::endl;
        }


        if(running) {
            timeSinceLastFrame += curTime - time;
            if(timeSinceLastFrame >= 0.5 * timePerFrame) {
                for(auto i = 0u; i < nPlayers; i++) {
                    playerInputs[i].second = INPUTS[i]();
                }
                while(timeSinceLastFrame >= 0.5 * timePerFrame) {
                    timeSinceLastFrame -= timePerFrame;
                    rbw.advance(playerInputs);
                }
            }
            windowState->keys.step();
        } else {
            windowState->keys.step();
            glfwWaitEvents();
            curTime = glfwGetTime();
        }
        time = curTime;

        int nW, nH;
        glfwGetFramebufferSize(win, &nW, &nH);
        if(nW != w || nH != h) {
            glViewport(0, 0, nW, nH);
            mat4 proj = projection(nW, nH, WORLD_SIZE);
            glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);

            w = nW;
            h = nH;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        wr.render(*rbw.latest());
        glfwSwapBuffers(win);
    }

    bool rematch = false;
    while(!glfwWindowShouldClose(win) && !windowState->keys.isPressed(GLFW_KEY_ESCAPE) && !(rematch = windowState->keys.isPressed(GLFW_KEY_R))) {
        glfwWaitEvents();

        int nW, nH;
        glfwGetFramebufferSize(win, &nW, &nH);
        if(nW != w || nH != h) {
            glViewport(0, 0, nW, nH);
            mat4 proj = projection(nW, nH, WORLD_SIZE);
            glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);

            w = nW;
            h = nH;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        wr.render(*rbw.latest());
        glfwSwapBuffers(win);
    }

    return rematch;
}

int main(int argc, char** argv) {
    using namespace std;
    using namespace gl;

    int width = 600, height = 600;

    if(argc >= 3) {
        try {
            int w = stoi(argv[1]);
            int h = stoi(argv[2]);

            width  = w;
            height = h;
        } catch(...) {
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
        if(!vs.compile()) {
            cerr << "Could not compile vertex shader\n\n" << vs.infoLog() << endl;
            return -1;
        }

        Shader fs(GL_FRAGMENT_SHADER);
        fs.source(fshaderSrc);
        if(!fs.compile()) {
            cerr << "Could not compile fragment shader\n\n" << fs.infoLog() << endl;
            return -1;
        }

        Program p;
        p.setVertexShader(vs);
        p.setFragmentShader(fs);
        p.bindAttrib(0, "pos");
        p.bindAttrib(1, "color");
        if(!p.link()) {
            cerr << "Could not link program\n\n" << p.infoLog() << endl;
            return -1;
        }
        p.use();

        WindowState ws;
        glfwSetWindowUserPointer(win, &ws);
        glfwSetKeyCallback(win, glfwKeyCallback);

        try {
            while(mainloop(win, p, 2));
        } catch(exception& ex) {
            cerr << ex.what() << endl;
        }
    }

    glfwDestroyWindow(win);

    shutdown();

    return 0;
}
