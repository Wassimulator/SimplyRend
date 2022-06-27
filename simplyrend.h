/*
   _____  _                    __        ____                    __
  / ___/ (_)____ ___   ____   / /__  __ / __ \ ___   ____   ____/ /
  \__ \ / // __ `__ \ / __ \ / // / / // /_/ // _ \ / __ \ / __  /
 ___/ // // / / / / // /_/ // // /_/ // _, _//  __// / / // /_/ /
/____//_//_/ /_/ /_// .___//_/ \__, //_/ |_| \___//_/ /_/ \____/
                   /_/        /____/

        v1.2
    by: Wassimulator

    Using OpenGL Version: 4.3
    A simple lightweight immediate mode renderer for 2D graphics, written in C and OpenGL.
    The approach is not the most efficient, but it's simple and it should work for small
    scale indie purposes. The goal behind this is to help get the program up and running
    quickly, while also setting up a foundation for expansion in features later.

    "I hate doing this twice", well this is why I made this.

# included libraries in package:
    1 - emaths.h        - v1.0  or higher by: Wassimulator
    2 - stb_rect_pack   - v1.01 or higher by: Sean Barrett
    3 - stb_image       - v2.26 or higher by: Sean Barrett
    4 - stb_truetype    - v1.26 or higher by: Sean Barrett

# Important notes:
    - Include OpenGL headers and load OpenGL functions with glad or otherwise before including this file,
      here's a permalink to save you the trouble: https://glad.dav1d.de/#language=c&specification=gl&api=gl%3D4.3&api=gles1%3Dnone&api=gles2%3Dnone&api=glsc2%3Dnone&profile=compatibility&loader=on
    - SymplyRend calls gladLoadGL() for you, you don't need to call it manually.
    - Call SR_Init() before using any other functions.
    - Call SR_StartFrame() at the beginning of each frame.
    - SimplyRend does not create window contexts, the user should
      handle that with SDL, or otherwise.
    - Use floating point color convension (0.0 to 1.0) for each value,
      otherwise use SR_Color_i(r, g, b, a) to convert to floating point colors.
    - the first two shader programs are reserved for rendering basic shapes
      and textured quads.
    - FrameWidth and FrameHeight are the draw logic size, not the window size.
      They are preferably set at the start and never changed during the program.
    - WindwWidth and WindowHeight are the window size, they can be changed at any time.
      The distinction is there to allow for window resizing while maintaining realtive
      positions.
    - you need to handle displaying the frame, whether with SDL or otherwise, Window management
      is not included in SimplyRend.

# Usage:

    # relevant front end functions:
        - SR_Init()                     // initilizes the library
        - SR_StartFrame()               // starts a new frame
        - SR_LoadFont()                 // loads a font
        - SR_LoadSprite()               // loads a sprite
        - SR_PackSpritesToTexture()     // generates a texture atlas out of a range of sprites
        - SR_PushSpriteRect()           // draws a sprite
        - SR_PushSpriteRectF()          // draws a sprite with floating point coordinates
        - SR_PushRect_Frame()           // draws a framed rectangle
        - SR_PushRect_Fill()            // draws a filled rectangle
        - SR_PushLine()                 // draws a two point line
        - SR_PushLines()                // draws a multiple point line
        - SR_PushText()                 // renders text
        - SR_PushLayer()                // starts a new render layer
        - SR_Render()                   // renders the current frame using all collected data

         more info at functions definitions. Search for the names.

    - Initialize SimplyRend with SR_Init()
    - Before the game loop load all your sprites by declaring SR_Sprite* pointers
      and loading them with SR_LoadSprite() to generate sprite info and load to RAM.
    - Use SR_PackSpritesToTexture() to pack a range of sprites into a single texture,
      be wary of hardware limitations, the function may not pack all sprites.
      You can use the function as many times as you want, even to pack single textures.
      The texture ID info will be stored in the sprite struct. But unless you plan on
      manipulating the behavior, you can ignore this.
    - Load fonts at the start of the program with SR_LoadFont()
    - Render Functions buffer render data on the RAM and the render Call in the end of the
      frame sends it to the GPU. Immediate mode.
      Render functions have multiple versions and can accept different parameters.

    - custom shader creation is still in development.

All relative functions and structs start with the prefix "SR_"
All Render related functions start with the prefix "SR_Push"

this is till work in progress and bugs may occur, as features get added. It is in a running state though.
*/

///////////////////////////////////////////////////////////////////////////////
// Includes:
#pragma once
#include <stdio.h>
#include <iostream>
#include <emaths.h>
#ifndef STB_IMAGE_STATIC
#define STB_IMAGE_STATIC
#endif
#include <stb_image.h>
#ifndef STB_RECT_PACK_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#define STBTT_STATIC

#define SR_SHADER_COLORED 0
#define SR_SHADER_TEXTURED 1

float Z_SHIFT = 0.00001f;
float Z_START = 0.0f;

///////////////////////////////////////////////////////////////////////////////
// Data structures:
typedef unsigned long long SR_uint;
struct SR_Point;
struct SR_PointF;
struct SR_Rect;
struct SR_RectF;
struct SR_Color;
struct SR_Sprite;
struct SR_Sprites;
struct SR_Vertex;
struct SR_ObjectRects;
struct SR_ObjectLines;
struct SR_RenderLayer;
struct SR_RenderLayers;
struct SR_Uniform;
struct SR_Uniforms;
struct SR_Program;
struct SR_Programs;
struct SR_Texture;
struct SR_Textures;
struct SR_Font;
struct SR_Fonts;
struct SR_Debug;
struct SR_Target;
struct SR_Buffers;

typedef struct SR_Point
{
    int x;
    int y;
} SR_Point;

typedef struct SR_PointF
{
    float x;
    float y;
    SR_PointF(float X, float Y) : x(X), y(Y){};
} SR_PointF;

typedef struct SR_Rect
{
    int x, y;
    int w, h;
} SR_Rect;

typedef struct SR_RectF
{
    float x;
    float y;
    float w;
    float h;
    SR_RectF() : x(0.0f), y(0.0f), w(0.0f), h(0.0f){};
    SR_RectF(float X, float Y, float W, float H) : x(X), y(Y), w(W), h(H){};
} SR_RectF;

typedef struct SR_Color
{
    union
    {
        float E[4];
        struct
        {
            float r, g, b, a;
        };
    };
    SR_Color() : E{0, 0, 0, 255} {}
    SR_Color(float r, float g, float b, float a) : E{r, g, b, a} {}
} SR_Color;

typedef struct SR_Sprite
{
    int w, h, n;
    int x, y;
    SR_Texture *texture;
    unsigned char *PixelsRGBA;
    SR_Color ModColor;
    int frames;
    bool animated;
    int frame_i = 0;
    int ID = 0;
} SR_Sprite;

typedef struct SR_Sprites
{
    SR_Sprite *S;
    int Count;
} SR_Sprites;

typedef struct SR_Vertex
{
    Emaths::v2 P;
    Emaths::v2 UV;
    SR_Color C;
    float Z;
} SR_Vertex;

typedef struct SR_ObjectRects
{
    struct glrect
    {
        SR_Vertex V[6];
    };

    GLuint VBO = 0;
    GLuint VAO = 0;
    glrect *R;
    int *render_layer_index;
    int Count;
} SR_or;

typedef struct SR_ObjectLines
{
    struct glline
    {
        SR_Vertex V[2];
    };

    GLuint VBO = 0;
    GLuint VAO = 0;
    glline *L;
    int *render_layer_index;
    int Count;
} SR_ol;
typedef struct SR_RenderLayer
{
    struct scissor
    {
        bool on = false;
        SR_Rect rect;
    };
    struct postprocess
    {
        bool on = false;
        SR_Target *Origin;
        SR_Target *Target;
        SR_Program *Program;
    };
    bool NoAspectFix = false;
    bool OneMinusAlpha = false;
    bool clear = false;
    SR_Color ClearColor;
    SR_Target *Target;
    scissor Clip;
    postprocess PP;
    SR_Texture *texture;
} SR_RenderLayer;

typedef struct SR_RenderLayers
{
    SR_Texture *last_tex_ptr = nullptr;
    bool persistent_target_on = false;
    SR_Target *persistent_target;
    bool persistent_clip_on = false;
    SR_Rect persistent_clip_rect;

    SR_RenderLayer *L;
    int count;
    int index;
} SR_RenderLayers;

typedef enum SR_Uniform_type
{
    SR_UNIFORM_INT,
    SR_UNIFORM_UINT,
    SR_UNIFORM_FLOAT
} SR_Uniform_type;

typedef struct SR_Uniform
{
    void *Data;
    int Count;
    SR_Uniform_type Type;
    GLint Location;
    SR_uint ID;
} SR_Uniform;

typedef struct SR_Uniforms
{
    SR_Uniform *U;
    int Count;
} SR_Uniforms;

typedef struct SR_Program
{
    SR_uint ID;
    GLuint glID;
    GLuint Vertex;
    GLuint Fragment;
    SR_Uniforms Uniforms;
} SR_Program;

typedef struct SR_Programs
{
    SR_Program *P;
    SR_uint Count;
} SR_Programs;

typedef struct SR_Texture
{
    GLuint glID;
    SR_uint ID;
} SR_Texture;

typedef struct SR_Textures
{
    SR_Texture *T;
    SR_uint Count;
} SR_Textures;

typedef struct SR_Font
{
    SR_uint ID;
    SR_Sprite *Sprite;
    int *sizes;
    stbtt_packedchar **CharData;
    int CharCount;
    int SizesCount;
    int First;
    unsigned char *Pixels;
    stbtt_fontinfo Info;
    stbtt_pack_context PackContex;
    SR_Texture *Atlas;
    int W, H;
} SR_Font;

typedef struct SR_Fonts
{
    SR_Font *F;
    SR_uint Count;
} SR_Fonts;

typedef struct SR_Debug
{
    SR_uint buffered_vertex_data_size = 0;
    SR_uint estimated_VRAM = 0;
    int draw_calls = 0;
} SR_Debug;

typedef struct SR_Target
{
    GLuint VBO;
    GLuint VAO;

    GLuint FBO;
    SR_Sprite *Sprite_color;
    SR_Sprite *Sprite_depth;
    SR_or::glrect Rect;
    bool DoNotTouch = false;
    int ID;
} SR_Target;

typedef struct SR_Buffers
{
    SR_Target *B;
    SR_uint Count;
} SR_Buffers;

typedef struct SR_Context
{
    SR_uint max_texture_dimension; // corresponds to, but is not necessarily equal to: GL_MAX_TEXTURE_SIZE

    SR_ObjectRects Rects;
    SR_ObjectRects TexRects;
    SR_ObjectLines Lines;
    SR_RenderLayers Layers;
    SR_Programs Programs;
    SR_Textures Textures;
    SR_Fonts Fonts;
    SR_Sprites Sprites;
    SR_Buffers Buffers;

    SR_Debug Debug;

    int FrameWidth;
    int FrameHeight;

    int WindowWidth;
    int WindowHeight;

    float z_index = Z_START;

    bool startframe = false;

    SR_Color DrawColor;
    SR_ObjectRects::glrect ScreenRect;

} SR_Context;

SR_Context simplyrend_context = {};
///////////////////////////////////////////////////////////////////////////////
// Functions:

bool SR_GL_error()
{
    int ErrorCode = glGetError();
    GLenum Error = glGetError();
    if (Error)
    {
        printf("Encountered OpenGL error code: %d %d\n", ErrorCode, Error);
        return true;
    }
    else
        return false;
}

SR_Color SR_Color_i(int r, int g, int b, int a)
{
    SR_Color C;
    C.r = r / 255.0f;
    C.g = g / 255.0f;
    C.b = b / 255.0f;
    C.a = a / 255.0f;
    return C;
}

// creates and pushes a new render program onto the array heap of programs:
SR_Program *SR_CreateProgram(const char *VertexCode, const char *FragmentCode)
{
    SR_Context *O = &simplyrend_context;
    SR_Program *result = nullptr;

    SR_Program *P = &O->Programs.P[O->Programs.Count];
    memset(P, 0, sizeof(SR_Program));
    int id = O->Programs.Count;
    O->Programs.Count++;

    GLuint Pipeline = glCreateProgram();
    GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glAttachShader(Pipeline, VertexShader);
    glAttachShader(Pipeline, FragmentShader);

    const GLint VertexCodeLength = strlen(VertexCode);
    glShaderSource(VertexShader, 1, &VertexCode, &VertexCodeLength);
    glCompileShader(VertexShader);

    const GLint FragmentCodeLength = strlen(FragmentCode);
    glShaderSource(FragmentShader, 1, &FragmentCode, &FragmentCodeLength);
    glCompileShader(FragmentShader);

    glLinkProgram(Pipeline);

    int NoErrors = 0;
    glGetProgramiv(Pipeline, GL_LINK_STATUS, &NoErrors);

    bool error = false;
    if (NoErrors == 0)
    {
        GLsizei BufferLength;
        GLchar Buffer[4096];
        glGetShaderInfoLog(FragmentShader, 4096, &BufferLength, Buffer);
        if (BufferLength > 0)
        {
            std::cerr << "Error (Shader):" << Buffer << std::endl;
            error = true;
        }
        glGetShaderInfoLog(VertexShader, 4096, &BufferLength, Buffer);
        if (BufferLength > 0)
        {
            std::cerr << "Error (Shader):" << Buffer << std::endl;
            error = true;
        }
        glGetProgramInfoLog(Pipeline, 4096, &BufferLength, Buffer);
        if (BufferLength > 0)
        {
            std::cerr << "Error(Program):" << Buffer << std::endl;
            error = true;
        }
    }
    if (!error)
    {
        P->glID = Pipeline;
        P->Vertex = VertexShader;
        P->Fragment = FragmentShader;
        P->ID = id;
        result = P;
    }
    else
    {
        O->Programs.Count--;
    }

    return result;
}
/*
Generate an SR_Target object and returns a pointer to it. The SR_Target is linked to a texture and a frame buffer object that
get generated using this call.
Call this once per target at the beginning of the program after initializing the library.
There is no Target destruction function yet as per this version of the library.
*/
SR_Target *SR_CreateTarget(SR_RectF *Rect)
{
    SR_Context *O = &simplyrend_context;

    SR_Sprite *S_color = &O->Sprites.S[O->Sprites.Count];
    int sprite_id = O->Sprites.Count;
    O->Sprites.Count++;
    SR_Sprite *S_Depth = &O->Sprites.S[O->Sprites.Count];
    int sprite_id_depth = O->Sprites.Count;

    SR_Target *B = &O->Buffers.B[O->Buffers.Count];
    int target_id = O->Buffers.Count;

    SR_Texture *Target_texture_color = &O->Textures.T[O->Textures.Count];
    int texture_id = O->Textures.Count;
    O->Textures.Count++;
    SR_Texture *Target_texture_depth = &O->Textures.T[O->Textures.Count];
    int texture_id_depth = O->Textures.Count;

    // Target has a pointer to a sprite which has a texture id of the target.
    // Target --> Sprite --> Texture_ID
    // TODO: Rects don't really need to be stored on the stack, they can be generated in the SR_Render call, or even in the vertex shader (pro mode)

    B->Sprite_color = S_color;
    B->Sprite_depth = S_Depth;
    B->ID = target_id;

    S_color->ID = sprite_id;
    S_color->texture = Target_texture_color;
    S_Depth->ID = sprite_id_depth;
    S_Depth->texture = Target_texture_depth;

    B->DoNotTouch = false;
    float x, y, w, h;
    if (Rect)
    {
        x = Rect->x;
        y = Rect->y;
        w = Rect->w;
        h = Rect->h;

        B->Rect.V[0].P = Emaths::v2(x, y);
        B->Rect.V[1].P = Emaths::v2(x + w, y);
        B->Rect.V[2].P = Emaths::v2(x, y + h);
        B->Rect.V[3].P = Emaths::v2(x, y + h);
        B->Rect.V[4].P = Emaths::v2(x + w, y);
        B->Rect.V[5].P = Emaths::v2(x + w, y + h);

        B->Rect.V[0].UV = Emaths::v2(0, 1);
        B->Rect.V[1].UV = Emaths::v2(1, 1);
        B->Rect.V[2].UV = Emaths::v2(0, 0);
        B->Rect.V[3].UV = Emaths::v2(0, 0);
        B->Rect.V[4].UV = Emaths::v2(1, 1);
        B->Rect.V[5].UV = Emaths::v2(1, 0);
    }
    else
    {
        w = O->WindowWidth;
        h = O->WindowHeight;
        S_color->w = w;
        S_color->h = h;
        B->Rect = O->ScreenRect;
    }

    bool success = false;

    // if (O->Buffers.Count > 0)
    {
        // TODO: You really don't need a seperate VBO and VAO if they all are the same unless the format changes, put this in global
        glGenVertexArrays(1, &B->VAO);
        glBindVertexArray(B->VAO);
        glGenBuffers(1, &B->VBO);
        glBindBuffer(GL_ARRAY_BUFFER, B->VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, P));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, C));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
        glBufferData(GL_ARRAY_BUFFER, sizeof(SR_ObjectRects::glrect) * 1, NULL, GL_STREAM_DRAW);

        glGenFramebuffers(1, &B->FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, B->FBO);

        glGenTextures(1, &Target_texture_color->glID);
        glBindTexture(GL_TEXTURE_2D, Target_texture_color->glID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Target_texture_color->glID, 0);

        glGenTextures(1, &Target_texture_depth->glID);
        glBindTexture(GL_TEXTURE_2D, Target_texture_depth->glID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Target_texture_depth->glID, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
            success = true;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // TODO: write an SR_CreateTexture function that takes care of this for you
        // idea: pool allocation: http://www.gingerbill.org/article/2019/02/16/memory-allocation-strategies-004/
        O->Debug.estimated_VRAM += w * h * 4;
    }
    // else
    //     success = true;

    O->Sprites.Count++;
    O->Textures.Count++;
    O->Buffers.Count++;

    return success ? B : nullptr;
}
SR_Target *SR_CreateTargetForSprite(SR_Sprite *Sprite)
{
    SR_Context *O = &simplyrend_context;

    SR_Sprite *S = Sprite;

    SR_Target *B = &O->Buffers.B[O->Buffers.Count];
    int target_id = O->Buffers.Count;

    SR_Texture *Target_texture = Sprite->texture;

    // Target has a pointer to a sprite which has a texture id of the target.
    // Target --> Sprite --> Texture_ID

    B->Sprite_color = S;
    B->DoNotTouch = true;
    B->ID = target_id;
    {
        B->Rect.V[0].P = Emaths::v2(0, 0);
        B->Rect.V[1].P = Emaths::v2(0 + S->w, 0);
        B->Rect.V[2].P = Emaths::v2(0, 0 + S->h);
        B->Rect.V[3].P = Emaths::v2(0, 0 + S->h);
        B->Rect.V[4].P = Emaths::v2(0 + S->w, 0);
        B->Rect.V[5].P = Emaths::v2(0 + S->w, 0 + S->h);

        B->Rect.V[0].UV = Emaths::v2(0, 1);
        B->Rect.V[1].UV = Emaths::v2(1, 1);
        B->Rect.V[2].UV = Emaths::v2(0, 0);
        B->Rect.V[3].UV = Emaths::v2(0, 0);
        B->Rect.V[4].UV = Emaths::v2(1, 1);
        B->Rect.V[5].UV = Emaths::v2(1, 0);
    }

    bool success = false;

    glGenVertexArrays(1, &B->VAO);
    glBindVertexArray(B->VAO);
    glGenBuffers(1, &B->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, B->VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, P));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, C));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
    glBufferData(GL_ARRAY_BUFFER, sizeof(SR_ObjectRects::glrect) * 1, NULL, GL_STREAM_DRAW);

    glGenFramebuffers(1, &B->FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, B->FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Target_texture->glID, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        success = true;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    O->Buffers.Count++;

    return success ? B : nullptr;
}
void SR_DestroyTarget(SR_Target *Target)
{
    if (Target == nullptr)
        return;
    SR_Context *O = &simplyrend_context;
    glDeleteFramebuffers(1, &Target->FBO);
    glDeleteVertexArrays(1, &Target->VAO);
    glDeleteBuffers(1, &Target->VBO);
    if (!Target->DoNotTouch)
    {
        glDeleteTextures(1, &Target->Sprite_color->texture->glID);
        glDeleteTextures(1, &Target->Sprite_depth->texture->glID);
        O->Debug.estimated_VRAM -= O->max_texture_dimension * O->max_texture_dimension * 4;
    }
    O->Buffers.Count--;
}

void SR_SetTarget(SR_Target *Target, bool persistent)
{
    SR_Context *O = &simplyrend_context;
    O->Layers.L[O->Layers.index].Target = Target;
    O->Layers.persistent_target_on = persistent;
    O->Layers.persistent_target = Target;
}

SR_Texture *SR_GetBufferText(SR_Target *Target)
{
    SR_Texture *result = nullptr;
    if (Target != 0)
    {
        SR_Context *O = &simplyrend_context;
        result = Target->Sprite_color->texture;
    }
    return result;
}
GLuint SR_GetBufferFBO(SR_Target *Target)
{
    GLuint result = 0;
    if (Target != 0)
    {
        SR_Context *O = &simplyrend_context;
        result = Target->FBO;
    }
    return result;
}

// since this uses OpenGL, the z order is important and pushing the render layer guarantees a seperate draw call from that point forward.
// This also resets the render target to the screen.
// TODO: consider making this "StarLayer" and "EndLayer"
int SR_PushLayer(bool clear, bool NoAspectFix = false, bool OneMinusAlpha = false)
{
    SR_Context *O = &simplyrend_context;
    O->Layers.index++;
    int layer = O->Layers.index;
    if (O->Layers.index >= O->Layers.count)
    {
        O->Layers.count++;
        O->TexRects.render_layer_index = (int *)realloc(O->TexRects.render_layer_index, (O->Layers.count + 1) * sizeof(int));
        O->Lines.render_layer_index = (int *)realloc(O->Lines.render_layer_index, (O->Layers.count + 1) * sizeof(int));
        O->Rects.render_layer_index = (int *)realloc(O->Rects.render_layer_index, (O->Layers.count + 1) * sizeof(int));
        O->Layers.L = (SR_RenderLayer *)realloc(O->Layers.L, (O->Layers.count + 1) * sizeof(SR_RenderLayer));
        memset(&O->Layers.L[O->Layers.count - 1], 0, sizeof(SR_RenderLayer));
    }
    O->Layers.last_tex_ptr = nullptr;

    O->Rects.render_layer_index[layer] = O->Rects.Count;
    O->TexRects.render_layer_index[layer] = O->TexRects.Count;
    O->Lines.render_layer_index[layer] = O->Lines.Count;

    if (O->startframe)
    {
        O->Layers.L[layer].Target = nullptr;
        O->Layers.L[layer].Clip.on = 0;
        O->Layers.L[layer].PP.on = 0;
        O->Layers.L[layer].clear = clear;
        O->Layers.L[layer].ClearColor = O->DrawColor;
        O->Layers.L[layer].NoAspectFix = NoAspectFix;
        O->Layers.L[layer].OneMinusAlpha = OneMinusAlpha;
    }
    if (O->Layers.persistent_target_on)
    {
        O->Layers.L[layer].Target = O->Layers.persistent_target;
    }
    if (O->Layers.persistent_clip_on)
    {
        O->Layers.L[layer].Clip.on = true;
        O->Layers.L[layer].Clip.rect = O->Layers.persistent_clip_rect;
    }

    return layer;
}

// call this at the beginning of every frame within the program loop, you may change the window width and height
// the change should reflect window resizing in the window context controlled by you outside this library
void SR_StartFrame(int WindowWidth, int WindowHeight)
{
    SR_Context *O = &simplyrend_context;
    O->startframe = false;
    SR_PushLayer(0);
    O->Layers.index = 0;
    SR_SetTarget(0, false);
    O->startframe = true;

    O->Rects.Count = 0;
    O->TexRects.Count = 0;
    O->Lines.Count = 0;
    O->z_index = Z_START;
    O->Layers.index = 0;
    O->Layers.last_tex_ptr = nullptr;

    for (int i = 0; i < O->Layers.count; i++)
    {
        O->Rects.render_layer_index[i] = 0;
        O->TexRects.render_layer_index[i] = 0;
        O->Lines.render_layer_index[i] = 0;
        O->Layers.L[i].Clip.on = false;
        O->Layers.L[i].PP.on = false;
        O->Layers.L[i].Target = nullptr;
    }
    if (WindowWidth != O->WindowWidth || WindowHeight != O->WindowHeight)
    {
        glViewport(0, 0, WindowWidth, WindowHeight);
        for (int i = 0; i < O->Buffers.Count; i++)
        {
            SR_Target *B = &O->Buffers.B[i];
            if (B->DoNotTouch)
                continue;
            glBindTexture(GL_TEXTURE_2D, B->Sprite_color->texture->glID);
            O->Debug.estimated_VRAM -= B->Sprite_color->w * B->Sprite_color->h * 4;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WindowWidth, WindowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            B->Sprite_color->w = WindowWidth;
            B->Sprite_color->h = WindowHeight;

            glBindTexture(GL_TEXTURE_2D, B->Sprite_depth->texture->glID);
            O->Debug.estimated_VRAM -= B->Sprite_depth->w * B->Sprite_depth->h * 4;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, WindowWidth, WindowHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
            B->Sprite_depth->w = WindowWidth;
            B->Sprite_depth->h = WindowHeight;

            O->Debug.estimated_VRAM += B->Sprite_color->w * B->Sprite_color->h * 4;
        }
    }

    O->WindowWidth = WindowWidth;
    O->WindowHeight = WindowHeight;
}
void SR_InitScreenRect()
{
    SR_Context *O = &simplyrend_context;

    SR_ObjectRects::glrect Screen;
    float x = -1;
    float y = -1;
    float w = 2;
    float h = 2;

    Screen.V[0].P = Emaths::v2(x, y);
    Screen.V[1].P = Emaths::v2(x + w, y);
    Screen.V[2].P = Emaths::v2(x, y + h);
    Screen.V[3].P = Emaths::v2(x, y + h);
    Screen.V[4].P = Emaths::v2(x + w, y);
    Screen.V[5].P = Emaths::v2(x + w, y + h);

    Screen.V[0].UV = Emaths::v2(0, 1);
    Screen.V[1].UV = Emaths::v2(1, 1);
    Screen.V[2].UV = Emaths::v2(0, 0);
    Screen.V[3].UV = Emaths::v2(0, 0);
    Screen.V[4].UV = Emaths::v2(1, 1);
    Screen.V[5].UV = Emaths::v2(1, 0);

    for (int i = 0; i < 6; i++)
    {
        Screen.V[i].C.r = 1;
        Screen.V[i].C.g = 1;
        Screen.V[i].C.b = 1;
        Screen.V[i].C.a = 1;
        Screen.V[i].Z = O->z_index;
    }
    O->z_index -= Z_SHIFT;

    O->ScreenRect = Screen;
}

/* Initializes the OpenGL context and the relevant data structures for SimplyRend,
 - MaxRects: Assign maximum number of rectangles extimated to be used as an upper bound,
 The larger MaxRects, the larger the RAM usage.
 - FrameWidth and FrameHeight are the dimensions of the framebuffer, these should be constant. */
void SR_Init(SR_uint MaxRects, int FrameWidth, int FrameHeight, int WindowWidth, int WindowHeight, int (*load_opengl)(void))
{
    int loadgl = load_opengl();
    memset(&simplyrend_context, 0, sizeof(SR_Context));
    SR_Context *O = &simplyrend_context;

    O->TexRects.render_layer_index = nullptr;
    O->Lines.render_layer_index = nullptr;
    O->Rects.render_layer_index = nullptr;
    O->Layers.L = nullptr;

    SR_InitScreenRect();

    O->FrameWidth = FrameWidth;
    O->FrameHeight = FrameHeight;
    O->WindowWidth = WindowWidth;
    O->WindowHeight = WindowHeight;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // allocate memory for the vertex buffers:
    O->Rects.R = (SR_or::glrect *)malloc(sizeof(SR_or::glrect) * MaxRects);
    O->TexRects.R = (SR_or::glrect *)malloc(sizeof(SR_or::glrect) * MaxRects);
    O->Lines.L = (SR_ol::glline *)malloc(sizeof(SR_ol::glline) * MaxRects);
    O->Sprites.S = (SR_Sprite *)realloc(O->Sprites.S, sizeof(SR_Sprite) * (MaxRects));

    O->Textures.T = (SR_Texture *)realloc(O->Textures.T, sizeof(SR_Texture) * (100));
    O->Buffers.B = (SR_Target *)realloc(O->Buffers.B, sizeof(SR_Target) * (100));
    O->Fonts.F = (SR_Font *)realloc(O->Fonts.F, sizeof(SR_Font) * (100));
    O->Programs.P = (SR_Program *)realloc(O->Programs.P, sizeof(SR_Program) * (100));

    // assign attributes to the vertex buffers:
    glGenVertexArrays(1, &O->Rects.VAO);
    glBindVertexArray(O->Rects.VAO);
    glGenBuffers(1, &O->Rects.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, O->Rects.VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, P));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, C));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
    glBufferData(GL_ARRAY_BUFFER, sizeof(SR_ObjectRects::glrect) * MaxRects, NULL, GL_STREAM_DRAW);

    glGenVertexArrays(1, &O->Lines.VAO);
    glBindVertexArray(O->Lines.VAO);
    glGenBuffers(1, &O->Lines.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, O->Lines.VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, P));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, C));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
    glBufferData(GL_ARRAY_BUFFER, sizeof(SR_ObjectLines::glline) * MaxRects, NULL, GL_STREAM_DRAW);

    glGenVertexArrays(1, &O->TexRects.VAO);
    glBindVertexArray(O->TexRects.VAO);
    glGenBuffers(1, &O->TexRects.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, O->TexRects.VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, P));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, C));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
    glBufferData(GL_ARRAY_BUFFER, sizeof(SR_ObjectRects::glrect) * MaxRects, NULL, GL_STREAM_DRAW);

    // create the default shaders:
    const char *VertexShaders[] =
        {
            R"###(
            #version 430 
            layout(location = 0) in vec2 Position;
            layout(location = 1) in vec4 ColorIn;
            layout(location = 2) in float z_order;
            smooth out vec4 ColorOut;
            layout(location = 0) uniform vec2 scale;
            layout(location = 1) uniform float FW;
            layout(location = 2) uniform float FH;

            vec2 Pixel2GL(vec2 P)
            {
                vec2 Res = P / vec2(FW, FH);
                Res = Res * 2 - 1;
                Res.y *= -1;
                return Res;
            }
            void main()
            {   
				gl_Position = vec4(scale * Pixel2GL(Position.xy), z_order, 1.0);
                ColorOut = ColorIn;
            }
    )###",

            R"###(
            #version 430 core
            layout (location = 0) in vec4 vertex; 
            layout (location = 1) in vec4 TexModColor;
            layout (location = 2) in float z_order;
            smooth out vec2 TexCoords;
			smooth out vec4 TexMod;
            layout(location = 0) uniform vec2 scale;
            layout(location = 1) uniform float FW;
            layout(location = 2) uniform float FH;

            vec2 Pixel2GL(vec2 P)
            {
                vec2 Res = P / vec2(FW, FH);
                Res = Res * 2 - 1;
                Res.y *= -1;
                return Res;
            }
            void main()
            {
				TexMod = TexModColor;
                gl_Position = vec4(scale * Pixel2GL(vertex.xy), z_order, 1.0);
                TexCoords = vertex.zw;
            } 
    )###"};

    const char *FragmentShaders[] =
        {
            R"###(
            #version 430 
            out vec4 FragmentShader;
            smooth in vec4 ColorOut;
            void main()
            {
            FragmentShader = ColorOut;
            } 
    )###",

            R"###(
            #version 430 core
            smooth in vec2 TexCoords;
            smooth in vec4 TexMod;
            out vec4 color;
            uniform sampler2D Sampler;
			vec2 uv_aa_smoothstep( vec2 uv, vec2 res, float width ) 
			{
			    vec2 pixels = uv * res;

			    vec2 pixels_floor = floor(pixels + 0.5);
			    vec2 pixels_fract = fract(pixels + 0.5);
			    vec2 pixels_aa = fwidth(pixels) * width * 0.5;
			    pixels_fract = smoothstep( vec2(0.5) - pixels_aa, vec2(0.5) + pixels_aa, pixels_fract );

			    return (pixels_floor + pixels_fract - 0.5) / res;
			}
            void main()
            {    
				// vec2 uvActual = uv_aa_smoothstep(TexCoords, textureSize(Sampler, 0),1.5);
				vec2 uvActual = TexCoords;
                vec4 T = texture(Sampler,   uvActual);
                if (TexMod.a != 1)
				color = vec4(T.a * TexMod.rgb,TexMod.a * T.a);
                else
				color = T * TexMod;

            }
    )###"};
    SR_CreateProgram(VertexShaders[0], FragmentShaders[0]); // 1st program for colored rendering
    SR_CreateProgram(VertexShaders[1], FragmentShaders[1]); // 2nd program for textured rendering

    SR_CreateTarget(0); // create the default target: the screen
}

SR_uint SR_AttachUniform(SR_Program *program, GLint Location, void *Data, int count, SR_Uniform_type Type)
{
    SR_uint result = -1;
    SR_Context *O = &simplyrend_context;
    SR_Program *P = program;

    P->Uniforms.U = (SR_Uniform *)realloc(P->Uniforms.U, sizeof(SR_Uniform) * (P->Uniforms.Count + 1));
    SR_Uniform *U = &P->Uniforms.U[P->Uniforms.Count];
    SR_uint id = P->Uniforms.Count;
    P->Uniforms.Count++;

    U->Location = Location;
    U->ID = id;
    U->Data = Data;
    U->Count = count;
    U->Type = Type;
    result = U->ID;

    return result;
}
/* Load sprite, loads an image file into a sprite that will be added to a texture atlas you create
it could be animated with custom frames (WOP), it could also be loaded from memory. */
SR_Sprite *SR_LoadSprite(char *filename, bool animated, int frames)
{
    SR_Context *O = &simplyrend_context;
    SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
    unsigned char *Pixels = stbi_load(filename, &Result->w, &Result->h, &Result->n, STBI_rgb_alpha);

    if (Pixels == NULL)
    {
        std::cerr << "Error loading image: " << filename << std::endl;
    }
    else
    {

        // O->Sprites.S = (SR_Sprite *)realloc(O->Sprites.S, sizeof(SR_Sprite) * (O->Sprites.Count + 1));
        SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
        SR_uint id = O->Sprites.Count;
        O->Sprites.Count++;

        Result->PixelsRGBA = Pixels;
        Result->frames = frames;
        Result->frame_i = 0;
        Result->animated = animated;
        Result->ModColor = {1.0f, 1.0f, 1.0f, 1.0f};
        Result->ID = id;
    }

    return Result;
}
SR_Sprite *SR_LoadSprite(char *filename)
{
    SR_Context *O = &simplyrend_context;
    SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
    unsigned char *Pixels = stbi_load(filename, &Result->w, &Result->h, &Result->n, STBI_rgb_alpha);
    // std::cout << stbi_failure_reason() << std::endl;

    if (Pixels == NULL)
    {
        std::cerr << "Error loading image: " << filename << std::endl;
    }
    else
    {
        SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
        SR_uint id = O->Sprites.Count;
        O->Sprites.Count++;

        Result->PixelsRGBA = Pixels;
        Result->frames = 0;
        Result->frame_i = 0;
        Result->animated = false;
        Result->ModColor = {1.0f, 1.0f, 1.0f, 1.0f};
        Result->ID = id;
    }

    return Result;
}
SR_Sprite *SR_LoadSprite_Memory(char *refname, uint8_t *Data, uint64_t size)
{
    SR_Context *O = &simplyrend_context;
    SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
    int x, y, n;
    unsigned char *Pixels = stbi_load_from_memory(Data, size, &x, &y, &n, 4);
    // std::cout << stbi_failure_reason() << std::endl;

    if (Pixels == NULL)
    {
        std::cerr << "Error loading image: " << refname << std::endl;
    }
    else
    {
        SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
        SR_uint id = O->Sprites.Count;
        O->Sprites.Count++;

        Result->w = x;
        Result->h = y;

        Result->PixelsRGBA = Pixels;
        Result->frames = 0;
        Result->frame_i = 0;
        Result->animated = false;
        Result->ModColor = {1.0f, 1.0f, 1.0f, 1.0f};
        Result->ID = id;
    }

    return Result;
}
SR_Sprite *SR_LoadSprite(unsigned char *Data, int w, int h)
{
    SR_Context *O = &simplyrend_context;
    // O->Sprites.S = (SR_Sprite *)realloc(O->Sprites.S, sizeof(SR_Sprite) * (O->Sprites.Count + 1));
    SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
    memset(Result, 0, sizeof(SR_Sprite));

    Result->PixelsRGBA = (unsigned char *)realloc(Result->PixelsRGBA, w * h * 4);
    memcpy(Result->PixelsRGBA, Data, w * h * 4);
    Result->w = w;
    Result->h = h;
    Result->ID = O->Sprites.Count;
    Result->ModColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Result->animated = false;

    O->Sprites.Count++;

    return Result;
}

// Deletes the texture from VRAM, be careful not to draw sprites that
// use the texture before deleting it, or after.
// Does not manipulate the texture array in the context,
// the respective SR_Texture should not be used after this call.
void SR_DeleteSpriteTexture(SR_Sprite *Sprite)
{
    SR_Context *O = &simplyrend_context;
    O->Debug.estimated_VRAM -= O->max_texture_dimension * O->max_texture_dimension * 4;

    glDeleteTextures(1, &Sprite->texture->glID);
}

// generates a new texture, pushes it into the texture array on the heap, and returns a pointer to it
SR_Texture *SR_GenerateTextureAtlas(stbrp_context *context, stbrp_rect *rects, int rectsnum)
{
    // TODO: This function is very inperformant, it should use existing RBA and flip on the GPU
    SR_Context *O = &simplyrend_context;
    // O->Textures.T = (SR_Texture *)realloc(O->Textures.T, sizeof(SR_Texture) * (O->Textures.Count + 1));
    SR_Texture *Target_texture = &O->Textures.T[O->Textures.Count];
    SR_uint ID = O->Textures.Count;
    O->Textures.Count++;

    unsigned char *PixelsRGBA = (unsigned char *)malloc(sizeof(unsigned char) * context->width * context->height * 4);

    for (int i = 0; i < rectsnum; i++)
    {
        if (rects[i].was_packed == 0)
            continue;
        int X = rects[i].x;
        int Y = rects[i].y;
        int W = rects[i].w;
        int H = rects[i].h;
        int id = rects[i].id;
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
            { // we flip the texture here -> ((H - 1 - y)
                PixelsRGBA[((Y + y) * context->width + (X + x)) * 4 + 0] = O->Sprites.S[id].PixelsRGBA[((H - 1 - y) * W + x) * 4 + 0];
                PixelsRGBA[((Y + y) * context->width + (X + x)) * 4 + 1] = O->Sprites.S[id].PixelsRGBA[((H - 1 - y) * W + x) * 4 + 1];
                PixelsRGBA[((Y + y) * context->width + (X + x)) * 4 + 2] = O->Sprites.S[id].PixelsRGBA[((H - 1 - y) * W + x) * 4 + 2];
                PixelsRGBA[((Y + y) * context->width + (X + x)) * 4 + 3] = O->Sprites.S[id].PixelsRGBA[((H - 1 - y) * W + x) * 4 + 3];
            }
    }
    glGenTextures(1, &Target_texture->glID);
    glBindTexture(GL_TEXTURE_2D, Target_texture->glID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context->width, context->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, PixelsRGBA);
    glGenerateMipmap(GL_TEXTURE_2D);

    O->Debug.estimated_VRAM += context->width * context->height * 4;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(PixelsRGBA);
    return Target_texture;
}

SR_uint SR_GetCurrentSpriteIndex()
{
    SR_Context *O = &simplyrend_context;
    return O->Sprites.Count;
}
// packs sprites from from_index to (i < from_index + count) into one texture
// and returns a pointer to the texture. NOT GLuint id, but the id within the sr context.
// if count is -1 it will pack all sprites from from_index to the end of the array
// TODO: Create a fucntion that generates a single texture for a sprite
// TODO: Pass an array of pointers is safer than keeping track of indices
SR_Texture *SR_PackSpritesToTexture(int from_index, int count)
{
    SR_Context *O = &simplyrend_context;
    SR_Sprite *S = O->Sprites.S;

    GLint MAX = 8192;
    GLint MAX_GPU = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MAX_GPU);
    MAX = MAX < MAX_GPU ? MAX : MAX_GPU;
    O->max_texture_dimension = MAX; // TODO: Variable texture sizes, becuase this is wasteful. Texture size should also be stored in the Texture struct as well

    int COUNT = count;
    int end = from_index + count;
    if (count < 0)
    {
        end = O->Sprites.Count;
        COUNT = end - from_index;
    }

    stbrp_context context;
    stbrp_rect *rects = (stbrp_rect *)malloc(sizeof(stbrp_rect) * COUNT);
    for (int i = 0; i < COUNT; i++)
    {
        int j = from_index + i;
        rects[i].i = i;
        rects[i].id = S[j].ID;
        rects[i].w = S[j].w;
        rects[i].h = S[j].h;
        rects[i].x = 0;
        rects[i].y = 0;
        rects[i].was_packed = 0;
    }

    const int nodeCount = MAX * 2;

    stbrp_node *nodes = (stbrp_node *)malloc(sizeof(stbrp_node) * nodeCount);
    stbrp_init_target(&context, MAX, MAX, nodes, nodeCount);
    stbrp_pack_rects(&context, rects, COUNT);
    SR_Texture *texture = SR_GenerateTextureAtlas(&context, rects, COUNT);

    for (int i = 0; i < COUNT; i++)
    {
        S[rects[i].id].texture = texture;
        S[rects[i].id].x = rects[i].x;
        S[rects[i].id].y = rects[i].y;
        // printf("Sprite %d packed to texture %d at %d,%d\n", rects[i].id, texture_id, rects[i].x, rects[i].y);
    }

    free(rects);
    free(nodes);
    for (int i = from_index; i < end; i++)
        free(O->Sprites.S[i].PixelsRGBA);

    return texture;
}

// Emaths::v2 (Emaths::v2 Point)
// {
//     SR_Context *O = &simplyrend_context;

//     Emaths::v2 Res = Point / Emaths::v2(O->FrameWidth, O->FrameHeight);
//     Res = Res * 2 - 1;
//     Res.y *= -1;

//     return Res;
// }

Emaths::v2 SR_UV_to_GL(Emaths::v2 Origin, int w, int h, float uv_x, float uv_y)
{
    SR_Context *O = &simplyrend_context;
    int MAX = O->max_texture_dimension;
    return (Origin + Emaths::v2(0.5, 0.5) + Emaths::v2((w - 1) * uv_x, (h - 1) * uv_y)) / (MAX);
}
Emaths::v2 SR_UV_to_GL(Emaths::v2 Origin, int w, int h, float uv_x, float uv_y, int MAX)
{
    SR_Context *O = &simplyrend_context;
    return (Origin + Emaths::v2(0.5, 0.5) + Emaths::v2((w - 1) * uv_x, (h - 1) * uv_y)) / (MAX);
}

void SR_BufferQuad_Fill(Emaths::v2 A1, Emaths::v2 A2, Emaths::v2 B1, Emaths::v2 B2, SR_Color Color = SR_Color(1, 1, 1, 1))
{
    SR_Context *O = &simplyrend_context;

    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;

    SR_ObjectRects *RO = &O->Rects;

    RO->R[RO->Count].V[0].P = A1;
    RO->R[RO->Count].V[1].P = A2;
    RO->R[RO->Count].V[2].P = B1;
    RO->R[RO->Count].V[3].P = B1;
    RO->R[RO->Count].V[4].P = B2;
    RO->R[RO->Count].V[5].P = A1;

    for (int i = 0; i < 6; i++)
    {
        RO->R[RO->Count].V[i].P = (RO->R[RO->Count].V[i].P);
        RO->R[RO->Count].V[i].C.r = r;
        RO->R[RO->Count].V[i].C.g = g;
        RO->R[RO->Count].V[i].C.b = b;
        RO->R[RO->Count].V[i].C.a = a;

        RO->R[RO->Count].V[i].Z = O->z_index;
    }
    O->z_index -= Z_SHIFT;
    RO->Count++;
}
void SR_BufferRect_Fill(const SR_RectF *Rect, SR_Color Color, float angle)
{
    SR_Context *O = &simplyrend_context;
    angle = angle * (float)M_PI / 180.0f;

    float x;
    float y;
    float w;
    float h;
    if (Rect == 0)
    {
        x = y = 0;
        w = O->FrameWidth;
        h = O->FrameHeight;
    }
    else
    {
        x = Rect->x;
        y = Rect->y;
        w = Rect->w;
        h = Rect->h;
    }
    float WW = O->WindowWidth * 0.5;
    float WH = O->WindowHeight * 0.5;
    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;

    SR_ObjectRects *RO = &O->Rects;

    RO->R[RO->Count].V[0].P = Emaths::v2(x, y);
    RO->R[RO->Count].V[1].P = Emaths::v2(x + w, y);
    RO->R[RO->Count].V[2].P = Emaths::v2(x, y + h);
    RO->R[RO->Count].V[3].P = Emaths::v2(x, y + h);
    RO->R[RO->Count].V[4].P = Emaths::v2(x + w, y);
    RO->R[RO->Count].V[5].P = Emaths::v2(x + w, y + h);

    if (angle != 0)
    {
        Emaths::v2 O = Emaths::v2(x + w * 0.5, y + h * 0.5);
        for (int i = 0; i < 6; i++)
        {
            RO->R[RO->Count].V[i].P = Rotate2D(RO->R[RO->Count].V[i].P, O, angle);
        }
    }

    for (int i = 0; i < 6; i++)
    {
        RO->R[RO->Count].V[i].P = (RO->R[RO->Count].V[i].P);
        RO->R[RO->Count].V[i].C.r = r;
        RO->R[RO->Count].V[i].C.g = g;
        RO->R[RO->Count].V[i].C.b = b;
        RO->R[RO->Count].V[i].C.a = a;

        RO->R[RO->Count].V[i].Z = O->z_index;
    }
    O->z_index -= Z_SHIFT;
    RO->Count++;
}

void SR_BufferRect_Frame(const SR_RectF *Rect, SR_Color Color, float angle)
{
    SR_Context *O = &simplyrend_context;

    angle = angle * (float)M_PI / 180.0f;

    float x;
    float y;
    float w;
    float h;
    if (Rect == 0)
    {
        x = y = 0;
        w = O->FrameWidth;
        h = O->FrameHeight;
    }
    else
    {
        x = Rect->x;
        y = Rect->y;
        w = Rect->w;
        h = Rect->h;
    }

    float WW = O->WindowWidth * 0.5;
    float WH = O->WindowHeight * 0.5;
    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;

    SR_ObjectLines *RO = &O->Lines;

    RO->L[RO->Count].V[0].P = Emaths::v2(x, y);
    RO->L[RO->Count].V[1].P = Emaths::v2(x + w, y);
    RO->L[RO->Count + 1].V[0].P = Emaths::v2(x + w, y);
    RO->L[RO->Count + 1].V[1].P = Emaths::v2(x + w, y + h);
    RO->L[RO->Count + 2].V[0].P = Emaths::v2(x + w, y + h);
    RO->L[RO->Count + 2].V[1].P = Emaths::v2(x, y + h);
    RO->L[RO->Count + 3].V[0].P = Emaths::v2(x, y + h);
    RO->L[RO->Count + 3].V[1].P = Emaths::v2(x, y);

    if (angle != 0)
    {
        Emaths::v2 O = Emaths::v2(x + w * 0.5, y + h * 0.5);
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 2; j++)
            {
                RO->L[RO->Count + i].V[j].P = Rotate2D(RO->L[RO->Count + i].V[j].P, O, angle);
            }
    }

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 2; j++)
        {
            RO->L[RO->Count + i].V[j].P = (RO->L[RO->Count + i].V[j].P);
            RO->L[RO->Count + i].V[j].C.r = r;
            RO->L[RO->Count + i].V[j].C.g = g;
            RO->L[RO->Count + i].V[j].C.b = b;
            RO->L[RO->Count + i].V[j].C.a = a;

            RO->L[RO->Count + i].V[j].Z = O->z_index;
        }

    O->z_index -= Z_SHIFT;
    RO->Count += 4;
}

void SR_BufferRect_Font(SR_Font *Font, const SR_RectF *SrcRect, const SR_RectF *DstRect, float angle, SR_Color ModColor)
{
    SR_Context *O = &simplyrend_context;

    angle = angle * (float)M_PI / 180.0f;

    float x;
    float y;
    float w;
    float h;

    if (DstRect == 0)
    {
        x = y = 0;
        w = O->FrameWidth;
        h = O->FrameHeight;
    }
    else
    {
        x = (float)DstRect->x;
        y = (float)DstRect->y;
        w = (float)DstRect->w;
        h = (float)DstRect->h;
    }
    float WW = O->WindowWidth * 0.5;
    float WH = O->WindowHeight * 0.5;

    int W = Font->W;
    int H = Font->H;

    Emaths::v2 Origin = Emaths::v2(0, 0);
    SR_ObjectRects *ROT = &O->TexRects;

    ROT->R[ROT->Count].V[0].P = Emaths::v2(x, y);
    ROT->R[ROT->Count].V[1].P = Emaths::v2(x + w, y);
    ROT->R[ROT->Count].V[2].P = Emaths::v2(x, y + h);
    ROT->R[ROT->Count].V[3].P = Emaths::v2(x, y + h);
    ROT->R[ROT->Count].V[4].P = Emaths::v2(x + w, y);
    ROT->R[ROT->Count].V[5].P = Emaths::v2(x + w, y + h);

    if (angle != 0)
    {
        Emaths::v2 O = Emaths::v2(x + w * 0.5, y + h * 0.5);
        for (int i = 0; i < 6; i++)
        {
            ROT->R[ROT->Count].V[i].P = Rotate2D(ROT->R[ROT->Count].V[i].P, O, angle);
        }
    }

    float u_0 = 0;
    float v_0 = 0;
    float u_1 = 1;
    float v_1 = 1;
    if (SrcRect != 0)
    {
        Emaths::v2 P = Emaths::v2((float)SrcRect->x, H - (float)SrcRect->h - (float)SrcRect->y);
        Origin = Origin + P;
        W = (float)SrcRect->w;
        H = (float)SrcRect->h;
    }

    ROT->R[ROT->Count].V[0].UV = SR_UV_to_GL(Origin, W, H, 0, 1, 500);
    ROT->R[ROT->Count].V[1].UV = SR_UV_to_GL(Origin, W, H, 1, 1, 500);
    ROT->R[ROT->Count].V[2].UV = SR_UV_to_GL(Origin, W, H, 0, 0, 500);

    ROT->R[ROT->Count].V[3].UV = SR_UV_to_GL(Origin, W, H, 0, 0, 500);
    ROT->R[ROT->Count].V[4].UV = SR_UV_to_GL(Origin, W, H, 1, 1, 500);
    ROT->R[ROT->Count].V[5].UV = SR_UV_to_GL(Origin, W, H, 1, 0, 500);

    float r = ModColor.r;
    float g = ModColor.g;
    float b = ModColor.b;
    float a = ModColor.a;

    for (int i = 0; i < 6; i++)
    {
        ROT->R[ROT->Count].V[i].P = (ROT->R[ROT->Count].V[i].P);
        ROT->R[ROT->Count].V[i].C.r = r;
        ROT->R[ROT->Count].V[i].C.g = g;
        ROT->R[ROT->Count].V[i].C.b = b;
        ROT->R[ROT->Count].V[i].C.a = a;

        ROT->R[ROT->Count].V[i].Z = O->z_index;
    }

    O->z_index -= Z_SHIFT;

    ROT->Count++;
}
void SR_BufferRect_Texture(SR_Sprite *Sprite, const SR_RectF *SrcRect, const SR_RectF *DstRect, float angle, SR_Color ModColor)
{
    SR_Context *O = &simplyrend_context;

    angle = angle * (float)M_PI / 180.0f;

    float x;
    float y;
    float w;
    float h;

    if (DstRect == 0)
    {
        x = y = 0;
        w = O->FrameWidth;
        h = O->FrameHeight;
    }
    else
    {
        x = (float)DstRect->x;
        y = (float)DstRect->y;
        w = (float)DstRect->w;
        h = (float)DstRect->h;
    }
    float WW = O->WindowWidth * 0.5;
    float WH = O->WindowHeight * 0.5;

    int W = Sprite->w;
    int H = Sprite->h;

    Emaths::v2 Origin = Emaths::v2(Sprite->x, Sprite->y);
    SR_ObjectRects *ROT = &O->TexRects;

    ROT->R[ROT->Count].V[0].P = Emaths::v2(x, y);
    ROT->R[ROT->Count].V[1].P = Emaths::v2(x + w, y);
    ROT->R[ROT->Count].V[2].P = Emaths::v2(x, y + h);
    ROT->R[ROT->Count].V[3].P = Emaths::v2(x, y + h);
    ROT->R[ROT->Count].V[4].P = Emaths::v2(x + w, y);
    ROT->R[ROT->Count].V[5].P = Emaths::v2(x + w, y + h);

    if (angle != 0)
    {
        Emaths::v2 O = Emaths::v2(x + w * 0.5, y + h * 0.5);
        for (int i = 0; i < 6; i++)
        {
            ROT->R[ROT->Count].V[i].P = Rotate2D(ROT->R[ROT->Count].V[i].P, O, angle);
        }
    }

    float u_0 = 0;
    float v_0 = 0;
    float u_1 = 1;
    float v_1 = 1;
    if (SrcRect != 0)
    {

        Emaths::v2 P = Emaths::v2((float)SrcRect->x, H - (float)SrcRect->h - (float)SrcRect->y);
        Origin = Origin + P;
        W = (float)SrcRect->w;
        H = (float)SrcRect->h;
    }

    ROT->R[ROT->Count].V[0].UV = SR_UV_to_GL(Origin, W, H, 0, 1);
    ROT->R[ROT->Count].V[1].UV = SR_UV_to_GL(Origin, W, H, 1, 1);
    ROT->R[ROT->Count].V[2].UV = SR_UV_to_GL(Origin, W, H, 0, 0);
    ROT->R[ROT->Count].V[3].UV = SR_UV_to_GL(Origin, W, H, 0, 0);
    ROT->R[ROT->Count].V[4].UV = SR_UV_to_GL(Origin, W, H, 1, 1);
    ROT->R[ROT->Count].V[5].UV = SR_UV_to_GL(Origin, W, H, 1, 0);

    float r = ModColor.r;
    float g = ModColor.g;
    float b = ModColor.b;
    float a = ModColor.a;

    for (int i = 0; i < 6; i++)
    {
        ROT->R[ROT->Count].V[i].P = (ROT->R[ROT->Count].V[i].P);
        ROT->R[ROT->Count].V[i].C.r = r;
        ROT->R[ROT->Count].V[i].C.g = g;
        ROT->R[ROT->Count].V[i].C.b = b;
        ROT->R[ROT->Count].V[i].C.a = a;

        ROT->R[ROT->Count].V[i].Z = O->z_index;
    }

    O->z_index -= Z_SHIFT;

    ROT->Count++;
}

void SR_BufferLine(int x1, int y1, int x2, int y2, SR_Color Color)
{
    SR_Context *O = &simplyrend_context;

    SR_ObjectLines *RO = &O->Lines;

    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;

    RO->L[RO->Count].V[0].P = (Emaths::v2(x1, y1));
    RO->L[RO->Count].V[1].P = (Emaths::v2(x2, y2));
    for (int j = 0; j < 2; j++)
    {
        RO->L[RO->Count].V[j].C.r = r;
        RO->L[RO->Count].V[j].C.g = g;
        RO->L[RO->Count].V[j].C.b = b;
        RO->L[RO->Count].V[j].C.a = a;

        RO->L[RO->Count].V[j].Z = O->z_index;
    }

    O->z_index -= Z_SHIFT;
    RO->Count++;
}
void SR_BufferLines(const SR_Point *points, int count, SR_Color Color)
{
    SR_Context *O = &simplyrend_context;

    SR_ObjectLines *RO = &O->Lines;

    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;

    for (int i = 0; i < count - 1; i++)
    {
        int x1 = points[i].x;
        int y1 = points[i].y;
        int x2 = points[i + 1].x;
        int y2 = points[i + 1].y;

        RO->L[RO->Count].V[0].P = (Emaths::v2(x1, y1));
        RO->L[RO->Count].V[1].P = (Emaths::v2(x2, y2));
        for (int j = 0; j < 2; j++)
        {
            RO->L[RO->Count].V[j].C.r = r;
            RO->L[RO->Count].V[j].C.g = g;
            RO->L[RO->Count].V[j].C.b = b;
            RO->L[RO->Count].V[j].C.a = a;

            RO->L[RO->Count].V[j].Z = O->z_index;
        }
        O->z_index -= Z_SHIFT;
        RO->Count++;
    }
}

void SR_SpritePre(SR_Sprite *Sprite)
{
    SR_Context *O = &simplyrend_context;
    if (Sprite->texture != O->Layers.last_tex_ptr)
    {
        if (O->Layers.last_tex_ptr != nullptr)
        {
            SR_Target *target = O->Layers.L[O->Layers.index].Target;
            SR_PushLayer(0);
            O->Layers.L[O->Layers.index].Target = target;
        }
        O->Layers.L[O->Layers.index].texture = Sprite->texture;
    }
    O->Layers.last_tex_ptr = Sprite->texture;
}
// Pushes a sprite rectangle onto the render queue, mod color modifies the color of the sprite
void SR_PushSpriteRect(SR_Sprite *Sprite, SR_Rect *SrcRect, SR_Rect *DstRect, SR_Color ModColor)
{
    SR_SpritePre(Sprite);
    Sprite->ModColor = ModColor;
    SR_RectF *Src = nullptr;
    SR_RectF *Dst = nullptr;
    SR_RectF S, D;
    if (SrcRect != nullptr)
    {
        S = {(float)SrcRect->x, (float)SrcRect->y, (float)SrcRect->w, (float)SrcRect->h};
        Src = &S;
    }
    if (DstRect != nullptr)
    {
        D = {(float)DstRect->x, (float)DstRect->y, (float)DstRect->w, (float)DstRect->h};
        Dst = &D;
    }
    SR_BufferRect_Texture(Sprite, Src, Dst, 0, ModColor);
}

void SR_PushSpriteRect(SR_Sprite *Sprite, SR_Rect *SrcRect, SR_Rect *DstRect)
{
    SR_SpritePre(Sprite);
    SR_Color ModColor = Sprite->ModColor;
    SR_RectF *Src = nullptr;
    SR_RectF *Dst = nullptr;
    SR_RectF S, D;
    if (SrcRect != nullptr)
    {
        S = {(float)SrcRect->x, (float)SrcRect->y, (float)SrcRect->w, (float)SrcRect->h};
        Src = &S;
    }
    if (DstRect != nullptr)
    {
        D = {(float)DstRect->x, (float)DstRect->y, (float)DstRect->w, (float)DstRect->h};
        Dst = &D;
    }
    SR_BufferRect_Texture(Sprite, Src, Dst, 0, ModColor);
}
// Pushes a sprite rectangle onto the render queue, mod color modifies the color of the sprite
void SR_PushSpriteRectF(SR_Sprite *Sprite, SR_RectF *SrcRect, SR_RectF *DstRect, float angle, SR_Color ModColor)
{
    SR_SpritePre(Sprite);
    Sprite->ModColor = ModColor;
    SR_BufferRect_Texture(Sprite, SrcRect, DstRect, angle, ModColor);
}

void SR_PushSpriteRectF(SR_Sprite *Sprite, SR_RectF *SrcRect, SR_RectF *DstRect, float angle)
{
    SR_SpritePre(Sprite);
    SR_Color ModColor = Sprite->ModColor;
    SR_BufferRect_Texture(Sprite, SrcRect, DstRect, angle, ModColor);
}

void SR_SetSpriteColorMod(SR_Sprite *Sprite, SR_Color ModColor)
{
    if (Sprite != nullptr)
        Sprite->ModColor = ModColor;
}

void SR_SetDrawColor(SR_Color Color)
{
    SR_Context *O = &simplyrend_context;
    O->DrawColor = Color;
}

void SR_SetDrawColor(float r, float g, float b, float a)
{
    SR_Context *O = &simplyrend_context;
    O->DrawColor = SR_Color(r, g, b, a);
}

void SR_PushQuad_Fill(Emaths::v2 A1, Emaths::v2 A2, Emaths::v2 B1, Emaths::v2 B2, SR_Color Color = SR_Color(1, 1, 1, 1))
{
    SR_Context *O = &simplyrend_context;
    SR_BufferQuad_Fill(A1, A2, B1, B2, Color);
}

//////////////// framed /////////////////////////////
// Pushes a rectangle frame onto the render queue, the color determines the color of the rectangle
// calling this without the color will use the draw color of the context, determined by SR_SetDrawColor()
void SR_PushRect_Frame(const SR_Rect *Rect, float angle, SR_Color Color)
{
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Frame(Src, Color, angle);
}
void SR_PushRect_Frame(const SR_Rect *Rect, float angle)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Frame(Src, O->DrawColor, angle);
}
void SR_PushRect_Frame(const SR_Rect *Rect)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Frame(Src, O->DrawColor, 0);
}
//////////////// F

void SR_PushRect_Frame(const SR_RectF *Rect, float angle, SR_Color Color)
{
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Frame(Src, Color, angle);
}
void SR_PushRect_Frame(const SR_RectF *Rect, float angle)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Frame(Src, O->DrawColor, angle);
}
void SR_PushRect_Frame(const SR_RectF *Rect)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Frame(Src, O->DrawColor, 0);
}

//////////////// Fill /////////////////////////////
// Pushes a filled rectangle  onto the render queue, the color determines the color of the rectangle
// calling this without the color will use the draw color of the context, determined by SR_SetDrawColor()
void SR_PushRect_Fill(const SR_Rect *Rect, float angle, SR_Color Color)
{
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Fill(Src, Color, angle);
}
void SR_PushRect_Fill(const SR_Rect *Rect, float angle)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Fill(Src, O->DrawColor, angle);
}
void SR_PushRect_Fill(const SR_Rect *Rect)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Fill(Src, O->DrawColor, 0);
}
//////////////// F

void SR_PushRect_Fill(const SR_RectF *Rect, float angle, SR_Color Color)
{
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Fill(Src, Color, angle);
}
void SR_PushRect_Fill(const SR_RectF *Rect, float angle)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Fill(Src, O->DrawColor, angle);
}
void SR_PushRect_Fill(const SR_RectF *Rect)
{
    SR_Context *O = &simplyrend_context;
    SR_RectF *Src = nullptr;
    SR_RectF S;
    if (Rect != nullptr)
    {
        S = {(float)Rect->x, (float)Rect->y, (float)Rect->w, (float)Rect->h};
        Src = &S;
    }
    SR_BufferRect_Fill(Src, O->DrawColor, 0);
}
//////////////// LINES /////////////////////////////ss
// Pushes a Line onto the render queue, the color determines the color of the rectangle
// calling this without the color will use the draw color of the context, determined by SR_SetDrawColor()
void SR_PushLine(int x1, int y1, int x2, int y2, SR_Color Color)
{
    SR_BufferLine(x1, y1, x2, y2, Color);
}
void SR_PushLine(int x1, int y1, int x2, int y2)
{
    SR_Context *O = &simplyrend_context;
    SR_BufferLine(x1, y1, x2, y2, O->DrawColor);
}
// Pushes a continuous segmented line onto the render queue, the color determines the color of the rectangle
// calling this without the color will use the draw color of the context, determined by SR_SetDrawColor()
void SR_PushLines(const SR_Point *points, int count, SR_Color Color)
{
    SR_BufferLines(points, count, Color);
}
void SR_PushLines(const SR_Point *points, int count)
{
    SR_Context *O = &simplyrend_context;
    SR_BufferLines(points, count, O->DrawColor);
}

void SR_Clear() // TODO:IMPORTANT: This is local and immediate, it should push a render command to the render queue, not render on the spot
{
    SR_Context *O = &simplyrend_context;
    GLuint FBO = 0;
    if (O->Layers.persistent_target_on)
    {
        SR_Target *Target = O->Layers.persistent_target;
        if (Target != nullptr)
        {
            FBO = Target->FBO;
        }
    }
    SR_SetDrawColor(O->DrawColor);
    SR_PushLayer(true);
    // SR_RectF Rect = {0, 0, (float)O->WindowWidth,  (float)O->WindowHeight};
    // SR_BufferRect_Fill(&Rect, O->DrawColor, 0);
    // glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glClearColor(O->DrawColor.r, O->DrawColor.g, O->DrawColor.b, O->DrawColor.a);
    // SR_PushLayer();
}
void SR_Clear(SR_Color Color)
{
    SR_Context *O = &simplyrend_context;
    GLuint FBO = 0;
    if (O->Layers.persistent_target_on)
    {
        SR_Target *Target = O->Layers.persistent_target;
        if (Target != nullptr)
        {
            FBO = Target->FBO;
        }
    }
    SR_SetDrawColor(Color);
    SR_PushLayer(true);
    // SR_RectF Rect = {0, 0,  (float)O->WindowWidth,  (float) (float)O->WindowHeight};
    // SR_BufferRect_Fill(&Rect, Color, 0);
    // glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    // glClearColor(Color.r, Color.g, Color.b, Color.a);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // SR_PushLayer();
}
void SR_Clear(float r, float g, float b, float a)
{
    SR_Context *O = &simplyrend_context;
    GLuint FBO = 0;
    if (O->Layers.persistent_target_on)
    {
        SR_Target *Target = O->Layers.persistent_target;
        if (Target != nullptr)
        {
            FBO = Target->FBO;
        }
    }
    SR_Color Color = {r, g, b, a};
    SR_SetDrawColor(Color);
    SR_PushLayer(true);
}

void SR_Clip(const SR_Rect *rect)
{
    SR_Context *O = &simplyrend_context;
    O->Layers.L[O->Layers.index].Clip.rect = *rect;
    O->Layers.L[O->Layers.index].Clip.on = true;
    O->Layers.persistent_clip_on = true;
    O->Layers.persistent_clip_rect = *rect;
    SR_PushLayer(0);
}
void SR_EndClip()
{
    SR_Context *O = &simplyrend_context;
    O->Layers.persistent_clip_on = false;
    SR_PushLayer(0);
}

void SR_PostProc(SR_Target *Origin, SR_Target *Target, SR_Program *Program)
{
    SR_Context *O = &simplyrend_context;
    int layer = SR_PushLayer(0);

    O->Layers.L[layer].PP.on = true;
    O->Layers.L[layer].PP.Origin = Origin;
    O->Layers.L[layer].PP.Target = Target;
    O->Layers.L[layer].PP.Program = Program;

    SR_PushLayer(0);
}

void SR_Render()
{
    SR_Context *O = &simplyrend_context;
    glViewport(0, 0, O->WindowWidth, O->WindowHeight);
    SR_SetTarget(0, true);

    SR_PushLayer(0);

    glBindBuffer(GL_ARRAY_BUFFER, O->Rects.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectRects::glrect) * O->Rects.Count, O->Rects.R);
    glBindBuffer(GL_ARRAY_BUFFER, O->Lines.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectLines::glline) * O->Lines.Count, O->Lines.L);
    glBindBuffer(GL_ARRAY_BUFFER, O->TexRects.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectRects::glrect) * O->TexRects.Count, O->TexRects.R);

    int drawcalls = 0;
    SR_uint data = sizeof(SR_ObjectRects::glrect) * O->Rects.Count +
                   sizeof(SR_ObjectLines::glline) * O->Lines.Count +
                   sizeof(SR_ObjectRects::glrect) * O->TexRects.Count;

    for (int i = 0; i < O->Layers.index; i++)
    {
        float Logical_W = (float)O->FrameWidth;
        float Logical_H = (float)O->FrameHeight;
        float Render_W = (float)O->WindowWidth;
        float Render_H = (float)O->WindowHeight;

        { // setting target buffer
            if (O->Layers.L[i].Target == nullptr)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, O->Layers.L[i].Target->FBO);
                Render_W = O->Layers.L[i].Target->Sprite_color->w;
                Render_H = O->Layers.L[i].Target->Sprite_color->h;
            }
        }
        if (O->Layers.L[i].NoAspectFix)
        {
            Logical_W = Render_W;
            Logical_H = Render_H;
        }
        if (O->Layers.L[i].OneMinusAlpha) // TODO: make an over operator bariable with better options and change accessibility
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
        else
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // LE DEAULT

        glDisable(GL_SCISSOR_TEST);

        // counting rects
        int FilledCount = O->Rects.render_layer_index[i + 1] - O->Rects.render_layer_index[i];
        int FramedCount = O->Lines.render_layer_index[i + 1] - O->Lines.render_layer_index[i];
        int TexturedCount = O->TexRects.render_layer_index[i + 1] - O->TexRects.render_layer_index[i];

        // if (0)
        if (O->Layers.L[i].Clip.on) // Clip code
        {
            glEnable(GL_SCISSOR_TEST);
            float scaleratio = (float)O->WindowWidth / O->FrameWidth;
            float x = O->Layers.L[i].Clip.rect.x * scaleratio;
            float y = (O->FrameHeight - O->Layers.L[i].Clip.rect.h - O->Layers.L[i].Clip.rect.y) * scaleratio;
            float w = O->Layers.L[i].Clip.rect.w * scaleratio;
            float h = O->Layers.L[i].Clip.rect.h * scaleratio;

            glScissor(x, y, w, h);
        }
        if (O->Layers.L[i].clear)
        {
            glClearColor(O->Layers.L[i].ClearColor.r, O->Layers.L[i].ClearColor.g, O->Layers.L[i].ClearColor.b, O->Layers.L[i].ClearColor.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        Emaths::v2 Scale = Emaths::v2(1.0f, 1.0f);

        float WAspect = (float)Render_W / Render_H;
        float FAspect = (float)Logical_W / Logical_H;

        if (WAspect > FAspect)
        {
            Scale.x = FAspect / WAspect;
        }
        else
        {
            Scale.y = WAspect / FAspect;
        }

        glViewport(0, 0, Render_W, Render_H);

        if (O->Layers.L[i].PP.on) // Post processing code
        {
            GLuint prog = O->Layers.L[i].PP.Program->glID;
            SR_Texture *orig_tex = SR_GetBufferText(O->Layers.L[i].PP.Origin);
            GLuint targ_fbo = SR_GetBufferFBO(O->Layers.L[i].PP.Target);

            SR_Target *TB = O->Layers.L[i].PP.Target;
            SR_Target *OB = O->Layers.L[i].PP.Origin;

            if (TB == nullptr)
                TB = OB;

            glDisable(GL_DEPTH_TEST);
            // glEnable(GL_DEPTH_TEST);

            glBindFramebuffer(GL_FRAMEBUFFER, targ_fbo);
            glUseProgram(prog);
            { // Passing Uniforms
                glUniform2f(0, Scale.x, Scale.y); //TODO: these Locations need to be reserved
                // glUniform1f(1, Logical_W);
                // glUniform1f(2, Logical_H);

                SR_Program *P = O->Layers.L[i].PP.Program;
                for (int u = 0; u < P->Uniforms.Count; u++)
                {
                    SR_Uniform *U = &P->Uniforms.U[u];
                    switch (U->Type)
                    {
                    case SR_UNIFORM_INT:
                        glUniform1iv(U->Location, U->Count, (const GLint *)U->Data);
                        break;
                    case SR_UNIFORM_UINT:
                        glUniform1uiv(U->Location, U->Count, (const GLuint *)U->Data);
                        break;
                    case SR_UNIFORM_FLOAT:
                        glUniform1fv(U->Location, U->Count, (const GLfloat *)U->Data);
                        break;
                    default:
                        break;
                    }
                }
            }
            GLint TextureUnifrom = glGetUniformLocation(prog, "Texture");
            glUniform1i(TextureUnifrom, 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, orig_tex->glID);

            glBindBuffer(GL_ARRAY_BUFFER, OB->VBO);
            for (int i = 0; i < 6; i++)
                TB->Rect.V[i].Z = O->z_index;
            O->z_index -= Z_SHIFT;
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectRects::glrect) * 1, &TB->Rect);
            glBindVertexArray(OB->VAO);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            drawcalls++;
            glEnable(GL_DEPTH_TEST);
        }
        else
        { // draw calls for regular shapes, filled, framed, and textured
            if (TexturedCount)
            {
                glUseProgram(O->Programs.P[SR_SHADER_TEXTURED].glID);
                glUniform2f(0, Scale.x, Scale.y);
                glUniform1f(1, Logical_W);
                glUniform1f(2, Logical_H);

                glActiveTexture(GL_TEXTURE0);
                SR_Texture *tex = O->Layers.L[i].texture;
                glBindTexture(GL_TEXTURE_2D, tex->glID);
                glBindVertexArray(O->TexRects.VAO);
                glBindBuffer(GL_ARRAY_BUFFER, O->TexRects.VBO);
                glDrawArrays(GL_TRIANGLES, O->TexRects.render_layer_index[i] * 6, TexturedCount * 6);
                drawcalls++;
            }

            if (FilledCount || FramedCount)
            {
                glUseProgram(O->Programs.P[SR_SHADER_COLORED].glID);
                glUniform2f(0, Scale.x, Scale.y);
                glUniform1f(1, Logical_W);
                glUniform1f(2, Logical_H);
            }
            if (FilledCount)
            {
                glBindVertexArray(O->Rects.VAO);
                glBindBuffer(GL_ARRAY_BUFFER, O->Rects.VBO);
                glDrawArrays(GL_TRIANGLES, O->Rects.render_layer_index[i] * 6, FilledCount * 6);
                drawcalls++;
            }
            if (FramedCount)
            {
                glBindVertexArray(O->Lines.VAO);
                glBindBuffer(GL_ARRAY_BUFFER, O->Lines.VBO);
                glDrawArrays(GL_LINES, O->Lines.render_layer_index[i] * 2, FramedCount * 2);
                drawcalls++;
            }
        }
        glDisable(GL_SCISSOR_TEST);
    }
    O->Debug.buffered_vertex_data_size = data;
    O->Debug.draw_calls = drawcalls;
}

SR_Font *SR_load_font_to_sprite(unsigned char *fontBuffer, int ASCII_start, int ASCII_end, int *sizes, int sizesCount)
{

    SR_Context *O = &simplyrend_context;

    // O->Fonts.F = (SR_Font *)realloc(O->Fonts.F, sizeof(SR_Font) * (O->Fonts.Count + 1));
    SR_Font *F = &O->Fonts.F[O->Fonts.Count];
    F->ID = O->Fonts.Count;

    O->Textures.Count++;
    O->Fonts.Count++;

    stbtt_InitFont(&F->Info, fontBuffer, 0);

    int CharCount = ASCII_end - ASCII_start;
    F->CharCount = CharCount;

    stbtt_pack_range *Ranges = (stbtt_pack_range *)malloc(sizeof(stbtt_pack_range) * sizesCount);
    F->sizes = (int *)malloc(sizeof(int) * sizesCount);
    for (int i = 0; i < sizesCount; i++)
    {
        Ranges[i].font_size = sizes[i];
        F->sizes[i] = sizes[i];
    }

    F->CharData = (stbtt_packedchar **)malloc(sizeof(stbtt_packedchar *) * CharCount * sizesCount);
    F->SizesCount = sizesCount;
    for (int i = 0; i < sizesCount; i++)
    {
        Ranges[i].first_unicode_codepoint_in_range = ASCII_start;
        Ranges[i].num_chars = CharCount + 1;
        Ranges[i].array_of_unicode_codepoints = nullptr;
        F->CharData[i] = (stbtt_packedchar *)malloc(sizeof(stbtt_packedchar) * (CharCount + 1));
        Ranges[i].chardata_for_range = F->CharData[i];
    }
    F->First = ASCII_start;

    int W = 2000, H = 2000;
    F->W = W;
    F->H = H;
    F->Pixels = (unsigned char *)malloc(W * H);
    stbtt_PackBegin(&F->PackContex, F->Pixels, W, H, 0, 1, NULL);
    stbtt_PackSetOversampling(&F->PackContex, 1, 1);
    stbtt_PackFontRanges(&F->PackContex, fontBuffer, 0, Ranges, sizesCount);
    stbtt_PackEnd(&F->PackContex);

    Uint8 *PixelsRGBA = (Uint8 *)calloc(W * H * 4, 1);
    for (int i = 0; i < W * H; ++i)
    {
        PixelsRGBA[i * 4 + 0] |= F->Pixels[i];
        PixelsRGBA[i * 4 + 1] |= F->Pixels[i];
        PixelsRGBA[i * 4 + 2] |= F->Pixels[i];
        PixelsRGBA[i * 4 + 3] |= F->Pixels[i];

        if (PixelsRGBA[i * 4] == 0)
            PixelsRGBA[i * 4 + 3] = 0;
    }

    F->Sprite = SR_LoadSprite(PixelsRGBA, W, H);

    return F;
}

/* Load a font from a file, this is called once per font
pass in '*sizes' an array of font sizes to account for, and pass 'sizesCont' as the size of the array
the 'ASCII_start' and  'ASCII_end' determine what range of ASCII letters you want the fonts to cover,
smaller range means smaller RAM and VRAM usage. */
SR_Font *SR_LoadFont(char *file, int ASCII_start, int ASCII_end, int *sizes, int sizesCount)
{
    int FileSize;
    unsigned char *fontBuffer = nullptr;

    FILE *fontFile = fopen(file, "rb");
    fseek(fontFile, 0, SEEK_END);
    FileSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    fontBuffer = (unsigned char *)malloc(FileSize);
    fread(fontBuffer, FileSize, 1, fontFile);
    fclose(fontFile);

    SR_Font *F = SR_load_font_to_sprite(fontBuffer, ASCII_start, ASCII_end, sizes, sizesCount);
    return F;
}

SR_Font *SR_LoadFontMemory(unsigned char *data, int ASCII_start, int ASCII_end, int *sizes, int sizesCount)
{
    return SR_load_font_to_sprite(data, ASCII_start, ASCII_end, sizes, sizesCount);
}

int SR_FindFontSizeIndex(SR_Font *Font, int size)
{
    SR_Context *O = &simplyrend_context;
    SR_Font *F = Font;

    int target = size;
    bool found = false;
    int result = 0;
    while (!found)
    {
        for (int i = 0; i < F->SizesCount; i++)
        {
            if (F->sizes[i] == size)
            {
                found = true;
                result = i;
            }
        }
        size--;
        size = Emaths::clamp(size, F->sizes[0], 100);
    }
    return result;
}
// Render a std::string of text to the screen using a font index (returned when you call SR_LoadFont())
// the size is ther desired size of the font, as per the defined list of sizes at SR_LoadFont(), if the passed size
// is not in the list, the next smaller size will be used.
void SR_PushText(SR_Font *Font, SR_Color Color, int size, SR_PointF Dest, char *Text, ...)
{
    char buff[256];
    va_list args;
    va_start(args, Text);
    vsnprintf(buff, 256, Text, args);
    va_end(args);

    SR_Context *O = &simplyrend_context;
    SR_Font *F = Font;
    SR_Sprite *S = F->Sprite;

    size = SR_FindFontSizeIndex(Font, size);

    int x = Dest.x, y = Dest.y;
    for (int i = 0; i < strlen(buff); i++)
    {
        SR_RectF Src;
        int I = buff[i] - F->First;
        Src.x = F->CharData[size][I].x0;
        Src.y = F->CharData[size][I].y0;
        Src.w = F->CharData[size][I].x1 - F->CharData[size][I].x0;
        Src.h = F->CharData[size][I].y1 - F->CharData[size][I].y0;

        SR_RectF Dst;
        Dst.x = x + F->CharData[size][I].xoff;
        Dst.y = y + F->CharData[size][I].yoff;
        Dst.w = F->CharData[size][I].x1 - F->CharData[size][I].x0;
        Dst.h = F->CharData[size][I].y1 - F->CharData[size][I].y0;

        SR_PushSpriteRectF(S, &Src, &Dst, 0, Color);

        x += F->CharData[size][I].xadvance;
    }
}

SR_Debug *SR_GetDebug()
{
    SR_Context *O = &simplyrend_context;
    return &O->Debug;
}

/*
    LICENSE: SimplyRend

    Copyright (c) 2022 Wassim Alhajomar / Mahmoud Wasim Alhaj Omar (@Wassimulator)

    This software is provided "as-is", without any express or implied warranty. In no event
    will the authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose, including commercial
    applications, and to alter it and redistribute it freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not claim that you
        wrote the original software. If you use this software in a product, an acknowledgment
        in the product documentation would be appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not be misrepresented
        as being the original software.

        3. This notice may not be removed or altered from any source distribution.

        4. Distribution in binary form must include the above copyright statement.
*/
