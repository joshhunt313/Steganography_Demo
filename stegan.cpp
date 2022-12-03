#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <GL/glut.h>
#include <math.h>

OIIO_NAMESPACE_USING
using namespace std;

int img_width = 500, img_height = 500, channels = 4;
unsigned char* pixmap;

ImageBuf readBuffer;
ImageBuf writeBuffer;


/**
 * GLUT display callback: displays current image in pixmap/buffer.
 * Displays based on the num of channels the image contains.
 */
void display()
{
    readBuffer.get_pixels(ROI::All(), TypeDesc::UINT8, pixmap);

    // Make a nice black background
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Fixes image shearing on images with odd width
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Handle # of channels
    switch (channels)
    {
    case 4:
        glDrawPixels(img_width, img_height, GL_RGBA, GL_UNSIGNED_BYTE, pixmap);
        break;
    case 3:
        glDrawPixels(img_width, img_height, GL_RGB, GL_UNSIGNED_BYTE, pixmap);
        break;
    case 1:
        glDrawPixels(img_width, img_height, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixmap);
        break;
    }
        

    glFlush();
}


/*
   Sets up the viewport and drawing coordinates
   This routine is called when the window is created and every time the window
   is resized, by the program or by the user
*/
void handleReshape(int w, int h)
{
    // set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // define the drawing coordinate system on the viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
}


void readImage(string inputName, string hiddenName)
{
    // If image can't be opened end program
    auto inp = ImageInput::open(inputName);
    if (!inp)
    {
        cout << "File not found. :(\n";
        exit(-1);
    }
    else
    {
        // Retrieve image specs
        const ImageSpec &spec = inp->spec();
        img_width = spec.width;
        img_height = spec.height;
        channels = spec.nchannels;

        pixmap = new unsigned char[4 * img_width * img_height * channels]();

        readBuffer = ImageBuf(inputName);
        readBuffer.read (0, 0, true, TypeDesc::FLOAT);
        readBuffer = ImageBufAlgo::flip(readBuffer);
    }
}

int main(int argc, char *argv[])
{
    cout << argv[1] << endl;
    if (argc != 3 && argc != 4) {
        cout << "Improper usage: ./stegan -e <input_image> <hidden_image>\n\t     OR ./stegan -d <input_image>\n";
        exit(-1);
    } else if (argv[1] == "-e") {
        // Encode image
        cout << "test\n";
    } else if (argv[1] == "-d") {
        // Decode image
    }

    // Start up the glut utilities
    glutInit(&argc, argv);

    // create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(img_width, img_height);
    glutCreateWindow("Convolution");

    // Callback routines
    glutDisplayFunc(display);       // display callback
    // glutKeyboardFunc(handleKey); 
    glutReshapeFunc(handleReshape); // window resize callback

    glutMainLoop();

    return 0;
}
