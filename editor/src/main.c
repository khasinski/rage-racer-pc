/*
 * A window around the save format.
 *
 * SDL3 for the window, its event loop and its file dialogs, so one program
 * runs on Windows, macOS and Linux without a platform branch; ImGui through
 * its OpenGL 3 backend, which is the pairing this repository already builds.
 */

#include "editor_ui.h"

#include "cimgui.h"
#include "cimgui_impl.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OpenGL 1.1, so every platform's own library has these. */
extern void glViewport(int x, int y, int width, int height);
extern void glClearColor(float r, float g, float b, float a);
extern void glClear(unsigned int mask);
extern void glReadPixels(int x, int y, int width, int height, unsigned int fmt,
                         unsigned int type, void *pixels);
#define RAGE_GL_COLOR_BUFFER_BIT 0x00004000u
#define RAGE_GL_RGB 0x1907u
#define RAGE_GL_UNSIGNED_BYTE 0x1401u

/*
 * One frame to a file. This exists so that the window can be checked without
 * a person looking at it, which is the only way an automated test can say the
 * editor draws anything at all.
 */
static int WriteFramePpm(const char *path, int width, int height) {
    unsigned char *pixels = malloc((size_t)width * height * 3);
    FILE *stream;
    int row;

    if (pixels == NULL) return 0;
    glReadPixels(0, 0, width, height, RAGE_GL_RGB, RAGE_GL_UNSIGNED_BYTE,
                 pixels);
    stream = fopen(path, "wb");
    if (stream == NULL) {
        free(pixels);
        return 0;
    }
    fprintf(stream, "P6\n%d %d\n255\n", width, height);
    /* OpenGL hands back the bottom row first. */
    for (row = height - 1; row >= 0; row--)
        fwrite(pixels + (size_t)row * width * 3, 1, (size_t)width * 3, stream);
    free(pixels);
    return fclose(stream) == 0;
}

typedef struct Shell {
    EditorState editor;
    SDL_Window *window;
    int pendingOpen;
    int pendingSave;
    char chosen[1024];
} Shell;

static const SDL_DialogFileFilter kFilters[] = {
    {"Rage Racer save", "*"},
};

static void ChoseFile(void *userdata, const char *const *files, int filter) {
    Shell *shell = userdata;

    (void)filter;
    /* A cancelled dialog hands back an empty list rather than an error. */
    if (files == NULL || files[0] == NULL) {
        shell->pendingOpen = 0;
        shell->pendingSave = 0;
        return;
    }
    snprintf(shell->chosen, sizeof(shell->chosen), "%s", files[0]);
}

static void DrawMenuBar(Shell *shell) {
    if (!igBeginMenuBar()) return;
    if (igBeginMenu("File", true)) {
        if (igMenuItem_Bool("New", NULL, false, true)) EditorNew(&shell->editor);
        if (igMenuItem_Bool("Open...", NULL, false, true)) {
            shell->pendingOpen = 1;
            shell->chosen[0] = '\0';
            SDL_ShowOpenFileDialog(ChoseFile, shell, shell->window, kFilters, 1,
                                   NULL, false);
        }
        if (igMenuItem_Bool("Save", NULL, false,
                            shell->editor.loaded &&
                                shell->editor.path[0] != '\0')) {
            EditorSave(&shell->editor, shell->editor.path);
        }
        if (igMenuItem_Bool("Save as...", NULL, false, shell->editor.loaded)) {
            shell->pendingSave = 1;
            shell->chosen[0] = '\0';
            SDL_ShowSaveFileDialog(ChoseFile, shell, shell->window, kFilters, 1,
                                   NULL);
        }
        igEndMenu();
    }
    igEndMenuBar();
}

/* The dialogs answer on their own thread on some platforms, so the answer is
 * picked up here rather than acted on inside the callback. */
static void CollectDialogAnswer(Shell *shell) {
    if (shell->chosen[0] == '\0') return;
    if (shell->pendingOpen) {
        EditorOpen(&shell->editor, shell->chosen);
        shell->pendingOpen = 0;
    } else if (shell->pendingSave) {
        EditorSave(&shell->editor, shell->chosen);
        shell->pendingSave = 0;
    }
    shell->chosen[0] = '\0';
}

int main(int argc, char **argv) {
    Shell shell;
    SDL_GLContext context;
    ImGuiIO *io;
    int running = 1;
    const char *screenshot = NULL;
    int frames = 0;

    memset(&shell, 0, sizeof(shell));
    shell.editor.region = RAGE_REGION_PAL;
    shell.editor.logoZoom = 6;
    snprintf(shell.editor.status, sizeof(shell.editor.status),
             "open a save, or start a new one");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL could not start: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    shell.window = SDL_CreateWindow("Rage Racer save editor", 1200, 800,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                        SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (shell.window == NULL) {
        fprintf(stderr, "no window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    context = SDL_GL_CreateContext(shell.window);
    if (context == NULL) {
        fprintf(stderr, "no OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(shell.window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(shell.window, context);
    SDL_GL_SetSwapInterval(1);

    igCreateContext(NULL);
    io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    igStyleColorsDark(NULL);
    ImGui_ImplSDL3_InitForOpenGL(shell.window, context);
    ImGui_ImplOpenGL3_Init("#version 150");

    {
        int arg;
        for (arg = 1; arg < argc; arg++) {
            if (strcmp(argv[arg], "--screenshot") == 0 && arg + 1 < argc)
                screenshot = argv[++arg];
            else if (strcmp(argv[arg], "--tab") == 0 && arg + 1 < argc)
                shell.editor.openTab = argv[++arg];
            else
                EditorOpen(&shell.editor, argv[arg]);
        }
    }

    while (running) {
        SDL_Event event;
        int width;
        int height;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = 0;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(shell.window))
                running = 0;
            /* Dropping a file on the window opens it. */
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != NULL)
                EditorOpen(&shell.editor, event.drop.data);
        }
        CollectDialogAnswer(&shell);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        {
            ImGuiViewport *viewport = igGetMainViewport();
            igSetNextWindowPos(viewport->WorkPos, ImGuiCond_Always,
                               (ImVec2_c){0.0f, 0.0f});
            igSetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
            if (igBegin("Rage Racer save editor", NULL,
                        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar)) {
                DrawMenuBar(&shell);
                igText("%s%s", shell.editor.status,
                       shell.editor.dirty ? "   (unsaved changes)" : "");
                igSeparator();
                EditorDrawWindow(&shell.editor);
            }
            igEnd();
        }

        igRender();
        SDL_GetWindowSizeInPixels(shell.window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.09f, 0.09f, 0.11f, 1.0f);
        glClear(RAGE_GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        /* A few frames first, so the layout has settled before it is read. */
        if (screenshot != NULL && ++frames == 3) {
            running = WriteFramePpm(screenshot, width, height) ? 0 : 0;
            if (running == 0)
                printf("wrote %s\n", screenshot);
        }
        SDL_GL_SwapWindow(shell.window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(shell.window);
    SDL_Quit();
    return 0;
}
