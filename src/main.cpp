#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cmath>

struct Camera {
    glm::vec3 position{0.0f, 1.0f, 5.0f};
    float pitch = 0.0f;
    float yaw = -90.0f;

    glm::mat4 getViewMatrix() const {
        glm::vec3 front{
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        };
        return glm::lookAt(position, position + glm::normalize(front), {0.0f, 1.0f, 0.0f});
    }
};

bool initSDL(SDL_Window** window, SDL_GLContext* context, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    *window = SDL_CreateWindow("FPS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!*window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    *context = SDL_GL_CreateContext(*window);
    if (!*context) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    return true;
}

void processInput(Camera& cam, float deltaTime, const Uint8* keystate, int dx, int dy) {
    const float sensitivity = 0.1f;
    const float speed = 5.0f;

    cam.yaw += dx * sensitivity;
    cam.pitch -= dy * sensitivity;
    if (cam.pitch > 89.0f) cam.pitch = 89.0f;
    if (cam.pitch < -89.0f) cam.pitch = -89.0f;

    glm::vec3 front{
        cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch)),
        sin(glm::radians(cam.pitch)),
        sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch))
    };
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3{0.0f, 1.0f, 0.0f}));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    if (keystate[SDL_SCANCODE_W]) cam.position += front * speed * deltaTime;
    if (keystate[SDL_SCANCODE_S]) cam.position -= front * speed * deltaTime;
    if (keystate[SDL_SCANCODE_A]) cam.position -= right * speed * deltaTime;
    if (keystate[SDL_SCANCODE_D]) cam.position += right * speed * deltaTime;
    if (keystate[SDL_SCANCODE_SPACE]) cam.position += up * speed * deltaTime;
    if (keystate[SDL_SCANCODE_LCTRL]) cam.position -= up * speed * deltaTime;
}

int main(int argc, char** argv) {
    const int width = 800, height = 600;
    SDL_Window* window = nullptr;
    SDL_GLContext context;
    if (!initSDL(&window, &context, width, height)) return -1;

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        return -1;
    }

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    GLuint vao; glGenVertexArrays(1, &vao); glBindVertexArray(vao);

    bool running = true;
    Camera cam;
    Uint32 lastTicks = SDL_GetTicks();

    while (running) {
        SDL_Event e; int dx = 0, dy = 0;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_MOUSEMOTION) { dx += e.motion.xrel; dy += e.motion.yrel; }
        }
        Uint32 currentTicks = SDL_GetTicks();
        float deltaTime = (currentTicks - lastTicks) / 1000.0f;
        lastTicks = currentTicks;

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_ESCAPE]) running = false;

        processInput(cam, deltaTime, keystate, dx, dy);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // for now no geometry, just clear and present
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
