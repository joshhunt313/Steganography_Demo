#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <GL/glut.h>
#include <math.h>
#include <cstddef>
#include <typeinfo>

#define HEADER_SIZE sizeof(int) * 8

OIIO_NAMESPACE_USING
using namespace std;

int img_width = 500, img_height = 500, channels = 4;
int hidden_width;
int hidden_height;
int hidden_channels;

unsigned char* pixmap;
unsigned char* hidden_pixmap;

ImageBuf readBuffer;
ImageBuf hiddenBuffer;
ImageBuf writeBuffer;


/**
 * GLUT display callback: displays current image in pixmap/buffer.
 * Displays based on the num of channels the image contains.
 */
void display()
{
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
    // Open input image and store
    auto inp = ImageInput::open(inputName);
    auto inp2 = ImageInput::open(hiddenName);

    if (!inp || !inp2)
    {
        cout << "File not found. :(\n";
        exit(-1);
    }
    else
    {
        // Retrieve input image specs
        const ImageSpec &spec = inp->spec();
        img_width = spec.width;
        img_height = spec.height;
        channels = spec.nchannels;
        cout << channels << endl;

        pixmap = new unsigned char[4 * img_width * img_height * channels]();

        readBuffer = ImageBuf(inputName);
        readBuffer.read (0, 0, true, TypeDesc::UINT8);
        readBuffer = ImageBufAlgo::flip(readBuffer);

        readBuffer.get_pixels(ROI::All(), TypeDesc::UINT8, pixmap);

        // ROI newReadROI (0, img_width*2, 0, img_height*2, 0, 1, channels);
        // readBuffer = ImageBufAlgo::resize(readBuffer, "", 0, newReadROI);
        // img_width *= 2;
        // img_height *= 2;

        const ImageSpec &h_spec = inp2->spec();
        hidden_width = h_spec.width;
        hidden_height = h_spec.height;
        hidden_channels = h_spec.nchannels;
        cout << hidden_channels << endl;

        hiddenBuffer = ImageBuf(hiddenName);
        hiddenBuffer.read (0, 0, true, TypeDesc::UINT8);
        hiddenBuffer = ImageBufAlgo::flip(hiddenBuffer);

        // Resize hidden image to make smaller to more easily fit in other image
        // int channelsToRead = 3;

        ROI newROI (0, hidden_width/4, 0, hidden_height/4, 0, 1, hidden_channels);
        hiddenBuffer = ImageBufAlgo::resize(hiddenBuffer, "", 0, newROI);
        hidden_width /= 4;
        hidden_height /= 4;

        // Iterate over input image and replace values with hidden pixel values

        hidden_pixmap = new unsigned char[4 * hidden_width * hidden_height * hidden_channels]();
        hiddenBuffer.get_pixels(ROI::All(), TypeDesc::UINT8, hidden_pixmap);

        // int h_index = 0;
        // int i = 0;
        // for (i = 0; (i < img_width * img_height * channels) && (h_index < hidden_width * hidden_height * hidden_channels); i += channels-1){
        //     // cout << pixmap[i] << endl;
        //     pixmap[i] = (pixmap[i] & 0xFC) | (hidden_pixmap[h_index]);
        //     if ((i + 1) % channels == 0) h_index++;
        //     if ((i + 3 >= img_width * img_height * channels) && (h_index < hidden_width * hidden_height * hidden_channels)) cout << "RAN OUT OF SPACE\n";
        // }

        int input_pix_len = sizeof(unsigned char) * 4 * img_width * img_height * channels;
        int hidden_pix_len = sizeof(unsigned char) * 4 * hidden_width * hidden_height * hidden_channels;

        // Encode hidden length at the beginning of input image
        for (int i = 0; i < HEADER_SIZE; i++) {
            pixmap[i] &= 0xFC;
            pixmap[i] |= (input_pix_len >> (HEADER_SIZE - 1 - i)) & 1UL;
        }

        // Encode hidden image
        for (int i = 0; i < hidden_pix_len; i++) {
            pixmap[i+HEADER_SIZE] &= 0xFC;
            pixmap[i+HEADER_SIZE] |= (hidden_pixmap[i/4] >> ((hidden_pix_len - 1 - i) % 4)) & 1;
        }

        // cout << h_index << " : " << i << endl;
    }
}


int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        cout << "Improper usage: ./stegan -e <input_image> <hidden_image>\n\t     OR ./stegan -d <input_image>\n";
        exit(-1);
    } else if (strcmp(argv[1], "-e") == 0) {
        // Encode image

        if (argc == 3) {
            cout << "Improper usage: ./stegan -e <input_image> <hidden_image>\n";
        } else {
            readImage(argv[2], argv[3]);
        }
    } else if (strcmp(argv[1], "-d") == 0) {
        // Decode image

        if (argc != 3) {
            cout << "Improper usage: ./stegan -d <input_image>\n";
        } else {

        }
    }

    // Start up the glut utilities
    glutInit(&argc, argv);

    // create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(img_width, img_height);
    glutCreateWindow("Steganography");

    // Callback routines
    glutDisplayFunc(display);       // display callback
    // glutKeyboardFunc(handleKey); 
    glutReshapeFunc(handleReshape); // window resize callback

    glutMainLoop();

    return 0;
}
