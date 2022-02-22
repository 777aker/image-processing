/*
HW06: Image processing

Kelley Kelley
CSCIx239 Spring 2022

Copied ex09

Key bindings:
  ESC exit
  - go back one mode
  + go forward one mode
  arrows change position
  pgup/pgdown zoom
  s/w change shadow multiplier for mode 3
*/

#include "CSCIx239.h"
#include <opencv2/videoio.hpp>
int shader; // shader
float asp = 1; // aspect ratio
float zoom = 1; // zoom level
float X = 0, Y = 0; // initial position
int mode = 3; // which mode to use
int maxmodes = 4; // modes keep expanding so making it more dynamic
// Images
int width, height; // original image size
unsigned char* rgba; // working array
int maxpasses = 5; // this is the number of textures and buffers I have
unsigned int img[5] = { 0,0,0,0,0 }; // image textures
unsigned int framebuf[5]; // frame buffers
// OpenCV camera
using namespace cv;
VideoCapture cam;
// so shadow effect effect
float shadows = 1.1;

// Capture images
int capture() {
    //  Capture image
    Mat frame;
    if (!cam.read(frame)) return 0;
    //  If it is not BGR color don't know what to do
    int k = frame.type();
    if (k != CV_8UC3) Fatal("Invalid frame type %d size %d %d channels %d\n", k, frame.cols, frame.rows, frame.channels());

    //  Extract data from frame
    int r = 2, g = 1, b = 0;
    unsigned char* dst = rgba;
    for (int i = 0; i < height; i++)
    {
        //  Frame origin is bottom left
        unsigned char* src = frame.data + frame.channels() * width * (height - 1 - i);
        for (int j = 0; j < width; j++)
        {
            *dst++ = src[r];
            *dst++ = src[g];
            *dst++ = src[b];
            *dst++ = 0xFF;
            src += frame.channels();
        }
    }

    //  Copy image to texture 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    ErrCheck("Capture");

    // first pass takes our original image and runs sobel
    glEnable(GL_TEXTURE_2D);
    glUseProgram(shader);
    // resolution uniforms
    int id = glGetUniformLocation(shader, "dX");
    glUniform1f(id, 1.0 / width);
    id = glGetUniformLocation(shader, "dY");
    glUniform1f(id, 1.0 / height);
    id = glGetUniformLocation(shader, "mode");
    if (mode != 4)
        glUniform1f(id, 0.0);
    else
        glUniform1f(id, 6.0);
    // identity projection
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // copy entire screen
    
    //  Input image
    id = glGetUniformLocation(shader, "img");
    glUniform1i(id, 0);
    //  Output to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuf[1]);
    //  Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    //  Redraw the screen
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(0, 1); glVertex2f(-1, +1);
    glTexCoord2f(1, 1); glVertex2f(+1, +1);
    glTexCoord2f(1, 0); glVertex2f(+1, -1);
    glEnd();

    // now we do our second pass which does various image processing stuff
    // 0 - subtracts sobel from image
    // 1 - adds sobel to image
    // 2 - vintage sobel
    // 3 - first pass uses sobel to average pixels only inside of our shape
    // ie it only averages pixels within it's edges
    id = glGetUniformLocation(shader, "mode");
    // I add 1 to mode bc technically mode is really which pass
    // we are on and mode, this is our second pass so add 1
    glUniform1f(id, 1.0+mode);
    id = glGetUniformLocation(shader, "img1");
    glUniform1i(id, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuf[2]);

    //  Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    //  Redraw the screen
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(0, 1); glVertex2f(-1, +1);
    glTexCoord2f(1, 1); glVertex2f(+1, +1);
    glTexCoord2f(1, 0); glVertex2f(+1, -1);
    glEnd();
    
    // mode 3 step one was averaging within edges
    // my final goal is a cartoony effect
    if (mode == 3) {
        // so mode 3 has a few more passes to make
        id = glGetUniformLocation(shader, "mode");
        // I know which mode we want to do hardcode case
        glUniform1f(id, 0);
        // this first pass will take our new image from the other passes
        // and sobel it
        id = glGetUniformLocation(shader, "img");
        glUniform1i(id, 2);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[3]);

        //  Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        //  Redraw the screen
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(0, 1); glVertex2f(-1, +1);
        glTexCoord2f(1, 1); glVertex2f(+1, +1);
        glTexCoord2f(1, 0); glVertex2f(+1, -1);
        glEnd();
       
        // now that we have edge combine new image with edge
        id = glGetUniformLocation(shader, "mode");
        // I know which mode we want to do hardcode case
        glUniform1f(id, 6);
        // this first pass will take our new image from the other passes
        // and laplace it
        id = glGetUniformLocation(shader, "img");
        glUniform1i(id, 2);
        id = glGetUniformLocation(shader, "img1");
        glUniform1i(id, 3);
        id = glGetUniformLocation(shader, "shadowmult");
        glUniform1f(id, shadows);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[4]);

        //  Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        //  Redraw the screen
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(0, 1); glVertex2f(-1, +1);
        glTexCoord2f(1, 1); glVertex2f(+1, +1);
        glTexCoord2f(1, 0); glVertex2f(+1, -1);
        glEnd();

        // at this point if you switch between mode 0 and 3 you can see
        // that I've just created a smoother edge detection with much less random
        // black spots and shadow multiplier makes weird effects I discovered
        // now I just want the lines to be a little dark so one lass sobel pass
        id = glGetUniformLocation(shader, "img");
        glUniform1i(id, 4);
        id = glGetUniformLocation(shader, "mode");
        glUniform1f(id, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[1]);
        //  Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        //  Redraw the screen
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(0, 1); glVertex2f(-1, +1);
        glTexCoord2f(1, 1); glVertex2f(+1, +1);
        glTexCoord2f(1, 0); glVertex2f(+1, -1);
        glEnd();

        // last pass combine sobel and image
        id = glGetUniformLocation(shader, "img");
        glUniform1i(id, 4);
        id = glGetUniformLocation(shader, "img1");
        glUniform1i(id, 2);
        id = glGetUniformLocation(shader, "mode");
        glUniform1f(id, 5);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[2]);
        //  Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        //  Redraw the screen
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(0, 1); glVertex2f(-1, +1);
        glTexCoord2f(1, 1); glVertex2f(+1, +1);
        glTexCoord2f(1, 0); glVertex2f(+1, -1);
        glEnd();
    }

    //  Disable textures and shaders
    glDisable(GL_TEXTURE_2D);
    glUseProgram(0);
    //  Output to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ErrCheck("Process");

    return 1;
}

// refresh display
void display(GLFWwindow* window) {
    //  Erase the window
    glClear(GL_COLOR_BUFFER_BIT);
    //  Window size
    int wid, hgt;
    glfwGetFramebufferSize(window, &wid, &hgt);
    //  Set projection and view
    glViewport(0, 0, wid, hgt);
    Projection(0, asp, 1);
    View(0, 0, 0, 0);

    //  Input image is from the last framebuffer
    glBindTexture(GL_TEXTURE_2D, img[2]);
    glClear(GL_COLOR_BUFFER_BIT);
    //  Redraw the screen
    float Casp = width / (float)height;
    glEnable(GL_TEXTURE_2D);
    glScaled(zoom, zoom, 1);
    glTranslated(X, Y, 0);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-Casp, -1);
    glTexCoord2f(0, 1); glVertex2f(-Casp, +1);
    glTexCoord2f(1, 1); glVertex2f(+Casp, +1);
    glTexCoord2f(1, 0); glVertex2f(+Casp, -1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    //  Display parameters
    SetColor(1, 1, 1);
    glWindowPos2i(5, 5);
    Print("Mode=%d,Shadows(mode 3 only)=%f", mode, shadows);
    //  Render the scene and make it visible
    ErrCheck("display");
    glFlush();
    glfwSwapBuffers(window);
}

// key pressed callback
void key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //  Discard key releases (keeps PRESS and REPEAT)
    if (action == GLFW_RELEASE) return;

    //  Check for shift
    //int shift = (mods & GLFW_MOD_SHIFT);

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, 1);
        break;
    case GLFW_KEY_KP_SUBTRACT:
    case GLFW_KEY_MINUS:
        if (mode == 0)
            mode = maxmodes-1;
        else
            mode -= 1;
        break;
    case GLFW_KEY_KP_ADD:
    case GLFW_KEY_EQUAL:
        mode = (mode + 1) % maxmodes;
        break;
    case GLFW_KEY_RIGHT:
        X -= 0.03 / zoom;
        break;
    case GLFW_KEY_LEFT:
        X += 0.03 / zoom;
        break;
    case GLFW_KEY_UP:
        Y -= 0.03 / zoom;
        break;
    case GLFW_KEY_DOWN:
        Y += 0.03 / zoom;
        break;
    case GLFW_KEY_PAGE_DOWN:
        zoom /= 1.1;
        break;
    case GLFW_KEY_PAGE_UP:
        zoom *= 1.1;
        break;
    case GLFW_KEY_S:
        shadows += .1;
        break;
    case GLFW_KEY_W:
        shadows -= .1;
        break;
    }

    if (zoom < 1) {
        zoom = 1;
        X = Y = 0;
    }
}

// window resize
void reshape(GLFWwindow* window, int width, int height) {
    //  Ratio of the width to the height of the window
    asp = (height > 0) ? (double)width / height : 1;
}

// main program with GLFW even loop
int main(int argc, char* argv[]) {
    // initialize GLFW
    GLFWwindow* window = InitWindow("HW06 Kelley Kelley", 1, 600, 600, &reshape, &key);

    // allow camera to be selected
    int kam = (argc > 1) ? strtod(argv[1], NULL) : 0;
    // intialize OpenCV
    if (!cam.open(kam)) Fatal("Could not initialize video source\n");

    // get image size
    width = cam.get(CAP_PROP_FRAME_WIDTH);
    height = cam.get(CAP_PROP_FRAME_HEIGHT);
    fprintf(stderr, "Video stream %dx%d\n", width, height);
    // working array
    rgba = (unsigned char*)malloc(4 * width * height);
    if (!rgba) Fatal("Cannot allocate %d bytes for image\n", 4 * width * height);
    // allocate frame buffers and textures
    glGenFramebuffers(maxpasses, framebuf);
    glGenTextures(maxpasses, img);
    for (int k = 0; k < maxpasses; k++) {
        // set texture parameters
        glActiveTexture(GL_TEXTURE0 + k);
        glBindTexture(GL_TEXTURE_2D, img[k]);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //  Bind frame buffer to texture
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[k]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img[k], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ErrCheck("Framebuffer");

    // load shader
    shader = CreateShaderProg(NULL, "hw06.frag");

    // event loop
    ErrCheck("init");
    while (!glfwWindowShouldClose(window)) {
        // capture image and display
        if (capture())
            display(window);
        // process events
        glfwPollEvents();
    }
    // shut down GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}