#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <GL/glut.h>

#define HEADER_SIZE 32

OIIO_NAMESPACE_USING
using namespace std;

int img_width = 500, img_height = 500, channels = 4;
int hidden_width = 500, hidden_height = 500, hidden_channels = 4;

unsigned char* pixmap;
unsigned char* hidden_pixmap;
unsigned char* test;

ImageBuf readBuffer;
ImageBuf hiddenBuffer;
// ImageBuf writeBuffer;


/**
 * GLUT display callback: displays current image in pixmap/buffer.
 * Displays based on the num of channels the image contains.
 */
void display()
{
    // Make a black background
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Fixes image shearing on images with odd width
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Handle # of channels
    switch (channels)
    {
    case 4:
        glDrawPixels(img_width, img_height, GL_RGBA, GL_UNSIGNED_BYTE, pixmap);
        // glDrawPixels(hidden_width, hidden_height, GL_RGBA, GL_UNSIGNED_BYTE, hidden_pixmap);
        break;
    case 3:
        glDrawPixels(img_width, img_height, GL_RGB, GL_UNSIGNED_BYTE, pixmap);
        // glDrawPixels(hidden_width, hidden_height, GL_RGB, GL_UNSIGNED_BYTE, hidden_pixmap);
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


void writeImage(unsigned char *p_map, string outputName, ImageSpec spec)
{
    ImageBuf writeBuffer = ImageBuf(spec, p_map);
    writeBuffer = ImageBufAlgo::flip(writeBuffer);
    writeBuffer.write(outputName, TypeDesc::UINT8);
}

void readImage(string inputName)
{
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
        cout << img_width << " x " << img_height << " : " << channels << endl;

        pixmap = new unsigned char[4 * img_width * img_height * channels]();

        readBuffer = ImageBuf(inputName);
        readBuffer.read(0, 0, true, TypeDesc::UINT8);

        // Eliminate alpha channel if is one
        readBuffer = ImageBufAlgo::channels(readBuffer, 3, {});
        channels = 3;

        readBuffer = ImageBufAlgo::flip(readBuffer);

        readBuffer.get_pixels(ROI::All(), TypeDesc::UINT8, pixmap);
    }
}


void decodeImage(unsigned char *buffer) 
{
    unsigned int len = 0;
	for(int i = 0; i < HEADER_SIZE; i++) {
		len = (len << 2) | (pixmap[i] & 3);
	}

    cout << "len: " << len << endl; 

	for(unsigned int i = 0; i < len; i++) {
		buffer[i/4] = (buffer[i/4] << 2) | (pixmap[i + HEADER_SIZE] & 3);
	}
}


void encodeImage(string inputName, string hiddenName)
{
    // Open input image and store
    auto inp2 = ImageInput::open(hiddenName);

    if (!inp2)
    {
        cout << "File not found. :(\n";
        exit(-1);
    }
    else
    {
        // Hidden image init
        const ImageSpec &h_spec = inp2->spec();
        hidden_width = h_spec.width;
        hidden_height = h_spec.height;
        hidden_channels = h_spec.nchannels;

        hiddenBuffer = ImageBuf(hiddenName);
        hiddenBuffer.read (0, 0, true, TypeDesc::UINT8);
        hiddenBuffer = ImageBufAlgo::flip(hiddenBuffer);

        // Eliminate alpha channel if is one
        hiddenBuffer = ImageBufAlgo::channels(hiddenBuffer, 3, {});
        hidden_channels = 3;

        // Resize hidden image to make smaller to more easily fit in other image
        hidden_width /= 4;
        hidden_height /= 4;
        ROI newROI (0, hidden_width, 0, hidden_height, 0, 1, hidden_channels);
        hiddenBuffer = ImageBufAlgo::fit(hiddenBuffer, "", 0, true, newROI);  

        const int hidden_pix_len = 4 * hidden_width * hidden_height * hidden_channels;
        const int input_pix_len = sizeof(unsigned char) * 4 * img_width * img_height * channels;

        // Store hidden buffer inside hidden_pixmap
        hidden_pixmap = new unsigned char[hidden_pix_len]();
        hiddenBuffer.get_pixels(ROI::All(), TypeDesc::UINT8, hidden_pixmap);

        // Encode hidden length at the beginning of input pixmap
        for (int i = 0; i < HEADER_SIZE; i++) {
            // Remove the two least significant bits of index
            pixmap[i] &= 0xFC;  // 0xFC --> 1111 1100

            // Add header size data to LSBs
            pixmap[i] |= (hidden_pix_len >> (HEADER_SIZE - 2 - (i*2))) & 0x3;   // 0x3 --> 0011
        }

        // Encode hidden image inside the two least significant bits of pixmap
        for (unsigned int i = 0; i < hidden_pix_len; i++) {
            // Remove two least significant bits of index
            pixmap[i+HEADER_SIZE] &= 0xFC;  // 0xFC --> 1111 1100

            // Add data two LSBs
            pixmap[i+HEADER_SIZE] |= (hidden_pixmap[i/4] >> ((hidden_pix_len - 2 - (i*2)) % 8)) & 0x3; // i/4 for 2 bits
        }  

        // cout << "Hidden Length: " << hidden_pix_len << endl;

        // DEBUG
        test = new unsigned char[hidden_pix_len]();
        decodeImage(test);

        // for (unsigned int i = 0; i < hidden_pix_len; i++) {
        //     if (int(hidden_pixmap[i] != int(test[i]))) {
        //         cout << "DIFFERENT @ " << i << endl;
        //         cout << int(hidden_pixmap[i]) << " --> " << int(test[i]) << endl;
        //         break;
        //     }
        // }
    }
}


int main(int argc, char *argv[])
{
    int width, height;

    if (argc != 3 && argc != 4) {
        cout << "Improper usage: ./stegan -e <input_image> <hidden_image>\n\t     OR ./stegan -d <input_image>\n";
        exit(-1);
    } else if (strcmp(argv[1], "-e") == 0) {
        // Encode image

        if (argc == 3) {
            cout << "Improper usage: ./stegan -e <input_image> <hidden_image>\n";
            exit(-1);
        } else {
            readImage(argv[2]);
            encodeImage(argv[2], argv[3]);
            writeImage(pixmap, "modified.png", ImageSpec(img_width, img_height, channels, TypeDesc::UINT8));
        }
    } else if (strcmp(argv[1], "-d") == 0) {
        // Decode image

        if (argc != 3) {
            cout << "Improper usage: ./stegan -d <input_image>\n";
            exit(-1);
        } else {
            readImage(argv[2]);
            cout << "Secret Image Width: ";
            cin >> width;
            cout << "Secret Image Height: ";
            cin >> height;
            unsigned char *temp = new unsigned char[width*height*3]();
            decodeImage(temp);

            width /= 4;
            height /= 4;
            ImageSpec spec(width, height, 3, TypeDesc::UINT8);
            ImageBuf buf = ImageBuf(spec, temp); 

            writeImage(temp, "secret.png", spec);

            pixmap = temp;
            img_width = width;
            img_height = height;
        }
    }

    // Start up the glut utilities
    glutInit(&argc, argv);

    // create the graphics window
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    if ((strcmp(argv[1], "-e") == 0))
        glutInitWindowSize(img_width, img_height);
    else
        glutInitWindowSize(width, height);
    glutCreateWindow("Steganography");

    // Callback routines
    glutDisplayFunc(display);
    // glutKeyboardFunc(handleKey); 
    glutReshapeFunc(handleReshape);

    glutMainLoop();

    return 0;
}
