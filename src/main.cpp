#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

#include "cloth.h"
#include "renderer.h"
#include "interaction.h"
#include "camera.h"

#include <cstdio>
#include <cmath>
#include <string>

static void grid_from_aspect(float aspect, int& out_cols, int& out_rows) {
    const int TARGET = 1600;
    out_cols = std::max(4, (int)std::round(std::sqrt((float)TARGET * aspect)));
    out_rows = std::max(4, (int)std::round(std::sqrt((float)TARGET / aspect)));
}

int main(int argc, char* argv[]) {
    std::string texture_path;
    if (argc >= 2) texture_path = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

    int win_w = 1280, win_h = 720;
    SDL_Window* window = SDL_CreateWindow("Cloth Simulation",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        std::fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::fprintf(stderr, "GLEW init failed\n");
        return 1;
    }
    std::printf("OpenGL %s\n", glGetString(GL_VERSION));

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    Cloth       cloth;
    Renderer    renderer;
    Interaction interaction;
    Camera      camera;

    auto apply_image = [&](const std::string& path) {
        int iw = 0, ih = 0;
        int new_cols, new_rows;
        if (!path.empty() && Renderer::image_size(path, iw, ih) && iw > 0 && ih > 0) {
            float aspect = (float)iw / (float)ih;
            grid_from_aspect(aspect, new_cols, new_rows);
            std::printf("Image %dx%d  -> grid %dx%d\n", iw, ih, new_cols, new_rows);
        } else {
            new_cols = new_rows = 40;
        }
        cloth.reset(new_cols, new_rows);
        renderer.reload_texture(path);
        camera.target = glm::vec3(
            (new_cols - 1) * Cloth::SPACING * 0.5f,
           -(new_rows - 1) * Cloth::SPACING * 0.5f,
            0.0f);
    };

    if (!renderer.init(texture_path)) {
        std::fprintf(stderr, "Renderer init failed\n");
        return 1;
    }
    if (!texture_path.empty()) {
        int iw = 0, ih = 0;
        if (Renderer::image_size(texture_path, iw, ih) && iw > 0 && ih > 0) {
            int nc, nr;
            grid_from_aspect((float)iw / (float)ih, nc, nr);
            std::printf("Image %dx%d  -> grid %dx%d\n", iw, ih, nc, nr);
            cloth.reset(nc, nr);
            camera.target = glm::vec3(
                (nc - 1) * Cloth::SPACING * 0.5f,
               -(nr - 1) * Cloth::SPACING * 0.5f,
                0.0f);
        }
    }

    auto apply_heaviness = [&](float h) {
        h = std::fmax(0.0f, std::fmin(1.0f, h));
        float log_min = std::log(0.9999f);
        float log_max = std::log(0.9940f);
        cloth.damping = std::exp(log_min + h * (log_max - log_min));
        cloth.wind_strength = 0.15f * (1.0f - h) * (1.0f - h);
    };

    float heaviness = 0.65f;
    apply_heaviness(heaviness);

    auto update_title = [&]() {
        int level = (int)std::roundf(heaviness * 10.0f);
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "Cloth Sim  |  Heaviness: %d/10  ([ lighter ] heavier)  "
            "| grid: %dx%d  | drag image here",
            level, cloth.cols, cloth.rows);
        SDL_SetWindowTitle(window, buf);
    };
    update_title();

    const float FIXED_DT = 1.0f / 60.0f;
    float  sim_time      = 0.0f;
    float  accumulator   = 0.0f;
    bool   running       = true;
    bool   right_drag    = false;
    int    last_mx       = 0, last_my = 0;

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 prev_time = SDL_GetPerformanceCounter();

    while (running) {
        Uint64 now  = SDL_GetPerformanceCounter();
        float  dt   = (float)(now - prev_time) / (float)perf_freq;
        prev_time   = now;
        if (dt > 0.05f) dt = 0.05f;

        SDL_GetWindowSize(window, &win_w, &win_h);
        float aspect = (float)win_w / (float)win_h;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_DROPFILE: {
                std::string dropped = event.drop.file;
                SDL_free(event.drop.file);
                apply_image(dropped);
                apply_heaviness(heaviness);
                update_title();
                sim_time = 0.0f;
                accumulator = 0.0f;
                break;
            }

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                if (event.key.keysym.sym == SDLK_r) {
                    cloth.reset();
                    apply_heaviness(heaviness);
                    sim_time = 0.0f;
                    accumulator = 0.0f;
                }
                if (event.key.keysym.sym == SDLK_LEFTBRACKET) {
                    heaviness = std::fmax(0.0f, heaviness - 0.1f);
                    apply_heaviness(heaviness);
                    update_title();
                }
                if (event.key.keysym.sym == SDLK_RIGHTBRACKET) {
                    heaviness = std::fmin(1.0f, heaviness + 0.1f);
                    apply_heaviness(heaviness);
                    update_title();
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float ndc_x =  (2.0f * event.button.x / win_w) - 1.0f;
                    float ndc_y = -(2.0f * event.button.y / win_h) + 1.0f;
                    interaction.on_press(cloth, camera, ndc_x, ndc_y, aspect);
                }
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    right_drag = true;
                    last_mx = event.button.x;
                    last_my = event.button.y;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT)
                    interaction.on_release(cloth);
                if (event.button.button == SDL_BUTTON_RIGHT)
                    right_drag = false;
                break;

            case SDL_MOUSEMOTION:
                if (interaction.dragging) {
                    float ndc_x =  (2.0f * event.motion.x / win_w) - 1.0f;
                    float ndc_y = -(2.0f * event.motion.y / win_h) + 1.0f;
                    interaction.on_drag(cloth, camera, ndc_x, ndc_y, aspect);
                }
                if (right_drag) {
                    camera.yaw   += (event.motion.x - last_mx) * 0.4f;
                    camera.pitch += (event.motion.y - last_my) * 0.3f;
                    camera.pitch  = std::max(-80.0f, std::min(80.0f, camera.pitch));
                    last_mx = event.motion.x;
                    last_my = event.motion.y;
                }
                break;

            case SDL_MOUSEWHEEL:
                camera.dist -= event.wheel.y * 0.15f;
                camera.dist  = std::max(0.5f, std::min(10.0f, camera.dist));
                break;
            }
        }

        accumulator += dt;
        if (accumulator > 0.1f) accumulator = 0.1f;

        while (accumulator >= FIXED_DT) {
            cloth.update(FIXED_DT, sim_time);
            sim_time += FIXED_DT;
            accumulator -= FIXED_DT;
        }

        renderer.render(cloth, camera, win_w, win_h);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
