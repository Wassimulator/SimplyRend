  # SimplyRend
    by: Wassimulator
    
    ![alt text](https://raw.githubusercontent.com/Wassimulator/SimplyRend/master/simplyrend.jpg)

    Using OpenGL Version: 4.3
    A simple lightweight immediate mode renderer for 2D graphics, written in C and OpenGL.
    The approach is not the most efficient, but it's simple and it should work for small
    scale indie purposes. The goal behind this is to help get the program up and running
    quickly, while also setting up a foundation for expansion in features later.

    "I hate doing this twice", well this is why I made this.

## included libraries in package:
    1 - emaths.h        - v1.0  by: Wassimulator
    2 - stb_rect_pack   - v1.01 by: Sean Barrett
    3 - stb_image       - v2.26 by: Sean Barrett
    4 - stb_truetype    - v1.26 by: Sean Barrett

### Important notes:
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

## Usage:

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
