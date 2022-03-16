/*
    ## SimplyRend ##
        v1.1
    by: Wassimulator

    Using OpenGL Version: 4.3
    A simple lightweight immediate mode renderer for 2D graphics, written in C and OpenGL.
    The approach is not the most efficient, but it's simple and it should work for small
    scale indie purposes. The goal behind this is to help get the program up and running
    quickly, while also setting up a foundation for expansion in features later.

    "I hate doing this twice", well this is why I made this.

# External Dependencies:
    - GLAD: default C/C++ glad library, use this permalink:
        https://glad.dav1d.de/#language=c&specification=gl&api=gl%3D4.3&api=gles1%3Dnone&api=gles2%3Dnone&api=glsc2%3Dnone&profile=compatibility&loader=on
# included libraries in package:
    1 - emaths.h        - v1.0 or higher
    2 - stb_rect_pack   - v1.01 or higher
    3 - stb_image       - v2.26 or higher
    4 - stb_truetype    - v1.26 or higher

# Important notes:
    - #include "glad.c" in your solution before this.
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
        - SR_PushRenderLayer()          // starts a new render layer
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

///////////////////////////////////////////////////////////////////////////////
// Data structures:

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
    unsigned long int texture_id;
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
    int32_t Z;
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

typedef struct SR_RenderLayers
{
    struct render_layer
    {
        struct scissor
        {
            bool on;
            SR_Rect rect;
        };
        scissor Clip;
        GLuint *Shader;
        int texture_ID;
    };

    int last_tex_id = -1;

    render_layer *L;
    int count;
    int index;
} SR_RenderLayers;

typedef struct SR_Uniform
{
    char *Name;
    GLuint Location;
    unsigned long int ID;
} SR_Uniform;

typedef struct SR_Uniforms
{
    SR_Uniform *U;
    int Count;
} SR_Uniforms;

typedef struct SR_Shader
{
    unsigned long int ID;
    GLuint Program;
    GLuint Vertex;
    GLuint Fragment;
    SR_Uniforms Uniforms;
} SR_Shader;

typedef struct SR_Shaders
{
    SR_Shader *S;
    unsigned long int Count;
} SR_Shaders;

typedef struct SR_Texture
{
    GLuint glID;
    unsigned long int ID;
} SR_Texture;

typedef struct SR_Textures
{
    SR_Texture *T;
    unsigned long int Count;
} SR_Textures;

typedef struct SR_Font
{
    unsigned long int ID;
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
    unsigned long int Count;
} SR_Fonts;

typedef struct SR_Context
{
    unsigned long int max_texture_dimension; // corresponds to, but is not necessarily equal to: GL_MAX_TEXTURE_SIZE

    SR_ObjectRects Rects;
    SR_ObjectRects TexRects;
    SR_ObjectLines Lines;
    SR_RenderLayers Layers;
    SR_Shaders Shaders;
    SR_Textures Textures;
    SR_Fonts Fonts;

    SR_Sprites Sprites;

    int FrameWidth;
    int FrameHeight;

    int WindowWidth;
    int WindowHeight;

    int z_index = 0;

    SR_Color DrawColor;

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
// since this uses OpenGL, the z order is important and pushing the render layer guarantees a seperate draw call from that point forward
void SR_PushRenderLayer()
{
    // if (G->OpenGL.render_layer_index < MAX_RENDER_LAYERS)
    SR_Context *O = &simplyrend_context;
    O->Layers.index++;
    int layer = O->Layers.index;
    if (O->Layers.index >= O->Layers.count)
    {
        O->Layers.count++;
        O->Layers.L = (SR_RenderLayers::render_layer *)realloc(O->Layers.L, O->Layers.count * sizeof(SR_RenderLayers::render_layer));
        O->Rects.render_layer_index = (int *)realloc(O->Rects.render_layer_index, O->Layers.count * sizeof(int));
        O->TexRects.render_layer_index = (int *)realloc(O->TexRects.render_layer_index, O->Layers.count * sizeof(int));
        O->Lines.render_layer_index = (int *)realloc(O->Lines.render_layer_index, O->Layers.count * sizeof(int));
    }
    O->Layers.last_tex_id = -1;

    O->Rects.render_layer_index[layer] = O->Rects.Count;
    O->TexRects.render_layer_index[layer] = O->TexRects.Count;
    O->Lines.render_layer_index[layer] = O->Lines.Count;
}

// call this at the beginning of every frame within the program loop, you may change the window width and height
// the change should reflect window resizing in the window context controlled by you outside this library
void SR_StartFrame(int WindowWidth, int WindowHeight)
{
    SR_Context *O = &simplyrend_context;
    SR_PushRenderLayer();
    O->Layers.index = 0;

    O->WindowWidth = WindowWidth;
    O->WindowHeight = WindowHeight;

    O->Rects.Count = 0;
    O->TexRects.Count = 0;
    O->Lines.Count = 0;
    O->z_index = 1 << 15;
    O->Layers.index = 0;
    O->Layers.last_tex_id = -1;

    for (int i = 0; i < O->Layers.count; i++)
    {
        O->Rects.render_layer_index[i] = 0;
        O->TexRects.render_layer_index[i] = 0;
        O->Lines.render_layer_index[i] = 0;
        O->Layers.L[i].Clip.on = false;
    }
}

// creates and pushes a new render program onto the array heap of programs:
unsigned long int SR_CreateProgram(const char *VertexCode, const char *FragmentCode)
{
    SR_Context *O = &simplyrend_context;
    int result = -1;

    O->Shaders.S = (SR_Shader *)realloc(O->Shaders.S, sizeof(SR_Shader) * (O->Shaders.Count + 1));
    SR_Shader *S = &O->Shaders.S[O->Shaders.Count];
    int id = O->Shaders.Count;
    O->Shaders.Count++;

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
            cerr << "Error (Shader):" << Buffer << endl;
            error = true;
        }
        glGetShaderInfoLog(VertexShader, 4096, &BufferLength, Buffer);
        if (BufferLength > 0)
        {
            cerr << "Error (Shader):" << Buffer << endl;
            error = true;
        }
        glGetProgramInfoLog(Pipeline, 4096, &BufferLength, Buffer);
        if (BufferLength > 0)
        {
            cerr << "Error(Program):" << Buffer << endl;
            error = true;
        }
    }
    if (!error)
    {
        S->Program = Pipeline;
        S->Vertex = VertexShader;
        S->Fragment = FragmentShader;
        S->ID = id;
        result = S->ID;
    }
    else
    {
        O->Shaders.Count--;
    }

    return result;
}

/* Initializes the OpenGL context and the relevant data structures for SimplyRend,
 - MaxRects: Assign maximum number of rectangles extimated to be used as an upper bound,
 The larger MaxRects, the larger the RAM usage.
 - FrameWidth and FrameHeight are the dimensions of the framebuffer, these should be constant. */
void SR_Init(unsigned long long int MaxRects, int FrameWidth, int FrameHeight)
{
    gladLoadGL();
    memset(&simplyrend_context, 0, sizeof(SR_Context));
    SR_Context *O = &simplyrend_context;

    O->FrameWidth = FrameWidth;
    O->FrameHeight = FrameHeight;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // allocate memory for the vertex buffers:
    O->Rects.R = (SR_or::glrect *)malloc(sizeof(SR_or::glrect) * MaxRects);
    O->TexRects.R = (SR_or::glrect *)malloc(sizeof(SR_or::glrect) * MaxRects);
    O->Lines.L = (SR_ol::glline *)malloc(sizeof(SR_ol::glline) * MaxRects);
    O->Sprites.S = (SR_Sprite *)realloc(O->Sprites.S, sizeof(SR_Sprite) * (MaxRects));

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
    glVertexAttribPointer(2, 1, GL_INT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
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
    glVertexAttribPointer(2, 1, GL_INT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
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
    glVertexAttribPointer(2, 1, GL_INT, GL_TRUE, sizeof(SR_Vertex), (void *)offsetof(SR_Vertex, Z));
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
            void main()
            {   
				gl_Position = vec4(Position.xy, z_order, 1.0);
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
            void main()
            {
				TexMod = TexModColor;
                gl_Position = vec4(vertex.xy, z_order, 1.0);
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
				vec2 uvActual = uv_aa_smoothstep(TexCoords, textureSize(Sampler, 0),1.5);
				color = texture(Sampler,   uvActual) * TexMod;
            }
    )###"};
    SR_CreateProgram(VertexShaders[0], FragmentShaders[0]); // 1st program for colored rendering
    SR_CreateProgram(VertexShaders[1], FragmentShaders[1]); // 2nd program for textured rendering
}

unsigned long int SR_GenerateUniform(unsigned long int shader_id, char *Name)
{
    unsigned long int result = -1;
    SR_Context *O = &simplyrend_context;
    SR_Shader *S = &O->Shaders.S[shader_id];

    S->Uniforms.U = (SR_Uniform *)realloc(S->Uniforms.U, sizeof(SR_Uniform) * (S->Uniforms.Count + 1));
    SR_Uniform *U = &S->Uniforms.U[S->Uniforms.Count];
    unsigned long int id = S->Uniforms.Count;
    S->Uniforms.Count++;

    GLuint Loc = glGetUniformLocation(S->Program, U->Name);
    bool error = SR_GL_error();
    if (!error)
    {
        U->Location = Loc;
        U->Name = (char *)malloc(strlen(Name) + 1);
        strcpy(U->Name, Name);
        U->ID = id;
        result = U->ID;
    }
    else
    {
        S->Uniforms.Count--;
    }
    return result;
}
/*
Load sprite, loads an image file into a sprite that will be added to a texture atlas you create
it could be animated with custom frames (WOP), it could also be loaded from memory. */
SR_Sprite *SR_LoadSprite(char *filename, bool animated, int frames)
{
    SR_Context *O = &simplyrend_context;
    SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
    unsigned char *Pixels = stbi_load(filename, &Result->w, &Result->h, &Result->n, STBI_rgb_alpha);

    if (Pixels == NULL)
    {
        cerr << "Error loading image: " << filename << endl;
    }
    else
    {

        O->Sprites.S = (SR_Sprite *)realloc(O->Sprites.S, sizeof(SR_Sprite) * (O->Sprites.Count + 1));
        SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
        unsigned long int id = O->Sprites.Count;
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
    // cout << stbi_failure_reason() << endl;

    if (Pixels == NULL)
    {
        cerr << "Error loading image: " << filename << endl;
    }
    else
    {

        SR_Sprite *Result = &O->Sprites.S[O->Sprites.Count];
        unsigned long int id = O->Sprites.Count;
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
SR_Sprite *SR_LoadSprite(unsigned char *Data, int w, int h)
{
    SR_Context *O = &simplyrend_context;
    O->Sprites.S = (SR_Sprite *)realloc(O->Sprites.S, sizeof(SR_Sprite) * (O->Sprites.Count + 1));
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

// generates a new texture, pushes it into the texture array on the heap, and returns its ID
unsigned long int SR_GenerateTextureAtlas(stbrp_context *context, stbrp_rect *rects, int rectsnum)
{
    SR_Context *O = &simplyrend_context;
    O->Textures.T = (SR_Texture *)realloc(O->Textures.T, sizeof(SR_Texture) * (O->Textures.Count + 1));
    SR_Texture *Target_texture = &O->Textures.T[O->Textures.Count];
    unsigned long int ID = O->Textures.Count;
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(PixelsRGBA);
    return ID;
}

unsigned long int SR_GetCurrentSpriteIndex()
{
    SR_Context *O = &simplyrend_context;
    return O->Sprites.Count;
}
// packs sprites from from_index to (i < from_index + count) into one texture
// and returns texture ID. NOT GLuint id, but the id within the sr context.
// if count is -1 it will pack all sprites from from_index to the end of the array
unsigned long int SR_PackSpritesToTexture(int from_index, int count)
{
    SR_Context *O = &simplyrend_context;
    SR_Sprite *S = O->Sprites.S;

    GLint MAX = 8192;
    GLint MAX_GPU = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MAX_GPU);
    MAX = min(MAX, MAX_GPU);
    O->max_texture_dimension = MAX;

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
    unsigned long int texture_id = SR_GenerateTextureAtlas(&context, rects, COUNT);

    for (int i = 0; i < COUNT; i++)
    {
        S[rects[i].id].texture_id = texture_id;
        S[rects[i].id].x = rects[i].x;
        S[rects[i].id].y = rects[i].y;
        printf("Sprite %d packed to texture %d at %d,%d\n", rects[i].id, texture_id, rects[i].x, rects[i].y);
    }

    free(rects);
    free(nodes);
    for (int i = from_index; i < end; i++)
        free(O->Sprites.S[i].PixelsRGBA);

    return texture_id;
}

Emaths::v2 SR_PixelToGL(Emaths::v2 Point)
{
    SR_Context *O = &simplyrend_context;

    float Resol = O->WindowWidth / O->FrameWidth;
    Emaths::v2 Window = Emaths::v2(O->WindowWidth, O->WindowHeight);
    float Aspect = O->FrameWidth / O->FrameHeight;

    Emaths::v2 Res = 2 * Resol * Point / Window - 1;
    Res.y *= -Aspect;
    return Res;
}

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
    float WW = O->FrameWidth * 0.5;
    float WH = O->FrameHeight * 0.5;
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
        RO->R[RO->Count].V[i].P = SR_PixelToGL(RO->R[RO->Count].V[i].P);
        RO->R[RO->Count].V[i].C.r = r;
        RO->R[RO->Count].V[i].C.g = g;
        RO->R[RO->Count].V[i].C.b = b;
        RO->R[RO->Count].V[i].C.a = a;

        RO->R[RO->Count].V[i].Z = O->z_index;
    }
    O->z_index -= (1 << 9);
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
            RO->L[RO->Count + i].V[j].P = SR_PixelToGL(RO->L[RO->Count + i].V[j].P);
            RO->L[RO->Count + i].V[j].C.r = r;
            RO->L[RO->Count + i].V[j].C.g = g;
            RO->L[RO->Count + i].V[j].C.b = b;
            RO->L[RO->Count + i].V[j].C.a = a;

            RO->L[RO->Count + i].V[j].Z = O->z_index;
        }

    O->z_index -= (1 << 9);
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
        w = O->WindowWidth;
        h = O->WindowHeight;
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
        ROT->R[ROT->Count].V[i].P = SR_PixelToGL(ROT->R[ROT->Count].V[i].P);
        ROT->R[ROT->Count].V[i].C.r = r;
        ROT->R[ROT->Count].V[i].C.g = g;
        ROT->R[ROT->Count].V[i].C.b = b;
        ROT->R[ROT->Count].V[i].C.a = a;

        ROT->R[ROT->Count].V[i].Z = O->z_index;
    }

    O->z_index -= (1 << 9);

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
        w = O->WindowWidth;
        h = O->WindowHeight;
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
        ROT->R[ROT->Count].V[i].P = SR_PixelToGL(ROT->R[ROT->Count].V[i].P);
        ROT->R[ROT->Count].V[i].C.r = r;
        ROT->R[ROT->Count].V[i].C.g = g;
        ROT->R[ROT->Count].V[i].C.b = b;
        ROT->R[ROT->Count].V[i].C.a = a;

        ROT->R[ROT->Count].V[i].Z = O->z_index;
    }

    O->z_index -= (1 << 9);

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

    RO->L[RO->Count].V[0].P = SR_PixelToGL(Emaths::v2(x1, y1));
    RO->L[RO->Count].V[1].P = SR_PixelToGL(Emaths::v2(x2, y2));
    for (int j = 0; j < 2; j++)
    {
        RO->L[RO->Count].V[j].C.r = r;
        RO->L[RO->Count].V[j].C.g = g;
        RO->L[RO->Count].V[j].C.b = b;
        RO->L[RO->Count].V[j].C.a = a;

        RO->L[RO->Count].V[j].Z = O->z_index;
    }

    O->z_index -= (1 << 9);
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

        RO->L[RO->Count].V[0].P = SR_PixelToGL(Emaths::v2(x1, y1));
        RO->L[RO->Count].V[1].P = SR_PixelToGL(Emaths::v2(x2, y2));
        for (int j = 0; j < 2; j++)
        {
            RO->L[RO->Count].V[j].C.r = r;
            RO->L[RO->Count].V[j].C.g = g;
            RO->L[RO->Count].V[j].C.b = b;
            RO->L[RO->Count].V[j].C.a = a;

            RO->L[RO->Count].V[j].Z = O->z_index;
        }
        O->z_index -= (1 << 9);
        RO->Count++;
    }
}

void SR_SpritePre(SR_Sprite *Sprite)
{
    SR_Context *O = &simplyrend_context;
    if (Sprite->texture_id != O->Layers.last_tex_id)
    {
        if (O->Layers.last_tex_id != -1)
            SR_PushRenderLayer();
        O->Layers.L[O->Layers.index].texture_ID = Sprite->texture_id;
    }
    O->Layers.last_tex_id = Sprite->texture_id;
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

//////////////// FRAME /////////////////////////////
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

void SR_Clear()
{
    SR_Context *O = &simplyrend_context;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(O->DrawColor.r, O->DrawColor.g, O->DrawColor.b, O->DrawColor.a);
}
void SR_Clear(SR_Color Color)
{
    SR_Context *O = &simplyrend_context;
    glClearColor(Color.r, Color.g, Color.b, Color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void SR_Clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SR_Clip(const SR_Rect *rect)
{
    SR_Context *O = &simplyrend_context;
    SR_PushRenderLayer();
    O->Layers.L[O->Layers.index].Clip.rect = *rect;
    O->Layers.L[O->Layers.index].Clip.on = true;
}
void SR_EndClip()
{
    SR_PushRenderLayer();
}

void SR_Render()
{
    SR_Context *O = &simplyrend_context;
    SR_PushRenderLayer();

    glBindBuffer(GL_ARRAY_BUFFER, O->Rects.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectRects::glrect) * O->Rects.Count, O->Rects.R);
    glBindBuffer(GL_ARRAY_BUFFER, O->Lines.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectLines::glline) * O->Lines.Count, O->Lines.L);
    glBindBuffer(GL_ARRAY_BUFFER, O->TexRects.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SR_ObjectRects::glrect) * O->TexRects.Count, O->TexRects.R);

    int count = 0;
    int data = sizeof(SR_ObjectRects::glrect) * O->Rects.Count +
               sizeof(SR_ObjectLines::glline) * O->Lines.Count +
               sizeof(SR_ObjectRects::glrect) * O->TexRects.Count;

    for (int i = 0; i < O->Layers.index; i++)
    {
        glDisable(GL_SCISSOR_TEST);

        int FilledCount = O->Rects.render_layer_index[i + 1] - O->Rects.render_layer_index[i];
        int FramedCount = O->Lines.render_layer_index[i + 1] - O->Lines.render_layer_index[i];
        int TexturedCount = O->TexRects.render_layer_index[i + 1] - O->TexRects.render_layer_index[i];

        if (O->Layers.L[i].Clip.on)
        {
            glEnable(GL_SCISSOR_TEST);
            float scaleratio = (float)O->WindowWidth / O->FrameWidth;
            float x = O->Layers.L[i].Clip.rect.x * scaleratio;
            float y = (O->FrameHeight - O->Layers.L[i].Clip.rect.h - O->Layers.L[i].Clip.rect.y) * scaleratio;
            float w = O->Layers.L[i].Clip.rect.w * scaleratio;
            float h = O->Layers.L[i].Clip.rect.h * scaleratio;

            glScissor(x, y, w, h);
        }

        if (TexturedCount)
        {
            glUseProgram(O->Shaders.S[SR_SHADER_TEXTURED].Program);
            glActiveTexture(GL_TEXTURE0);
            int texid = O->Layers.L[i].texture_ID;
            glBindTexture(GL_TEXTURE_2D, O->Textures.T[texid].glID);
            glBindVertexArray(O->TexRects.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, O->TexRects.VBO);
            glDrawArrays(GL_TRIANGLES, O->TexRects.render_layer_index[i] * 6, TexturedCount * 6);
            count++;
        }

        if (FilledCount || FramedCount)
            glUseProgram(O->Shaders.S[SR_SHADER_COLORED].Program);
        if (FilledCount)
        {
            glBindVertexArray(O->Rects.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, O->Rects.VBO);
            glDrawArrays(GL_TRIANGLES, O->Rects.render_layer_index[i] * 6, FilledCount * 6);
            count++;
        }
        if (FramedCount)
        {
            glBindVertexArray(O->Lines.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, O->Lines.VBO);
            glDrawArrays(GL_LINES, O->Lines.render_layer_index[i] * 2, FramedCount * 2);
            count++;
        }
    }
}
/* Load a font from a file, this is called once per font
pass in '*sizes' an array of font sizes to account for, and pass 'sizesCont' as the size of the array
the 'ASCII_start' and  'ASCII_end' determine what range of ASCII letters you want the fonts to cover,
smaller range means smaller RAM and VRAM usage. */
unsigned long int SR_LoadFont(char *file, int ASCII_start, int ASCII_end, int *sizes, int sizesCount)
{
    long FileSize;
    unsigned char *fontBuffer;

    FILE *fontFile = fopen(file, "rb");
    fseek(fontFile, 0, SEEK_END);
    FileSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    fontBuffer = (unsigned char *)malloc(FileSize);
    fread(fontBuffer, FileSize, 1, fontFile);
    fclose(fontFile);

    SR_Context *O = &simplyrend_context;

    O->Fonts.F = (SR_Font *)realloc(O->Fonts.F, sizeof(SR_Font) * (O->Fonts.Count + 1));
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

    int W = 750, H = 750;
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

    return F->ID;
}

int SR_FindFontSizeIndex(unsigned long int FontID, int size)
{
    SR_Context *O = &simplyrend_context;
    SR_Font *F = &O->Fonts.F[FontID];

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
// Render a string of text to the screen using a font index (returned when you call SR_LoadFont())
// the size is ther desired size of the font, as per the defined list of sizes at SR_LoadFont(), if the passed size 
// is not in the list, the next smaller size will be used.
void SR_PushText(unsigned long int FontID, SR_Color Color, int size, SR_PointF Dest, char *Text, ...)
{
    char buff[256];
    va_list args;
    va_start(args, Text);
    vsnprintf(buff, 256, Text, args);
    va_end(args);
    string String = buff;
    char *cstr = new char[String.length() + 1];
    strcpy(cstr, String.c_str());

    SR_Context *O = &simplyrend_context;
    SR_Font *F = &O->Fonts.F[FontID];
    SR_Sprite *S = F->Sprite;

    size = SR_FindFontSizeIndex(FontID, size);

    int x = Dest.x, y = Dest.y;
    for (int i = 0; i < strlen(cstr); i++)
    {
        SR_RectF Src;
        int I = cstr[i] - F->First;
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
    delete[] cstr;
}

/*
MIT License
Copyright (c) 2017 Wassim Alhajomar / Mahmoud Wasim Alhaj Omar. @Wassimulator.
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/