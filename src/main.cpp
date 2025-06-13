#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>
#include <random>
#include "stb_image.h"

struct Camera {
    glm::vec3 position{0.0f, 1.0f, 0.0f};
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

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return shader;
}

GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Program link error: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

GLuint loadTexture(const std::string& path) {
    int w, h, channels;
    stbi_uc* data = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed to load " << path << std::endl;
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(data);
    return tex;
}

std::filesystem::path findImagesDir(const char* exePath) {
    namespace fs = std::filesystem;
    fs::path dir{"images"};
    if (fs::exists(dir)) return dir;

    fs::path exeDir = fs::absolute(exePath).parent_path();
    dir = exeDir / ".." / "images";
    if (fs::exists(dir)) return fs::canonical(dir);

    dir = exeDir / "images";
    if (fs::exists(dir)) return fs::canonical(dir);

    return "images"; // fallback
}

std::vector<GLuint> loadNoTextureVariants(const std::filesystem::path& dir) {
    namespace fs = std::filesystem;
    std::vector<GLuint> textures;
    if (!fs::exists(dir)) {
        std::cerr << "Images directory not found: " << dir << std::endl;
        return textures;
    }
    for (auto& p : fs::directory_iterator(dir)) {
        std::string fname = p.path().filename().string();
        if (fname.rfind("no_texture", 0) == 0 && p.path().extension() == ".png") {
            GLuint tex = loadTexture(p.path().string());
            if (tex) textures.push_back(tex);
        }
    }
    return textures;
}

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

void processInput(Camera& cam, float deltaTime, float& velY, bool& onGround,
                  const Uint8* keystate, int dx, int dy) {
    const float sensitivity = 0.1f;
    const float speed = 5.0f;
    const float gravity = 9.8f;
    const float jumpSpeed = 5.0f;

    cam.yaw += dx * sensitivity;
    cam.pitch -= dy * sensitivity;
    if (cam.pitch > 89.0f) cam.pitch = 89.0f;
    if (cam.pitch < -89.0f) cam.pitch = -89.0f;

    glm::vec3 front{
        cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch)),
        0.0f,
        sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch))
    };
    front = glm::normalize(front);
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3{0.0f, 1.0f, 0.0f}));

    glm::vec3 move(0.0f);
    if (keystate[SDL_SCANCODE_W]) move += front;
    if (keystate[SDL_SCANCODE_S]) move -= front;
    if (keystate[SDL_SCANCODE_A]) move -= right;
    if (keystate[SDL_SCANCODE_D]) move += right;
    if (glm::length(move) > 0.0f) cam.position += glm::normalize(move) * speed * deltaTime;

    if (keystate[SDL_SCANCODE_SPACE] && onGround) {
        velY = jumpSpeed;
        onGround = false;
    }

    velY -= gravity * deltaTime;
    cam.position.y += velY * deltaTime;
    if (cam.position.y < 1.0f) {
        cam.position.y = 1.0f;
        velY = 0.0f;
        onGround = true;
    }
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

    const char* vsSrc =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aColor;\n"
        "layout(location = 2) in vec2 aTex;\n"
        "out vec3 vColor;\n"
        "out vec2 vTex;\n"
        "uniform mat4 uMVP;\n"
        "void main() {\n"
        "    vColor = aColor;\n"
        "    vTex = aTex;\n"
        "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}";

    const char* fsSrc =
        "#version 330 core\n"
        "in vec3 vColor;\n"
        "in vec2 vTex;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTex;\n"
        "void main() {\n"
        "    FragColor = texture(uTex, vTex) * vec4(vColor, 1.0);\n"
        "}";

    GLuint program = createProgram(vsSrc, fsSrc);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uTex"), 0);

    std::filesystem::path imageDir = findImagesDir(argv[0]);
    std::vector<GLuint> noTextures = loadNoTextureVariants(imageDir);
    if (noTextures.empty()) {
        std::cerr << "No placeholder textures found" << std::endl;
        return -1;
    }

    std::mt19937 rng(SDL_GetTicks());
    std::uniform_int_distribution<size_t> dist(0, noTextures.size() - 1);
    GLuint faceTex[6];
    for (int i = 0; i < 6; ++i) faceTex[i] = noTextures[dist(rng)];

    float vertices[] = {
        // pos                 // color          // tex
        -10.f,0.f,-10.f, 0.7f,0.7f,0.7f, 0.f,0.f,
         10.f,0.f,-10.f, 0.7f,0.7f,0.7f, 1.f,0.f,
         10.f,5.f,-10.f, 0.7f,0.7f,0.7f, 1.f,1.f,
        -10.f,5.f,-10.f, 0.7f,0.7f,0.7f, 0.f,1.f,

        -10.f,0.f,10.f, 0.7f,0.7f,0.7f, 0.f,0.f,
         10.f,0.f,10.f, 0.7f,0.7f,0.7f, 1.f,0.f,
         10.f,5.f,10.f, 0.7f,0.7f,0.7f, 1.f,1.f,
        -10.f,5.f,10.f, 0.7f,0.7f,0.7f, 0.f,1.f,

        -10.f,0.f,-10.f, 0.7f,0.7f,0.7f, 0.f,0.f,
        -10.f,0.f,10.f, 0.7f,0.7f,0.7f, 1.f,0.f,
        -10.f,5.f,10.f, 0.7f,0.7f,0.7f, 1.f,1.f,
        -10.f,5.f,-10.f,0.7f,0.7f,0.7f, 0.f,1.f,

         10.f,0.f,-10.f,0.7f,0.7f,0.7f, 0.f,0.f,
         10.f,0.f,10.f, 0.7f,0.7f,0.7f, 1.f,0.f,
         10.f,5.f,10.f, 0.7f,0.7f,0.7f, 1.f,1.f,
         10.f,5.f,-10.f,0.7f,0.7f,0.7f, 0.f,1.f,

        -10.f,5.f,-10.f,0.7f,0.7f,0.7f, 0.f,0.f,
         10.f,5.f,-10.f,0.7f,0.7f,0.7f, 1.f,0.f,
         10.f,5.f,10.f, 0.7f,0.7f,0.7f, 1.f,1.f,
        -10.f,5.f,10.f, 0.7f,0.7f,0.7f, 0.f,1.f,

        -10.f,0.f,-10.f,0.7f,0.7f,0.7f, 0.f,0.f,
         10.f,0.f,-10.f,0.7f,0.7f,0.7f, 1.f,0.f,
         10.f,0.f,10.f, 0.7f,0.7f,0.7f, 1.f,1.f,
        -10.f,0.f,10.f, 0.7f,0.7f,0.7f, 0.f,1.f
    };

    unsigned int indices[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        8,9,10, 10,11,8,
        12,13,14, 14,15,12,
        16,17,18, 18,19,16,
        20,21,22, 22,23,20
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);


    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width / float(height), 0.1f, 100.0f);

    bool running = true;
    Camera cam;
    float velY = 0.0f;
    bool onGround = true;
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

        processInput(cam, deltaTime, velY, onGround, keystate, dx, dy);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = cam.getViewMatrix();
        glm::mat4 mvp = projection * view;
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(VAO);
        for (int i = 0; i < 6; ++i) {
            glBindTexture(GL_TEXTURE_2D, faceTex[i]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));
        }
        glBindVertexArray(0);
        SDL_GL_SwapWindow(window);
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
