#include <GL/glew.h>
#include <GL/gl.h>
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

#include "lcycle/World.hpp"
#include "lcycle/Cycle.hpp"

#include "gfx/WorldRenderer.hpp"

const char* vshaderSrc =
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

const char* fshaderSrc =
    "#version 330\n"
    ""
    "in vec4 fColor;\n"
    "out vec4 color;\n"
    ""
    "void main() {\n"
    "  color = fColor;\n"
    "}\n";

static inline double toRad(double degrees) { return degrees * 3.1415926 / 180.0; }

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
    if(!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 8);  // 8xMSAA

    GLFWwindow* win = glfwCreateWindow(width, height, "Light cycles", nullptr, nullptr);

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);  // vsync
    glEnable(GL_MULTISAMPLE);

    // make sure we get what we want
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if(glewErr != GLEW_OK) {
        glfwDestroyWindow(win);
        return nullptr;
    }

    return win;
}

void shutdown() {
    glfwTerminate();
}

std::function<lcycle::CycleInput()> mkInputFunc(GLFWwindow* win, int lKey, int rKey) {
    return [win, lKey, rKey]() -> lcycle::CycleInput {
        bool l = glfwGetKey(win, lKey);
        bool r = glfwGetKey(win, rKey);

        if(l && r) return {0.0};
        else if(l) return {-1.0};
        else if(r) return {1.0};
        else       return {0.0};
    };
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

    std::vector<Player> players;
    players.reserve(nPlayers);

    const double anglePerPlayer = 2*3.14159 / nPlayers;
    double curAngle = 0.0;
    for(size_t i = 0; i < nPlayers; i++) {
        Cycle c({(float)cos(curAngle), (float)sin(curAngle)}, curAngle);
        players.push_back({c, INPUTS[i], NAMES[i], P_COLORS[i], T_COLORS[i]});
        curAngle += anglePerPlayer;
    }

    World wo(50.0, 0.2, std::move(players));
    WorldRenderer wr(wo);

    mathfu::mat4 mdl  = mathfu::mat4::Identity();
    mathfu::mat4 view = mathfu::mat4::LookAt(mathfu::vec3(0.0), mathfu::vec3(0.0, 0.0, 1.0),
                                             mathfu::vec3(0.0, 1.0, 0.0), 1.0);
    mathfu::mat4 proj = mathfu::mat4::Ortho(-30, 30, -30, 30, 0.1, 2.0);

    glUniformMatrix4fv(p.getUniform("model"), 1, false, &mdl[0]);
    glUniformMatrix4fv(p.getUniform("viewProjection"), 1, false, &(proj * view)[0]);

    int w, h;
    glfwGetFramebufferSize(win, &w, &h);
    glViewport(0, 0, w, h);

    bool running  = false;
    bool pPressed = false;
    double time = glfwGetTime();
    while(!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        if(glfwGetKey(win, GLFW_KEY_ESCAPE)) { break; }
        if(wo.players().size() == 1) {
            std::cout << "The winner is: " << wo.players()[0].name << std::endl;
            running = false;
            break;
        } else if(wo.players().size() == 0) {
            std::cout << "Draw" << std::endl;
            running = false;
            break;
        }

        bool pNow = glfwGetKey(win, GLFW_KEY_P);
        if(!pPressed && pNow) running = !running;
        pPressed = pNow;

        int nW, nH;
        glfwGetFramebufferSize(win, &nW, &nH);
        if(nW != w || nH != h) glViewport(0, 0, w, h);
        w = nW; h = nH;

        double curTime = glfwGetTime();
        if(running) {
            wo.runFor(curTime - time);
        } else {
            glfwWaitEvents();
            curTime = glfwGetTime();
        }
        time = curTime;

        glClear(GL_COLOR_BUFFER_BIT);
        wr.render();
        glfwSwapBuffers(win);
    }

    bool rematch = false;
    while(!running && !glfwWindowShouldClose(win) && !glfwGetKey(win, GLFW_KEY_ESCAPE) && !(rematch = glfwGetKey(win, GLFW_KEY_R))) {
        glfwWaitEvents();
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

    try {
        while(mainloop(win, p, 2));
    } catch(exception& ex) {
        cerr << ex.what() << endl;
    }

    glfwDestroyWindow(win);

    shutdown();

    return 0;
}
