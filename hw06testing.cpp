/*
HW06: Image processing

Kelley Kelley
CSCIx239 Spring 2022

Copied ex09

Key bindings:
*/

#include "CSCIx239.h"
#include <opencv2/videoio.hpp>
int shader; // shader
int N = 1; // number of passes
float asp = 1; // aspect ratio
float zoom = 1; // zoom level
float X = 0, Y = 0; // initial position
// Images
int width, height; // original image size
unsigned char* rgba; // working array
unsigned int img[2] = { 0,0 }; // image textures
unsigned int framebuf[2]; // frame buffers
// OpenCV camera
using namespace cv;
VideoCapture cam;

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
    glBindTexture(GL_TEXTURE_2D, img[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    ErrCheck("Capture");

    // ping-pong between framebuffers
    glEnable(GL_TEXTURE_2D);
    glUseProgram(shader);
    // resolution uniforms
    int id = glGetUniformLocation(shader, "dX");
    glUniform1f(id, 1.0 / width);
    id = glGetUniformLocation(shader, "dY");
    glUniform1f(id, 1.0 / height);
    // identity projection
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // copy entire screen
    for (int i = 0; i < N; i++) {
        //  Input image is from the last framebuffer
        glBindTexture(GL_TEXTURE_2D, img[i % 2]);
        //  Output to alternate framebuffers
        glBindFramebuffer(GL_FRAMEBUFFER, framebuf[(i + 1) % 2]);
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
    glBindTexture(GL_TEXTURE_2D, img[N % 2]);
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
    Print("Passes=%d", N);
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
        if (N > 1) N--;
        break;
    case GLFW_KEY_KP_ADD:
    case GLFW_KEY_EQUAL:
        N++;
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
    glGenFramebuffers(2, framebuf);
    glGenTextures(2, img);
    for (int k = 0; k < 2; k++) {
        // set texture parameters
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